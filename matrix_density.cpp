#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

#define BUFFER_SIZE 8192

int main(int argc, char ** argv)
{

    if(argc != 3)
    {
        std::cout << "Usage: matrix_density <matrix> <header>" << std::endl;
        return 1;
    }

    //Number of bit set in an unsigned char [0-255]
    const unsigned char TABLE[] = {
        0,1,1,2,1,2,2,3,
        1,2,2,3,2,3,3,4,
        1,2,2,3,2,3,3,4,
        2,3,3,4,3,4,4,5,
        1,2,2,3,2,3,3,4,
        2,3,3,4,3,4,4,5,
        2,3,3,4,3,4,4,5,
        3,4,4,5,4,5,5,6,
        1,2,2,3,2,3,3,4,
        2,3,3,4,3,4,4,5,
        2,3,3,4,3,4,4,5,
        3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,
        3,4,4,5,4,5,5,6,
        3,4,4,5,4,5,5,6,
        4,5,5,6,5,6,6,7,
        1,2,2,3,2,3,3,4,
        2,3,3,4,3,4,4,5,
        2,3,3,4,3,4,4,5,
        3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,
        3,4,4,5,4,5,5,6,
        3,4,4,5,4,5,5,6,
        4,5,5,6,5,6,6,7,
        2,3,3,4,3,4,4,5,
        3,4,4,5,4,5,5,6,
        3,4,4,5,4,5,5,6,
        4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,
        4,5,5,6,5,6,6,7,
        4,5,5,6,5,6,6,7,
        5,6,6,7,6,7,7,8
    };


    std::string input = argv[1];
    const unsigned HEADER = (unsigned)atol(argv[2]);

    int fdin = open(argv[1], O_RDONLY);

    if(fdin < 0)
        throw std::runtime_error("Didn't manage to open: '" + input + "'");

    lseek(fdin, HEADER, SEEK_SET);

    char buffer[BUFFER_SIZE];
    unsigned read_bytes = 0;
    std::uint64_t ones = 0;
    std::uint64_t total_read_bytes = 0;

    while((read_bytes = read(fdin, buffer, BUFFER_SIZE)))
    {
        for(unsigned i = 0; i < read_bytes; ++i)
            ones += TABLE[(unsigned char)buffer[i]];
        total_read_bytes += read_bytes;
    }

    std::uint64_t total_read_bits = total_read_bytes*8;
    std::uint64_t zeroes = total_read_bits - ones;
    long double density = ((long double)(ones) / ((long double)(total_read_bits)));

    std::cout << "Read bytes:\t" << total_read_bytes << "\nRead bits:\t" << total_read_bits << "\n#ones:\t\t" << ones << "\n#zeroes:\t" << zeroes << "\nDensity:\t" << density << std::endl;
    
    close(fdin);
}
