// vector[rev8(order[rev8(i)])]


//Bring back filling columns (columns that are not samples but there to fill last byte of each row)
void immutable_filling_columns_inplace(std::vector<std::uint64_t>& order, const std::size_t SAMPLES)
{
    const std::size_t COLUMNS = (SAMPLES + 7) / 8 * 8;

    if(COLUMNS == SAMPLES)
        return;

    const std::size_t FILL = COLUMNS - SAMPLES;
    const std::size_t A = COLUMNS - 8;
    const std::size_t B = COLUMNS - 8 + SAMPLES - 1;

    std::size_t k = 0;

    //Write all integers that are not fillers
    for(std::size_t i = 0; i < COLUMNS-8; ++i)
    {
        while(order[i+k] >= A && order[i+k] <= B)
            ++k;

        order[i] = order[i+k];
    }

    k = 0;

    //Write all remaining integers from end to fillers
    for(std::size_t i = COLUMNS-1; i > COLUMNS-8+FILL-1; --i)
    {
        while(order[i-k] >= A && order[i-k] <= B)
            ++k;

        order[i] = order[i-k];
    }

    //Write fillers
    for(std::uint64_t i = COLUMNS-8; i < COLUMNS+FILL-8; ++i)
        order[i] = i;
}

//Function mapping byte inner bits position (MSB position is 0, LSB position is 7) to reversed order
constexpr std::uint64_t rev8(std::uint64_t x)
{
    /*
     0 -->  7
     1 -->  6
     2 -->  5
     3 -->  4
     4 -->  3
     5 -->  2
     6 -->  1
     7 -->  0
     8 --> 15
     9 --> 14
    10 --> 13
    11 --> 12
    ...    
    */

    //Equivalent instructions (two last instructions have exactly the same assembly code)
    //return 7 - (x % 8) + (x / 8) * 8;
    //return 7 - (x & 0x7U) + (x / 8) * 8;
    //return (x | 0x7U) - (x & 0x7U);
    //return (x & ~0x7U) | ~(x & 0x7U);
    return (x | 0x7U) - (x & 0x7U);
}

void reorder_json(json& index_json, const std::string& index_name, const std::size_t SAMPLES, const std::vector<std::uint64_t>& order)
{
    //Read samples from index.json and put them in a vector
    std::vector<std::string> lines = indexjson["index"][index_name]["samples"].get<std::vector<std::string>>();

    //Replace each element by the one that should be placed according to the permutation
    for(std::size_t i = 0; i < SAMPLES; ++i)
        indexjson["index"][index_name]["samples"][i] = lines[rev8(order[rev8(i)])];

    //Code to write out JSON [...]
}


//Read lines from FOF, store them in vector of string, reorder vector, overwrite FOF with reordered lines
void reorder_fof(const std::string& fof_path, const std::size_t SAMPLES, const std::vector<unsigned>& order)
{
    std::vector<std::string> fof_lines;
    fof_lines.resize(SAMPLES);

    std::ifstream ifdfof(fof_path);

    //Store all lines
    std::size_t i = 0;
    std::string line;
    while (std::getline(ifdfof, line) && i < SAMPLES)
        fof_lines[i++] = line;

    ifdfof.close();
    std::ofstream ofdfof(fof_path);

    for(i = 0; i < SAMPLES; ++i)
        ofdfof << samples[rev8(order[rev8(i)])] << '\n';

    ofdfof.close();
}
