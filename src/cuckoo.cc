#include "cuckoo.h"

namespace unbalanced_psi {

    u64 cuckoo_hash(const vector<u8>& entry, u64 hash_n, u64 table_size) {
        // interpret the first eight bytes as the seed to the hash
        u64 value = *reinterpret_cast<const u64*>(&entry[0]);
        block seed(hash_n, value);
        PRNG prng(seed);
        return prng.get<u64>() % table_size;
    }

    /**
     * Cuckoo Table : used by the server
     */
    CuckooTable::CuckooTable(const PSIParams& params) :
        hashes(params.cuckoo_hashes),
        table(params.cuckoo_size, Hashtable(params.hashtable_size)) { };

    void CuckooTable::insert(const vector<u8>& entry) {
        vector<u64> indexes;
        for (auto hash_n = 0; hash_n < hashes; hash_n++) {
            u64 index = cuckoo_hash(entry, hash_n, table.size());

            // if we've already added this entry at index, don't do it again
            if (std::find(indexes.begin(), indexes.end(), index) != indexes.end()) {
                continue;
            }

            table[index].insert(entry);
            indexes.push_back(index);
        }
    }

    void CuckooTable::pad() {
        for (auto i = 0; i < table.size(); i++) {
            table[i].pad();
        }
    }

    u64 CuckooTable::buckets() {
        return table.size();
    }

    /**
     * Cuckoo Vector : used by the client
     */
    CuckooVector::CuckooVector(const PSIParams& params) :
        hashes(params.cuckoo_hashes), table(params.cuckoo_size, std::nullopt) { };

    void CuckooVector::insert(const vector<u8>& entry, u64 query) {
        auto to_insert = std::optional<tuple<vector<u8>, u64>>{std::make_tuple(entry, query)};
        u64 i = 0;

        while (true) {
            if (i > MAX_INSERT) { throw std::runtime_error("reached maximum insertion attempts"); }

            u64 hash_n = i % hashes;
            u64 index = cuckoo_hash(std::get<0>(to_insert.value()), hash_n, table.size());

            // insert if empty
            if (!table[index]) {
                table[index] = to_insert;
                break;
            }

            // evict otherwise
            auto evicted = table[index];
            table[index] = to_insert;
            to_insert = evicted;

            i++;
        }
    }

    tuple<vector<hash_type>, vector<u64>> CuckooVector::split() {
        u64 BLANK_QUERY = 0;
        hash_type BLANK_RESULT = hash_type(HASH_3_SIZE, 0);

        vector<hash_type> results;
        vector<u64> queries;
        for (auto i = 0; i < table.size(); i++) {
            if (table[i]) {
                auto [ result, query ] = table[i].value();
                results.push_back(result);
                queries.push_back(query);
            } else {
                results.push_back(BLANK_RESULT);
                queries.push_back(BLANK_QUERY);
            }
        }
        return std::make_tuple(results, queries);
    }

    u64 CuckooVector::buckets() {
        return table.size();
    }
}
