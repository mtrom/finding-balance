#include "../defines.h"

#include "test_utils.h"

using namespace unbalanced_psi;

int main() {
    TestCollection tests([](TestCollection& th) {
        th.add("test_generate_datasets_overlap ", test_generate_datasets_overlap);
        th.add("test_write_read_dataset        ", test_write_read_dataset);
        th.add("test_read_dataset              ", test_read_dataset);
    });

    tests.runAll();
}
