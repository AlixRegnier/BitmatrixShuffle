#include <reorder.h>
#include <chrono>

#define DECLARE_TIMER std::chrono::time_point<std::chrono::high_resolution_clock> __start_timer, __stop_timer; std::size_t __integral_time

#define START_TIMER __start_timer = std::chrono::high_resolution_clock::now(); \
                    std::cout << std::flush

#define END_TIMER __stop_timer = std::chrono::high_resolution_clock::now(); \
                  __integral_time = static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(__stop_timer - __start_timer).count())

#define SHOW_TIMER std::cout << std::setprecision(3) << (__integral_time / 1000.0) << "s" << std::endl

#define ALLOCATE_MATRIX(nrows, ncols) new char[(nrows)*(ncols/8)]

namespace Reorder 
{
    // from https://mischasan.wordpress.com/2011/10/03/the-full-sse2-bit-matrix-transpose-routine/
    #define INP(x, y) inp[(x)*ncols/8 + (y)/8]
    #define OUT(x, y) out[(y)*nrows/8 + (x)/8]
    
    // II is defined as either (i) or (i ^ 7); i for LSB first, i^7 for MSB first
    #define II (i^7) 

    void __sse_trans(std::uint8_t const *inp, std::uint8_t *out, long nrows, long ncols)
    {
        ssize_t rr, cc, i, h;
        union
        {
            __m128i x;
            std::uint8_t b[16];
        } tmp;

        if(nrows % 8 != 0 || ncols % 8 != 0)
            throw std::invalid_argument("Matrix transposition: Number of columns and of rows must be both multiple of 8.");
    
        // Do the main body in 16x8 blocks:
        for ( rr = 0; rr + 16 <= nrows; rr += 16 )
        {
            for ( cc = 0; cc < ncols; cc += 8 )
            {
                for ( i = 0; i < 16; ++i )
                    tmp.b[i] = INP(rr + II, cc);
                for ( i = 8; --i >= 0; tmp.x = _mm_slli_epi64(tmp.x, 1))
                    *(std::uint16_t *) &OUT(rr, cc + II) = _mm_movemask_epi8(tmp.x);
            }
        }
    
        if ( nrows % 16 == 0 )
            return;
        rr = nrows - nrows % 16;
    
        // The remainder is a block of 8x(16n+8) bits (n may be 0).
        //  Do a PAIR of 8x8 blocks in each step:
        for ( cc = 0; cc + 16 <= ncols; cc += 16 )
        {
            for ( i = 0; i < 8; ++i )
            {
                tmp.b[i] = h = *(std::uint16_t const *) &INP(rr + II, cc);
                tmp.b[i + 8] = h >> 8;
            }
            for ( i = 8; --i >= 0; tmp.x = _mm_slli_epi64(tmp.x, 1))
            {
                OUT(rr, cc + II) = h = _mm_movemask_epi8(tmp.x);
                OUT(rr, cc + II + 8) = h >> 8;
            }
        }
        if ( cc == ncols )
            return;
    
        //  Do the remaining 8x8 block:
        for ( i = 0; i < 8; ++i )
            tmp.b[i] = INP(rr + II, cc);
        for ( i = 8; --i >= 0; tmp.x = _mm_slli_epi64(tmp.x, 1))
            OUT(rr, cc + II) = _mm_movemask_epi8(tmp.x);
    }
    
    #undef II
    #undef OUT
    #undef INP

    //Bring back filling columns (columns that are not samples but there to fill last byte of each row)
    void immutable_filling_columns_inplace(std::vector<unsigned>& order, const unsigned COLUMNS, const unsigned FILL)
    {
        if(FILL == 0)
            return;

        const unsigned A = COLUMNS - 8;
        const unsigned B = COLUMNS - 8 + FILL - 1;
        
        unsigned k = 0;

        //Write all integers that are not fillers
        for(unsigned i = 0; i < COLUMNS-8; ++i)
        {
            while(order[i+k] >= A && order[i+k] <= B)
                ++k;

            order[i] = order[i+k];
        }

        k = 0;

        //Write all remaining integers from end to fillers
        for(unsigned i = COLUMNS-1; i > COLUMNS-8+FILL-1; --i)
        {
            while(order[i-k] >= A && order[i-k] <= B)
                ++k;

            order[i] = order[i-k];
        }

        //Write fillers
        for(unsigned i = COLUMNS-8; i < COLUMNS+FILL-8; ++i)
            order[i] = i;
    }
    
