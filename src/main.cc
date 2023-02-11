#include "server.h"
#include "utils.h"

namespace unbalanced_psi {

    void main() {
        Curve curve;
        auto alice = hash_to_group_element(1235);
        auto bob   = hash_to_group_element(1235);
        std::cout << (alice == bob) << "\n";
    }
}

int main() {
    unbalanced_psi::test_encrypt();

    return 0;
}
