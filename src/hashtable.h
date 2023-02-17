#pragma once

#include "defines.h"

namespace unbalanced_psi {
    class Hashtable {
        vector<vector<Point>> table;
        osuCrypto::Blake2 hashf;

        public:
        Hashtable(int size);

        void insert(Point element);

        /**
         * @return bucket with most collisions
         */
        int max_bucket();

        private:
        int hash(Point element);
    };
}
