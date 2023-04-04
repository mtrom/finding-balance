#include "server.h"

namespace unbalanced_psi {

    /**
     * @param <filename> input file for dataset
     * @param <encrypted> true if the data is already encrypted
     * @param <seed> seed for randomness in selecting private key
     */
    Server::Server(std::string filename, bool encrypted, uint64_t seed) : ios(IOS_THREADS) {
        // randomly sample secret key
        // TODO: something better than a set seed?
        block bseed(seed);
        PRNG prng(bseed);
        key.randomize(prng);

        if (!encrypted) {
            dataset = read_dataset(filename);
            hashtable.resize(dataset.size());
        } else {
            hashtable.from_file(filename);
        }

        // TODO: delete this
        vector<u8> bytes(key.sizeBytes());
        key.toBytes(bytes.data());
        std::cout << to_hex(bytes.data(), bytes.size()) << std::endl;
    }

    void Server::offline() {
        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            Point encrypted = hash_to_group_element(dataset[i]); // h(x)
            encrypted = encrypted * key;                         // h(x)^a

            hashtable.insert(dataset[i], encrypted);
        }

        // add any required padding
        hashtable.pad();
    }

    void Server::online() {
        // set up network connections
        auto ip = std::string("127.0.0.1");
        auto port = 1212;

        Session ddh_session(ios, ip + ':' + std::to_string(port), SessionMode::Server, std::string("ddh_session"));
        Channel ddh_channel = ddh_session.addChannel();

        std::cout << "[server] waiting on request..." << std::endl;
        ddh_channel.waitForConnection();

        vector<u8> request;
        ddh_channel.recv(request);
        std::cout << "[server] request recieved: " << request.size() << std::endl;

        vector<u8> response(request.size());

        Point point;
        for (auto i = 0; i < request.size() / point.sizeBytes(); i++) {
            point.fromBytes(request.data() + (i * point.sizeBytes()));
            point = point * key;
            point.toBytes(response.data() + (i * point.sizeBytes()));
        }

        std::cout << "[server] sending response: " << response.size() << std::endl;
        ddh_channel.send(response);
    }

    void Server::to_file(std::string filename) {
        hashtable.to_file(filename);
    }

    int Server::size() {
        return dataset.size();
    }
}
