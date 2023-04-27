#include <algorithm>
#include <cmath>
#include <fstream>
#include <ostream>
#include <random>

#include "hashtable.h"
#include "utils.h"

namespace unbalanced_psi {


    Hashtable::Hashtable(u64 buckets, u64 max) : size(0), max_bucket(max) {
        resize(buckets, max_bucket);
    }

    Hashtable::Hashtable(std::string filename) {
        from_file(filename);
    }

    void Hashtable::resize(u64 buckets, u64 max) {
        max_bucket = max;
        table.clear();
        table.resize(buckets, vector<Point>());
    }

    u64 Hashtable::hash(INPUT_TYPE element, u64 table_size) {
        block seed(element);
        PRNG prng(seed);
        u64 output = prng.get<u64>();
        return output % table_size;
    }

    u64 Hashtable::hash(Point encrypted, u64 table_size) {
        vector<u8> bytes(sizeof(u64));
        hash_group_element(encrypted, sizeof(u64), bytes.data());
        u64 output = 0;
        for (u8 byte : bytes) {
            output = output << 8;
            output += byte;
        }
        return output % table_size;
    }

    void Hashtable::insert(INPUT_TYPE element, Point encrypted) {
        u64 index = hash(element, table.size());
        table[index].push_back(encrypted);
        size++;

        if (table[index].size() > max_bucket) {
            throw std::overflow_error("more than max_bucket collisions");
        }
    }

    void Hashtable::insert(Point encrypted) {
        u64 index = hash(encrypted, table.size());
        table[index].push_back(encrypted);
        size++;
    }

    void Hashtable::pad(u64 min, u64 max) {
        auto before = size;

        block seed(PADDING_SEED);
        PRNG prng(seed);

        for (u64 i = min; i < max; i++) {
            while (table[i].size() < max_bucket) {
                Point random_element(prng);
                table[i].push_back(random_element);
                size++;
            }
        }

        std::clog << "[ hash ] added " << size - before << " mock elements to pad" << std::endl;
    }

    void Hashtable::pad() {
        pad(0, table.size());
    }

    void Hashtable::shuffle() {
        auto prng = std::default_random_engine{};
        for (auto i = 0; i < table.size(); i++) {
            std::shuffle(table[i].begin(), table[i].end(), prng);
        }
    }

    vector<u8> Hashtable::apply_hash(int hash_length) {
        vector<u8> output(size * hash_length);
        u8 *ptr = output.data();

        for (auto bucket : table) {
            for (Point element : bucket) {
                hash_group_element(element, hash_length, ptr);
                ptr += hash_length;
            }
        }

        return output;
    }

    void Hashtable::to_file(std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot open " + filename);
        }

        u64 buckets = table.size();
        u64 bucket_bytes = table.front().size() * Point::size;
        std::clog << "[ hash ] writing buckets & size: each "
            << sizeof(u64) << " bytes: (" << buckets << ", "
            << bucket_bytes << ")" << std::endl;
        file.write((const char*) &buckets, sizeof(u64));
        file.write((const char*) &bucket_bytes, sizeof(u64));

        vector<u8> bytes(size * Point::size);
        u8 *ptr = bytes.data();

        for (auto i = 0; i < table.size(); i++) {
            auto bucket = table[i];
            for (Point element : bucket) {
                element.toBytes(ptr);
                std::clog << "[ server ] writing (" << i << ")\t:";
                std::clog << to_hex(ptr, Point::size) << std::endl;
                ptr += Point::size;
            }
        }
        /*
        std::clog << "[ hash ] writing " << bytes.size()
                  << " bytes: " << to_hex(bytes.data(), bytes.size()) << std::endl;
        */
        file.write((const char*) bytes.data(), bytes.size());
        file.close();
    }

    void Hashtable::from_file(std::string filename) {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot open " + filename);
        }

        u64 buckets;
        u64 bucket_bytes;
        file.read((char*) &buckets, sizeof(u64));
        file.read((char*) &bucket_bytes, sizeof(u64));
        max_bucket = bucket_bytes / Point::size;

        size = 0;
        table.clear();
        table.resize(buckets, vector<Point>());

        // std::clog << "[ hash ] buckets = " << buckets << ", size = " << filesize << std::endl;

        vector<u8> bytes(buckets * bucket_bytes);
        file.read((char *) bytes.data(), bytes.size());

        for (u8 *ptr = bytes.data(); ptr < &bytes.back(); ptr += Point::size) {
            // std::clog << "[ hash ] reading in element #" << i << ": " << to_hex(ptr, Point::size) << std::endl;
            Point element;
            element.fromBytes(ptr);

            u64 index = size / max_bucket;
            table[index].push_back(element);
            size++;
        }
    }

    u64 Hashtable::buckets() {
        return table.size();
    }

    void Hashtable::log() {
        for (auto i = 0; i < table.size(); i++) {
            auto bucket = table[i];
            for (Point element : bucket) {
                std::clog << "[  hash  ] bucket (" << i << ")\t:";
                std::clog << to_hex(element) << std::endl;
            }
        }
    }
}
