#pragma once

#include "defines.h"

namespace unbalanced_psi {
    class Hashtable {
        vector<vector<Point>> table;

        public:
        i64 size;

        Hashtable(i64 buckets);
        Hashtable(std::string filename);

        void insert(Point element);

        void tofile(std::string filename);

        void fromfile(std::string filename);

        i64 buckets();

        std::string to_number(vector<u8> &bytes);

        /**
         * @return bucket with most collisions
         */
        int max_bucket();

        private:
        i64 hash(Point element);
    };
}
