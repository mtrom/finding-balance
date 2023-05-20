#pragma once

#include "defines.h"

namespace unbalanced_psi {
    class Hashtable {

        public:

        // vector of buckets which are collections of hashed group elements
        vector<vector<u8>> table;

        // size of largest bucket
        u64 width;

        // number of entries inserted (does not include padding)
        u64 size;

        /**
         * setup hashtable with given number of buckets
         */
        Hashtable(u64 buckets);

        /**
         * hash encrypted input element to a bucket
         */
        static u64 hash(const vector<u8>& entry, u64 table_size);
        static u64 hash(const u8* entry, u64 table_size);

        /**
         * insert encrypted into a bucket given by it's own hash
         */
        void insert(const vector<u8>& entry);

        /**
         * pad all buckets with random elements to be bucket_size, or if
         *  bucket_size isn't specified (i.e., is 0) then pad to the size
         *  of the bucket with the most collisions
         */
        void pad();

        /**
         * number of buckets in the table
         */
        u64 buckets();

        /**
         * write hashtable to file
         */
        void to_file(string filename);

        /**
         * write contents of hash table to log for debugging
         */
        void log();
    };
}
