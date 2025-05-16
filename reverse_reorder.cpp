#include <reverse_reorder.h>

namespace Reorder 
{
    //Functions needed
    char get_bit_from_position(const char * const BYTES, const unsigned POSITION)
    {
        //Equivalent instruction that is compiled to the same assembly code than second one
        //return (bytes[position >> 3] >> (8 - (position % 8) - 1)) & (char)1;

        return (BYTES[POSITION >> 3] >> (~POSITION & 0x7U)) & (char)1;
    }

    void launch(const std::vector<char*>& MATRICES, const unsigned SAMPLES, const unsigned HEADER, const char * const ORDER)
    {
        if(SAMPLES == 0)
            throw std::runtime_error("SAMPLES can't be equal to 0");

        if(MATRICES.size() == 0)
            throw std::runtime_error("Got empty vector of matrix path");

        //Number of matrix columns
        const unsigned COLUMNS = (SAMPLES+7)/8*8;

        //Size of matrix row in bytes
        const unsigned ROW_LENGTH = (SAMPLES+7)/8;

        //Deserialize binary order
        int fdorder = open(ORDER, O_RDONLY);

        if(fdorder < 0)
            throw std::runtime_error("Failed to open a file descriptor on serialized order");

        std::vector<unsigned> reversed_order;
        reversed_order.resize(COLUMNS);

        {
            std::vector<unsigned> order;
            order.resize(COLUMNS);

            read(fdorder, reinterpret_cast<char*>(order.data()), COLUMNS*sizeof(unsigned));
            close(fdorder);

            //Reverse order
            for(unsigned i = 0; i < COLUMNS; ++i)
                reversed_order[order[i]] = i;
        }

        //Reorder all matrices
        for(unsigned i = 0; i < MATRICES.size(); i++)
        {
            int fd = open(MATRICES[i], O_RDWR);

            if(fd < 0)
                throw std::runtime_error("Failed to open a file descriptor on matrix");

            //Get file size
            const long unsigned FILE_SIZE = lseek(fd, 0, SEEK_END);
            
            //Number of matrix rows
            const long unsigned NB_ROWS = (FILE_SIZE - HEADER) / ROW_LENGTH;

            lseek(fd, 0, SEEK_SET);
            
            std::cout << "\rReordering matrix " << (i+1) << "/" << MATRICES.size() << " ..." << std::flush;

            char* mapped_file = (char*)mmap(nullptr, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

             //Tell system that data will be accessed sequentially
            posix_madvise(mapped_file, FILE_SIZE, POSIX_MADV_SEQUENTIAL);

            //Reorder matrix columns
            reorder_matrix(mapped_file, HEADER, COLUMNS, ROW_LENGTH, NB_ROWS, reversed_order);
                    
            //Unmap file in memory and close file descriptor 
            munmap(mapped_file, FILE_SIZE);
            close(fd);
        }
        std::cout << std::endl;
    }

    void permute_buffer_order(const char * const BUFFER, char * outBuffer, const unsigned * const ORDER, const unsigned ORDER_LENGTH)
    {
        //Permute bits within bytes of a buffer according to a given order
        //order[i] tells that the position i has to store the bit at position order[i]

        for(unsigned i = 0; i < ORDER_LENGTH; ++i)
            outBuffer[i >> 3] = (outBuffer[i >> 3] << 1) | get_bit_from_position(BUFFER, ORDER[i]);
    }

    void reorder_matrix(char * mapped_file, const unsigned HEADER, const unsigned COLUMNS, const unsigned ROW_LENGTH, const long unsigned NB_ROWS, const std::vector<unsigned>& ORDER)
    {
        //Buffer to copy a row
        char * buffer = new char[ROW_LENGTH];

        long unsigned index = 0;

        //Permutate columns
        for(unsigned i = 0; i < NB_ROWS; ++i)
        {
            std::memcpy(buffer, mapped_file+index+HEADER, ROW_LENGTH);

            //Permutate row bits
            permute_buffer_order(buffer, mapped_file+index+HEADER, ORDER.data(), COLUMNS);
            index += ROW_LENGTH;
        }

        delete[] buffer;
    }
};

int main(int argc, char ** argv)
{
    if(argc < 5)
    {
        std::cout << "Usage: reverse_reorder <samples> <header> <order> <matrices...>\n\nsamples\t\tThe number of samples in a matrix (will be set to the upper multiple of 8 if given number is not a multiple of 8)\nheader\t\tSize of a matrix header (49 for kmtricks)\norder\t\tBinary serialized order computed from reference matrix\nmatrices\tAll matrices which columns will be reordered according to a same order (reversed from serialized order). Warning: all matrices must have the same size.\n\n";
        return 1;
    }

    unsigned samples = (unsigned)atol(argv[1]);
    unsigned header =  (unsigned)atol(argv[2]);
    char* order =      argv[3];

    std::vector<char*> matrices;
    matrices.resize(argc - 4);

    for(int i = 4; i < argc; ++i)
        matrices[i-4] = argv[i];

    Reorder::launch(matrices, samples, header, order);
}
