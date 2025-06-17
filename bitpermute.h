#ifndef BITPERMUTE_H
#define BITPERMUTE_H

#include <vector>
#include <cstring>

#define GET_ROW_PTR(x) (mapped_file+HEADER+((std::size_t)(x))*ROW_LENGTH)

namespace Reorder
{
    //Extract bit from a buffer of bytes (big-endian), result can only be 0x00 or 0x01
    char get_bit_from_position(const char* const BYTES, const unsigned POSITION);
    
    //Swaps the bits of a buffer into another buffer according to given order
    void permute_buffer_order(const char* const BUFFER, char* outBuffer, const unsigned* const ORDER, const unsigned ORDER_LENGTH);
    
    //Reorder matrix columns (bit-swapping on memory-mapped file)
    void reorder_matrix_columns(char* mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const std::size_t NB_ROWS, const std::vector<unsigned>& ORDER);

    //Reorder matrix rows (row-swapping on memory-mapped file)
    void reorder_matrix_rows(char* mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const std::size_t NB_ROWS, const std::vector<unsigned>& ORDER);
};   

#endif