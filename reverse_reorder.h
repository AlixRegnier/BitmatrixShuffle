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

#include <bitpermute.h>
namespace Reorder 
{
    //Take as input the program arguments (parsed)
    void launch(const std::vector<char*>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, const char * const ORDER);
};

#endif
