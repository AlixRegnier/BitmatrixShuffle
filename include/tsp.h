#ifndef TSP_H
#define TSP_H

#include <distance_matrix.h>
#include <vptree.h>
#include <utils.h>
#include <bitmatrixshuffle.h>

namespace bms
{
    struct
    {
        std::size_t index;
        double distance;
    } typedef IndexDistance;

    //Build a path by iteratively add closest vertex (compare closest from tail and closest from head), less sensitive to the first chosen vertex
    std::size_t build_double_ended_NN(const char* const MATRIX, DistanceMatrix& distanceMatrix, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<std::uint64_t>& order);

    //Distance computation between two columns
    double columns_hamming_distance(const char* const MATRIX, const std::size_t NB_ROWS, const std::size_t COLUMN_A, const std::size_t COLUMN_B);

    //Get Nearest-Neighbor using VPTree
    IndexDistance find_closest_vertex(const DistanceMatrix& DISTANCE_MATRIX, const std::size_t VERTEX, const std::vector<bool>& ALREADY_ADDED);

    //Hamming distance between two buffers
    std::size_t hamming_distance(const char* const BUFFER1, const char* const BUFFER2, const std::size_t LENGTH);
};

#endif