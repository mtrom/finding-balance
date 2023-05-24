#pragma once

#include <optional>

#include "defines.h"
#include "hashtable.h"
#include "utils.h"

#define MAX_INSERT u64(100)

namespace unbalanced_psi {

    // hash function both the table and vector use
    u64 cuckoo_hash(const hash_type& entry, u64 hash_n, u64 table_size);

    // oprf output and its pir query
    using cuckoo_tuple = tuple<hash_type, u64>;

    /**
     * hashtables where each element is added once for each cuckoo hash
     */
    class CuckooTable {
        public:

        vector<Hashtable> table;

        // number of hashes to cuckoo hash with
        u64 hashes;

        /**
         * setup cuckoo table with given parameters
         */
        CuckooTable(const PSIParams& params);

        /**
         * insert entry into a hashtable for each value of
         *  cuckoo_hash(entry, i, params.cuckoo_size)
         *  for i = 0 to hashes
         */
        void insert(const vector<u8>& entry);

        /**
         * pad each hashtable with 0s so they are all rectangular
         */
        void pad();

        /**
         * number of buckets in the table
         */
        u64 buckets();
    };

    /**
     * vector where insertions are done using cuckoo hashing
     */
    class CuckooVector {
        public:

        vector<std::optional<cuckoo_tuple>> table;

        // number of hashes to cuckoo hash with
        u64 hashes;

        /**
         * setup vector with given parameters
         */
        CuckooVector(const PSIParams& params);

        /**
         * try and insert into vector using cuckoo hashing
         */
        void insert(const vector<u8>& entry, u64 query);

        /**
         * split the tuples into two datasets, with 0s for empty buckets
         */
        tuple<vector<hash_type>, vector<u64>> split();

        /**
         * number of buckets in the vector
         */
        u64 buckets();
    };
}
