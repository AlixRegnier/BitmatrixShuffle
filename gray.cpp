#include <gray.h>

#define IS_POWER_2(x) ((x & (x-1)) == 0)

void encodeGray(ByteWrapper& wrapped_buffer)
{
    wrapped_buffer ^= (wrapped_buffer >> 1);
}
    
unsigned get_next_power_of_two(std::uint32_t x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;

    return (unsigned)x;
}


void decodeGray(ByteWrapper& wrapped_buffer)
{
    const unsigned LENGTH = wrapped_buffer.size();

    unsigned power = get_next_power_of_two(LENGTH*8);
    while(power >>= 1)
        wrapped_buffer ^= wrapped_buffer >> power;
}