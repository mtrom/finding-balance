#include "server.h"

namespace unbalanced_psi {

    Server::Server(const vector<INPUT_TYPE>& inputs)
        : dataset(inputs), encrypted(dataset.size()), hashtable(dataset.size()), ios(IOS_THREADS) {

        // randomly sample secret key
        block seed(std::rand()); // TODO: stop using rand()
        PRNG prng(seed);
        key.randomize(prng);

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            encrypted[i] = hash_to_group_element(dataset[i]); // h(x)
            encrypted[i] = encrypted[i] * key;                // h(x)^a

            hashtable.insert(encrypted[i]);
        }
    }

    void Server::run() {
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

        hashtable.tofile("bin/server.db");
        Hashtable readin(hashtable.buckets());
        readin.fromfile("bin/server.db");
        std::cout << "readin.size = " << readin.size << std::endl;
    }

    int Server::size() {
        return dataset.size();
    }
}