    void launch(const std::vector<std::string>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, std::vector<unsigned>& order)
    {
        DECLARE_TIMER;

        if(SAMPLES == 0)
            throw std::invalid_argument("SAMPLES can't be equal to 0");

        if(MATRICES.size() == 0)
            throw std::invalid_argument("Got empty vector of matrix path");
        
        int fd = open(MATRICES[0].c_str(), O_RDONLY); //Open reference matrix in read-only
        
        if(fd < 0)
            throw std::runtime_error("Failed to open a file descriptor on reference matrix");

        //Get file size
        const std::size_t FILE_SIZE = lseek(fd, 0, SEEK_END);
        close(fd);

        //Number of matrix columns
        const unsigned COLUMNS = (SAMPLES+7)/8*8;

        //Size of matrix row in bytes
        const unsigned ROW_LENGTH = (SAMPLES+7)/8;

        //Number of matrix rows
        const std::size_t NB_ROWS = (FILE_SIZE - HEADER) / ROW_LENGTH;

        //Map file in memory
        char* mapped_file;
        
        //Shift blank columns (when samples is not a multiple of 8) to their original location
        immutable_filling_columns_inplace(order, COLUMNS, COLUMNS-SAMPLES);

        //Overshoot NB_ROWS to be a multiple of 8
        const std::size_t OVERSHOOT_NB_ROWS = (NB_ROWS + 7) / 8 * 8;

        //Compute overshooted file size
        const std::size_t OVERSHOOT_FILE_SIZE = HEADER + OVERSHOOT_NB_ROWS * ROW_LENGTH;

        char * transposed_matrix = ALLOCATE_MATRIX(OVERSHOOT_NB_ROWS, COLUMNS);

        //Reorder all matrices
        for(unsigned i = 0; i < MATRICES.size(); i++)
        {
            std::cout << "Reordering matrix '" << MATRICES[i] << "' " << (i+1) << "/" << MATRICES.size() << " ... " << std::endl;

            fd = open(MATRICES[i].c_str(), O_RDWR);
            mapped_file = (char*)mmap(nullptr, OVERSHOOT_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

            if(mapped_file == MAP_FAILED)
                throw std::runtime_error("mmap() failed");
            
            //Tell system that data will be accessed randomly
            posix_madvise(mapped_file, OVERSHOOT_FILE_SIZE, POSIX_MADV_NORMAL);
            
            std::cout << "\tTranspose matrix for reordering columns ... ";
            START_TIMER;
            __sse_trans(reinterpret_cast<const std::uint8_t*>(mapped_file+HEADER), reinterpret_cast<std::uint8_t*>(transposed_matrix), OVERSHOOT_NB_ROWS, COLUMNS);
            END_TIMER; SHOW_TIMER;

            //Reorder matrix columns (transposed matrix rows)
            std::cout << "\tReordering matrix columns ... ";
            START_TIMER;
            reorder_matrix_rows(transposed_matrix, 0, OVERSHOOT_NB_ROWS/8, ROW_LENGTH*8, order);
            END_TIMER; SHOW_TIMER;

            std::cout << "\tTranspose matrix back ... ";
            //Transpose back (overshooted rows will be written back in memory mapped overshoot but won't be added to file, that's how mmap works with overshoot memory)
            START_TIMER;
            __sse_trans(reinterpret_cast<const std::uint8_t*>(transposed_matrix), reinterpret_cast<std::uint8_t*>(mapped_file+HEADER), COLUMNS, OVERSHOOT_NB_ROWS);
            END_TIMER; SHOW_TIMER;
            
            //Unmap file in memory and close file descriptor 
            munmap(mapped_file, OVERSHOOT_FILE_SIZE);
            close(fd);
        }
        
        delete[] transposed_matrix;
        std::cout << std::endl;
    }
};


//Function mapping byte inner bits position (MSB position is 0, LSB position is 7) to reversed order
constexpr unsigned rev8(unsigned x)
{
    /*
     0 -->  7
     1 -->  6
     2 -->  5
     3 -->  4
     4 -->  3
     5 -->  2
     6 -->  1
     7 -->  0
     8 --> 15
     9 --> 14
    10 --> 13
    11 --> 12
    ...    
    */

    //Equivalent instructions (two last instructions have exactly the same assembly code)
    //return 7 - (x % 8) + (x / 8) * 8;
    //return 7 - (x & 0x7U) + (x / 8) * 8;
    //return (x | 0x7U) - (x & 0x7U);
    //return (x & ~0x7U) | ~(x & 0x7U);
    return (x | 0x7U) - (x & 0x7U);
}

void usage()
{
    std::cout << "Usage: reverse_bitmatrixshuffle -i <index> [-n <index_name>]\n\n-i, --index STR\t\t\tPath to kmindex index directory.\n-n, --index-name STR\t\tIndex name.\n\n-h, --help\t\t\tPrint this help.\n";
}

int main(int argc, char ** argv)
{
    std::string index_path;
    std::string index_name;

    bool index_name_flag = false;
    
    try 
    {
        cxxopts::Options options("Reverse BitmatrixShuffle", "Program reordering bitmatrix columns in a more compressive way (path TSP using Nearest-Neighbor)\n");
        
        options.add_options()
            ("i,index", "Path to kmindex index directory.", cxxopts::value<std::string>())
            ("n,index-name", "Index name.", cxxopts::value<std::string>())
            ("h,help", "Print usage.");

        auto args = options.parse(argc, argv);

        if (args.count("help"))
        {
            usage();
            return 0;
        }

        // Check required argument
        if (!args.count("index"))
        {
            std::cerr << "Error: -i/--index is required\n";
            usage();
            return 1;
        }

        // Get required argument
        index_path = args["index"].as<std::string>();
        
        // Validate that index path exists and is a directory
        if (!std::filesystem::exists(index_path)) 
        {
            std::cerr << "Error: Index directory '" << index_path << "' does not exist\n";
            return 2;
        }
        
        if (!std::filesystem::is_directory(index_path))
        {
            std::cerr << "Error: Index path '" << index_path << "' is not a directory\n";
            return 2;
        }

        if(args.count("index-name"))
        {
            index_name = args["index-name"].as<std::string>();
            index_name_flag = true;
        }
    } 
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 2;
    } 
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }

    std::string json_path = index_path + "/index.json";

    //Open file descriptor on input json file
    std::ifstream fdin(json_path);

    if(!fdin.is_open())
    {
        std::cerr << "Error: Could not read file '" << json_path << "'\n";
        return 2;
    }

    //Deserialize JSON file
    json indexjson = json::parse(fdin);
    fdin.close();
    
    //Check if JSON file has "index" as 0-depth key
    if(!indexjson.contains("index"))
        throw std::runtime_error("Index was not recognized.");

    //If no index name specified, find the first one
    if(!index_name_flag)
    {
        auto kv = indexjson["index"].items();

        if(kv.begin() == kv.end())
        {
            std::cerr << "Error: No registered index found in 'index.json'\n";
            return 2;
        }

        index_name = kv.begin().key();
    }
    //Check if JSON file -> ["index"] has <index_name> as key
    else if(!indexjson["index"].contains(index_name))
        throw std::runtime_error("Index '" + index_name + "' doesn't exist");

    if(!indexjson["index"][index_name].contains("nb_samples"))
        throw std::runtime_error("Index '" + index_name + "' has no field 'nb_samples'");

    if(!indexjson["index"][index_name].contains("nb_partitions"))
        throw std::runtime_error("Index '" + index_name + "' has no field 'nb_partitions'");

    //Get current number of samples
    unsigned nb_samples = indexjson["index"][index_name]["nb_samples"].get<unsigned>();

    unsigned columns = (nb_samples+7) / 8 * 8;
    
    //Get current number of partitions
    unsigned nb_partitions = indexjson["index"][index_name]["nb_partitions"].get<unsigned>();

    //Check each partitions
    std::vector<std::string> matrices;
    matrices.reserve(nb_partitions);

    for(unsigned i = 0; i < nb_partitions; ++i)
    {
        std::string p = index_path + "/" + index_name + "/matrices/matrix_" + std::to_string(i) + ".cmbf";

        if(std::filesystem::exists(p))
            matrices.push_back(p);
        else
            std::cout << "File '" + p + "' does not exist (skipped)\n";
    }
    
    std::string in_order = index_path + "/" + index_name + "/order.bin";

    std::vector<unsigned> order;
    order.resize(columns);

    std::vector<unsigned> reversed_order;
    reversed_order.resize(columns);

    std::cout << "Parameters:\nIndex name:\t" << index_name << "\n#samples:\tt" << nb_samples << std::endl;

    //Serialize column order
    std::cout << "Deserializing column order ..." << std::endl;
    int fdorder = open(in_order.c_str(), O_RDONLY);
    read(fdorder, reinterpret_cast<char*>(order.data()), sizeof(unsigned)*order.size());
    close(fdorder);

    //Compute reverse order
    for(unsigned i = 0; i < columns; ++i)
        reversed_order[order[i]] = i;

    Reorder::launch(matrices, nb_samples, 49, reversed_order);

    //Read samples from index.json and put them in a vector
    std::vector<std::string> samples = indexjson["index"][index_name]["samples"].get<std::vector<std::string>>();

    //Replace each element by the one that should be placed according to the permutation
    for(unsigned i = 0; i < nb_samples; ++i)
        indexjson["index"][index_name]["samples"][i] = samples[rev8(reversed_order[rev8(i)])];

    //Apply modifications to JSON file
    std::ofstream fdout(json_path, std::ofstream::out);
    fdout << std::setw(4) << indexjson << std::endl;
    fdout.close();

    //Clear JSON object
    indexjson.clear();

    //Modify kmindex FOF
    std::string fof_path = index_path + "/" + index_name + "/kmtricks.fof";

    std::ifstream ifdfof(fof_path);
    
    if (!ifdfof.is_open()) 
    {
        std::cerr << "Error: Could not read file '" << fof_path << "'\n";
        return 2;
    }

    //Store all lines
    unsigned i = 0;
    std::string line;
    while (std::getline(ifdfof, line) && i < nb_samples)
        samples[i++] = line;

    ifdfof.close();
    std::ofstream ofdfof(fof_path);

    if (!ofdfof.is_open()) 
    {
        std::cerr << "Error: Could not write file '" << fof_path << "'\n";
        return 2;
    }

    for(i = 0; i < nb_samples; ++i)
        ofdfof << samples[rev8(reversed_order[rev8(i)])] << '\n';

    ofdfof.close();
}

#undef ALLOCATE_MATRIX