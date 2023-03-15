#pragma once

#include "defines.h"

namespace unbalanced_psi {
    class Hashtable {
        vector<vector<Point>> table;

        public:
        u64 size;

        Hashtable(u64 buckets);
        Hashtable(std::string filename);

        void insert(Point element);

        void tofile(std::string filename);

        void fromfile(std::string filename);

        u64 buckets();

        std::string to_number(vector<u8> &bytes);

        /**
         * @return bucket with most collisions
         */
        int max_bucket();

        private:
        u64 hash(Point element);
    };
}
