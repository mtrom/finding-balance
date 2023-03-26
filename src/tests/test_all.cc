#include "../defines.h"

#include "test_utils.h"
#include "test_hashtable.h"

using namespace unbalanced_psi;

int main() {
    TestCollection tests([](TestCollection& th) {
        th.add("test_generate_datasets_overlap ", test_generate_datasets_overlap);
        th.add("test_write_read_dataset        ", test_write_read_dataset);
        th.add("test_read_dataset              ", test_read_dataset);
        th.add("test_hash_repeat               ", test_hash_repeat);
        th.add("test_hash_range                ", test_hash_range);
        th.add("test_hashtable_insert_one      ", test_hashtable_insert_one);
        th.add("test_hashtable_insert_many     ", test_hashtable_insert_many);
        th.add("test_hashtable_pad_empty       ", test_hashtable_pad_empty);
        th.add("test_hashtable_pad_one         ", test_hashtable_pad_one);
        th.add("test_hashtable_pad_many        ", test_hashtable_pad_many);
        th.add("test_hashtable_from_file       ", test_hashtable_from_file);
        th.add("test_hashtable_to_from_file       ", test_hashtable_from_file);
    });

    tests.runAll();
}
