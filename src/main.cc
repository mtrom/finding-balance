#include <thread>
#include <stdlib.h>

#include "client.h"
#include "server.h"
#include "utils.h"

namespace unbalanced_psi {

    void main() {
        vector<u32> server_dataset;
        vector<u32> client_dataset;
        auto datasets = unbalanced_psi::generate_datasets(64, 16, 4);

        std::thread sthread([&datasets]() {
            Server server(std::get<0>(datasets));
            server.run();
        });

        // TODO: REMOVE THIS
        sleep(5);

        std::thread cthread([&datasets]() {
            Client client(std::get<1>(datasets));
            client.run();
        });

        std::cout << "[ main ] waiting on client to finish..." << std::endl;
        cthread.join();
        std::cout << "[ main ] waiting on server to finish..." << std::endl;
        sthread.join();
        std::cout << "[ main ] done." << std::endl;

    }
}

int main() {
    unbalanced_psi::main();

    return 0;
}
