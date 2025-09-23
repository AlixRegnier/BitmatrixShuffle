#include <bitmatrixshuffle.h>
#include <zstd/BlockCompressorZSTD.h>

//AVX2/SSE2
#include <immintrin.h>
#include <emmintrin.h>
#include <stdint.h>

#define GET_ROW_PTR(x) (mapped_file+HEADER+((std::size_t)(x))*ROW_LENGTH)
#define GET_BLOCK_PTR(x) (mapped_file+HEADER+((std::size_t)(x))*BLOCK_SIZE)

namespace bms
{
    // Code from https://mischasan.wordpress.com/2011/10/03/the-full-sse2-bit-matrix-transpose-routine/
    #define INP(x, y) inp[(x)*ncols/8 + (y)/8]
    #define OUT(x, y) out[(y)*nrows/8 + (x)/8]

    // II is defined as either (i) or (i ^ 7); i for LSB first, i^7 for MSB first
    #define II (i^7) 

    void __sse2_trans(std::uint8_t const *inp, std::uint8_t *out, long nrows, long ncols)
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
    /*//Extract bit from a buffer of bytes (big-endian), result can only be 0x00 or 0x01
    char get_bit_from_position(const char* const BYTES, const unsigned POSITION);

    //Swaps the bits of a buffer into another buffer according to given order
    void permute_buffer_order(const char* const BUFFER, char* outBuffer, const unsigned* const ORDER, const unsigned ORDER_LENGTH);
    
    //Retrieve bit at given POSITION in buffer BYTES
    char get_bit_from_position(const char * const BYTES, const unsigned POSITION)
    {
        //Equivalent instruction that is compiled to the same assembly code than second one
        //return (bytes[position >> 3] >> (8 - (position % 8) - 1)) & (char)1;

        return (BYTES[POSITION >> 3] >> (~POSITION & 0x7U)) & (char)1;
    }

    //Swaps the bits of a buffer into another buffer according to given order)
    void permute_row_bits(const char * const BUFFER, char * outBuffer, const unsigned * const ORDER, const std::size_t ORDER_LENGTH)
    {
        //Permute bits within bytes of a buffer according to a given order
        //order[i] tells that the position i has to store the bit at position order[i]

        for(unsigned i = 0; i < ORDER_LENGTH; ++i)
            outBuffer[i >> 3] = (outBuffer[i >> 3] << 1) | get_bit_from_position(BUFFER, ORDER[i]);
    }

    //DEPRECATED: Reorder matrix columns row by row (bit-swapping on memory-mapped file)
    //Very slow but is SIMD-free
    void reorder_matrix_columns(char * mapped_file, const unsigned HEADER, const std::size_t NB_COLS, const std::size_t ROW_LENGTH, const std::size_t NB_ROWS, const std::vector<std::uint64_t>& ORDER)
    {
        //Buffer to copy a row
        char * buffer = new char[ROW_LENGTH];

        //Swap columns in each rows
        for(std::size_t i = 0; i < NB_ROWS; ++i)
        {
            std::memcpy(buffer, GET_ROW_PTR(i), ROW_LENGTH);
            
            //Swap row bits
            permute_buffer_order(buffer, GET_ROW_PTR(i), ORDER.data(), NB_COLS);
        }

        delete[] buffer;
    }*/

    std::size_t target_block_nb_rows(const std::size_t NB_COLS, const std::size_t BLOCK_TARGET_SIZE)
    {
        const std::size_t ROW_LENGTH = (NB_COLS + 7) / 8;

        //Compute the number of rows in a block and round to next multiple of 8 
        return (((BLOCK_TARGET_SIZE+ROW_LENGTH-1) / ROW_LENGTH) + 7) / 8 * 8;
    }

    std::size_t target_block_size(const std::size_t NB_COLS, const std::size_t BLOCK_TARGET_SIZE)
    {
        const std::size_t ROW_LENGTH = (NB_COLS + 7) / 8;

        //Block size will most of time be slightly bigger than targeted size
        return ROW_LENGTH * target_block_nb_rows(NB_COLS, BLOCK_TARGET_SIZE); 
    }


    std::size_t estimate_computations(std::size_t n1, std::size_t n2, std::size_t x1)
    {
        #define OMEGA(n) ((n)*std::log2((n)))
        #define BIGO(n)  (((n)*((n)-1))/2.0)

        return (std::size_t)(OMEGA(n2) + (BIGO(n2)-OMEGA(n2))/(BIGO(n1)-OMEGA(n1)) * (x1-OMEGA(n1)));

        #undef OMEGA
        #undef BIGO
    }

