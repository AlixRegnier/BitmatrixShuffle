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

        //Swap columns in each rows
        for(std::size_t i = 0; i < NB_ROWS; ++i)
        {
            std::memcpy(buffer, GET_ROW_PTR(i), ROW_LENGTH);
            
            //Swap row bits
            permute_buffer_order(buffer, GET_ROW_PTR(i), ORDER.data(), COLUMNS);
        }

        delete[] buffer;
    }

    void reorder_matrix_rows(char * mapped_file, const unsigned HEADER, const unsigned ROW_LENGTH, const std::size_t NB_ROWS, const std::vector<unsigned>& ORDER)
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
            
            // Start of a new cycle
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
                    // Move element and continue cycle
                    std::memcpy(GET_ROW_PTR(current), GET_ROW_PTR(next), ROW_LENGTH);
                    current = next;
                }
            }
        }

        delete[] buffer;
    }
};