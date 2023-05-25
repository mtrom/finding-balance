#include "utils.h"

#include <algorithm>
#include <cmath>
#include <gsl/span>
#include <ostream>
#include <random>
#include <string>

#include <apsi/util/utils.h>

using namespace std::chrono;

namespace unbalanced_psi {

    vector<INPUT_TYPE> generate_dataset(int size) {
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

    void write_results(const vector<hash_type>& results, std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) { throw std::runtime_error("cannot open " + filename); }
        for (auto i = 0; i < results.size(); i++) {
            file.write((const char*) results[i].data(), results[i].size());
        }
        file.close();
    }

    Point hash_to_group_element(INPUT_TYPE input) {
        Point element(gsl::span<const u8>{
            static_cast<const u8*>(static_cast<void*>(&input)),
            sizeof(INPUT_TYPE)
        });
        return element;
    }

    void hash_group_element(const Point& element, int length, u8* dest) {
        array<u8, Point::hash_size> item_hash_and_label_key;
        element.extract_hash(item_hash_and_label_key);
        apsi::util::copy_bytes(item_hash_and_label_key.data(), length, dest);
    }

    std::string to_hex(const Point& point) {
        vector<u8> bytes(Point::save_size);
        point.save(Point::point_save_span_type{bytes.data(), bytes.size()});
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
        duration<float> elapsed = stop - start;
        std::cout << std::fixed << std::setprecision(3);
        std::cout << color << message << " (s)\t: ";
        std::cout << elapsed.count() << RESET << std::endl;
    }
}
