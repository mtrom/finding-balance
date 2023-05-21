#include "test_cuckoo.h"

#include <cryptoTools/Common/TestCollection.h>

#include "../cuckoo.h"
#include "../utils.h"

namespace unbalanced_psi {

    using UnitTestFail = osuCrypto::UnitTestFail;

    void test_cuckoo_hash_repeat() {
        vector<u8> input{9, 8, 7, 6, 5, 4, 3, 2, 1};
        u64 HASH_N = 7;
        u64 TABLE_SIZE = 1024;

        u64 first  = cuckoo_hash(input, HASH_N, TABLE_SIZE);
        u64 second = cuckoo_hash(input, HASH_N, TABLE_SIZE);

        if (first != second) {
            throw UnitTestFail("hash result changed");
        }
    }

    void test_cuckoo_hash_diff() {

        vector<u8> input{9, 8, 7, 6, 5, 4, 3, 2, 1};
        u64 HASH_A = 6;
        u64 HASH_B = 7;
        u64 TABLE_SIZE = 1024;

        // technically there's a chance they are the same but very low
        u64 first  = cuckoo_hash(input, HASH_A, TABLE_SIZE);
        u64 second = cuckoo_hash(input, HASH_B, TABLE_SIZE);

        if (first == second) {
            throw UnitTestFail("hash result same between hashes");
        }
    }

    void test_cuckoo_table_insert_one() {
        u64 CUCKOO_N = 10;
        u64 HASHES = 3;

        PSIParams PARAMS(
            CUCKOO_N, HASHES, 10, 1
        );

        CuckooTable cuckoo(PARAMS);

        cuckoo.insert(vector<u8>{9, 8, 7, 6, 5, 4, 3, 2, 1, 0});

        auto actual = 0;
        for (auto i = 0; i < cuckoo.buckets(); i++) {
            actual += cuckoo.table[i].size;
        }

        if (actual == HASHES) {
            throw UnitTestFail(
                "CuckooTable.insert() added " + std::to_string(actual)
                + " elements vs. # of hashes " + std::to_string(HASHES)
            );
        }
    }

    void test_cuckoo_table_insert_many() {
        u64 CUCKOO_N = 10;
        u64 HASHES = 3;
        auto MANY = 100;

        PSIParams PARAMS(
            CUCKOO_N, HASHES, 10, 1
        );

        CuckooTable cuckoo(PARAMS);

        for (auto element = 0; element < MANY; element++) {
            cuckoo.insert(vector<u8>(HASH_3_SIZE, element));
        }

        auto actual = 0;
        for (auto i = 0; i < cuckoo.buckets(); i++) {
            actual += cuckoo.table[i].size;
        }

        if (actual < MANY or actual > MANY * HASHES) {
            throw UnitTestFail(
                "CuckooTable.insert() added " + std::to_string(actual)
                + " elements which is less or more than expected"
            );
        }
    }

    void test_cuckoo_vector_insert() {
        u64 CUCKOO_N = 10;
        u64 HASHES = 3;

        PSIParams PARAMS(
            CUCKOO_N, HASHES, 10, 1
        );

        CuckooVector cuckoo(PARAMS);

        cuckoo.insert(vector<u8>{9, 8, 7, 6, 5, 4, 3, 2, 1, 0}, 42);

        auto actual = 0;
        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (cuckoo.table[i]) {
                actual++;
            }
        }

        if (actual != 1) {
            throw UnitTestFail("CuckooVector.insert() didn't insert element only once");
        }
    }

    void test_cuckoo_vector_insert_many() {
        u64 CUCKOO_N = 20;
        u64 HASHES = 3;
        auto MANY = 10;

        PSIParams PARAMS(
            CUCKOO_N, HASHES, 10, 1
        );

        CuckooVector cuckoo(PARAMS);

        for (auto element = 0; element < MANY; element++) {
            cuckoo.insert(vector<u8>(HASH_3_SIZE, element), u64(element));
        }

        auto actual = 0;
        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (cuckoo.table[i]) {
                actual++;
            }
        }

        if (actual != MANY) {
            throw UnitTestFail("size incorrect after many CuckooVector.insert()");
        }
    }

    void test_cuckoo_vector_insert_overfill() {
        u64 CUCKOO_N = 10;
        u64 HASHES = 3;
        auto MANY = 100;

        PSIParams PARAMS(
            CUCKOO_N, HASHES, 10, 1
        );

        CuckooVector cuckoo(PARAMS);

        bool caught = false;
        for (auto element = 0; element < MANY; element++) {
            try {
                cuckoo.insert(vector<u8>(HASH_3_SIZE, element), u64(element));
            } catch (std::runtime_error err) {
                caught = true;
            }
        }

        if (!caught) {
            throw UnitTestFail("CuckooVector.insert() didn't hit max iterations when overfilled");
        }
    }
}
