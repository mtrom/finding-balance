#pragma once

#include "defines.h"

namespace unbalanced_psi {
    class Hashtable {

        public:
        vector<vector<Point>> table;
        u64 size;

        Hashtable();
        Hashtable(u64 buckets);
        Hashtable(std::string filename);

        void resize(u64 buckets);

        static u64 hash(INPUT_TYPE element, u64 table_size);

        void insert(INPUT_TYPE element, Point encrypted);

        void to_file(std::string filename);

        void from_file(std::string filename);

        void pad();

        void pad(u64 min_bucket, u64 max_bucket);

        void shuffle();

        u64 buckets();

        void concat(Hashtable other);

        void log();

        /**
         * @return bucket with most collisions
         */
        int max_bucket();
    };
}