    void compute_order_from_matrix_columns(const std::string& MATRIX_PATH, const unsigned HEADER, const std::size_t NB_COLS, const std::size_t NB_ROWS, std::size_t groupsize, std::size_t subsampled_rows, std::vector<std::uint64_t>& order)
    {
        DECLARE_TIMER;
        START_TIMER;

        int fd = open(MATRIX_PATH.c_str(), O_RDONLY); 
        if(fd < 0)
            throw std::runtime_error("BMS-ERROR: Failed to open a file descriptor on reference matrix");

        const std::size_t ROW_LENGTH = (NB_COLS + 7) / 8;
        const std::size_t FILE_SIZE = HEADER + ROW_LENGTH * NB_ROWS;

        order.resize(ROW_LENGTH * 8);

        const char * const mapped_file = (const char * const)mmap(nullptr, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
        
        subsampled_rows = subsampled_rows / 8 * 8;
        if(subsampled_rows == 0)
            subsampled_rows = NB_ROWS / 8 * 8;

        if(subsampled_rows > NB_ROWS)
            throw std::invalid_argument("BMS-ERROR: Number of subsampled rows can't be greater to the number of rows in the binary matrix. Maybe one of the parameters is wrong ?");

        if(subsampled_rows % 8 != 0)
            throw std::invalid_argument("BMS-ERROR: Number of subsampled rows is not a multiple of 8. Maybe your matrix has less than 8 rows ?");
       
        if(groupsize % 8 != 0)
            throw std::invalid_argument("BMS-ERROR: The size of a group of columns must be a multiple of 8 (for transposition)");

        if(groupsize == 0)
            groupsize = ROW_LENGTH * 8;

        char * transposed_matrix = BMS_ALLOCATE_MATRIX(subsampled_rows, ROW_LENGTH*8);
        __sse2_trans(reinterpret_cast<const std::uint8_t*>(mapped_file+HEADER), reinterpret_cast<std::uint8_t*>(transposed_matrix), subsampled_rows, ROW_LENGTH*8);

        std::size_t last_group_size;
        
        //Number of group of columns
        const std::size_t NB_GROUPS = (ROW_LENGTH*8+groupsize-1)/groupsize;

        if((ROW_LENGTH*8) % groupsize == 0)
            last_group_size = groupsize;
        else
            last_group_size = (ROW_LENGTH*8) % groupsize;

        std::size_t offset = 0; //Offset for global order assignation


        std::size_t computed_distances = 0;

        DistanceMatrix distanceMatrix(groupsize);
        for(std::size_t i = 0; i < NB_GROUPS-1; ++i)
        {
            //Find a suboptimal path minimizing the weight of edges and visiting each node once
            computed_distances += build_double_ended_NN(transposed_matrix, distanceMatrix, subsampled_rows, offset, order);
            
            offset += groupsize;
            distanceMatrix.reset(); //Clean distance matrix
        }

        distanceMatrix.resize(last_group_size);
        computed_distances += build_double_ended_NN(transposed_matrix, distanceMatrix, subsampled_rows, offset, order);
        
        
        END_TIMER;
        metrics["time_permutation(s)"] = GET_TIMER; 
        
        std::size_t max_computable_distances = (groupsize * (groupsize - 1) / 2) * (NB_GROUPS - 1) + last_group_size * (last_group_size - 1) / 2;
        metrics["computed_distances"] = computed_distances;
        metrics["max_computable_distances"] = max_computable_distances;
        metrics["pct_computed_distances(%)"] = 100.0 * computed_distances / max_computable_distances;

        //Try distance estimation to further extract error bounds
        #define FAKE_SIZE 4096
        if(ROW_LENGTH*8 > FAKE_SIZE)
        {
            DistanceMatrix fake_distanceMatrix(FAKE_SIZE);
            std::vector<std::uint64_t> fake_order;
            fake_order.resize(FAKE_SIZE);
            std::size_t fake_computed_distances = build_double_ended_NN(transposed_matrix, fake_distanceMatrix, subsampled_rows, 0, fake_order);
            std::size_t fake_max_computable_distances = (FAKE_SIZE * (FAKE_SIZE - 1)) / 2.0;
            metrics["fake_computed_distances"] = fake_computed_distances;
            metrics["fake_max_computable_distances"] = fake_max_computable_distances;
            metrics["fake_pct_computed_distances(%)"] = 100.0 * fake_computed_distances / fake_max_computable_distances;
            
            std::size_t estimated_computed_distances = estimate_computations(FAKE_SIZE, groupsize, fake_computed_distances) * NB_GROUPS + estimate_computations(FAKE_SIZE, last_group_size, fake_computed_distances);
            metrics["estimated_computed_distances"] = estimated_computed_distances;

            metrics["estimation_error(%)"] = 100.0 * (std::max(estimated_computed_distances, computed_distances) - std::min(estimated_computed_distances, computed_distances)) / estimated_computed_distances;
        }
        #undef FAKE_SIZE

        double original_consecutive_distances_sum = 0.0;
        double new_consecutive_distances_sum = 0.0;

        for(unsigned i = 0; i + 1 < ROW_LENGTH*8; ++i)
        {
            original_consecutive_distances_sum += columns_hamming_distance(transposed_matrix, subsampled_rows, i, i+1);
            new_consecutive_distances_sum += columns_hamming_distance(transposed_matrix, subsampled_rows, order[i], order[i+1]);
        }

        double original_consecutive_distances_average = original_consecutive_distances_sum / (ROW_LENGTH*8 - 1);
        double new_consecutive_distances_average = new_consecutive_distances_sum / (ROW_LENGTH*8 - 1);

        double original_consecutive_distances_variance = 0.0;
        double new_consecutive_distances_variance = 0.0;

        //Compute variance
        for(unsigned i = 0; i + 1 < ROW_LENGTH*8; ++i)
        {
            original_consecutive_distances_variance += std::pow(columns_hamming_distance(transposed_matrix, subsampled_rows, i, i+1) - original_consecutive_distances_average, 2);
            new_consecutive_distances_variance += std::pow(columns_hamming_distance(transposed_matrix, subsampled_rows, order[i], order[i+1]) - new_consecutive_distances_average, 2);
        }

        //N-1: fix population bias
        original_consecutive_distances_variance /= ROW_LENGTH*8 - 2;
        new_consecutive_distances_variance /= ROW_LENGTH*8 - 2;

        //Compute stdev from variance
        double original_consecutive_distances_stdev = std::sqrt(original_consecutive_distances_variance);
        double new_consecutive_distances_stdev = std::sqrt(new_consecutive_distances_variance);

        metrics["avg_consecutive_column_distance_original"] = original_consecutive_distances_average;
        metrics["avg_consecutive_column_distance_reorder"] = new_consecutive_distances_average;
        metrics["var_consecutive_column_distance_original"] = original_consecutive_distances_variance;
        metrics["var_consecutive_column_distance_reorder"] = new_consecutive_distances_variance;
        metrics["stdev_consecutive_column_distance_original"] = original_consecutive_distances_stdev;
        metrics["stdev_consecutive_column_distance_reorder"] = new_consecutive_distances_stdev;
        metrics["metric_reordering_compressibility_factor"] = 1.0 * original_consecutive_distances_average / new_consecutive_distances_average;

        BMS_DELETE_MATRIX(transposed_matrix);
    }

    void reorder_matrix_columns(const std::string& MATRIX_PATH, const unsigned HEADER, const std::size_t NB_COLS, const std::size_t NB_ROWS, const std::vector<std::uint64_t>& ORDER, const std::size_t BLOCK_TARGET_SIZE)
    {
        //Get row length in bytes
        const std::size_t ROW_LENGTH = (NB_COLS + 7) / 8;

        //Compute the number of rows in a block and round to next multiple of 8 
        const std::size_t BLOCK_NB_ROWS = target_block_nb_rows(NB_COLS, BLOCK_TARGET_SIZE);

        //Block size will most of time be slightly bigger than targeted size
        const std::size_t BLOCK_SIZE = target_block_size(NB_COLS, BLOCK_TARGET_SIZE); 

        //The last block may not be full
        const std::size_t NB_BLOCKS = (NB_ROWS+BLOCK_NB_ROWS-1) / BLOCK_NB_ROWS; 
        
        //Overshoot allows to consider last block as full and to apply operations, overshooted rows won't be written 
        const std::size_t FILE_SIZE = HEADER + NB_ROWS * ROW_LENGTH;

        //Compute last block size
        std::size_t last_block_size = (NB_ROWS % BLOCK_NB_ROWS) * ROW_LENGTH;
        if(last_block_size == 0)
            last_block_size = BLOCK_SIZE; //Each blocks are full, so last is full

        char * buffered_block = BMS_ALLOCATE_MATRIX(BLOCK_NB_ROWS, ROW_LENGTH*8);
        char * transposed_block = BMS_ALLOCATE_MATRIX(BLOCK_NB_ROWS, ROW_LENGTH*8);
    
        int fd = open(MATRIX_PATH.c_str(), O_RDWR);
        char * mapped_file = (char*)mmap(nullptr, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if(mapped_file == MAP_FAILED)
            throw std::runtime_error("BMS-ERROR: mmap() failed for reordering matrix");

        //Tell system that data will be accessed sequentially
        posix_madvise(mapped_file, FILE_SIZE, POSIX_MADV_SEQUENTIAL);

        std::size_t i = 0;
        for(; i + 1 < NB_BLOCKS; ++i)
        {
            //Copy block from disk to memory
            std::memcpy(buffered_block, GET_BLOCK_PTR(i), BLOCK_SIZE);

            //Transpose matrix block
            __sse2_trans(reinterpret_cast<const std::uint8_t*>(buffered_block), reinterpret_cast<std::uint8_t*>(transposed_block), BLOCK_NB_ROWS, ROW_LENGTH*8);

            //Reorder block columns (by reordering transposed block rows)
            reorder_matrix_rows(transposed_block, 0, BLOCK_NB_ROWS/8, ORDER);

            //Transpose matrix block back
            __sse2_trans(reinterpret_cast<const std::uint8_t*>(transposed_block), reinterpret_cast<std::uint8_t*>(buffered_block), ROW_LENGTH*8, BLOCK_NB_ROWS);

            //Copy block from memory to disk
            std::memcpy(GET_BLOCK_PTR(i), buffered_block, BLOCK_SIZE);
        }

        std::memcpy(buffered_block, GET_BLOCK_PTR(i), last_block_size);
        __sse2_trans(reinterpret_cast<const std::uint8_t*>(buffered_block), reinterpret_cast<std::uint8_t*>(transposed_block), BLOCK_NB_ROWS, ROW_LENGTH*8);
        reorder_matrix_rows(transposed_block, 0, BLOCK_NB_ROWS/8, ORDER);
        __sse2_trans(reinterpret_cast<const std::uint8_t*>(transposed_block), reinterpret_cast<std::uint8_t*>(buffered_block), ROW_LENGTH*8, BLOCK_NB_ROWS);
        std::memcpy(GET_BLOCK_PTR(i), buffered_block, last_block_size);

        BMS_DELETE_MATRIX(buffered_block);
        BMS_DELETE_MATRIX(transposed_block);

        munmap(mapped_file, FILE_SIZE);
        close(fd);
    }

    void reorder_matrix_columns_and_compress(const std::string& MATRIX_PATH, const std::string& OUTPUT_PATH, const std::string& OUTPUT_EF_PATH, const std::string& CONFIG_PATH, const unsigned HEADER, const std::size_t NB_COLS, const std::size_t NB_ROWS, const std::vector<std::uint64_t>& ORDER, std::size_t BLOCK_TARGET_SIZE)
    {
        DECLARE_TIMER;

        //Get row length in bytes
        const std::size_t ROW_LENGTH = (NB_COLS + 7) / 8;

        //Compute the number of rows in a block and round to next multiple of 8 
        const std::size_t BLOCK_NB_ROWS = target_block_nb_rows(NB_COLS, BLOCK_TARGET_SIZE);

        //Block size will most of time be slightly bigger than targeted size
        const std::size_t BLOCK_SIZE = target_block_size(NB_COLS, BLOCK_TARGET_SIZE); 

        //The last block may not be full
        const std::size_t NB_BLOCKS = (NB_ROWS+BLOCK_NB_ROWS-1) / BLOCK_NB_ROWS; 

        metrics["nb_blocks"] = NB_BLOCKS;
        
        const std::size_t FILE_SIZE = HEADER + NB_ROWS * ROW_LENGTH;
        
        //Compute last block size
        std::size_t last_block_size = (NB_ROWS % BLOCK_NB_ROWS) * ROW_LENGTH;
        if(last_block_size == 0)
            last_block_size = BLOCK_SIZE; //Each blocks are full, so last is full

        char * buffered_block = BMS_ALLOCATE_MATRIX(BLOCK_NB_ROWS, ROW_LENGTH*8);
        char * transposed_block = BMS_ALLOCATE_MATRIX(BLOCK_NB_ROWS, ROW_LENGTH*8);
    
        int fd = open(MATRIX_PATH.c_str(), O_RDONLY);

        //Since no modifications will be applied to original matrix, open it with MAP_PRIVATE mode
        char * const mapped_file = (char* const)mmap(nullptr, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);

        if(mapped_file == MAP_FAILED)
            throw std::runtime_error("BMS-ERROR: mmap() failed for reordering matrix");

        //Tell system that data will be accessed sequentially
        posix_madvise(mapped_file, FILE_SIZE, POSIX_MADV_SEQUENTIAL);

        std::size_t time_compression = 0;
        std::size_t time_reorder = 0;

        START_TIMER;
        BlockCompressorZSTD block_compressor(OUTPUT_PATH, OUTPUT_EF_PATH, CONFIG_PATH);
        block_compressor.write_header(mapped_file, HEADER);
        END_TIMER;
        time_compression += __integral_time;

        std::size_t i = 0;
        //Process each blocks except the last
        for(; i + 1 < NB_BLOCKS; ++i)
        {
            START_TIMER;
            //Copy block from disk to memory
            std::memcpy(buffered_block, GET_BLOCK_PTR(i), BLOCK_SIZE);
            
            //Transpose matrix block
            __sse2_trans(reinterpret_cast<const std::uint8_t*>(buffered_block), reinterpret_cast<std::uint8_t*>(transposed_block), BLOCK_NB_ROWS, ROW_LENGTH*8);
            
            //Reorder block columns (by reordering transposed block rows)
            reorder_matrix_rows(transposed_block, 0, BLOCK_NB_ROWS/8, ORDER);
            
            //Transpose matrix block back
            __sse2_trans(reinterpret_cast<const std::uint8_t*>(transposed_block), reinterpret_cast<std::uint8_t*>(buffered_block), ROW_LENGTH*8, BLOCK_NB_ROWS);
            END_TIMER;
            time_reorder += __integral_time;

            //Bring buffered block to compressor
            START_TIMER;
            block_compressor.append_block(reinterpret_cast<std::uint8_t*>(buffered_block), BLOCK_SIZE);
            END_TIMER;
            time_compression += __integral_time;
        }

        //Handle last block that may be smaller, only block effective size differs
        START_TIMER;
        std::memcpy(buffered_block, GET_BLOCK_PTR(i), last_block_size);
        __sse2_trans(reinterpret_cast<const std::uint8_t*>(buffered_block), reinterpret_cast<std::uint8_t*>(transposed_block), BLOCK_NB_ROWS, ROW_LENGTH*8);
        reorder_matrix_rows(transposed_block, 0, BLOCK_NB_ROWS/8, ORDER);
        __sse2_trans(reinterpret_cast<const std::uint8_t*>(transposed_block), reinterpret_cast<std::uint8_t*>(buffered_block), ROW_LENGTH*8, BLOCK_NB_ROWS);
        END_TIMER;
        time_reorder += __integral_time;

        START_TIMER;
        block_compressor.append_block(reinterpret_cast<std::uint8_t*>(buffered_block), last_block_size);

        //Close
        block_compressor.close();
        END_TIMER;
        time_compression += __integral_time;

        metrics["time_compression(s)"] = time_compression / 1000.0;
        metrics["time_reorder(s)"] = time_reorder / 1000.0;
        
        BMS_DELETE_MATRIX(buffered_block);
        BMS_DELETE_MATRIX(transposed_block);

        munmap(mapped_file, FILE_SIZE);
        close(fd);
    }

    //Linear complexity, permute objects inplace by processing cycles
    void reorder_matrix_rows(char * mapped_file, const unsigned HEADER, const std::size_t ROW_LENGTH, const std::vector<std::uint64_t>& ORDER)
    {
        //Buffer to store a row
        char * buffer = new char[ROW_LENGTH];

        std::vector<bool> visited;
        visited.resize(ORDER.size());
    
        //Process each cycle in the permutation
        for (std::size_t i = 0; i < ORDER.size(); ++i) 
        {
            if (visited[i]) 
                continue;
            
            //Start of a new cycle
            std::size_t current = i;
            std::memcpy(buffer, GET_ROW_PTR(i), ROW_LENGTH);
            
            //Follow the cycle
            while (!visited[current]) 
            {
                visited[current] = true;
                std::size_t next = ORDER[current];
                
                if (next == i) 
                {
                    //End of cycle - place the temp value
                    std::memcpy(GET_ROW_PTR(current), buffer, ROW_LENGTH);
                } 
                else 
                {
                    //Move element and continue cycle
                    std::memcpy(GET_ROW_PTR(current), GET_ROW_PTR(next), ROW_LENGTH);
                    current = next;
                }
            }
        }

        delete[] buffer;
    }

    //Get an order that can be used to retrieve original matrix
    void reverse_order(const std::vector<std::uint64_t>& ORDER, std::vector<std::uint64_t>& reversed_order)
    {
        reversed_order.resize(ORDER.size());

        for(std::size_t i = 0; i < ORDER.size(); ++i)
            reversed_order[ORDER[i]] = i;
    }

};

#undef GET_ROW_PTR
#undef GET_BLOCK_PTR