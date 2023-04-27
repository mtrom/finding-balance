#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "defines.h"

using namespace std::chrono;

#define RED   "\033[0;31m"
#define GREEN "\033[0;32m"
#define BLUE  "\033[0;34m"
#define CYAN  "\033[0;36m"
#define WHITE "\033[0;37m"
#define RESET "\033[0m"

// # of bytes in output of hash h: h(x)^a
#define HASH_1_SIZE 32

// # of bytes in output of hash g: g(h(x)^a)
#define HASH_3_SIZE 10

namespace unbalanced_psi {

    /**
     * generate a mock dataset
     *
     * @param <size> desired number of elements in the dataset
     * @return random vector of numbers
     */
    vector<INPUT_TYPE> generate_dataset(int size);

    /**
     * generate two mock datasets for the server and client
     *
     * @param <server_size> number of elements in the server's dataset
     * @param <client_size> number of elements in the client's dataset
     * @param <overlap>     _minimum_ number of elements shared between the two datasets
     * @return random vector of numbers
     */
    tuple<vector<INPUT_TYPE>,vector<INPUT_TYPE>> generate_datasets(int server_size, int client_size, int overlap);

    /**
     * write arbitrary dataset to binary file
     *
     * @param <dataset> dataset to write to file
     * @param <filename> filename to write to
     */
    template <typename T>
    void write_dataset(vector<T>& dataset, std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) { throw std::runtime_error("cannot open " + filename); }
        file.write((const char*) dataset.data(), dataset.size() * sizeof(T));
    }

    /**
     * read arbitrary dataset to binary file
     *
     * @param <dataset> dataset to write to file
     * @param <filename> filename to write to
     */
    template <typename T>
    vector<T> read_dataset(const std::string filename) {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file) { throw std::runtime_error("cannot open " + filename); }

        file.seekg (0, file.end);
        int bytes = file.tellg();
        file.seekg (0, file.beg);

        vector<T> dataset(bytes / sizeof(T));
        file.read((char*) dataset.data(), bytes);
        return dataset;
    }

    /**
     * hash a set element to a group element for ddh
     *
     * @params <input> set element to hash
     * @return random group element
     */
    Point hash_to_group_element(INPUT_TYPE input);

    /**
     * hash a vector of elements to group elements for ddh
     *
     * @params <inputs> elements to hash
     * @return random group elements
     */
    vector<Point> hash_to_group_elements(vector<INPUT_TYPE> inputs, RandomOracle oracle);

    /**
     * hash a group element to a byte vector of specified size
     *
     * @params <element> to be hashed
     * @params <length> length of the byte vector required
     * @params <dest> pointer to put hash value
     * @returns random byte vector
     */
    void hash_group_element(const Point& element, int length, u8* dest);


    /**
     * convert a Point into a hex string for debugging
     */
    std::string to_hex(Point point);

    /**
     * convert a byte vector into a hex string for debugging
     */
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
