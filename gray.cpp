#include <gray.h>
#define IS_POWER_2(x) ((x & (x-1)) == 0)

std::uint64_t to_gray_uint64(std::uint64_t previous, std::uint64_t current)
{
    //sizeof(std::uint64_t)*8 - 1 = 63
    return current ^ ((previous << 63) | (current >> 1));
}

std::uint8_t to_gray_uint8(std::uint8_t previous, std::uint8_t current)
{
    //sizeof(char)*8 - 1 = 7
    return current ^ ((previous << 7) | (current >> 1));
}

std::uint64_t gray_to_binary_uint64(std::uint64_t x) 
{
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return x;
}

std::uint8_t gray_to_binary_uint8(std::uint8_t x) 
{
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return x;
}

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

void encodeGray(char* buffer, const unsigned LENGTH)
{
    if(buffer == nullptr)
        throw std::invalid_argument("Input pointer cannot be null");

    size_t i = 0;
    uint64_t previous_a = 0;
    uint64_t* a;

    for (; i + 8 <= LENGTH; i += 8) 
    {
        //Get next 64-bit integer
        a = reinterpret_cast<uint64_t*>(buffer + i);

        //Compute Gray code
        uint64_t tmp = to_gray_uint64(previous_a, *a);

        //Store 64-bit integer
        previous_a = *a;

        //Replace by Gray code
        *a = tmp;
    }

    char previous_a_chr = (char)(previous_a & 0xFF);
    char* a_chr;

    // Handle remaining bytes
    for (; i < LENGTH; ++i) 
    {
        //Get next 8-bit integer
        a_chr = buffer+i;

        //Compute Gray code
        char tmp = (char)to_gray_uint8(previous_a_chr, *a_chr);

        //Store 8-bit integer
        previous_a_chr = *a_chr;

        //Replace by Gray code
        *a_chr = tmp;
    }
}
/*
void decodeGray(char* buffer, const unsigned LENGTH)
{
    if(buffer == nullptr)
        throw std::invalid_argument("Input pointer cannot be null");

    size_t i = 0;
    uint64_t previous_a = 0;
    uint64_t* a;

    for (; i + 8 <= LENGTH; i += 8) 
    {
        //Get next 64-bit integer
        a = reinterpret_cast<uint64_t*>(buffer + i);

        //Decode Gray code
        uint64_t tmp = gray_to_binary_uint64(previous_a, *a);

        //Store 64-bit integer
        previous_a = *a;

        //Replace by decoded code
        *a = tmp;
    }

    char previous_a_chr = (char)(previous_a & 0xFF);
    char* a_chr;

    // Handle remaining bytes
    for (; i < LENGTH; ++i) 
    {
        //Get next 8-bit integer
        a_chr = buffer+i;

        //Decode Gray code
        char tmp = (char)gray_to_binary_uint8(previous_a_chr, *a_chr);

        //Store 8-bit integer
        previous_a_chr = *a_chr;

        //Replace by decoded code
        *a_chr = tmp;
    }
}
*/

void decodeGray(char* buffer, const unsigned LENGTH)
{
    if(buffer == nullptr)
        throw std::invalid_argument("Input pointer cannot be null");

    size_t i = 0;
    size_t ITERATIONS = 0;

    //Find first MSB set
    for(; i < LENGTH; ++i)
    {
        if(buffer[i])
        {
            char b = buffer[i];

            while(b >>= 1)
                ++ITERATIONS;

            ITERATIONS = (LENGTH-i)*8-ITERATIONS;
            
            break;
        }
    }

    //Inversed Gray code of 0 is 0
    if(ITERATIONS == 0)
        return;

    BitPacker wrapped_buffer(buffer, LENGTH, false); //Wrapper on <buffer> memory

    //If LENGTH is a power of 2, decoding can be done more efficiently
    if(IS_POWER_2(LENGTH))
    {
        unsigned power = LENGTH;
        while(power >>= 1)
            wrapped_buffer ^= wrapped_buffer >> power;

        return;
    }

    BitPacker copied_buffer(buffer, LENGTH, true); //Wrapper on copied <buffer>

    wrapped_buffer.fill('\0'); //Set all bytes to null

    //Decode Gray code
    i = 0;
    while(i++ < ITERATIONS)
    {
        wrapped_buffer ^= copied_buffer;
        copied_buffer >>= 1;
    }
}