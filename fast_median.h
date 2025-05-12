#ifndef FAST_MEDIAN_H
#define FAST_MEDIAN_H

#include <vector>
#include <algorithm>

double nlogn_median(std::vector<double>& distances);
double quickselect(std::vector<double>& distances, unsigned k);
double quickselect_median(std::vector<double>& distances);
double approximate_median(std::vector<double>& distances);

#endif