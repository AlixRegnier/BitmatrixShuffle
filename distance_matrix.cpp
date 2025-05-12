#include <distance_matrix.h>

//DistanceMatrix implementation
DistanceMatrix::DistanceMatrix(unsigned size)
{
    resize(size);
}

#include <iostream>

double DistanceMatrix::get(int x, int y) const
{
    if(x == y)
        return 0.0;

    return matrix[index(x, y)];
}

void DistanceMatrix::set(int x, int y, double distance)
{
    if(x == y)
        return;

    matrix[index(x, y)] = distance;
}

void DistanceMatrix::resize(unsigned size)
{
    _size = size;
    matrix.resize(size*(size-1)/2, NULL_DISTANCE); //Temporary to full matrix space, will later be encoded in triangle matrix
}

unsigned DistanceMatrix::width() const
{
    return _size;
}

std::size_t DistanceMatrix::index(int x, int y) const
{    
    if(x < y)
        std::swap(x, y);

    std::size_t i = x * (x-1) / 2 + y;
    if(i >= matrix.size())
        throw std::runtime_error("ERROR: Out of range");

    return i;
}
