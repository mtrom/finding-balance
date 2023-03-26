#include "test_hashtable.h"

#include <stdio.h>

#include "../utils.h"

namespace unbalanced_psi {

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
        INPUT_TYPE element = 42;
        u64 TABLE_SIZE = 1024;

        Curve curve;
        Point encrypted = hash_to_group_element(element);

        vector<Point> expected{ encrypted };
        u64 hashvalue = Hashtable::hash(element, TABLE_SIZE);

        Hashtable hashtable(TABLE_SIZE);
        hashtable.insert(element, encrypted);

        if (hashtable.table[hashvalue] != expected) {
            throw UnitTestFail("element not found in expected bucket");
        }
    }

    void test_hashtable_insert_many() {
        u64 TABLE_SIZE = 16;
        u64 TARGET_HASH = 5;
        u64 TARGET_ELEMENTS = 2;

        Curve curve;
        Hashtable hashtable(TABLE_SIZE);

        vector<Point> expected;

        INPUT_TYPE element = 0;
        while (expected.size() < TARGET_ELEMENTS) {
            Point encrypted = hash_to_group_element(element);

            try {
                hashtable.insert(element, encrypted);
            } catch (std::overflow_error err) {
                // expect to overflow the log2 bounds here
            }

            if (Hashtable::hash(element, TABLE_SIZE) == TARGET_HASH) {
                expected.push_back(encrypted);
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
        u64 expected = 3;

        Hashtable hashtable(TABLE_SIZE);
        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != expected) {
                throw UnitTestFail("at least one bucket not padded to log2 size");
            }
        }
    }

    void test_hashtable_pad_one() {
        u64 TABLE_SIZE = 16;
        u64 expected = 4;

        Curve curve;
        INPUT_TYPE element = 42;
        Point encrypted = hash_to_group_element(element);

        Hashtable hashtable(TABLE_SIZE);
        hashtable.insert(element, encrypted);
        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != expected) {
                throw UnitTestFail("at least one bucket not padded to log2 size");
            }
        }
    }

    void test_hashtable_pad_many() {
        u64 TABLE_SIZE = 16;
        u64 expected = 4;

        Hashtable hashtable(TABLE_SIZE);

        Curve curve;
        for (INPUT_TYPE i = 0; i < TABLE_SIZE; i++) {
            Point encrypted = hash_to_group_element(i);
            hashtable.insert(i, encrypted);
        }

        hashtable.pad();

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != expected) {
                throw UnitTestFail("at least one bucket not padded to log2 size");
            }
        }
    }

    void test_hashtable_from_file() {
        Hashtable hashtable("src/tests/test.edb");

        if (hashtable.buckets() != 1024) {
            throw UnitTestFail("expected 1024 buckets from test.edb");
        } else if (hashtable.size != 10240) {
            throw UnitTestFail("expected 10240 elements in test.edb");
        }

        for (int i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != 10) {
                throw UnitTestFail("expected all buckets to have 10 items in test.edb");
            }
        }
    }

    void test_hashtable_to_from_file() {
        u64 TABLE_SIZE = 8;
        vector<INPUT_TYPE> elements{ 0, 1, 2, 3, 4, 5, 6, 7 };
        std::string FILENAME = "/tmp/dataset.edb";

        Curve curve;
        Hashtable expected(TABLE_SIZE);
        for (INPUT_TYPE element : elements) {
            Point encrypted = hash_to_group_element(element);
            expected.insert(element, encrypted);
        }

        expected.to_file(FILENAME);
        Hashtable actual(FILENAME);

        if (expected.buckets() != actual.buckets()) {
            throw UnitTestFail("incorrect number of buckets");
        }

        int errors = 0;
        for (auto bucket = 0; bucket < actual.buckets(); bucket++) {
            if (expected.table[bucket].size() != actual.table[bucket].size()) {
                throw UnitTestFail("incorrect number of elements in bucke");
            }
            for (auto element = 0; bucket < actual.table[bucket].size(); element++) {
                if (expected.table[bucket][element] != actual.table[bucket][element]) {
                    errors++;
                }
            }
        }

        if (errors != 0) {
            char errmsg[30];
            std::snprintf(errmsg, sizeof(errmsg), "found %d mismatched elements", errors);
            throw UnitTestFail(errmsg);
        }

    }
}
