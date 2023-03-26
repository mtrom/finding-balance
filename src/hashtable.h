#pragma once

#include "defines.h"

namespace unbalanced_psi {
    class Hashtable {

        public:
        vector<vector<Point>> table;
        u64 size;

        Hashtable(u64 buckets);
        Hashtable(std::string filename);

        static u64 hash(INPUT_TYPE element, u64 table_size);

        void insert(INPUT_TYPE element, Point encrypted);

        void to_file(std::string filename);

        void from_file(std::string filename);

        void pad();

        u64 buckets();

        /**
         * @return bucket with most collisions
         */
        int max_bucket();
    };
}
