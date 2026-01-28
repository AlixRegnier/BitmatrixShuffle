#ifndef VPTREE_IMPL_H
#define VPTREE_IMPL_H

#include <vptree.h>

namespace bms 
{
    template <class T>
    //Modification: distances vector is copied when median needs to be computed (nlogn sort), then we can retrieve corresponding distances in same order
    //distances vector scope is bigger because used for vertex left/right assignation
    //We don't distFunct->get anymore, set to return BMS_NULL_DISTANCE. 
    void VPTree<T>::init(const std::vector<T>& vertices)
    {
        if(vertices.size() == 0)
            throw std::runtime_error("ERROR: Attempted to initialize VPTree (or one of its nodes) with no elements");

        if(vertices.size() == 1)
        {
            pivot = vertices[0];
            return;
        }

        //Select a random vertex as pivot
        std::size_t pivotIndex = RNG::rand_uint32_t(0, vertices.size());
        pivot = vertices[pivotIndex];

        //Distance scope
        {
            std::vector<double> distances;
            distances.reserve(vertices.size() - 1);

            //Before pivot
            for(std::size_t i = 0; i < pivotIndex; ++i)
            {
                double d = distFunc->compute(pivot, vertices[i]);
                distances.push_back(d);
            }

            //After pivot
            for(std::size_t i = pivotIndex+1; i < vertices.size(); ++i)
            {
                double d = distFunc->compute(pivot, vertices[i]);
                distances.push_back(d);
            }

            //nlog(n) median but should be quick as it needs to sort small lists which size decrease
            threshold = nlogn_median(distances);
            //threshold = quickselect_median(distances);

            //Divide space by two
            std::vector<T> leftVertices, rightVertices;
            leftVertices.reserve(vertices.size()/2);
            rightVertices.reserve(vertices.size()/2);

            //Before pivot
            for(std::size_t i = 0; i < pivotIndex; ++i)
            {
                if(distances[i] < threshold)
                    leftVertices.push_back(vertices[i]);
                else
                    rightVertices.push_back(vertices[i]);
            }

            //After pivot
            for(std::size_t i = pivotIndex+1; i < vertices.size(); ++i)
            {
                if(distance[i-1] < threshold) //Shifted by one because pivot 
                    leftVertices.push_back(vertices[i]);
                else
                    rightVertices.push_back(vertices[i]);
            }
        }

        if(leftVertices.size() > 0)
            left = new VPTree(leftVertices, distFunc);

        if(rightVertices.size() > 0)
            right = new VPTree(rightVertices, distFunc);
    }

    template <class T>
    VPTree<T>::VPTree(const std::vector<T>& vertices, DistanceFunctions<T>* distFunc)
    {
        this->distFunc = distFunc;
        init(vertices);
    }

    template <class T>
    VPTree<T>::~VPTree()
    {
        if(left != nullptr)
            delete left;

        if(right != nullptr)
            delete right;

        left = right = nullptr;
    }

    template <class T>
    DistanceFunctions<T> VPTree<T>::bind_distance_functions(std::function<double(T, T)> computeDistFunc, std::function<double(T, T)> getDistFunc, std::function<void(T, T, double)> storeDistFunc)
    {
        DistanceFunctions<T> df = {computeDistFunc, getDistFunc, storeDistFunc };
        return df;
    }

    template <class T>
    void VPTree<T>::get_unvisited_nearest_neighbor(T query, const std::vector<bool>& alreadyAdded, double* tau, T* currentResult)
    {
        if(query < 0)
            throw std::runtime_error("ERROR: Can't query invalid vertex");

        //Check if distance already has been computed
        double distance = distFunc->compute(pivot, query);

        if(!alreadyAdded[pivot] && distance < *tau) //See if it prevents algorithm from converging (since tau is not updated), it shouldn't as there are no cycles
        {
            *tau = distance;
            *currentResult = pivot;
        }

        if(distance < threshold)
        {
            if(left != nullptr && !left->skip && (distance - *tau) <= threshold)
                left->get_unvisited_nearest_neighbor(query, alreadyAdded, tau, currentResult);

            if(right != nullptr && !right->skip && (distance + *tau) >= threshold)
                right->get_unvisited_nearest_neighbor(query, alreadyAdded, tau, currentResult);
        }
        else
        {
            if(right != nullptr && !right->skip && (distance + *tau) >= threshold)
                right->get_unvisited_nearest_neighbor(query, alreadyAdded, tau, currentResult);

            if(left != nullptr && !left->skip && (distance - *tau) <= threshold)
                left->get_unvisited_nearest_neighbor(query, alreadyAdded, tau, currentResult);
        }

        //Node can be skipped if both children can be skipped and current pivot was already added
        skip = (left == nullptr || left->skip) && (right == nullptr || right->skip) && alreadyAdded[pivot];
    }
};

#endif