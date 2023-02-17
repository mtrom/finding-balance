#include "server.h"
#include "utils.h"
#include "hashtable.h"

namespace unbalanced_psi {

    void main() {
        auto dataset = generate_dataset(1000);
        Server server(dataset);
        std::cout << server.size() << "\n";
    }
}

int main() {
    unbalanced_psi::main();

    return 0;
}
