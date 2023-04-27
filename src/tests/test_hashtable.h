#pragma once

#include "../hashtable.h"

namespace unbalanced_psi {
    void test_hash_repeat();
    void test_hash_range();
    void test_hashtable_insert_one();
    void test_hashtable_insert_one_point();
    void test_hashtable_insert_many();
    void test_hashtable_insert_many_points();
    void test_hashtable_pad_empty();
    void test_hashtable_pad_one();
    void test_hashtable_pad_many();
    void test_hashtable_from_file();
    void test_hashtable_to_from_file();
    void test_hashtable_shuffle();
    void test_hashtable_apply_hash();
}
