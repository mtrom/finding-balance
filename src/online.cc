#include <fstream>
#include <cryptoTools/Common/CLP.h>

#include "client.h"
#include "server.h"

using namespace unbalanced_psi;

#define IOS_THREADS 3

int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    std::ofstream nullstream;
    std::clog.rdbuf(nullstream.rdbuf());

    IOService ios(IOS_THREADS);

    if (parser.isSet("client") || parser.isSet("-client")) {
        auto dataset = parser.getOr<std::string>("-db", "out/client.db");
        auto answer  = parser.getOr<std::string>("-db", "out/answer.edb");

        Client client(dataset, answer);

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Client, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        Timer offline("[ client ] offline (2):\t");
        client.offline();
        offline.stop();

        Timer online("[ client ] online:\t");
        auto found = client.online(channel);
        online.stop();

        std::cout << "[ client ] found " << std::to_string(found);
        std::cout << " elements in the intersection" << std::endl;

        return 0;
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        auto input = parser.getOr<std::string>("-db", "out/server.edb");

        Server server(input, true);

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Server, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        Timer online("[ server ] online:\t");
        server.online(channel);
        online.stop();

        return 0;
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
