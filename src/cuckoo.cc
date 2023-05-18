#include "cuckoo.h"

namespace unbalanced_psi {

    Cuckoo::Cuckoo(u64 h, u64 buckets)
        : hashes(h), table(buckets, vector<INPUT_TYPE>()) {
    };

    u64 Cuckoo::hash(INPUT_TYPE element, u64 hash_n) {
        block seed(hash_n, element);
        PRNG prng(seed);
        return prng.get<u64>() % table.size();
    }

    void Cuckoo::insert(INPUT_TYPE element) {
        INPUT_TYPE to_insert = element;
        u64 i = 0;

        while (true) {
            if (i > MAX_INSERT) { throw std::runtime_error("reached maximum insertion attempts"); }

            u64 hash_n = i % hashes;
            u64 index = hash(to_insert, hash_n);

            // insert if empty
            if (table[index].empty()) {
                table[index].push_back(to_insert);
                break;
            }

            // evict otherwise
            auto removed = table[index].back();
            table[index].pop_back();
            table[index].push_back(to_insert);
            to_insert = removed;

            i++;
        }
    }

    void Cuckoo::insert_all(INPUT_TYPE element) {
        vector<u64> indexes;
        for (auto hash_n = 0; hash_n < hashes; hash_n++) {
            u64 index = hash(element, hash_n);

            // if we've already added this element at index, don't do it again
            if (std::find(indexes.begin(), indexes.end(), index) != indexes.end()) {
                continue;
            }

            table[index].push_back(element);
            indexes.push_back(index);
        }
    }

    void Cuckoo::insert_all(vector<INPUT_TYPE> elements) {
        // if we aren't cuckoo hashing don't bother with the hashes
        if (table.size() == 1) {
            table[0] = elements;
        } else {
            for (auto element : elements) {
                insert_all(element);
            }
        }
    }

    void Cuckoo::pad(u64 to) {
        if (to == 0) { return; }
        block seed(PADDING_SEED);
        PRNG prng(seed);

        for (u64 index = 0; index < table.size(); index++) {
            if (table[index].size() > to) {
                throw std::runtime_error(
                    "cuckoo bucket already exceeded padding: "
                    + std::to_string(table[index].size()) + " vs. "
                    + std::to_string(to)
                );
            }
            while (table[index].size() < to) {
                table[index].push_back(prng.get<INPUT_TYPE>());
            }
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
