#include <cryptoTools/Common/TestCollection.h>

#include "test_cuckoo.h"
#include "test_hashtable.h"
#include "test_utils.h"

using namespace unbalanced_psi;

int main() {

    osuCrypto::TestCollection tests([](osuCrypto::TestCollection& th) {
        th.add("test_generate_datasets_overlap    ", test_generate_datasets_overlap);
        th.add("test_write_read_dataset           ", test_write_read_dataset);
        th.add("test_read_dataset                 ", test_read_dataset);
        th.add("test_hash_to_group_element_same   ", test_hash_to_group_element_same);
        th.add("test_hash_to_group_element_diff   ", test_hash_to_group_element_diff);
        th.add("test_hash_group_element_same      ", test_hash_group_element_same);
        th.add("test_hash_group_element_diff      ", test_hash_group_element_diff);
        th.add("test_hash_repeat                  ", test_hash_repeat);
        th.add("test_hash_range                   ", test_hash_range);
        th.add("test_hashtable_insert_one         ", test_hashtable_insert_one);
        th.add("test_hashtable_insert_many        ", test_hashtable_insert_many);
        th.add("test_hashtable_pad_empty          ", test_hashtable_pad_empty);
        th.add("test_hashtable_pad_one            ", test_hashtable_pad_one);
        th.add("test_hashtable_pad_many           ", test_hashtable_pad_many);
        th.add("test_cuckoo_hash_repeat           ", test_cuckoo_hash_repeat);
        th.add("test_cuckoo_hash_diff             ", test_cuckoo_hash_diff);
        th.add("test_cuckoo_table_insert_one      ", test_cuckoo_table_insert_one);
        th.add("test_cuckoo_table_insert_many     ", test_cuckoo_table_insert_many);
        th.add("test_cuckoo_vector_insert         ", test_cuckoo_vector_insert);
        th.add("test_cuckoo_vector_insert_many    ", test_cuckoo_vector_insert_many);
        th.add("test_cuckoo_vector_insert_overfill", test_cuckoo_vector_insert_overfill);
    });

    tests.runAll();
}
