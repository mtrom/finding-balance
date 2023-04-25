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

    auto cuckoo_n = parser.get<u64>("-cuckoo-n");

    if (parser.isSet("client") || parser.isSet("-client")) {
        Client::run_prep_queries(
            parser.get<u64>("-server-log"), cuckoo_n
        );
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        Server::run_offline(cuckoo_n);
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
    return 0;
}
