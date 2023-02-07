#include "utils.h"

namespace unbalanced_psi {

  int main() {
    // unbalanced_psi::test_encrypt();

    int server_size = 10;
    int client_size = 10;
    int overlap     = 2;

    vector<u32> server;
    vector<u32> client;
    auto datasets = unbalanced_psi::generate_datasets(server_size, client_size, overlap);
    std::tie(server, client) = datasets;



    std::cout << "Server:\n";
    for (u32 el: server) {
      std::cout << el << "\n";
    }
    std::cout << "Client:\n";
    for (u32 el: client) {
      std::cout << el << "\n";
    }
    return 0;
  }
}

int main() {
  return unbalanced_psi::main();
}
