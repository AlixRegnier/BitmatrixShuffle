#include <distance_matrix.h>

//DistanceMatrix implementation
DistanceMatrix::DistanceMatrix(std::size_t size)
{
    resize(size);
}

#include <iostream>

double DistanceMatrix::get(std::size_t x, std::size_t y) const
{
    if(x == y)
        return 0.0;

    return matrix[index(x, y)];
}

void DistanceMatrix::set(std::size_t x, std::size_t y, double distance)
{
    if(x == y)
        return;

    matrix[index(x, y)] = distance;
}

void DistanceMatrix::resize(std::size_t size)
{
    _size = size;
    matrix.resize(size*(size-1)/2, NULL_DISTANCE); //Triangle matrix filled with NULL_DISTANCE
}

std::size_t DistanceMatrix::width() const
{
    return _size;
}

std::size_t DistanceMatrix::index(std::size_t x, std::size_t y) const
{    
    if(x < y)
        std::swap(x, y);

    std::size_t i = x * (x-1) / 2 + y;
    if(i >= matrix.size())
        throw std::runtime_error("ERROR: Out of range");

    return i;
}
