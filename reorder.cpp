#include <reorder.h>

namespace Reorder 
{
    // from https://mischasan.wordpress.com/2011/10/03/the-full-sse2-bit-matrix-transpose-routine/
    #define INP(x, y) inp[(x)*ncols/8 + (y)/8]
    #define OUT(x, y) out[(y)*nrows/8 + (x)/8]
    
    // II is defined as either (i) or (i ^ 7);
    // i^7 may be if we consider this bit order: [0,1,2,3,4,5,6,7][8,9,10,11,...]...
    #define II (i^7) 

    void __sse_trans(uint8_t const *inp, uint8_t *out, int nrows, int ncols)
    {
        ssize_t rr, cc, i, h;
        union
        {
            __m128i x;
            uint8_t b[16];
        } tmp;

        if(nrows % 8 != 0 || ncols % 8 != 0)
            throw std::invalid_argument("Matrix transposition: Number of columns and of rows must be both multiple of 8.");
    
        // Do the main body in 16x8 blocks:
        for ( rr = 0; rr <= nrows - 16; rr += 16 )
        {
            for ( cc = 0; cc < ncols; cc += 8 )
            {
                for ( i = 0; i < 16; ++i )
                    tmp.b[i] = INP(rr + II, cc);
                for ( i = 8; --i >= 0; tmp.x = _mm_slli_epi64(tmp.x, 1))
                    *(uint16_t *) &OUT(rr, cc + II) = _mm_movemask_epi8(tmp.x);
            }
        }
    
        if ( nrows % 16 == 0 )
            return;
        rr = nrows - nrows % 16;
    
        // The remainder is a block of 8x(16n+8) bits (n may be 0).
        //  Do a PAIR of 8x8 blocks in each step:
        for ( cc = 0; cc <= ncols - 16; cc += 16 )
        {
            for ( i = 0; i < 8; ++i )
            {
                tmp.b[i] = h = *(uint16_t const *) &INP(rr + II, cc);
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
    void build_NN(const char* const transposed_matrix, DistanceMatrix * DISTANCE_MATRIX, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<unsigned>& order)
    {
        //Pick a random first vertex
        unsigned firstVertex = RNG::rand_uint32_t(0, DISTANCE_MATRIX->width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(DISTANCE_MATRIX->width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::vector<unsigned> path = {firstVertex};
        path.reserve(DISTANCE_MATRIX->width());

        //Build vector of indices for VPTree
        std::vector<unsigned> vertices;
        vertices.resize(DISTANCE_MATRIX->width());
        for(unsigned i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        unsigned counter = 0;

        DistanceFunctions df = VPTree<unsigned>::bindDistanceFunctions(
            [=, &counter](unsigned a, unsigned b) -> double {
                ++counter;
                return columns_hamming_distance(transposed_matrix, SUBSAMPLED_ROWS, a+OFFSET, b+OFFSET);
            },
            [&DISTANCE_MATRIX](unsigned a, unsigned b) -> double { return DISTANCE_MATRIX->get(a, b); },
            [&DISTANCE_MATRIX](unsigned a, unsigned b, double d) { DISTANCE_MATRIX->set(a, b, d); }
        );

        VPTree root(vertices, &df);

        //Added second vertex to data structures
        //Find next vertices to add by checking which is the minimum to take
        for(unsigned i = 1; i < DISTANCE_MATRIX->width(); ++i)
        {
            IndexDistance match = find_closest_vertex(root, path[i-1], alreadyAdded);
            alreadyAdded[match.index] = true;
            path.push_back(match.index);
        }

        std::cout << "Maximum number of distance computations: " << (DISTANCE_MATRIX->width() * (DISTANCE_MATRIX->width() - 1) / 2) << std::endl;
        std::cout << "Distances computed with VPTree: " << counter << std::endl;

        //Store global order
        for(unsigned i = 0; i < DISTANCE_MATRIX->width(); ++i)
            order[i+OFFSET] = path[i] + OFFSET; //Add offset because columns are addressed by their global location
    }


    //TSP path
    void build_double_end_NN(const char* const transposed_matrix, DistanceMatrix * DISTANCE_MATRIX, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<unsigned>& order)
    {
        //Pick a random first vertex
        unsigned firstVertex = RNG::rand_uint32_t(0, DISTANCE_MATRIX->width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(DISTANCE_MATRIX->width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::deque<unsigned> orderDeque = {firstVertex};

        //Build vector of indices for VPTree
        std::vector<unsigned> vertices;
        vertices.resize(DISTANCE_MATRIX->width());
        for(unsigned i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        unsigned counter = 0;

        //Use counter to count how many distance computation could be avoided by using a VPTree
        DistanceFunctions df = VPTree<unsigned>::bindDistanceFunctions(
            [=, &counter](unsigned a, unsigned b) -> double {
                ++counter;
                return columns_hamming_distance(transposed_matrix, SUBSAMPLED_ROWS, a+OFFSET, b+OFFSET);
            },
            [&DISTANCE_MATRIX](unsigned a, unsigned b) -> double { return DISTANCE_MATRIX->get(a, b); },
            [&DISTANCE_MATRIX](unsigned a, unsigned b, double d) { DISTANCE_MATRIX->set(a, b, d); }
        );

        VPTree root(vertices, &df);

        //Find second vertex
        IndexDistance second = find_closest_vertex(root, firstVertex, alreadyAdded);
        
        //Added second vertex to data structures
        orderDeque.push_back(second.index);
        alreadyAdded[second.index] = true;

        //Find closest vertices from path front and back
        IndexDistance a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);
        IndexDistance b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);

        //Find next vertices to add by checking which is the minimum to take
        for(unsigned i = 2; i < DISTANCE_MATRIX->width(); ++i)
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

        std::cout << "Expected distance computations: " << (DISTANCE_MATRIX->width() * (DISTANCE_MATRIX->width() - 1) / 2) << std::endl;
        std::cout << "Distances computed with VPTree: " << counter << std::endl;

        //Store global order
        for(unsigned i = 0; i < DISTANCE_MATRIX->width(); ++i)
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


    //Functions needed
    char get_bit_from_position(const char * const BYTES, const unsigned POSITION)
    {
        //Equivalent instruction that is compiled to the same assembly code than second one
        //return (bytes[position >> 3] >> (8 - (position % 8) - 1)) & (char)1;

        return (BYTES[POSITION >> 3] >> (~POSITION & 0x7U)) & (char)1;
    }



    const char* const get_transposed_matrix(const char * const MAPPED_FILE, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const unsigned NB_GROUPS, const unsigned GROUPSIZE, const std::size_t SUBSAMPLED_ROWS, std::vector<DistanceMatrix*>& distanceMatrices)
    {
        unsigned last_group_size;
        
        if(SUBSAMPLED_ROWS % 8 != 0)
            throw std::invalid_argument("The number of subsampled rows must be a multiple of 8 (for transposition)");

        if(GROUPSIZE % 8 != 0)
            throw std::invalid_argument("The size of a group of columns must be a multiple of 8 (for transposition)");

        if(COLUMNS % GROUPSIZE == 0)
            last_group_size = GROUPSIZE;
        else
            last_group_size = COLUMNS % GROUPSIZE;
    
        //Create matrices
        //Don't forget to free them later!
        for(unsigned i = 0; i < NB_GROUPS-1; ++i)
            distanceMatrices[i] = new DistanceMatrix(GROUPSIZE);
        distanceMatrices[NB_GROUPS-1] = new DistanceMatrix(last_group_size);

        //Transposed buffer containing concatenated columns
        char * transposed_matrix = new char[(8*ROW_LENGTH)*(SUBSAMPLED_ROWS/8)];

        /**
         * Hamming distance (dH) can be computed like this: dH(A,B) = H(A ^ B) ;; with ^ as XOR symbol, and H as the Hamming weight aka popcount
         * 
         * SUBSAMPLED_ROWS/8 is the size of a row in the transposed buffer (as rows are now columns, so the width of transposed matrix) ;; /8 to get width in bytes (=in size of char) instead of bits
         */

        //Matrix transposition
        __sse_trans(reinterpret_cast<const std::uint8_t*>(MAPPED_FILE+HEADER), reinterpret_cast<std::uint8_t*>(transposed_matrix), SUBSAMPLED_ROWS, ROW_LENGTH*8);
        
        return static_cast<const char* const>(transposed_matrix);
    }

    //SSE2 implementation of Hamming distance
    size_t hamming_distance(const char* const BUFFER1, const char* const BUFFER2, const size_t LENGTH) 
    {
        if (BUFFER1 == nullptr || BUFFER2 == nullptr)
            throw std::invalid_argument("Input pointers cannot be null");

        size_t hammingDistance = 0;
        size_t i = 0;
    
        // Process 16 bytes at a time using SSE2
        for (; i + 16 <= LENGTH; i += 16) 
        {
            // Load 16 bytes from each pointer
            __m128i xmm1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(BUFFER1 + i));
            __m128i xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(BUFFER2 + i));
    
            uint64_t* u64_1 = reinterpret_cast<uint64_t*>(&xmm1);
            uint64_t* u64_2 = reinterpret_cast<uint64_t*>(&xmm2);
            
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
    }


    //In some tools, bytes
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

    void launch(const char * const REFERENCE_MATRIX, const std::vector<char*>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, const unsigned GROUPSIZE, std::size_t subsampled_rows, const char * const OUT_ORDER)
    {
        if(SAMPLES == 0)
            throw std::runtime_error("SAMPLES can't be equal to 0");

        if(MATRICES.size() == 0)
            throw std::runtime_error("Got empty vector of matrix path");

        int fd = open(REFERENCE_MATRIX, O_RDONLY); //Open file in both read/write modes
        
        if(fd < 0)
            throw std::runtime_error("Failed to open a file descriptor on reference matrix");

        //Get file size
        const std::size_t FILE_SIZE = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        //Map file in memory
        char* mapped_file = (char*)mmap(nullptr, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);

        //Number of matrix columns
        const unsigned COLUMNS = (SAMPLES+7)/8*8;

        //Size of matrix row in bytes
        const unsigned ROW_LENGTH = (SAMPLES+7)/8;

        //Number of matrix rows
        const std::size_t NB_ROWS = (FILE_SIZE - HEADER) / ROW_LENGTH;

        if(NB_ROWS < subsampled_rows)
            throw std::runtime_error("Number of subsampled rows can't be greater to the number of rows in the binary matrix. Maybe one of the parameters is wrong ?");

        //Number of group of columns
        const unsigned NB_GROUPS = (COLUMNS+GROUPSIZE-1)/GROUPSIZE;

        //If defined to 0: Sample all matrix rows (at the number of rows must be a multiple of 8) 
        if(subsampled_rows == 0)
            subsampled_rows = NB_ROWS / 8 * 8;

        std::vector<DistanceMatrix*> distanceMatrices;
        std::vector<unsigned> order;

        distanceMatrices.resize(NB_GROUPS);
        order.resize(COLUMNS);

        //Compute distance matrix
        std::cout << "Transposing submatrix of reference matrix..." << std::endl;
        const char* const transposed_matrix = get_transposed_matrix(mapped_file, HEADER, COLUMNS, ROW_LENGTH, NB_GROUPS, GROUPSIZE, subsampled_rows, distanceMatrices);

        //Approximate TSP path with double ended Nearest-Neighbor; compute order
        std::cout << "Computing order..." << std::endl;
        TSP_NN(transposed_matrix, distanceMatrices, subsampled_rows, order);

        //Unmap reference matrix and close its file descriptor 
        munmap(mapped_file, FILE_SIZE);
        close(fd);

        //Free transposed matrix
        delete[] transposed_matrix;

        //Free distance matrices from memory
        for(DistanceMatrix* m : distanceMatrices)
            delete m;
        
        //Shift blank columns (when samples is not a multiple of 8) to their original location
        immutable_filling_columns_inplace(order, COLUMNS, COLUMNS-SAMPLES);

        //Serialize order
        std::cout << "Serializing order..." << std::endl;
        int fdorder = open(OUT_ORDER, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        write(fdorder, reinterpret_cast<const char*>(order.data()), sizeof(unsigned)*order.size());
        close(fdorder);


        //Reorder all matrices
        for(unsigned i = 0; i < MATRICES.size(); i++)
        {
            std::cout << "\rReordering matrix " << (i+1) << "/" << MATRICES.size() << " ..." << std::flush;
            fd = open(MATRICES[i], O_RDWR);
            mapped_file = (char*)mmap(nullptr, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

             //Tell system that data will be accessed sequentially
            posix_madvise(mapped_file, FILE_SIZE, POSIX_MADV_SEQUENTIAL);
            //Reorder matrix columns
            reorder_matrix(mapped_file, HEADER, COLUMNS, ROW_LENGTH, NB_ROWS, order);

            //Unmap file in memory and close file descriptor 
            munmap(mapped_file, FILE_SIZE);
            close(fd);
        }

        std::cout << std::endl;
    }

    void permute_buffer_order(const char * const BUFFER, char * outBuffer, const unsigned * const ORDER, const unsigned ORDER_LENGTH)
    {
        //Permute bits within bytes of a buffer according to a given order
        //order[i] tells that the position i has to store the bit at position order[i]

        for(unsigned i = 0; i < ORDER_LENGTH; ++i)
            outBuffer[i >> 3] = (outBuffer[i >> 3] << 1) | get_bit_from_position(BUFFER, ORDER[i]);
    }

    void reorder_matrix(char * mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const std::size_t NB_ROWS, const std::vector<unsigned>& ORDER)
    {
        //Buffer to copy a row
        char * buffer = new char[ROW_LENGTH];

        std::size_t index = 0;

        //Permutate columns
        for(std::size_t i = 0; i < NB_ROWS; ++i)
        {
            std::memcpy(buffer, mapped_file+index+HEADER, ROW_LENGTH);
            
            //Permutate row bits
            permute_buffer_order(buffer, mapped_file+index+HEADER, ORDER.data(), COLUMNS);
            index += ROW_LENGTH;
        }

        delete[] buffer;
    }

    void TSP_NN(const char* const transposed_matrix, const std::vector<DistanceMatrix*>& DISTANCE_MATRICES, const std::size_t SUBSAMPLED_ROWS, std::vector<unsigned>& order)
    {
        std::size_t offset = 0; //Offset for global order assignation

        for(unsigned i = 0; i < DISTANCE_MATRICES.size(); ++i)
        {
            //Find a suboptimal path minimizing the weight of edges and visiting each node once
            build_double_end_NN(transposed_matrix, DISTANCE_MATRICES[i], SUBSAMPLED_ROWS, offset, order);
            
            offset += DISTANCE_MATRICES[i]->width();
        }
    }
};

int main(int argc, char ** argv)
{
    if(argc < 8)
    {
        std::cout << "Usage: reorder <ref_matrix> <samples> <header> <groupsize> <subsampled_rows> <out_order> <matrices...>\n\nref_matrix\tMatrix that will be used to compute path TSP\nsamples\t\tThe number of samples in a matrix (will be set to the upper multiple of 8 if given number is not a multiple of 8)\nheader\t\tSize of a matrix header (49 for kmtricks)\ngroupsize\tGroup of columns to reorder (must be a multiple of 8)\nsubsampled_rows\tNumber of rows to be subsampled (30.000 should be suffisant)\nout_order\tBinary serialized order computed from reference matrix\nmatrices\tAll matrices which columns will be reordered according to a same order (computed from reference matrix). Warning: all matrices must have the same size.\n\n";
        return 1;
    }

    RNG::set_seed(42);

    char* reference_matrix = argv[1];
    unsigned samples = (unsigned)atol(argv[2]);
    unsigned header = (unsigned)atol(argv[3]);
    unsigned groupsize = (unsigned)atol(argv[4]);
    std::size_t subsampled_rows = (std::size_t)atoll(argv[5]);
    char* out_order = argv[6];

    std::vector<char*> matrices;
    matrices.resize(argc - 7);

    for(int i = 7; i < argc; ++i)
        matrices[i-7] = argv[i];

    Reorder::launch(reference_matrix, matrices, samples, header, groupsize, subsampled_rows, out_order);
}
