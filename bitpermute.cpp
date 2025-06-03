#include <bitpermute.h>

namespace Reorder
{
    //Retrieve bit at given POSITION in buffer BYTES
    char get_bit_from_position(const char * const BYTES, const unsigned POSITION)
    {
        //Equivalent instruction that is compiled to the same assembly code than second one
        //return (bytes[position >> 3] >> (8 - (position % 8) - 1)) & (char)1;

        return (BYTES[POSITION >> 3] >> (~POSITION & 0x7U)) & (char)1;
    }

    //Swaps the bits of a buffer into another buffer according to given order)
    void permute_buffer_order(const char * const BUFFER, char * outBuffer, const unsigned * const ORDER, const unsigned ORDER_LENGTH)
    {
        //Permute bits within bytes of a buffer according to a given order
        //order[i] tells that the position i has to store the bit at position order[i]

        for(unsigned i = 0; i < ORDER_LENGTH; ++i)
            outBuffer[i >> 3] = (outBuffer[i >> 3] << 1) | get_bit_from_position(BUFFER, ORDER[i]);
    }

    //Reorder matrix (bit-swapping on memory-mapped file)
    void reorder_matrix_columns(char * mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const std::size_t NB_ROWS, const std::vector<unsigned>& ORDER)
    {
        //Buffer to copy a row
        char * buffer = new char[ROW_LENGTH];

        std::size_t index = 0;

        //Swap columns in each rows
        for(std::size_t i = 0; i < NB_ROWS; ++i)
        {
            std::memcpy(buffer, mapped_file+index+HEADER, ROW_LENGTH);
            
            //Swap row bits
            permute_buffer_order(buffer, mapped_file+index+HEADER, ORDER.data(), COLUMNS);
            index += ROW_LENGTH;
        }

        delete[] buffer;
    }

    void reorder_matrix_rows(char * mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const std::size_t NB_ROWS, const std::vector<unsigned>& ORDER)
    {
        //Buffer to store a row
        char * buffer = new char[ROW_LENGTH];

        std::vector<unsigned> permutation_copy;
        permutation_copy.assign(ORDER.begin(), ORDER.end());

        //Permutate rows
        for(unsigned i = 0; i < NB_ROWS; ++i)
        {
            if(permutation_copy[i] == i)
                continue;

            
            char * i_ptr = mapped_file+HEADER+i*ROW_LENGTH;

            //Save current element into buffer
            std::memcpy(buffer, i_ptr, ROW_LENGTH);

            //Cycle
            unsigned j = i;
            char * j_ptr = mapped_file+HEADER+j*ROW_LENGTH;
            while(permutation_copy[j] != i)
            {
                unsigned next_j = permutation_copy[j];
                char * next_j_ptr = mapped_file+HEADER+next_j*ROW_LENGTH;
                
                std::memcpy(j_ptr, next_j_ptr, ROW_LENGTH);
                permutation_copy[j] = j;

                j = next_j;
                j_ptr = next_j_ptr;
            }
            
            std::memcpy(j_ptr, buffer, ROW_LENGTH);
            permutation_copy[j] = j;
        }

        delete[] buffer;
    }
};