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
    
    //TSP path
    void build_NN(const char* const transposed_matrix, DistanceMatrix& distanceMatrix, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<unsigned>& order)
    {
        //Pick a random first vertex
        unsigned firstVertex = RNG::rand_uint32_t(0, distanceMatrix.width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(distanceMatrix.width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::vector<unsigned> path = {firstVertex};
        path.reserve(distanceMatrix.width());

        //Build vector of indices for VPTree
        std::vector<unsigned> vertices;
        vertices.resize(distanceMatrix.width());
        for(unsigned i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        unsigned counter = 0;

        DistanceFunctions df = VPTree<unsigned>::bindDistanceFunctions(
            [=, &counter](unsigned a, unsigned b) -> double {
                ++counter;
                return columns_hamming_distance(transposed_matrix, SUBSAMPLED_ROWS, a+OFFSET, b+OFFSET);
            },
            [&distanceMatrix](unsigned a, unsigned b) -> double { return distanceMatrix.get(a, b); },
            [&distanceMatrix](unsigned a, unsigned b, double d) { distanceMatrix.set(a, b, d); }
        );

        VPTree<unsigned> root(vertices, &df);

        //Added second vertex to data structures
        //Find next vertices to add by checking which is the minimum to take
        for(unsigned i = 1; i < distanceMatrix.width(); ++i)
        {
            IndexDistance match = find_closest_vertex(root, path[i-1], alreadyAdded);
            alreadyAdded[match.index] = true;
            path.push_back(match.index);
        }

        std::cout << "\tComputed distances (VPTree): " << counter << "/" << (distanceMatrix.width() * (distanceMatrix.width() - 1) / 2) <<  std::endl;

        //Store global order
        for(unsigned i = 0; i < distanceMatrix.width(); ++i)
            order[i+OFFSET] = path[i] + OFFSET; //Add offset because columns are addressed by their global location
    }


    //TSP path
    void build_double_end_NN(const char* const transposed_matrix, DistanceMatrix& distanceMatrix, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<unsigned>& order)
    {
        //Pick a random first vertex
        unsigned firstVertex = RNG::rand_uint32_t(0, distanceMatrix.width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(distanceMatrix.width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::deque<unsigned> orderDeque = {firstVertex};

        //Build vector of indices for VPTree
        std::vector<unsigned> vertices;
        vertices.resize(distanceMatrix.width());
        for(unsigned i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        unsigned counter = 0;

        //Use counter to count how many distance computation could be avoided by using a VPTree
        DistanceFunctions df = VPTree<unsigned>::bindDistanceFunctions(
            [=, &counter](unsigned a, unsigned b) -> double {
                ++counter;
                return columns_hamming_distance(transposed_matrix, SUBSAMPLED_ROWS, a+OFFSET, b+OFFSET);
            },
            [&distanceMatrix](unsigned a, unsigned b) -> double { return distanceMatrix.get(a, b); },
            [&distanceMatrix](unsigned a, unsigned b, double d) { distanceMatrix.set(a, b, d); }
        );

        VPTree<unsigned> root(vertices, &df);

        //Find second vertex
        IndexDistance second = find_closest_vertex(root, firstVertex, alreadyAdded);
        
        //Added second vertex to data structures
        orderDeque.push_back(second.index);
        alreadyAdded[second.index] = true;

        //Find closest vertices from path front and back
        IndexDistance a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);
        IndexDistance b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);

        //Find next vertices to add by checking which is the minimum to take
        for(unsigned i = 2; i < distanceMatrix.width(); ++i)
        {
            if(a.distance < b.distance)
            {
                orderDeque.push_front(a.index);
                alreadyAdded[a.index] = true;

                if(a.index == b.index)
                    b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);

                a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);
            }
            else
            {
                orderDeque.push_back(b.index);
                alreadyAdded[b.index] = true;

                if(b.index == a.index)
                    a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);

                b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);
            }
        }

        std::cout << "\tComputed distances (VPTree): " << counter << "/" << (distanceMatrix.width() * (distanceMatrix.width() - 1) / 2) <<  std::endl;

        //Store global order
        for(unsigned i = 0; i < distanceMatrix.width(); ++i)
            order[i+OFFSET] = orderDeque[i] + OFFSET; //Add offset because columns are addressed by their global location
    }

    double columns_hamming_distance(const char* const transposed_matrix, const std::size_t MAX_ROW, const unsigned COLUMN_A, const unsigned COLUMN_B)
    {
        return 1.0*hamming_distance(transposed_matrix + (COLUMN_A)*(MAX_ROW/8), transposed_matrix + (COLUMN_B)*(MAX_ROW/8), MAX_ROW/8) / MAX_ROW;
    }


    IndexDistance find_closest_vertex(VPTree<unsigned>& VPTREE, const unsigned VERTEX, const std::vector<bool>& ALREADY_ADDED)
    {
        IndexDistance nn = {0, 2.0};
        VPTREE.getUnvisitedNearestNeighbor(VERTEX, ALREADY_ADDED, &nn.distance, &nn.index);

        return nn;
    }

    //SSE2 implementation of Hamming distance
    /*size_t hamming_distance(const char* const BUFFER1, const char* const BUFFER2, const size_t LENGTH) 
    {
        if (BUFFER1 == nullptr || BUFFER2 == nullptr)
            throw std::invalid_argument("Input pointers cannot be null");

        std::size_t hammingDistance = 0;
        std::size_t i = 0;
    
        // Process 16 bytes at a time using SSE2
        for (; i + 16 <= LENGTH; i += 16) 
        {
            // Load 16 bytes from each pointer
            __m128i xmm1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(BUFFER1 + i));
            __m128i xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(BUFFER2 + i));
    
            std::uint64_t* u64_1 = reinterpret_cast<std::uint64_t*>(&xmm1);
            std::uint64_t* u64_2 = reinterpret_cast<std::uint64_t*>(&xmm2);
            
            hammingDistance += __builtin_popcountll(u64_1[0] ^ u64_2[0]);
            hammingDistance += __builtin_popcountll(u64_1[1] ^ u64_2[1]);
        }
    
        // Handle remaining bytes
        for (; i < LENGTH; ++i) 
        {
            unsigned char x = BUFFER1[i] ^ BUFFER2[i];
            hammingDistance += __builtin_popcount(x);
        }
    
        return hammingDistance;
    }*/

    //AVX2 implementation of Hamming distance
    size_t hamming_distance(const char* const BUFFER1, const char* const BUFFER2, const size_t LENGTH) 
    {
        if (BUFFER1 == nullptr || BUFFER2 == nullptr)
            throw std::invalid_argument("Input pointers cannot be null");

        std::size_t hammingDistance = 0;
        std::size_t i = 0;
    
        // Process 32 bytes at a time using AVX2
        for (; i + 32 <= LENGTH; i += 32) 
        {
            // Load 32 bytes from each pointer
            __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(BUFFER1 + i));
            __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(BUFFER2 + i));
            __m256i c = _mm256_xor_si256(a, b);

            std::uint64_t* u64_c = reinterpret_cast<std::uint64_t*>(&c);
            
            hammingDistance += __builtin_popcountll(u64_c[0]);
            hammingDistance += __builtin_popcountll(u64_c[1]);
            hammingDistance += __builtin_popcountll(u64_c[2]);
            hammingDistance += __builtin_popcountll(u64_c[3]);
        }
    
        // Handle remaining bytes
        for (; i < LENGTH; ++i) 
        {
            unsigned char x = BUFFER1[i] ^ BUFFER2[i];
            hammingDistance += __builtin_popcount(x);
        }
    
        return hammingDistance;
    }


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
    
    void launch(const char * const REFERENCE_MATRIX, const std::vector<std::string>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, unsigned groupsize, std::size_t subsampled_rows, std::vector<unsigned>& order)
    {
        DECLARE_TIMER;

        if(SAMPLES == 0)
            throw std::invalid_argument("SAMPLES can't be equal to 0");

        if(MATRICES.size() == 0)
            throw std::invalid_argument("Got empty vector of matrix path");
        
        if(groupsize % 8 != 0)
            throw std::invalid_argument("The size of a group of columns must be a multiple of 8 (for transposition)");

        int fd = open(REFERENCE_MATRIX, O_RDONLY); //Open reference matrix in read-only
        
        if(fd < 0)
            throw std::runtime_error("Failed to open a file descriptor on reference matrix");

        //Get file size
        const std::size_t FILE_SIZE = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        //Number of matrix columns
        const unsigned COLUMNS = (SAMPLES+7)/8*8;

        //Size of matrix row in bytes
        const unsigned ROW_LENGTH = (SAMPLES+7)/8;

        //Number of matrix rows
        const std::size_t NB_ROWS = (FILE_SIZE - HEADER) / ROW_LENGTH;

        //Map file in memory
        char* mapped_file = (char*)mmap(nullptr, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);

        if(NB_ROWS < subsampled_rows)
            throw std::invalid_argument("Number of subsampled rows can't be greater to the number of rows in the binary matrix. Maybe one of the parameters is wrong ?");

        //If defined to 0: Sample all matrix rows (at the number of rows must be a multiple of 8) 
        if(subsampled_rows == 0)
            subsampled_rows = NB_ROWS / 8 * 8;

        //If defined to 0: Compute one big path TSP (TODO: adapt to automatized group size according to little VPTree exp)
        if(groupsize == 0)
            groupsize = COLUMNS;

        if(subsampled_rows % 8 != 0)
            throw std::invalid_argument("The number of subsampled rows must be a multiple of 8 (for transposition)");

        std::cout << "Group size:\t" << groupsize << "Subsampled rows:\t" << subsampled_rows << "\n\n";

        //Compute distance matrix
        std::cout << "Transpose submatrix from reference submatrix '" << REFERENCE_MATRIX << "\' ... ";
        START_TIMER;
        char * transposed_matrix = ALLOCATE_MATRIX(subsampled_rows, COLUMNS);
        __sse_trans(reinterpret_cast<const std::uint8_t*>(mapped_file+HEADER), reinterpret_cast<std::uint8_t*>(transposed_matrix), subsampled_rows, COLUMNS);
        END_TIMER; SHOW_TIMER;

        //Approximate TSP path with double ended Nearest-Neighbor; compute order
        std::cout << "Computing column order using TSP ... " << std::endl;
        START_TIMER;
        TSP_NN(transposed_matrix, COLUMNS, groupsize, subsampled_rows, order);
        END_TIMER; 
        std::cout << "VPTree & NN path: ";
        SHOW_TIMER;

        //Unmap reference matrix and close its file descriptor 
        munmap(mapped_file, FILE_SIZE);
        close(fd);

        //Free transposed matrix
        delete[] transposed_matrix;
        
        //Shift blank columns (when samples is not a multiple of 8) to their original location
        immutable_filling_columns_inplace(order, COLUMNS, COLUMNS-SAMPLES);

        //Overshoot NB_ROWS to be a multiple of 8
        const std::size_t OVERSHOOT_NB_ROWS = (NB_ROWS + 7) / 8 * 8;

        //Compute overshooted file size
        const std::size_t OVERSHOOT_FILE_SIZE = HEADER + OVERSHOOT_NB_ROWS * ROW_LENGTH;

        transposed_matrix = ALLOCATE_MATRIX(OVERSHOOT_NB_ROWS, COLUMNS);

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

    void TSP_NN(const char* const transposed_matrix, const unsigned COLUMNS, const unsigned GROUPSIZE, const std::size_t SUBSAMPLED_ROWS, std::vector<unsigned>& order)
    {
        unsigned last_group_size;
        
        //Number of group of columns
        const unsigned NB_GROUPS = (COLUMNS+GROUPSIZE-1)/GROUPSIZE;

        if(COLUMNS % GROUPSIZE == 0)
            last_group_size = GROUPSIZE;
        else
            last_group_size = COLUMNS % GROUPSIZE;

        std::size_t offset = 0; //Offset for global order assignation

        DistanceMatrix distanceMatrix(GROUPSIZE);
        for(unsigned i = 0; i < NB_GROUPS-1; ++i)
        {
            //Find a suboptimal path minimizing the weight of edges and visiting each node once
            build_double_end_NN(transposed_matrix, distanceMatrix, SUBSAMPLED_ROWS, offset, order);
            
            offset += GROUPSIZE;
            distanceMatrix.reset(); //Clean distance matrix
        }

        distanceMatrix.resize(last_group_size);
        build_double_end_NN(transposed_matrix, distanceMatrix, SUBSAMPLED_ROWS, offset, order);
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
    std::cout << "Usage: bitmatrixshuffle -i <index> [-n <index_name>] [-g <groupsize>] [-s <subsampled_rows>]\n\n-i, --index STR\t\t\tPath to kmindex index directory.\n-g, --groupsize INT\t\tGroup size {0}.\n-n, --index-name STR\t\tIndex name.\n-s, --subsampled-rows INT\tNumber of subsampled rows to compute a distance {20000}.\n\n-h, --help\t\t\tPrint this help.\n";
}

int main(int argc, char ** argv)
{
    std::string index_path;
    std::string index_name;

    unsigned groupsize = 0;
    std::size_t subsampled_rows = 20000;

    bool index_name_flag = false;
    
    try 
    {
        cxxopts::Options options("BitmatrixShuffle", "Program reordering bitmatrix columns in a more compressive way (path TSP using Nearest-Neighbor)\n");
        
        options.add_options()
            ("i,index", "Path to kmindex index directory.", cxxopts::value<std::string>())
            ("n,index-name", "Index name.", cxxopts::value<std::string>())
            ("g,groupsize", "Group size {0}.", cxxopts::value<unsigned>())
            ("s,subsampled-rows", "Number of subsampled rows to compute a distance {20000}.", cxxopts::value<std::size_t>())
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

        // Get optional arguments
        if (args.count("groupsize"))
            groupsize = args["groupsize"].as<unsigned>();

        if (args.count("subsampled-rows")) 
            subsampled_rows = args["subsampled-rows"].as<std::size_t>();

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

    RNG::set_seed(42);

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


    //Select random reference partition (excepted 0)
    unsigned reference_partition = RNG::rand_uint32_t(1, nb_partitions);

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


    std::string reference_matrix = index_path + "/" + index_name + "/matrices/matrix_" + std::to_string(reference_partition) + ".cmbf";
    std::string out_order = index_path + "/" + index_name + "/order.bin";

    std::vector<unsigned> order;
    order.resize(columns);

    std::cout << "Parameters:\nIndex name:\t" << index_name << "\n#samples:\tt" << nb_samples << std::endl;
    Reorder::launch(reference_matrix.c_str(), matrices, nb_samples, 49, groupsize, subsampled_rows, order);

    //Serialize column order
    std::cout << "Serializing column order ..." << std::endl;
    int fdorder = open(out_order.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fdorder, reinterpret_cast<const char*>(order.data()), sizeof(unsigned)*order.size());
    close(fdorder);

    //Read samples from index.json and put them in a vector
    std::vector<std::string> samples = indexjson["index"][index_name]["samples"].get<std::vector<std::string>>();

    //Replace each element by the one that should be placed according to the permutation
    for(unsigned i = 0; i < nb_samples; ++i)
        indexjson["index"][index_name]["samples"][i] = samples[rev8(order[rev8(i)])];

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
        ofdfof << samples[rev8(order[rev8(i)])] << '\n';

    ofdfof.close();
}

#undef ALLOCATE_MATRIX