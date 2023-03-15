#include "utils.h"

#include <fstream>
#include <iostream>
#include <random>
#include <string>

#define HASH_SIZE 32

namespace unbalanced_psi {

    /**
     * generate a mock dataset
     *
     * @param <size> desired number of elements in the dataset
     * @return random vector of numbers
     */
    vector<INPUT_TYPE> generate_dataset(int size) {
        // TODO: how to generate a random seed? chicken and egg...
        block seed(std::rand());
        PRNG prng(seed);
        vector<INPUT_TYPE> dataset(size);
        prng.get(dataset.data(), dataset.size());
        return dataset;
    }

    /**
     * generate two mock datasets for the server and client
     *
     * @param <server_size> number of elements in the server's dataset
     * @param <client_size> number of elements in the client's dataset
     * @param <overlap>     _minimum_ number of elements shared between the two datasets
     * @return random vector of numbers
     */
    tuple<vector<INPUT_TYPE>,vector<INPUT_TYPE>> generate_datasets(int server_size, int client_size, int overlap) {
        vector<INPUT_TYPE> both   = generate_dataset(overlap);
        vector<INPUT_TYPE> server = generate_dataset(server_size - overlap);
        vector<INPUT_TYPE> client = generate_dataset(client_size - overlap);

        server.insert(server.end(), both.begin(), both.end());
        client.insert(client.end(), both.begin(), both.end());

        tuple<vector<INPUT_TYPE>,vector<INPUT_TYPE>> datasets(server, client);
        return datasets;
    }

    /**
     * hash a set element to a group element for ddh
     *
     * @param <input> set element to hash
     * @return random group element
     */
    Point hash_to_group_element(INPUT_TYPE input) {
        RandomOracle oracle(HASH_SIZE);

        oracle.Reset();
        oracle.Update(input);

        vector<u8> hashed(HASH_SIZE);
        oracle.Final(hashed.data());

        Point point;
        point.fromHash(hashed.data(), HASH_SIZE);
        return point;
    }

    /**
     * convert a byte vector into a hex string for debugging
     */
    std::string to_hex(u8 *bytes, u64 size) {
        static const char* digits = "0123456789ABCDEF";

        std::string output((size_t) size * 2, 'X');
        for (auto i = 0; i < size; i++) {
            output[2 * i] = digits[bytes[i] & 0x0F];
            output[(2 * i) + 1] = digits[(bytes[i] >> 4) & 0x0F];
        }

        return output;
    }
}
