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
        auto dataset = parser.getOr<std::string>("-db", "out/client.db");
        auto answer  = parser.getOr<std::string>("-db", "out/answer.edb");

        Client client(dataset);

        client.online();
        client.finalize(answer);

        return 0;
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        auto input = parser.getOr<std::string>("-db", "out/server.edb");

        Server server(input, true);
        server.online();

        return 0;
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
