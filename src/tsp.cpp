#include <tsp.h>
#include <deque>

//AVX2/SSE2
#include <immintrin.h>
#include <emmintrin.h>
#include <stdint.h>

namespace bms {

    //TSP path, returns the number of computed distances
    std::size_t build_NN(const char* const MATRIX, DistanceMatrix& distanceMatrix, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<std::uint64_t>& order)
    {
        //Pick a random first vertex
        std::uint64_t firstVertex = RNG::rand_uint32_t(0, distanceMatrix.width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(distanceMatrix.width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::vector<std::uint64_t> path = {firstVertex};
        path.reserve(distanceMatrix.width());

        //Build vector of indices for VPTree
        std::vector<std::uint64_t> vertices;
        vertices.resize(distanceMatrix.width());
        for(std::size_t i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        std::size_t counter = 0;

        DistanceFunctions df = VPTree<std::uint64_t>::bind_distance_functions(
            [=, &counter](std::uint64_t a, std::uint64_t b) -> double {
                ++counter;
                return columns_hamming_distance(MATRIX, SUBSAMPLED_ROWS, a+OFFSET, b+OFFSET);
            },
            [&distanceMatrix](std::uint64_t a, std::uint64_t b) -> double { return distanceMatrix.get(a, b); },
            [&distanceMatrix](std::uint64_t a, std::uint64_t b, double d) { distanceMatrix.set(a, b, d); }
        );

        VPTree<std::uint64_t> root(vertices, &df);

        //Added second vertex to data structures
        //Find next vertices to add by checking which is the minimum to take
        for(std::size_t i = 1; i < distanceMatrix.width(); ++i)
        {
            IndexDistance match = find_closest_vertex(root, path[i-1], alreadyAdded);
            alreadyAdded[match.index] = true;
            path.push_back(match.index);
        }

        //std::cout << "\tComputed distances (VPTree): " << counter << "/" << (distanceMatrix.width() * (distanceMatrix.width() - 1) / 2) <<  std::endl;

        //Store global order
        for(std::size_t i = 0; i < distanceMatrix.width(); ++i)
            order[i+OFFSET] = path[i] + OFFSET; //Add offset because columns are addressed by their global location
    
        return counter;
    }


    //TSP path filled by both ends, less sensitive of the first chosen vertex, returns the number of computed distances
    std::size_t build_double_ended_NN(const char* const MATRIX, DistanceMatrix& distanceMatrix, const std::size_t SUBSAMPLED_ROWS, const std::size_t OFFSET, std::vector<std::uint64_t>& order)
    {
        //Pick a random first vertex
        std::uint64_t firstVertex = RNG::rand_uint32_t(0, distanceMatrix.width());
        
        //Vector of added vertices (set true for the first vertex)
        std::vector<bool> alreadyAdded;
        alreadyAdded.resize(distanceMatrix.width());
        alreadyAdded[firstVertex] = true;

        //Deque for building path with first vertex as starting point
        std::deque<std::uint64_t> orderDeque = {firstVertex};

        //Build vector of indices for VPTree
        std::vector<std::uint64_t> vertices;
        vertices.resize(distanceMatrix.width());
        for(std::size_t i = 0; i < vertices.size(); ++i)
            vertices[i] = i;

        std::size_t counter = 0;

        //Use counter to count how many distance computation could be avoided by using a VPTree
        DistanceFunctions df = VPTree<std::uint64_t>::bind_distance_functions(
            [=, &counter](std::uint64_t a, std::uint64_t b) -> double {
                ++counter;
                return columns_hamming_distance(MATRIX, SUBSAMPLED_ROWS, a+OFFSET, b+OFFSET);
            },
            [&distanceMatrix](std::uint64_t a, std::uint64_t b) -> double { return distanceMatrix.get(a, b); },
            [&distanceMatrix](std::uint64_t a, std::uint64_t b, double d) { distanceMatrix.set(a, b, d); }
        );

        VPTree<std::uint64_t> root(vertices, &df);

        //Find second vertex
        IndexDistance second = find_closest_vertex(root, firstVertex, alreadyAdded);
        
        //Added second vertex to data structures
        orderDeque.push_back(second.index);
        alreadyAdded[second.index] = true;

        //Find closest vertices from path front and back
        IndexDistance a = find_closest_vertex(root, orderDeque.front(), alreadyAdded);
        IndexDistance b = find_closest_vertex(root, orderDeque.back(), alreadyAdded);

        //Find next vertices to add by checking which is the minimum to take
        for(std::size_t i = 2; i < distanceMatrix.width(); ++i)
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

        //std::cout << "\tComputed distances (VPTree): " << counter << "/" << (distanceMatrix.width() * (distanceMatrix.width() - 1) / 2) <<  std::endl;

        //Store global order
        for(std::size_t i = 0; i < distanceMatrix.width(); ++i)
            order[i+OFFSET] = orderDeque[i] + OFFSET; //Add offset because columns are addressed by their global location
        
        return counter;
    }

    IndexDistance find_closest_vertex(VPTree<std::uint64_t>& VPTREE, const std::uint64_t VERTEX, const std::vector<bool>& ALREADY_ADDED)
    {
        IndexDistance nn = {0, 2.0};
        VPTREE.get_unvisited_nearest_neighbor(VERTEX, ALREADY_ADDED, &nn.distance, &nn.index);

        return nn;
    }

    //AVX2 implementation of Hamming distance
    size_t hamming_distance(const char* const BUFFER1, const char* const BUFFER2, const size_t LENGTH) 
    {
        if (BUFFER1 == nullptr || BUFFER2 == nullptr)
            throw std::invalid_argument("BMS-ERROR: Input pointers cannot be null to compute Hamming distance");

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

    double columns_hamming_distance(const char* const transposed_matrix, const std::size_t MAX_ROW, const std::uint64_t COLUMN_A, const std::uint64_t COLUMN_B)
    {
        return 1.0*hamming_distance(transposed_matrix + (COLUMN_A)*(MAX_ROW/8), transposed_matrix + (COLUMN_B)*(MAX_ROW/8), MAX_ROW/8) / MAX_ROW;
    }
};