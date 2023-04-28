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

    auto cuckoo_n = parser.get<u64>("-cuckoo-n");

    if (parser.isSet("client") || parser.isSet("-client")) {
        Client client(CLIENT_INPUT, parser.get<u64>("-bucket-n"));

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Client, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        client.offline();

        vector<u8> ready;
        channel.recv(ready);

        Timer online("[ client ] ddh online", BLUE);
        auto [ hashed, queries, comm ] = client.online(channel);
        online.stop();

        std::cout << "[  both  ] online comm (bytes)\t: " << comm << std::endl;

        write_dataset(hashed, CLIENT_ONLINE_OUTPUT);
        write_dataset(queries, CLIENT_QUERY_OUTPUT);
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        Server server(
            SERVER_OFFLINE_INPUT,
            parser.get<u64>("-bucket-n"),
            parser.get<u64>("-bucket-size")
        );

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Server, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        Timer timer("[ server ] ddh offline", RED);
        vector<u8> database = server.offline();
        timer.stop();

        vector<u8> ready { 1 };
        channel.send(ready);

        Timer online("[ server ] ddh online", RED);
        server.online(channel);
        online.stop();

        write_dataset<u8>(database, SERVER_OFFLINE_OUTPUT);
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
    return 0;
}
