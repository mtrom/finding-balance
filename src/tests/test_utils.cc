#include "test_utils.h"
#include "../utils.h"

namespace unbalanced_psi {

    void compare_vectors(vector<INPUT_TYPE> expected, vector<INPUT_TYPE> actual, std::string err_msg) {
        if (expected == actual) { return; }

        if (expected.size() != actual.size()) {
            throw UnitTestFail(
                err_msg + " : expected size "
                + std::to_string(expected.size()) + " vs. actual "
                + std::to_string(actual.size())
            );
        }

        std::string changes = "";
        for (auto i = 0; i < expected.size(); i++) {
            if (expected[i] != actual[i]) {
                changes += "index " + std::to_string(i) + ": expected ";
                changes += std::to_string(expected[i]) + " vs. actual ";
                changes += std::to_string(actual[i]) + "\n";
            }
        }

        throw UnitTestFail(err_msg + "\n" + changes);
    }

    void test_generate_datasets_overlap() {
        int EXPECTED = 3;
        auto [server, client] = unbalanced_psi::generate_datasets(100, 10, EXPECTED);

        vector<int> overlap(server.size() + client.size());
        vector<int>::iterator iter;

        std::sort(server.begin(), server.end());
        std::sort(client.begin(), client.end());

        iter = std::set_intersection(
            server.begin(),
            server.end(),
            client.begin(),
            client.end(),
            overlap.begin()
        );

        overlap.resize(iter - overlap.begin());

        if (overlap.size() != EXPECTED) {
            throw UnitTestFail(
                "expected intersection of " + std::to_string(EXPECTED) +
                " elements but found " + std::to_string(overlap.size())
            );
        }
    }

    void test_read_dataset() {
        vector<INPUT_TYPE> EXPECTED{1234, 123456, 12345678, 1234567890};
        auto actual = read_dataset("src/tests/test.db");
        compare_vectors(EXPECTED, actual, "dataset from test.db not correct");
    }

    void test_write_read_dataset() {
        auto original = generate_dataset(100);
        write_dataset(original, "/tmp/dataset.db");
        auto readin = read_dataset("/tmp/dataset.db");

        compare_vectors(original, readin, "dataset changed when written to file");
    }
}
