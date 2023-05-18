#include <fstream>
#include <cryptoTools/Common/CLP.h>

#include "client.h"
#include "server.h"

using namespace unbalanced_psi;

int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    if (!parser.isSet("-debug")) {
        std::ofstream nullstream;
        std::clog.rdbuf(nullstream.rdbuf());
    }

    auto params = PSIParams(
        parser.get<u64>("-cuckoo-n"),
        parser.get<u64>("-cuckoo-pad"),
        parser.get<u64>("-cuckoo-hashes"),
        parser.get<u64>("-hashtable-n"),
        parser.getOr<int>("-threads", 1)
    );

    IOService ios(params.threads);
    ios.mPrint = false;

    if (parser.isSet("client") || parser.isSet("-client")) {
        Client client(CLIENT_INPUT, params);

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Client, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        Timer offline("[ client ] ddh offline", BLUE);
        client.offline();
        offline.stop();

        vector<u8> ready(1);
        channel.recv(ready.data(), 1);
        channel.resetStats();

        Timer online("[ client ] ddh online", BLUE);
        auto [ hashed, queries ] = client.online(channel);
        online.stop();

        std::cout << "[  both  ] online comm (bytes)\t: ";
        std::cout << channel.getTotalDataSent() + channel.getTotalDataRecv() << std::endl;

        channel.close();
        session.stop();

        if (params.cuckoo_size == 1) {
            write_dataset(hashed, CLIENT_ONLINE_OUTPUT);
            write_dataset(queries, CLIENT_QUERY_OUTPUT);
            return 0;
        }
        for (auto i = 0; i < queries.size(); i++) {
            write_dataset<u8>(
                hashed.data() + (i * HASH_3_SIZE),
                HASH_3_SIZE,
                CLIENT_ONLINE_OUTPUT_PREFIX
                + std::to_string(i)
                + CLIENT_ONLINE_OUTPUT_SUFFIX
            );
            write_dataset<u64>(
                queries.data() + i,
                1,
                CLIENT_QUERY_OUTPUT_PREFIX
                + std::to_string(i)
                + CLIENT_QUERY_OUTPUT_SUFFIX
            );
        }
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        Server server(SERVER_OFFLINE_INPUT, params);

        // set up network connections
        Session session(ios, "127.0.0.1:1212", SessionMode::Server, "pirpsi");
        Channel channel = session.addChannel();
        channel.waitForConnection();

        Timer timer("[ server ] ddh offline", RED);
        auto hashtables = server.offline();
        timer.stop();

        vector<u8> ready { 1 };
        channel.send(ready);
        channel.resetStats();

        Timer online("[ server ] ddh online", RED);
        server.online(channel);
        online.stop();

        channel.close();
        session.stop();

        if (params.cuckoo_size == 1) {
            hashtables[0].to_file(SERVER_OFFLINE_OUTPUT);
            return 0;
        }

        for (auto i = 0; i < hashtables.size(); i++) {
            hashtables[i].to_file(
                SERVER_OFFLINE_OUTPUT_PREFIX
                + std::to_string(i)
                + SERVER_OFFLINE_OUTPUT_SUFFIX
            );
        }
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
    return 0;
}
