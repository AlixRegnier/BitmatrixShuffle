#ifndef GRAY_H
#define GRAY_H

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <bitpacker.h>

std::uint64_t to_gray_uint64(std::uint64_t previous, std::uint64_t current);

std::uint8_t to_gray_uint8(std::uint8_t previous, std::uint8_t current);

std::uint64_t gray_to_binary_uint64(std::uint64_t previous, std::uint64_t current);

std::uint8_t gray_to_binary_uint8(std::uint8_t previous, std::uint8_t current);

//Code on fly to Gray code and then compare
/*bool grayCompare(const char* const BUFFER1, const char* const BUFFER2, const size_t LENGTH) 
{
    if (BUFFER1 == nullptr || BUFFER2 == nullptr)
        throw std::invalid_argument("Input pointers cannot be null");

    size_t i = 0;
    uint64_t previous_a = previous_b = 0;
    uint64_t a, b;

    for (; i + 8 <= LENGTH; i += 8) 
    {
        a = *reinterpret_cast<uint64_t*>(BUFFER1 + i);
        b = *reinterpret_cast<uint64_t*>(BUFFER2 + i);
        
        if(grayCode(previous_a, a) < grayCode(previous_b, b))
            return true;

        if(grayCode(previous_a, a) > grayCode(previous_b, b))
            return false;

        previous_a = a;
        previous_b = b;
    }

    unsigned char previous_a_chr = (unsigned char)(previous_a & 0xFF);
    unsigned char previous_b_chr = (unsigned char)(previous_b & 0xFF);
    unsigned char a_chr, b_chr;

    // Handle remaining bytes
    for (; i < LENGTH; ++i) 
    {
        a_chr = (unsigned char)BUFFER1[i];
        b_chr = (unsigned char)BUFFER2[i];

        if(grayCode(previous_a_chr, a_chr) < grayCode(previous_b_chr, b_chr))
            return true;

        if(grayCode(previous_a_chr, a_chr) > grayCode(previous_b_chr, b_chr))
            return false;

        previous_a_chr = a_chr;
        previous_b_chr = b_chr;
    }

    return false;
}*/

void encodeGray(char* buffer, const unsigned LENGTH);

void decodeGray(char* buffer, const unsigned LENGTH);

#endif