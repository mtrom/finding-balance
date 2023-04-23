#include <fstream>
#include <cryptoTools/Common/CLP.h>
#include "../CTPL/ctpl.h"

#include "client.h"
#include "server.h"

using namespace unbalanced_psi;

int client_offline(u64 server_log, u64 instances) {
    // if not cuckoo hashing
    if (instances == 1) {
        Client client("out/client.db");

        Timer timer("[ client ] ddh offline (1)", BLUE);
        client.prepare_queries(1 << server_log);
        timer.stop();

        client.to_file("out/queries.db");

        return 0;
    }


    vector<Client> clients;
    for (auto i = 0; i < instances; i++) {
        clients.push_back(Client("out/" + std::to_string(i) + "/client.db"));
    }

    ctpl::thread_pool threads(MAX_THREADS);
    vector<std::future<void>> futures(instances);

    Timer timer("[ client ] ddh offline (1)", BLUE);
    for (auto i = 0; i < instances; i++) {
        futures[i] = threads.push([&clients, i, &server_log](int) {
            clients[i].prepare_queries(1 << server_log);
        });
    }
    for (auto i = 0; i < instances; i++) {
        futures[i].get();
    }
    timer.stop();

    for (auto i = 0; i < instances; i++) {
        clients[i].to_file("out/" + std::to_string(i) + "/queries.db");
    }

    return 0;
}

int server_offline(u64 instances) {
    // if not cuckoo hashing
    if (instances == 1) {
        Server server("out/server.db");

        Timer timer("[ server ] ddh offline", RED);
        server.offline();
        timer.stop();

        server.to_file("out/server.edb");
        return 0;
    }

    vector<Server> servers;
    for (auto i = 0; i < instances; i++) {
        servers.push_back(Server("out/" + std::to_string(i) + "/server.db"));
    }

    ctpl::thread_pool threads(MAX_THREADS);
    vector<std::future<void>> futures(instances);

    Timer timer("[ server ] ddh offline", RED);
    for (auto i = 0; i < instances; i++) {
        futures[i] = threads.push([&servers, i](int) {
            Curve c;
            servers[i].offline();
        });
    }
    for (auto i = 0; i < instances; i++) {
        futures[i].get();
    }
    timer.stop();

    for (auto i = 0; i < instances; i++) {
        servers[i].to_file("out/" + std::to_string(i) + "/server.edb");
    }

    return 0;
}


int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    std::ofstream nullstream;
    std::clog.rdbuf(nullstream.rdbuf());

    if (parser.isSet("client") || parser.isSet("-client")) {
        return client_offline(
            parser.get<u64>("-server-log"),
            parser.get<u64>("-cuckoo-n")
        );
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        return server_offline(
            parser.get<u64>("-cuckoo-n")
        );
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
