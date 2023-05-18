#include "test_hashtable.h"

#include <stdio.h>
#include <algorithm>
#include <random>

#include "../utils.h"

#define HASH_SIZE 10

namespace unbalanced_psi {

// fourq doesn't have direct Point comparison so comparing hex values
#define POINT_VECTORS_NOT_EQUAL(x, y) compare_point_vectors(x, y)

    bool compare_point_vectors(vector<Point> actual, vector<Point> expected) {
        if (actual.size() != expected.size()) {
            return true;
        }
        for (auto i = 0; i < actual.size(); i++) {
            if (to_hex(actual[i]) != to_hex(expected[i])) {
                return true;
            }
        }

        return false;
    }

    void test_hash_repeat() {

        u64 TABLE_SIZE(1024);

        Point encrypted = hash_to_group_element(42);
        vector<u8> hashed(HASH_SIZE);
        hash_group_element(encrypted, hashed.size(), hashed.data());

        u64 first  = Hashtable::hash(hashed, TABLE_SIZE);
        u64 second = Hashtable::hash(hashed, TABLE_SIZE);

        if (first != second) {
            throw UnitTestFail("hash result changed");
        }
    }

    void test_hash_range() {
        INPUT_TYPE SAMPLES = 1000;
        u64 TABLE_SIZE(1024);

        for (INPUT_TYPE i = 0; i < SAMPLES; i++) {
            Point encrypted = hash_to_group_element(i);
            vector<u8> hashed(HASH_SIZE);
            hash_group_element(encrypted, hashed.size(), hashed.data());
            u64 result = Hashtable::hash(hashed, TABLE_SIZE);

            if (result > TABLE_SIZE) {
                throw UnitTestFail("hash result out of range");
            }
        }
    }

    void test_hashtable_insert_one() {
        INPUT_TYPE element = 42;
        u64 TABLE_SIZE = 1024;

        Point encrypted = hash_to_group_element(element);
        vector<u8> expected(HASH_SIZE);
        hash_group_element(encrypted, expected.size(), expected.data());

        u64 hashvalue = Hashtable::hash(expected, TABLE_SIZE);

        Hashtable hashtable(TABLE_SIZE);
        hashtable.insert(expected);

        auto actual = hashtable.table[hashvalue];

        if (expected != actual) {
            throw UnitTestFail("element not found in expected bucket");
        }
    }

    void test_hashtable_insert_many() {
        u64 TABLE_SIZE = 16;
        u64 TARGET_HASH = 5;
        u64 TARGET_ELEMENTS = 2;

        Hashtable hashtable(TABLE_SIZE);

        vector<u8> expected;

        INPUT_TYPE element = 0;
        while (expected.size() < TARGET_ELEMENTS) {
            Point encrypted = hash_to_group_element(element);
            vector<u8> hashed(HASH_SIZE);
            hash_group_element(encrypted, hashed.size(), hashed.data());
            hashtable.insert(hashed);

            if (Hashtable::hash(hashed, TABLE_SIZE) == TARGET_HASH) {
                expected.insert(expected.begin(), hashed.begin(), hashed.end());
            }
            element++;
        }

        if (hashtable.table[TARGET_HASH] != expected) {
            char errmsg[85];
            std::snprintf(
                errmsg, sizeof(errmsg),
                "correct elements not found in bucket\n"
                "expected.size() = %u vs. actual.size() = %u",
                (unsigned int) expected.size(),
                (unsigned int) hashtable.table[TARGET_HASH].size()
            );
            throw UnitTestFail(errmsg);
        }
    }

    void test_hashtable_pad_empty() {
        u64 TABLE_SIZE = 8;

        Hashtable hashtable(TABLE_SIZE);
        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != 0) {
                throw UnitTestFail("at least one bucket is >0");
            }
        }
    }

    void test_hashtable_pad_one() {
        u64 TABLE_SIZE = 16;

        INPUT_TYPE element = 42;
        Point encrypted = hash_to_group_element(element);
        vector<u8> hashed(HASH_SIZE);
        hash_group_element(encrypted, hashed.size(), hashed.data());

        Hashtable hashtable(TABLE_SIZE);
        hashtable.insert(hashed);
        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != HASH_SIZE) {
                hashtable.log();
                throw UnitTestFail("at least one bucket not padded to expected size");
            }
        }
    }

    void test_hashtable_pad_many() {
        u64 TABLE_SIZE = 16;

        Hashtable hashtable(TABLE_SIZE);

        for (INPUT_TYPE i = 0; i < TABLE_SIZE; i++) {
            Point encrypted = hash_to_group_element(i);
            vector<u8> hashed(HASH_SIZE);
            hash_group_element(encrypted, hashed.size(), hashed.data());
            hashtable.insert(hashed);
        }

        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != hashtable.width) {
                hashtable.log();
                throw UnitTestFail("at least one bucket not padded to width");
            }
        }
    }
}
