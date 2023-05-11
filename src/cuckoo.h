#pragma once

#include "defines.h"
#include "utils.h"

#define MAX_INSERT u64(10)

namespace unbalanced_psi {
    class Cuckoo {

        public:
        vector<vector<INPUT_TYPE>> table;
        u64 size;
        u64 hashes;
        PRNG prng;

        Cuckoo();
        Cuckoo(u64 hashes, u64 buckets);

        void resize(u64 hashes, u64 buckets);

        u64 hash(INPUT_TYPE element, u64 hash_n);

        void insert(INPUT_TYPE element);

        void insert(vector<INPUT_TYPE> elements);

        void insert_all(INPUT_TYPE element);

        void insert_all(vector<INPUT_TYPE> elements);

        void pad(u64 to);

        u64 buckets();

        void log();
    };
}
