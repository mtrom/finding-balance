#include "test_cuckoo.h"

#include <cryptoTools/Common/TestCollection.h>

#include "../cuckoo.h"

namespace unbalanced_psi {

    using UnitTestFail = osuCrypto::UnitTestFail;

    void test_cuckoo_hash_repeat() {

        INPUT_TYPE input(42);
        u64 HASH_N(7);

        Cuckoo cuckoo(10, 100);

        u64 first  = cuckoo.hash(input, HASH_N);
        u64 second = cuckoo.hash(input, HASH_N);

        if (first != second) {
            throw UnitTestFail("hash result changed");
        }
    }

    void test_cuckoo_hash_diff() {

        INPUT_TYPE input(42);
        u64 HASH_A(6);
        u64 HASH_B(7);

        Cuckoo cuckoo(10, 1000);

        // technically there's a chance they are the same but very low
        u64 first  = cuckoo.hash(input, HASH_A);
        u64 second = cuckoo.hash(input, HASH_B);

        if (first == second) {
            throw UnitTestFail("hash result same between hashes");
        }
    }

    void test_cuckoo_insert_one() {
        INPUT_TYPE input(42);

        Cuckoo cuckoo(10, 1000);

        cuckoo.insert(input);

        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (!cuckoo.table[i].empty() && cuckoo.table[i].back() == input) {
                return;
            }
        }

        throw UnitTestFail("Cuckoo.insert() didn't insert element");
    }

    void test_cuckoo_insert_many() {
        Cuckoo cuckoo(3, 1000);

        // silly way to check this... but when an element is found
        // in the cuckoo hashtable, set it to `found`
        INPUT_TYPE found = 42;
        vector<INPUT_TYPE> inputs { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        for (INPUT_TYPE input : inputs) {
            cuckoo.insert(input);
        }

        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (!cuckoo.table[i].empty() && inputs[cuckoo.table[i].back()] == found) {
                throw UnitTestFail("element inserted in at least two buckets");
            } else if (!cuckoo.table[i].empty()) {
                inputs[cuckoo.table[i].back()] = found;
            }
        }

        for (INPUT_TYPE input : inputs) {
            if (input != found) {
                throw UnitTestFail("Cuckoo.insert() didn't insert element");
            }
        }
    }

    void test_cuckoo_insert_overfill() {
        Cuckoo cuckoo(3, 9);

        vector<INPUT_TYPE> inputs { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        bool caught = false;

        for (INPUT_TYPE input : inputs) {
            try {
                cuckoo.insert(input);
            } catch (std::runtime_error err) {
                caught = true;
            }
        }

        if (!caught) {
            throw UnitTestFail("Cuckoo.insert() didn't hit max iterations when overfilled");
        }
    }

    void test_cuckoo_insert_all() {
        INPUT_TYPE input(42);
        u64 hashes(3);

        Cuckoo cuckoo(hashes, 100);

        cuckoo.insert_all(input);

        u64 found(0);
        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (!cuckoo.table[i].empty() && cuckoo.table[i].back() == input) {
                found++;
            }
        }

        if (found != hashes) {
            throw UnitTestFail("Cuckoo.insert_all() didn't insert element enough times");
        }
    }

    void test_cuckoo_pad_empty() {
        u64 padding = 5;

        Cuckoo cuckoo(3, 10);

        cuckoo.pad(padding);

        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (cuckoo.table[i].size() != padding) {
                throw UnitTestFail("Cuckoo.pad() didn't pad to correct number");
            }
        }
    }

    void test_cuckoo_pad_one() {
        u64 padding = 5;

        Cuckoo cuckoo(3, 10);

        cuckoo.insert(42);

        cuckoo.pad(padding);

        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (cuckoo.table[i].size() != padding) {
                throw UnitTestFail("Cuckoo.pad() didn't pad to correct number");
            }
        }
    }
    void test_cuckoo_pad_many() {
        vector<INPUT_TYPE> inputs { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        u64 padding = 5;

        Cuckoo cuckoo(3, 20);

        for (auto input : inputs) {
            cuckoo.insert(input);
        }

        cuckoo.pad(padding);

        for (auto i = 0; i < cuckoo.buckets(); i++) {
            if (cuckoo.table[i].size() != padding) {
                throw UnitTestFail("Cuckoo.pad() didn't pad to correct number");
            }
        }
    }
}
