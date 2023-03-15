#include <cmath>
#include <fstream>
#include <ostream>

#include "hashtable.h"
#include "utils.h"

namespace unbalanced_psi {

    Hashtable::Hashtable(u64 buckets) : size(0) {
        table.resize(buckets, vector<Point>());
    };

    Hashtable::Hashtable(std::string filename) {
        fromfile(filename);
    };


    u64 Hashtable::hash(Point element) {
        // TODO: make a more uniform hash?
        vector<u8> bytes(sizeof(element.sizeBytes()));
        element.toBytes(bytes.data());

        u32 output = 0;

        for (u8 byte : bytes) {
            output = output << 8;
            output += byte;
        }

        return output % table.size();
    }

    void Hashtable::insert(Point element) {
        u64 index = hash(element);
        table[index].push_back(element);

        if (table[index].size() > log2(table.size())) {
            // throw std::overflow_error("more than log2(size) collisions");
        }

        size++;
    }


    void Hashtable::tofile(std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) {
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
        }

        u64 buckets = table.size();
        u64 filesize = log2(size);
        std::cout << "[ hash ] writing buckets & size: each " << sizeof(u64) << " bytes" << std::endl;
        file.write((const char*) &buckets, sizeof(u64));
        file.write((const char*) &filesize, sizeof(u64));

        vector<u8> bytes(size * Point::size);
        u8 *ptr = bytes.data();

        for (vector<Point> bucket : table) {
            for (Point element : bucket) {
                element.toBytes(ptr);
                ptr += Point::size;
            }
        }
        std::cout << "[ hash ] writing " << bytes.size()
                  << " bytes: " << to_hex(bytes.data(), bytes.size()) << std::endl;
        file.write((const char*) bytes.data(), bytes.size());
        file.close();
    }

    void Hashtable::fromfile(std::string filename) {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file) {
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
        }

        u64 buckets;
        u64 filesize;
        file.read((char*) &buckets, sizeof(u64));
        file.read((char*) &filesize, sizeof(u64));
        filesize = pow(2, filesize);

        table.resize(buckets, vector<Point>());

        std::cout << "[ hash ] buckets = " << buckets << ", size = " << filesize << std::endl;

        vector<u8> bytes(filesize * Point::size);
        file.read((char *) bytes.data(), bytes.size());

        u8 *ptr = bytes.data();
        for (auto i = 0; i < filesize; i++) {
            std::cout << "[ hash ] reading in element #" << i << ": " << to_hex(ptr, Point::size) << std::endl;
            Point element;
            element.fromBytes(ptr);
            ptr += Point::size;

            insert(element);
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
