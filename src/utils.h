#pragma once

#include <vector>

#include "defines.h"

using namespace std::chrono;

#define RED   "\033[0;31m"
#define GREEN "\033[0;32m"
#define BLUE  "\033[0;34m"
#define CYAN  "\033[0;36m"
#define WHITE "\033[0;37m"
#define RESET "\033[0m"

namespace unbalanced_psi {
    vector<INPUT_TYPE> generate_dataset(int size);
    tuple<vector<INPUT_TYPE>,vector<INPUT_TYPE>> generate_datasets(int server_size, int client_size, int overlap);

    void write_dataset(vector<INPUT_TYPE> dataset, std::string filename);
    vector<INPUT_TYPE> read_dataset(std::string filename);

    Point hash_to_group_element(INPUT_TYPE input);
    vector<Point> hash_to_group_elements(vector<INPUT_TYPE> inputs, RandomOracle oracle);

    std::string to_hex(Point point);
    std::string to_hex(u8 *bytes, u64 size);

    class Timer {
        public:
            Timer(std::string msg, std::string color = WHITE);
            void stop();
        private:
            std::string message;
            std::string color;
            std::chrono::time_point<std::chrono::high_resolution_clock> start;
    };
}
