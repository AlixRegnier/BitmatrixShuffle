#ifndef REORDER_H
#define REORDER_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <sys/mman.h>
#include <random>
#include <unistd.h>
#include <vector>

//SIMD transposition
#include <emmintrin.h>
#include <stdint.h>

//VPTree
#include <vptree.h>
#include <distance_matrix.h>

namespace Reorder 
{
    struct
    {
        int index;
        double distance;
    } typedef IndexDistance;

    
    //Build a path by taking closest vertex (from tail)
    void build_NN(const char* const TRANSPOSED_MATRIX, DistanceMatrix * DISTANCE_MATRIX, const unsigned SUBSAMPLED_ROWS, const unsigned OFFSET, std::vector<int>& order);

    //Build a path by taking closest vertex (compare closest from tail and  closest from head)
    void build_double_end_NN(const char* const  TRANSPOSED_MATRIX, DistanceMatrix * DISTANCE_MATRIX, const unsigned SUBSAMPLED_ROWS, const unsigned OFFSET, std::vector<int>& order);
    
    //Distance computation between two columns
    double columns_hamming_distance(const char* const  TRANSPOSED_MATRIX, const unsigned MAX_ROW, const unsigned COLUMN_A, const unsigned COLUMN_B);
    
    //Nearest-Neighbor
    IndexDistance find_closest_vertex(VPTree<int>& VPTREE, const unsigned VERTEX, const std::vector<bool>& ALREADY_ADDED);
        
    //Extract bit from a buffer of bytes (big-endian), result can only be 0x00 or 0x01
    char get_bit_from_position(const char* const BYTES, const unsigned POSITION);
    
    //Precall of SSE bitmatrix transposition, return a new buffer (should be free from memory when transposed bitmatrix is no longer needed)
    const char * const get_transposed_matrix(const char * const MAPPED_FILE, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const unsigned NB_GROUPS, const unsigned GROUPSIZE, const unsigned long SUBSAMPLEDROWS, std::vector<DistanceMatrix*>& distanceMatrices);

    //SSE hamming distance between two buffers
    size_t hamming_distance(const char* const BUFFER1, const char* const BUFFER2, const size_t LENGTH);
    
    //Fix order (if there are filling blank columns)
    void immutable_filling_columns_inplace(std::vector<int>& order, const unsigned COLUMNS, const unsigned FILL);
    
    //Take as input the program arguments (parsed)
    void launch(const std::string& MATRIX, const unsigned SAMPLES, const unsigned HEADER, const unsigned GROUPSIZE, unsigned subsampled_rows);

    //Swaps the bits of a buffer into another buffer according to given order
    void permute_buffer_order(const char* const BUFFER, char* outBuffer, const int* const ORDER, const unsigned ORDER_LENGTH);

    //Reorder matrix (bit-swapping on memory-mapped file)
    void reorder_matrix(char* mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const unsigned NB_ROWS, const std::vector<int>& ORDER);

    //Start multiple path TSP instances to be solved using Nearest-Neighbor
    void TSP_NN(const char* const transposed_matrix, const std::vector<DistanceMatrix*>& DISTANCE_MATRICES, const unsigned SUBSAMPLED_ROWS, std::vector<int>& order);
};

#endif
