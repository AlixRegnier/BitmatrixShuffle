#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sys/mman.h>
#include <vector>
#include <cstring>

namespace Reorder 
{
    //Functions needed
    char get_bit_from_position(const char * const BYTES, const unsigned POSITION)
    {
        //Equivalent instruction that is compiled to the same assembly code than second one
        //return (bytes[position >> 3] >> (8 - (position % 8) - 1)) & (char)1;

        return (BYTES[POSITION >> 3] >> (~POSITION & 0x7U)) & (char)1;
    }


    void permute_buffer_order(const char * const BUFFER, char * outBuffer, const unsigned * const ORDER, const unsigned ORDER_LENGTH)
    {
        //Permute bits within bytes of a buffer according to a given order
        //order[i] tells that the position i has to store the bit at position order[i]

        //Inplace permutation should be possible but would need more operations to save overwritten bits
        for(unsigned i = 0; i < ORDER_LENGTH; ++i)
            outBuffer[i >> 3] = (outBuffer[i >> 3] << 1) | get_bit_from_position(BUFFER, ORDER[i]);
    }

    void reorder_matrix(char * mapped_file, const unsigned HEADER, const unsigned NB_ROWS, const unsigned ROW_LENGTH, const std::vector<unsigned>& PERMUTATION)
    {
        //Buffer to store a row
        char * buffer = new char[ROW_LENGTH];

        std::vector<unsigned> permutation_copy;
        permutation_copy.assign(PERMUTATION.begin(), PERMUTATION.end());

        //Permutate rows
        for(unsigned i = 0; i < NB_ROWS; ++i)
        {
            if(permutation_copy[i] == i)
                continue;

            
            char * i_ptr = mapped_file+HEADER+i*ROW_LENGTH;

            //Save current element into buffer
            std::memcpy(buffer, i_ptr, ROW_LENGTH);

            //Cycle
            unsigned j = i;
            char * j_ptr = mapped_file+HEADER+j*ROW_LENGTH;
            while(permutation_copy[j] != i)
            {
                unsigned next_j = permutation_copy[j];
                char * next_j_ptr = mapped_file+HEADER+next_j*ROW_LENGTH;

                std::memcpy(j_ptr, next_j_ptr, ROW_LENGTH);
                permutation_copy[j] = j;

                j = next_j;
                j_ptr = next_j_ptr;
            }
            
            std::memcpy(j_ptr, buffer, ROW_LENGTH);
            permutation_copy[j] = j;
        }

        delete[] buffer;
    }

    void launch(const std::string& MATRIX, const std::string& PERMUTATION_FILE, const unsigned SAMPLES, const unsigned HEADER)
    {
        int fd = open(MATRIX.c_str(), O_RDWR); //Open file in both read/write modes

        //Get file size
        const long FILE_SIZE = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        //Map file in memory
        char* mapped_file = (char*)mmap(nullptr, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        posix_madvise(mapped_file, FILE_SIZE, POSIX_MADV_SEQUENTIAL);

        //Size of matrix row in bytes
        const unsigned ROW_LENGTH = (SAMPLES+7)/8;

        //Number of matrix rows
        const unsigned NB_ROWS = (FILE_SIZE-HEADER) / ROW_LENGTH;

        //Store final permutation (store new row index)
        std::vector<unsigned> permutation;
        permutation.resize(NB_ROWS);


        int fd_permutation = open(PERMUTATION_FILE.c_str(), O_RDONLY);
        read(fd_permutation, reinterpret_cast<char*>(permutation.data()), sizeof(unsigned)*permutation.size());
        close(fd_permutation);

        std::vector<unsigned> reverse_permutation;
        reverse_permutation.resize(NB_ROWS);

        //Compute reverse permutation
        for(unsigned i = 0; i < NB_ROWS; ++i)
            reverse_permutation[permutation[i]] = i;

        std::cout << "row_length: " << ROW_LENGTH << std::endl;
        std::cout << "file_size: " << FILE_SIZE << std::endl;
        std::cout << "header: " << HEADER << std::endl;
        std::cout << "pi size: " << permutation.size() << std::endl;

        //Reorder matrix
        reorder_matrix(mapped_file, HEADER, NB_ROWS, ROW_LENGTH, reverse_permutation);
        
        //Unmap file in memory and close file descriptor 
        munmap(mapped_file, FILE_SIZE);
        close(fd);
    }
};

int main(int argc, char ** argv)
{
    if(argc != 5)
    {
        std::cout << "Usage: reverse_row_reorder <matrix> <samples> <header> <permutation>\n\n";
        return 1;
    }

    std::string matrix = (argv[1]);
    unsigned samples = (unsigned)atol(argv[2]);
    unsigned header = (unsigned)atol(argv[3]);
    std::string permutation = (argv[4]);

    Reorder::launch(matrix, permutation, samples, header);
}
