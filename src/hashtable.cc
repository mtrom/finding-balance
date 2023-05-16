#include "../CTPL/ctpl.h"

#include "hashtable.h"
#include "utils.h"

namespace unbalanced_psi {

    Hashtable::Hashtable(u64 buckets, u64 b_size) :
        size(0), bucket_size(b_size), max_bucket(b_size) {
        resize(buckets, bucket_size);
    }

    void Hashtable::resize(u64 buckets, u64 b_size) {
        bucket_size = b_size;
        table.clear();
        table.resize(buckets, vector<Point>());
    }

    u64 Hashtable::hash(INPUT_TYPE element, u64 table_size) {
        block seed(element);
        PRNG prng(seed);
        u64 output = prng.get<u64>();
        return output % table_size;
    }

    u64 Hashtable::hash(const Point& encrypted, u64 table_size) {
        vector<u8> bytes(sizeof(u64));
        hash_group_element(encrypted, sizeof(u64), bytes.data());
        u64 output = 0;
        for (u8 byte : bytes) {
            output = output << 8;
            output += byte;
        }
        return output % table_size;
    }

    void Hashtable::insert(INPUT_TYPE element, const Point& encrypted) {
        u64 index = hash(element, table.size());
        table[index].push_back(encrypted);
        size++;

        if (table[index].size() > max_bucket) { max_bucket = table[index].size(); }

        if (table[index].size() > bucket_size && bucket_size != 0) {
            throw std::overflow_error("more than bucket_size collisions");
        }
    }

    void Hashtable::insert(const Point& encrypted) {
        u64 index = hash(encrypted, table.size());
        table[index].push_back(encrypted);
        size++;

        if (table[index].size() > max_bucket) { max_bucket = table[index].size(); }

        if (table[index].size() > bucket_size && bucket_size != 0) {
            throw std::overflow_error("more than bucket_size collisions");
        }
    }

    void Hashtable::partial_pad(u64 min, u64 max) {
        block seed(PADDING_SEED);
        PRNG prng(seed);

        u64 pad_to = bucket_size;

        // if no bucket size is given, use the bucket with the most collisions
        if (bucket_size == 0) { pad_to = max_bucket; }

        for (u64 i = min; i < max && i < table.size(); i++) {
            while (table[i].size() < pad_to) {
                auto random_element = hash_to_group_element(prng.get<INPUT_TYPE>());
                table[i].push_back(random_element);
                size++;
            }
        }
    }

    void Hashtable::pad() {
        partial_pad(0, table.size());
    }

    void Hashtable::pad(int threads) {
        if (threads == 1) { return pad(); }
        vector<std::future<void>> futures(threads);

        ctpl::thread_pool tpool(threads);

        // each thread takes on this many buckets
        //  basically ceil(table.size() / threads)
        u64 buckets = table.size() / threads + (table.size() % threads != 0);
        for (u64 i = 0; i < threads; i++) {
            futures[i] = tpool.push([this, i, buckets](int) {
                return this->partial_pad(i * buckets, (i + 1) * buckets);
            });
        }

        for (auto i = 0; i < threads; i++) {
            futures[i].get();
        }
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
