#include "utils.h"

#include <fstream>
#include <iostream>
#include <random>
#include <string>

#define HASH_SIZE 32

namespace unbalanced_psi {


    template<typename T>
        void print(T arg, std::string label) {
            if (!label.empty()) {
                std::cout << label + ": ";
            }
            std::cout << arg << "\n";
            /*
               std::cout << " (";
               std::cout << typeid(arg).name();
               std::cout << ")\n";
               */
        }

    template<typename T>
        void print(T arg) {
            print(arg, "");
        }

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

    void test_encrypt() {

        // PRNG init
        block seed;
        PRNG prng(seed);

        // initialize the curve
        Curve curve;

        // Alice's element
        Point hx = hash_to_group_element(1235);

        // Bob's element
        Point hy = hash_to_group_element(1235);

        // Alice's key
        Number a(prng);
        print(a, "a's key");

        // Bob's key
        Number b(prng);
        print(b, "b's key");

        Point hxa = hx * a;
        Point hxab = hxa * b;

        Point hyb = hy * b;
        Point hyba = hyb * a;

        print(hxab == hyba, "final answer");
    }

}
