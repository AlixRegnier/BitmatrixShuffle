#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sys/mman.h>
#include <vector>
#include <cstring>

#include <bitpermute.h>

namespace Reorder 
{
    void launch(const std::string& MATRIX, const std::string& PERMUTATION_FILE, const unsigned SAMPLES, const unsigned HEADER)
    {
        int fd = open(MATRIX.c_str(), O_RDWR); //Open file in both read/write modes

        //Get file size
        const std::size_t FILE_SIZE = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        //Map file in memory
        char* mapped_file = (char*)mmap(nullptr, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        posix_madvise(mapped_file, FILE_SIZE, POSIX_MADV_SEQUENTIAL);

        //Size of matrix row in bytes
        const unsigned ROW_LENGTH = (SAMPLES+7)/8;

        //Number of matrix rows
        const std::size_t NB_ROWS = (FILE_SIZE-HEADER) / ROW_LENGTH;

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
        std::cout << "nb_rows: " << NB_ROWS << std::endl;

        //Reorder matrix
        reorder_matrix_rows(mapped_file, HEADER, ROW_LENGTH, NB_ROWS, reverse_permutation);
        
        //Unmap file in memory and close file descriptor 
        munmap(mapped_file, FILE_SIZE);
        close(fd);
    }
};

int main(int argc, char ** argv)
{
    if(argc != 5)
    {
        std::cout << "Usage: reverse_row <samples> <header> <permutation> <matrix>\n\n";
        return 1;
    }

    std::string permutation = (argv[1]);
    unsigned samples = (unsigned)atol(argv[2]);
    unsigned header = (unsigned)atol(argv[3]);
    std::string matrix = (argv[4]);

    Reorder::launch(matrix, permutation, samples, header);
}
