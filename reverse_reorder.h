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

//Argument parser
#include <cxxopts.hpp>

//JSON parser
#include <nlohmann/json.hpp>


namespace Reorder 
{
    //Take as input the program arguments (parsed)
    void launch(const std::vector<std::string>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, std::vector<unsigned>& order);
};

#endif
