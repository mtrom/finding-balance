#include <cryptoTools/Common/CLP.h>

#include "client.h"
#include "server.h"

using namespace unbalanced_psi;


int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    if (parser.isSet("client") || parser.isSet("-client")) {
        auto input = parser.getOr<std::string>("-db", "out/client.db");
        auto output = parser.getOr<std::string>("-out", "out/queries.db");
        auto server_n = parser.get<u64>("-server-n");

        Client client(input);
        client.offline(server_n);
        client.to_file(output);

        return 0;
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        auto input = parser.getOr<std::string>("-db", "out/server.db");
        auto output = parser.getOr<std::string>("-out", "out/server.edb");

        Server server(input);
        server.offline();
        server.to_file(output);

        return 0;
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
