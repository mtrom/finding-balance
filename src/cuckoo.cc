#include "cuckoo.h"
#include "utils.h"

namespace unbalanced_psi {

    Cuckoo::Cuckoo() : size(0) {
    }

    Cuckoo::Cuckoo(u64 hashes, u64 buckets) : size(0) {
        resize(hashes, buckets);
    };

    void Cuckoo::resize(u64 h, u64 buckets) {
        hashes = h;
        table.clear();
        table.resize(buckets, vector<INPUT_TYPE>());
    }

    u64 Cuckoo::hash(INPUT_TYPE element, u64 hash_n) {
        block seed(hash_n, element);
        PRNG prng(seed);
        u64 output = prng.get<u64>();
        return output % table.size();
    }

    void Cuckoo::insert(INPUT_TYPE element) {
        u64 i = 0;
        INPUT_TYPE to_insert = element;

        while (true) {
            u64 hash_n = i % hashes;
            u64 index = hash(to_insert, hash_n);
            if (table[index].empty()) {
                table[index].push_back(to_insert);
                break;
            } else {
                auto removed = table[index].back();
                table[index].pop_back();
                table[index].push_back(to_insert);
                to_insert = removed;
            }

            i++;
            if (i > MAX_INSERT) {
                throw std::runtime_error("reached maximum insertion attempts");
            }
        }
        size++;
    }

    void Cuckoo::insert_all(INPUT_TYPE element) {
        vector<u64> indexes;
        for (auto hash_n = 0; hash_n < hashes; hash_n++) {
            u64 index = hash(element, hash_n);

            if (std::find(indexes.begin(), indexes.end(), index) != indexes.end()) {
                // TODO: should we?
                // throw std::runtime_error("collision in hash different functions");
                continue;
            }

            table[index].push_back(element);
            indexes.push_back(index);
            size++;
        }
    }


    void Cuckoo::pad(u64 to) {
        block seed(PADDING_SEED);
        PRNG prng(seed);

        for (u64 index = 0; index < table.size(); index++) {
            if (table[index].size() > to) {
                throw std::runtime_error("cuckoo bucket already exceeded padding");
            }
            while (table[index].size() < to) {
                table[index].push_back(prng.get<INPUT_TYPE>());
                size++;
            }
        }
    }

    void Cuckoo::to_file(std::string fn_prefix, std::string fn_suffix) {
        for (int i = 0; i < table.size(); i++) {
            std::string fn = fn_prefix + std::to_string(i) + fn_suffix;
            write_dataset(table[i], fn);
        }
    }

    u64 Cuckoo::buckets() {
        return table.size();
    }

    void Cuckoo::log() {
        for (auto i = 0; i < table.size(); i++) {
            auto bucket = table[i];
            for (INPUT_TYPE element : bucket) {
                std::clog << "[ cuckoo ] bucket (" << i << ")\t:";
                std::clog << element << std::endl;
            }
        }
    }
}
