#include <cmath>
#include <fstream>
#include <ostream>

#include "hashtable.h"
#include "utils.h"

namespace unbalanced_psi {

    Hashtable::Hashtable() : size(0) {
    }

    Hashtable::Hashtable(u64 buckets) : size(0) {
        resize(buckets);
    };

    Hashtable::Hashtable(std::string filename) {
        from_file(filename);
    };

    void Hashtable::resize(u64 buckets) {
        table.clear();
        table.resize(buckets, vector<Point>());
    }

    u64 Hashtable::hash(INPUT_TYPE element, u64 table_size) {
        block seed(element);
        PRNG prng(seed);
        u64 output = prng.get<u64>();
        return output % table_size;
    }

    void Hashtable::insert(INPUT_TYPE element, Point encrypted) {
        u64 index = hash(element, table.size());
        table[index].push_back(encrypted);
        size++;

        if (element == INPUT_TYPE(3759285698)) {
            std::clog << "[ server ] query for 3759285698 is " << index << std::endl;
            std::clog << "[ server ] encrypted: " << to_hex(encrypted) << std::endl;
        }

        if (table[index].size() > log2(table.size())) {
            std::clog << "more than log2(size) collisions" << std::endl;
            // throw std::overflow_error("more than log2(size) collisions");
        }
    }

    void Hashtable::pad() {
        auto before = size;
        for (u64 i = 0; i < table.size(); i++) {
            while (table[i].size() < log2(table.size())) {
                table[i].push_back(hash_to_group_element(MOCK_ELEMENT));
                size++;
            }
        }

        std::clog << "[ hash ] added " << size - before << " mock elements to pad" << std::endl;
    }

    void Hashtable::to_file(std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) {
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
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
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
        }

        u64 buckets;
        u64 bucket_bytes;
        file.read((char*) &buckets, sizeof(u64));
        file.read((char*) &bucket_bytes, sizeof(u64));
        u64 bucket_size = bucket_bytes / Point::size;

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

            u64 index = size / bucket_size;
            table[index].push_back(element);
            size++;
        }
    }

    u64 Hashtable::buckets() {
        return table.size();
    }

    int Hashtable::max_bucket() {
        int collisions = 0;
        for (vector<Point> bucket : table) {
            if (collisions < bucket.size()) { collisions = bucket.size(); }
        }
        return collisions;
    }
}
