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
    ios.mPrint = false;

    if (parser.isSet("client") || parser.isSet("-client")) {
        auto dataset  = parser.getOr<std::string>("-db", "out/client.db");
        auto answer   = parser.getOr<std::string>("-ans", "out/answer.edb");
        auto expected = parser.get<u64>("-expected");

        Client client(dataset, answer);

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Client, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        Timer offline("[ client ] ddh offline (2)", BLUE);
        client.offline();
        offline.stop();

        vector<u8> ready;
        channel.recv(ready);

        Timer online("[ client ] ddh online", BLUE);
        auto [ actual, comm ] = client.online(channel);
        online.stop();

        std::cout << "[  both  ] online comm (bytes)\t: " << comm << std::endl;

        if (actual == expected) {
            std::cout << GREEN << "\n[ client ] SUCCESS" << RESET << std::endl;
        } else {
            std::cout << RED << "[ client ] FAILURE: expected ";
            std::cout << expected << " but found " << actual;
            std::cout << RESET << std::endl;
        }

        return 0;
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        auto input = parser.getOr<std::string>("-db", "out/server.edb");

        Server server;

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Server, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        vector<u8> ready { 1 };
        channel.send(ready);

        Timer online("[ server ] ddh online", RED);
        server.online(channel);
        online.stop();

        return 0;
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
