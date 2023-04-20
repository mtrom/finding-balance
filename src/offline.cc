#include <fstream>
#include <cryptoTools/Common/CLP.h>

#include "client.h"
#include "server.h"

using namespace unbalanced_psi;


int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    std::ofstream nullstream;
    std::clog.rdbuf(nullstream.rdbuf());

    if (parser.isSet("client") || parser.isSet("-client")) {
        auto input = parser.getOr<std::string>("-db", "out/client.db");
        auto output = parser.getOr<std::string>("-out", "out/queries.db");
        auto server_log = parser.get<u64>("-server-log");

        Client client(input);

        Timer timer("[ client ] ddh offline (1)", BLUE);
        client.prepare_queries(1 << server_log);
        timer.stop();

        client.to_file(output);

        return 0;
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        auto input = parser.getOr<std::string>("-db", "out/server.db");
        auto output = parser.getOr<std::string>("-out", "out/server.edb");

        Server server(input);

        Timer timer("[ server ] ddh offline", RED);
        server.offline();
        timer.stop();

        server.to_file(output);

        return 0;
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
