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
#include <stdexcept>
#include <sys/mman.h>
#include <random>
#include <unistd.h>
#include <vector>

//AVX2/SSE2
#include <immintrin.h>
#include <emmintrin.h>
#include <stdint.h>

//VPTree
#include <vptree.h>
#include <distance_matrix.h>

//Bit permutation
#include <bitpermute.h>

namespace Reorder 
{
    struct
    {
        unsigned index;
        double distance;
    } typedef IndexDistance;

    
    //Build a path by taking closest vertex (from tail)
    void build_NN(const char* const TRANSPOSED_MATRIX, DistanceMatrix& distanceMatrix, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<unsigned>& order);

    //Build a path by taking closest vertex (compare closest from tail and closest from head)
    void build_double_end_NN(const char* const TRANSPOSED_MATRIX, DistanceMatrix& distanceMatrix, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<unsigned>& order);
    
    //Distance computation between two columns
    double columns_hamming_distance(const char* const TRANSPOSED_MATRIX, const std::size_t MAX_ROW, const unsigned COLUMN_A, const unsigned COLUMN_B);
    
    //Nearest-Neighbor
    IndexDistance find_closest_vertex(VPTree<unsigned>& VPTREE, const unsigned VERTEX, const std::vector<bool>& ALREADY_ADDED);
        
    //Precall of SSE bitmatrix transposition, return a new buffer (should be free from memory when transposed bitmatrix is no longer needed)
    const char * const get_transposed_matrix(const char * const MAPPED_FILE, const unsigned HEADER, const unsigned ROW_LENGTH,  const std::size_t SUBSAMPLED_ROWS);

    //SSE hamming distance between two buffers
    size_t hamming_distance(const char* const BUFFER1, const char* const BUFFER2, const size_t LENGTH);
    
    //Fix order (if there are filling blank columns)
    void immutable_filling_columns_inplace(std::vector<unsigned>& order, const unsigned COLUMNS, const unsigned FILL);
    
    //Take as input the program arguments (parsed)
    void launch(const char * const REFERENCE_MATRIX, const std::vector<char*>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, const unsigned GROUPSIZE, std::size_t subsampled_rows, const char * const OUT_ORDER, const char * const OUT_ROW_ORDER);

    //Start multiple path TSP instances to be solved using Nearest-Neighbor
    void TSP_NN(const char* const transposed_matrix, const unsigned COLUMNS, const unsigned GROUPSIZE, const std::size_t SUBSAMPLED_ROWS, std::vector<unsigned>& order);
};

#endif
