#include <thread>
#include <stdlib.h>

#include <cryptoTools/Common/CLP.h>

#include "client.h"
#include "server.h"
#include "utils.h"

using namespace unbalanced_psi;

void run_both(
        std::string server_in,
        std::string server_out,
        std::string client_in,
        std::string client_out
    ) {

    std::thread sthread([&server_in, &server_out]() {
        Server server(server_in);
        server.offline();
        server.online();
        server.to_file(server_out);
    });

    std::thread cthread([&client_in, &client_out]() {
        Client client(client_in);
        client.offline();
        client.online();
        client.to_file(client_out);
    });

    std::cout << "[ main ] waiting on client to finish..." << std::endl;
    cthread.join();
    std::cout << "[ main ] waiting on server to finish..." << std::endl;
    sthread.join();
    std::cout << "[ main ] done." << std::endl;
}


int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    if (parser.isSet("both")) {
        run_both(
            parser.get<std::string>("server-in"),
            parser.get<std::string>("server-out"),
            parser.get<std::string>("client-in"),
            parser.get<std::string>("client-out")
        );
    } else if (parser.isSet("gen-data")) {
        auto [server, client] = generate_datasets(
            parser.get<int>("server-n"),
            parser.get<int>("client-n"),
            parser.get<int>("overlap")
        );

        write_dataset(server, parser.get<std::string>("server_fn"));
        write_dataset(client, parser.get<std::string>("client_fn"));
    }

    return 0;
}
