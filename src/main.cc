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
        std::string client_out,
        std::string answer_fn,
        u64 server_n
    ) {

    std::thread sthread([&server_in, &server_out]() {
        Server server(server_in);
        server.offline();
        server.online();
        server.to_file(server_out);
    });

    std::thread cthread([&client_in, &client_out, &server_n, &answer_fn]() {
        Client client(client_in, server_n);
        client.offline();
        client.online();
        client.to_file(client_out);
        if (answer_fn != "") {
            client.finalize(answer_fn);
        }
    });

    std::clog << "[ main ] waiting on client to finish..." << std::endl;
    cthread.join();
    std::clog << "[ main ] waiting on server to finish..." << std::endl;
    sthread.join();
    std::clog << "[ main ] done." << std::endl;
}


int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    if (parser.isSet("both")) {
        run_both(
            parser.get<std::string>("server-in"),
            parser.get<std::string>("server-out"),
            parser.get<std::string>("client-in"),
            parser.get<std::string>("client-out"),
            parser.getOr<std::string>("answer-fn", ""),
            parser.get<u64>("server-n")
        );
    } else if (parser.isSet("gen-data")) {
        auto [server, client] = generate_datasets(
            parser.get<int>("server-n"),
            parser.get<int>("client-n"),
            parser.get<int>("overlap")
        );

        write_dataset(server, parser.get<std::string>("server-fn"));
        write_dataset(client, parser.get<std::string>("client-fn"));
    }

    return 0;
}
