#pragma once

#include "../defines.h"

namespace unbalanced_psi {
    void compare_vectors(vector<INPUT_TYPE> expected, vector<INPUT_TYPE> actual, std::string err_msg);
    void test_generate_datasets_overlap();
    void test_read_dataset();
    void test_write_read_dataset();
    void test_hash_to_group_element_same();
    void test_hash_to_group_element_diff();
    void test_hash_group_element_same();
    void test_hash_group_element_diff();
}
