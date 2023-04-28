#pragma once

#include "defines.h"

namespace unbalanced_psi {
    class Hashtable {

        public:

        // vector of buckets which are collections of group elements
        vector<vector<Point>> table;

        // number of elements in hashtable, including padding
        u64 size;

        // size of each bucket after padding
        u64 bucket_size;

        /**
         * setup hashtable with given number of buckets of max size
         */
        Hashtable(u64 buckets, u64 bucket_size);

        /**
         * setup hashtable from file
         */
        Hashtable(std::string filename);

        /**
         * clear contents of hashtable and resize to new parameters
         */
        void resize(u64 buckets, u64 bucket_size);

        /**
         * hash input element to a bucket
         */
        static u64 hash(INPUT_TYPE element, u64 table_size);

        /**
         * hash encrypted input element to a bucket
         */
        static u64 hash(Point encrypted, u64 table_size);

        /**
         * insert encrypted into a bucket given by element's hash
         */
        void insert(INPUT_TYPE element, Point encrypted);

        /**
         * insert encrypted into a bucket given by it's own hash
         */
        void insert(Point encrypted);

        /**
         * write hashtable to file
         */
        void to_file(std::string filename);

        /**
         * read hashtable in from file
         */
        void from_file(std::string filename);

        /**
         * pad all buckets with random elements to be bucket_size, or if
         *  bucket_size isn't specified (i.e., is 0) then pad to the size
         *  of the bucket with the most collisions
         */
        void pad();

        /**
         * pad buckets from min_bucket to max_bucket
         */
        void pad(u64 min_bucket, u64 max_bucket);

        /**
         * permute elements in each bucket
         */
        void shuffle();

        /**
         * apply a hash to each bucket and return the result
         *
         * @params <hash_length> number of bytes each element should be hashed to
         * @return byte vector of hashed elements ordered by bucket
         */
        vector<u8> apply_hash(int hash_length);

        /**
         * number of buckets in the table
         */
        u64 buckets();

        /**
         * size of the bucket with the most elements
         */
        int max_bucket();

        /**
         * write contents of hash table to log for debugging
         */
        void log();
    };
}
