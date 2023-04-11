#include <cmath>

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
        std::clog << to_hex(bytes.data(), bytes.size()) << std::endl;
    }

#if OFFLINE_THREADS == 1
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
#else
    void Server::offline() {
        std::future<Hashtable> futures[OFFLINE_THREADS];

        for (auto i = 0; i < OFFLINE_THREADS; i++) {
            u64 min_bucket = i * (hashtable.buckets() / OFFLINE_THREADS);
            u64 max_bucket = (i + 1) * (hashtable.buckets() / OFFLINE_THREADS);
            futures[i] = std::async(&Server::partial_offline, this, min_bucket, max_bucket);
            std::clog << "[ thread" << i << " ] from " << min_bucket << " to " << max_bucket << std::endl;
        }

        for (auto i = 0; i < OFFLINE_THREADS; i++) {
            Hashtable other = futures[i].get();
            std::clog << "[ thread" << i << " ] got our table of size " << other.size << std::endl;
            hashtable.concat(other);
        }

        for (u64 i = 0; i < hashtable.buckets(); i++) {
            if (hashtable.table[i].size() != log2(dataset.size())) {
                std::cerr << "padding failure " << hashtable.table[i].size() << "\n";
                throw std::runtime_error("padding failure");
            }
        }
    }

    Hashtable Server::partial_offline(u64 min_bucket, u64 max_bucket) {

        Curve c;

        Hashtable ht(hashtable.buckets());

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            auto hash = Hashtable::hash(dataset[i], hashtable.buckets());
            if (min_bucket <= hash && hash < max_bucket) {
                Point encrypted = hash_to_group_element(dataset[i]); // h(x)
                encrypted = encrypted * key;                         // h(x)^a

                ht.insert(dataset[i], encrypted);
            }
        }
        // add any required padding
        ht.pad(min_bucket, max_bucket);

        return ht;
    }
#endif


    void Server::online() {
        // set up network connections
        auto ip = std::string("127.0.0.1");
        auto port = 1212;

        Session ddh_session(ios, ip + ':' + std::to_string(port), SessionMode::Server, std::string("ddh_session"));
        Channel ddh_channel = ddh_session.addChannel();

        std::clog << "[server] waiting on request..." << std::endl;
        ddh_channel.waitForConnection();

        Timer timer("[ server ] online:\t");

        vector<u8> request;
        ddh_channel.recv(request);
        std::clog << "[server] request recieved: " << request.size() << std::endl;

        vector<u8> response(request.size());

        Point point;
        for (auto i = 0; i < request.size() / point.sizeBytes(); i++) {
            point.fromBytes(request.data() + (i * point.sizeBytes()));
            point = point * key;
            point.toBytes(response.data() + (i * point.sizeBytes()));
        }

        std::clog << "[server] sending response: " << response.size() << std::endl;
        ddh_channel.send(response);

        timer.stop();
    }

    void Server::to_file(std::string filename) {
        hashtable.to_file(filename);
    }

    int Server::size() {
        return dataset.size();
    }
}
