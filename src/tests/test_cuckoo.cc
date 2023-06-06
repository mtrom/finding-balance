#include "test_cuckoo.h"

#include <cryptoTools/Crypto/RandomOracle.h>
#include <cryptoTools/Common/TestCollection.h>
#include <kuku/kuku.h>

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

    float run_trials(u64 thread, u64 trials, PSIParams params, u64 elements) {

        std::mt19937_64 gen(std::time(nullptr));
        std::uniform_int_distribution<u64> random(0, 1 << 20);

        vector<vector<u8>> entries(elements, vector<u8>(HASH_3_SIZE));
        float errors = 0;
        for (u64 i = thread * trials; i < (thread + 1) * trials; i++) {
            try {
                CuckooVector cuckoo(params);

                for (auto j = 0; j < entries.size(); j++) {
                    osuCrypto::RandomOracle oracle(HASH_3_SIZE);
                    // elaborate way to have non-deterministic elements in each trial
                    u64 seed = random(gen);
                    seed = (seed << 20) + i;
                    seed = (seed << 32) + j;
                    oracle.Update(seed);
                    oracle.Final(entries[j].data());
                    cuckoo.insert(entries[j], 0);
                }
            } catch (std::runtime_error err) {
                errors++;
            }
        }
        return errors;
    }

    void test_cuckoo_failure_rate() {
        // don't want to run this with test suite, but remove this to test failure rates
        return;

        u64 ELEMENTS = 16;
        u64 HASHES = 3;
        u64 TRIALS = 1 << 16;
        u64 THREADS = 32;

        for (u64 CUCKOO_N = ELEMENTS * 1.5; CUCKOO_N < ELEMENTS * 3; CUCKOO_N++) {
            PSIParams PARAMS(CUCKOO_N, HASHES, 0, 0);

            float errors = 0;
            vector<future<float>> futures(THREADS);
            for (auto i = 0; i < THREADS; i++) {
                futures[i] = std::async(
                    std::launch::async,
                    run_trials,
                    i,
                    TRIALS / THREADS,
                    PARAMS,
                    ELEMENTS
                );
            }
            for (auto i = 0; i < THREADS; i++) {
                errors += futures[i].get();
            }

            std::cout << "\n" << CUCKOO_N << ": " << (errors / TRIALS) << " (" << errors << ")" << std::endl;
            if (errors == 0) { break; }
        }
    }
}
