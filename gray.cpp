#include <gray.h>
#define IS_POWER_2(x) ((x & (x-1)) == 0)

void encodeGray(BitWrapper& wrapped_buffer)
{
    wrapped_buffer ^= (wrapped_buffer >> 1);
}

void decodeGray(BitWrapper& wrapped_buffer)
{
    const unsigned LENGTH = wrapped_buffer.get_size();

    //If mask.get_size() is a power of 2, decoding can be done more efficiently
    if(IS_POWER_2(LENGTH*8))
    {
        unsigned power = LENGTH*8;
        while(power >>= 1)
            wrapped_buffer ^= (wrapped_buffer >> power);

        return;
    }

    //Number of iterations (determined by the position of the first set bit)
    unsigned ITERATIONS = 0; 

    //Find first MSB set
    for(unsigned i = 0; i < LENGTH; ++i)
    {
        if(wrapped_buffer[i])
        {
            char b = wrapped_buffer[i];

            do
            {
                ++ITERATIONS;
            } while(b >>= 1);


            ITERATIONS = (LENGTH-i-1)*8+ITERATIONS;
            break;
        }
    }

    //Inversed Gray code of 0 is 0
    if(ITERATIONS == 0)
        return;

    BitWrapper mask = wrapped_buffer; //Wrapper on copied <wrapped_buffer>

    //Decode Gray code
    for(unsigned i = 0; i < ITERATIONS; ++i)
    {
        mask >>= 1;
        wrapped_buffer ^= mask;
    }
}