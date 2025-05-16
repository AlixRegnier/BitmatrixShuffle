#ifndef REORDER_H
#define REORDER_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

namespace Reorder 
{
    //Extract bit from a buffer of bytes (big-endian), result can only be 0x00 or 0x01
    char get_bit_from_position(const char* const BYTES, const unsigned POSITION);
    
    //Take as input the program arguments (parsed)
    void launch(const std::vector<char*>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, const char * const ORDER);

    //Swaps the bits of a buffer into another buffer according to given order
    void permute_buffer_order(const char* const BUFFER, char* outBuffer, const unsigned* const ORDER, const unsigned ORDER_LENGTH);

    //Reorder matrix (bit-swapping on memory-mapped file)
    void reorder_matrix(char* mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const long unsigned NB_ROWS, const std::vector<unsigned>& ORDER);
};

#endif
