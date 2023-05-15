#include "test_hashtable.h"

#include <stdio.h>
#include <algorithm>
#include <random>

#include "../utils.h"

namespace unbalanced_psi {

// fourq doesn't have direct Point comparison so comparing hex values
#if _USE_FOUR_Q_
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
#else
#define POINT_VECTORS_NOT_EQUAL(x, y) (x != y)
#endif


    void test_hash_repeat() {

        INPUT_TYPE input(42);
        u64 TABLE_SIZE(1024);

        u64 first  = Hashtable::hash(input, TABLE_SIZE);
        u64 second = Hashtable::hash(input, TABLE_SIZE);

        if (first != second) {
            throw UnitTestFail("hash result changed");
        }
    }

    void test_hash_range() {

        int SAMPLES = 1000;
        u64 TABLE_SIZE(1024);

        for (int i = 0; i < SAMPLES; i++) {
            u64 result = Hashtable::hash(INPUT_TYPE(i), TABLE_SIZE);

            if (result > TABLE_SIZE) {
                throw UnitTestFail("hash result out of range");
            }
        }
    }

    void test_hashtable_insert_one() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        INPUT_TYPE element = 42;
        u64 TABLE_SIZE = 1024;
        u64 MAX_BUCKET = 10;

        Point encrypted = hash_to_group_element(element);

        vector<Point> expected{ encrypted };
        u64 hashvalue = Hashtable::hash(element, TABLE_SIZE);

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.insert(element, encrypted);

        auto actual = hashtable.table[hashvalue];

        if (POINT_VECTORS_NOT_EQUAL(actual, expected)) {
            throw UnitTestFail("element not found in expected bucket");
        }
    }

    void test_hashtable_insert_many() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        u64 TABLE_SIZE = 16;
        u64 MAX_BUCKET = 10;
        u64 TARGET_HASH = 5;
        u64 TARGET_ELEMENTS = 2;

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);

        vector<Point> expected;

        INPUT_TYPE element = 0;
        while (expected.size() < TARGET_ELEMENTS) {
            Point encrypted = hash_to_group_element(element);

            try {
                hashtable.insert(element, encrypted);
            } catch (std::overflow_error err) {
                // expect to overflow the max_bucket bounds here
            }

            if (Hashtable::hash(element, TABLE_SIZE) == TARGET_HASH) {
                expected.push_back(encrypted);
            }
            element++;
        }

        if (POINT_VECTORS_NOT_EQUAL(hashtable.table[TARGET_HASH], expected)) {
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

    void test_hashtable_insert_one_point() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        INPUT_TYPE element = 42;
        u64 TABLE_SIZE = 1024;
        u64 MAX_BUCKET = 10;

        Point encrypted = hash_to_group_element(element);

        vector<Point> expected{ encrypted };
        u64 hashvalue = Hashtable::hash(encrypted, TABLE_SIZE);

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.insert(encrypted);

        if (POINT_VECTORS_NOT_EQUAL(hashtable.table[hashvalue], expected)) {
            throw UnitTestFail("element not found in expected bucket");
        }
    }

    void test_hashtable_insert_many_points() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        u64 TABLE_SIZE = 16;
        u64 MAX_BUCKET = 10;
        u64 TARGET_HASH = 5;
        u64 TARGET_ELEMENTS = 2;

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);

        vector<Point> expected;

        INPUT_TYPE element = 0;
        while (expected.size() < TARGET_ELEMENTS) {
            Point encrypted = hash_to_group_element(element);

            hashtable.insert(encrypted);

            if (Hashtable::hash(encrypted, TABLE_SIZE) == TARGET_HASH) {
                expected.push_back(encrypted);
            }
            element++;
        }

        if (POINT_VECTORS_NOT_EQUAL(hashtable.table[TARGET_HASH], expected)) {
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
        u64 MAX_BUCKET = 3;

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != MAX_BUCKET) {
                throw UnitTestFail("at least one bucket not padded to max bucket size");
            }
        }
    }

    void test_hashtable_pad_one() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        u64 TABLE_SIZE = 16;
        u64 MAX_BUCKET = 4;

        INPUT_TYPE element = 42;
        Point encrypted = hash_to_group_element(element);

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.insert(element, encrypted);
        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != MAX_BUCKET) {
                throw UnitTestFail("at least one bucket not padded to max bucket size");
            }
        }
    }

    void test_hashtable_pad_many() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        u64 TABLE_SIZE = 16;
        u64 MAX_BUCKET = 10;

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);

        for (INPUT_TYPE i = 0; i < TABLE_SIZE; i++) {
            Point encrypted = hash_to_group_element(i);
            hashtable.insert(i, encrypted);
        }

        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != MAX_BUCKET) {
                throw UnitTestFail("at least one bucket not padded to max bucket size");
            }
        }
    }

    void test_hashtable_pad_empty_threads() {
        u64 TABLE_SIZE = 8;
        u64 MAX_BUCKET = 3;
        u8 THREADS = 8;

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.pad(THREADS);

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != MAX_BUCKET) {
                throw UnitTestFail("at least one bucket not padded to max bucket size");
            }
        }
    }

    void test_hashtable_pad_one_threads() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        u64 TABLE_SIZE = 16;
        u64 MAX_BUCKET = 4;
        u8 THREADS = 8;

        INPUT_TYPE element = 42;
        Point encrypted = hash_to_group_element(element);

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.insert(element, encrypted);
        hashtable.pad(THREADS);

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != MAX_BUCKET) {
                throw UnitTestFail("at least one bucket not padded to max bucket size");
            }
        }
    }

    void test_hashtable_pad_many_threads() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        u64 TABLE_SIZE = 16;
        u64 MAX_BUCKET = 10;
        u8 THREADS = 8;

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);

        for (INPUT_TYPE i = 0; i < TABLE_SIZE; i++) {
            Point encrypted = hash_to_group_element(i);
            hashtable.insert(i, encrypted);
        }

        hashtable.pad(THREADS);

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != MAX_BUCKET) {
                throw UnitTestFail("at least one bucket not padded to max bucket size");
            }
        }
    }

    void test_hashtable_shuffle() {
#if !_USE_FOUR_Q_
        Curve c;
#endif
        u64 TABLE_SIZE = 1;
        u64 MAX_BUCKET = 1000;

        INPUT_TYPE element = 42;
        Point encrypted = hash_to_group_element(element);

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.insert(element, encrypted);
        hashtable.pad();
        hashtable.shuffle();

        auto bucket = hashtable.table[0];
        bool found = false;

        for (auto i = 0; i < bucket.size(); i++) {
            // comparing hex since fourq doesn't have direct Point comparison
            if (to_hex(bucket[i]) != to_hex(encrypted)) { continue; }
            if (i == 0) {
                throw UnitTestFail("inserted element still at front of bucket");
            }
            found = true;
        }

        if (!found) {
            throw UnitTestFail("inserted element disappeared after shuffling");
        }
    }

    void test_hashtable_apply_hash() {
        u64 TABLE_SIZE = 3;
        u64 MAX_BUCKET = 10;
        u64 HASH_SIZE  = 10;

        Hashtable hashtable(TABLE_SIZE, MAX_BUCKET);
        hashtable.pad();
        vector<u8> hashed = hashtable.apply_hash(HASH_SIZE);

        if (hashed.size() != TABLE_SIZE * MAX_BUCKET * HASH_SIZE) {
            throw UnitTestFail("didn't get expected size for hashed hashtable");
        }
    }
}
