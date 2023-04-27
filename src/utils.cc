#include "utils.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <random>
#include <string>

using namespace std::chrono;

namespace unbalanced_psi {

    vector<INPUT_TYPE> generate_dataset(int size) {
        // TODO: how to generate a random seed? chicken and egg...
        block seed(std::rand());
        PRNG prng(seed);
        vector<INPUT_TYPE> dataset(size);
        prng.get(dataset.data(), dataset.size());

        return dataset;
    }

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
        RandomOracle oracle(HASH_1_SIZE);

        oracle.Reset();
        oracle.Update(input);

        vector<u8> hashed(HASH_1_SIZE);
        oracle.Final(hashed.data());

        Point point;
        point.fromHash(hashed.data(), HASH_1_SIZE);
        return point;
    }

    void hash_group_element(const Point& element, int length, u8* dest) {
        RandomOracle oracle(length);
        vector<u8> bytes(Point::size);
        element.toBytes(bytes.data());

        oracle.Reset();
        oracle.Update(bytes.data(), bytes.size());
        oracle.Final(dest);
    }

    std::string to_hex(Point point) {
        vector<u8> bytes(Point::size);
        point.toBytes(bytes.data());
        return to_hex(bytes.data(), bytes.size());
    }

    std::string to_hex(u8 *bytes, u64 size) {
        static const char* digits = "0123456789ABCDEF";

        std::string output((size_t) size * 2, 'X');
        for (auto i = 0; i < size; i++) {
            output[2 * i] = digits[bytes[i] & 0x0F];
            output[(2 * i) + 1] = digits[(bytes[i] >> 4) & 0x0F];
        }

        return output;
    }

    Timer::Timer(std::string msg, std::string color) : message(msg), color(color) {
        start = high_resolution_clock::now();
    }

    void Timer::stop() {
        auto stop = high_resolution_clock::now();
        duration<float, std::milli> elapsed = stop - start;
        std::cout << std::fixed << std::setprecision(3);
        std::cout << color << message << " (ms)\t: ";
        std::cout << elapsed.count() << RESET << std::endl;
    }
}
