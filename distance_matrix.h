#ifndef DISTANCE_MATRIX_H
#define DISTANCE_MATRIX_H

#include <stdexcept>
#include <vector>

#include <utils.h>

class DistanceMatrix
{
    public:
        DistanceMatrix() = default;
        DistanceMatrix(unsigned size);

        void resize(unsigned size);
        unsigned width() const;
        double get(int x, int y) const;
        void set(int x, int y, double distance);
    private:
        std::size_t index(int x, int y) const;
        std::vector<double> matrix;
        unsigned _size;
};

#endif