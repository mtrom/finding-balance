#pragma once

#include <vector>

#include "defines.h"

namespace unbalanced_psi {
    vector<INPUT_TYPE> generate_dataset(int size);
    tuple<vector<INPUT_TYPE>,vector<INPUT_TYPE>> generate_datasets(int server_size, int client_size, int overlap);

    void write_dataset(vector<INPUT_TYPE> dataset, std::string filename);
    vector<INPUT_TYPE> read_dataset(std::string filename);

    Point hash_to_group_element(INPUT_TYPE input);
    vector<Point> hash_to_group_elements(vector<INPUT_TYPE> inputs, RandomOracle oracle);

    std::string to_hex(Point point);
    std::string to_hex(u8 *bytes, u64 size);
}
