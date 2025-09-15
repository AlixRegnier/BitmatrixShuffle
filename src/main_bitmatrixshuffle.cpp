#include <bitmatrixshuffle.h>
#include <cxxopts.hpp>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

void usage()
{
    std::cout << "Usage: bitmatrixshuffle -i <path> [--header <header_size>] [-c <columns>] [-g <groupsize>] [-s <subsampled_rows>] [-f <path> [-r]] [-t <path>] [--compress-to <path>]\n\n";
}

int main(int argc, char ** argv)
{    
    std::string input_path;
    std::string compress_to;
    std::string output_path;
    std::string output_ef_path;
    std::string in_order_path;
    std::string out_order_path;
    
    unsigned header = 0;
    std::size_t groupsize = 0;
    std::size_t subsampled_rows = 20000;
    std::size_t columns;
    std::size_t target_block_size = 8388608; //8MiB

    bool compress = false;
    bool reverse = false;
    bool serialize_order = false;
    bool deserialize_order = false;

    try 
    {
        cxxopts::Options options("BitmatrixShuffle", "Program reordering bitmatrix columns in a more compressive way (path TSP using Nearest-Neighbor)\n");
        
        options.add_options()
            ("i,input", "Matrix to reorder.", cxxopts::value<std::string>())
            ("header", "Matrix header size {0}.", cxxopts::value<unsigned>())
            ("c,columns", "Matrix columns", cxxopts::value<std::size_t>())
            ("compress-to", "Compress matrix to new file.", cxxopts::value<std::string>())
            ("f,from-order", "Deserialized order file.", cxxopts::value<std::string>())
            ("t,to-order", "Compute new order and serialized order to file.", cxxopts::value<std::string>())
            ("r,reverse", "Needs --from-order, should be used to retrieve original matrix")
            ("g,groupsize", "Group size {columns}. Must be a multiple of 8. By default rounded to next multiple of 8 if not yet.", cxxopts::value<std::size_t>())
            ("s,subsampled-rows", "Number of subsampled rows to compute a distance {20000}.", cxxopts::value<std::size_t>())
            ("b,block-size", "Targeted block size for transposition and compression {8388608=8MiB}. Literals not handled yet.", cxxopts::value<std::size_t>());

        auto args = options.parse(argc, argv);

        if (args.count("help"))
        {
            usage();
            return 0;
        }

        // Check required argument
        if (!args.count("input"))
        {
            std::cerr << "Error: -i/--input is required\n";
            usage();
            return 1;
        }

        // Get required argument
        input_path = args["input"].as<std::string>();
        
        // Validate that index path exists and is a directory
        if (!std::filesystem::exists(input_path)) 
        {
            std::cerr << "Error: Input matrix '" << input_path << "' does not exist\n";
            return 2;
        }

        if (!args.count("columns"))
        {
            std::cerr << "Error: Number of columns required\n";
            return 2;
        }

        columns = args["columns"].as<std::size_t>();

        // Get optional arguments
        if (args.count("groupsize"))
            groupsize = args["groupsize"].as<std::size_t>();
        else
            groupsize = (columns + 7) / 8 * 8;
    
        if(args.count("header"))
            header = args["header"].as<unsigned>();

        if (args.count("subsampled-rows")) 
            subsampled_rows = args["subsampled-rows"].as<std::size_t>();

        if (args.count("compress-to"))
        {
            output_path = args["compress-to"].as<std::string>();
            output_ef_path = output_path + ".ef";
            compress = true;
        }

        if(args.count("reverse"))
            if(args.count("from-order"))
                reverse = true;
            else
            {
                std::cerr << "Can't use 'reverse' option if no order was given with '--from-order'\n";
                return 2;
            }

        if(args.count("from-order"))
        {
            in_order_path = args["from-order"].as<std::string>();
            deserialize_order = true;
        }

        if(args.count("to-order"))
        {
            out_order_path = args["to-order"].as<std::string>();
            serialize_order = true;
        }

        if(args.count("block-size"))
            target_block_size = args["block-size"].as<std::size_t>();

    } 
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 2;
    } 
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }


    //Get file size to get the number of rows
    int fd = open(input_path.c_str(), O_RDONLY); //Open matrix in read-only
    
    if(fd < 0)
    {
        std::cerr << "Failed to open a file descriptor on reference matrix\n";
        return 2;
    }

    const std::size_t FILE_SIZE = lseek(fd, 0, SEEK_END);
    close(fd);



    const std::size_t ROW_LENGTH = (columns + 7) / 8;
    const std::size_t NB_ROWS = (FILE_SIZE - header) / ROW_LENGTH;

    std::vector<std::uint64_t> order;

    //Compute order (or deserialize if given)
    if(deserialize_order)
    {
        order.resize(ROW_LENGTH*8);
        fd = open(in_order_path.c_str(), O_RDONLY);

        if(fd < 0)
        {
            std::cerr << "Error: Couldn't deserialize order, open syscall failed\n";
            return 2;
        }

        read(fd, reinterpret_cast<char*>(order.data()), order.size()*sizeof(std::uint64_t));
        close(fd);
    }
    else
    {
        bms::compute_order_from_matrix_columns(input_path, header, columns, NB_ROWS, groupsize, subsampled_rows, order);
    }

    //Compute reversed order
    if(reverse)
    {
        std::vector<std::uint64_t> order_tmp(order);
        bms::reverse_order(order_tmp, order);
    }

    if(compress)
    {
        //Compute block size according to the number of columns
        const std::size_t BLOCK_SIZE = bms::target_block_size(columns, target_block_size); //Unused
        const std::size_t BLOCK_NB_ROWS = bms::target_block_nb_rows(columns, target_block_size);

        std::string config_path = "config.cfg";
        if (!std::filesystem::exists(config_path))
        {
            std::ofstream config_file(config_path, std::ios::out);
            config_file << "samples = " << columns << "\n";
            config_file << "bitvectorsperblock = " << BLOCK_NB_ROWS << "\n";
            config_file << "preset = 6" << std::endl;
        }

        //Reorder and compression matrix
        bms::reorder_matrix_columns_and_compress(input_path, output_path, output_path + ".ef", "config.cfg", header, columns, NB_ROWS, order, target_block_size);
    }
    else
    {
        //Reorder matrix
        bms::reorder_matrix_columns(input_path, header, columns, NB_ROWS, order, target_block_size); 
    }

            
    //Serialize order
    if(serialize_order)
    {
        fd = open(out_order_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if(fd < 0)
        {
            std::cerr << "Error: Couldn't serialize order, open syscall failed\n";
            return 2;
        }

        write(fd, reinterpret_cast<char*>(order.data()), order.size()*sizeof(std::uint64_t));
        close(fd);
    }
}