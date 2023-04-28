#include <cmath>

#include "../CTPL/ctpl.h"

#include "server.h"

namespace unbalanced_psi {

    Server::Server(std::string filename, u64 hsize, u64 bsize)
        : dataset(read_dataset<INPUT_TYPE>(filename)), hashtable_size(hsize), bucket_size(bsize) {
        // randomly sample secret key
        PRNG prng(SERVER_SEED);
        key.randomize(prng);
    }

    void Server::run_offline(u64 hashtable_size, u64 bucket_size) {
        Server server(SERVER_OFFLINE_INPUT, hashtable_size, bucket_size);

        Timer timer("[ server ] ddh offline", RED);
        auto [ b_size, database ] = server.offline();
        timer.stop();

        // prepend the database with the b_size as bytes
        vector<u8> output(sizeof(b_size));
        std::memcpy(output.data(), &b_size, sizeof(b_size));
        database.insert(database.begin(), output.begin(), output.end());

        write_dataset<u8>(database, SERVER_OFFLINE_OUTPUT);
    }

    void Server::run_offline(u64 instances, u64 hashtable_size, u64 bucket_size) {

        // if there is only one instance, don't bother with the thread pool
        if (instances == 1) {
            return Server::run_offline(hashtable_size, bucket_size);
        }

        vector<Server> servers;
        for (auto i = 0; i < instances; i++) {
            servers.push_back(
                Server(
                    SERVER_OFFLINE_INPUT_PREFIX
                    + std::to_string(i)
                    + SERVER_OFFLINE_INPUT_SUFFIX,
                    hashtable_size,
                    bucket_size
                )
            );
        }

        ctpl::thread_pool threads(MAX_THREADS);
        vector<std::future<tuple<u64, vector<u8>>>> futures(instances);

        Timer timer("[ server ] ddh offline", RED);
        for (auto i = 0; i < instances; i++) {
            futures[i] = threads.push([&servers, i](int) {
                Curve c; // inits relic on the thread
                return servers[i].offline();
            });
        }

        vector<tuple<u64, vector<u8>>> outputs(instances);
        for (auto i = 0; i < instances; i++) {
            outputs[i] = futures[i].get();
        }
        timer.stop();

        for (auto i = 0; i < instances; i++) {
            auto [b_size, database] = outputs[i];
            // prepend the database with the bucket_size as bytes
            vector<u8> output(sizeof(b_size));
            std::memcpy(output.data(), &b_size, sizeof(b_size));
            database.insert(database.begin(), output.begin(), output.end());

            write_dataset<u8>(
                database,
                SERVER_OFFLINE_OUTPUT_PREFIX + std::to_string(i) + SERVER_OFFLINE_OUTPUT_SUFFIX
            );
        }
    }

    tuple<u64, vector<u8>> Server::offline() {
        Hashtable hashtable(hashtable_size, bucket_size);

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            Point encrypted = hash_to_group_element(dataset[i]); // h(x)
            encrypted = encrypted * key;                         // h(x)^a

            hashtable.insert(encrypted);
        }

        // add any required padding and shuffle
        hashtable.pad();
        hashtable.shuffle();
        return std::make_tuple(
            u64(hashtable.max_bucket()),
            hashtable.apply_hash(HASH_3_SIZE)
        );
    }

    void Server::online(Channel channel) {
        // receive client's encrypted dataset
        vector<u8> request;
        channel.recv(request);

        vector<u8> response(request.size());

        // encrypt each point under the server's key
        Point point;
        for (auto i = 0; i < request.size() / point.sizeBytes(); i++) {
            point.fromBytes(request.data() + (i * point.sizeBytes()));
            point = point * key;
            point.toBytes(response.data() + (i * point.sizeBytes()));
        }

        channel.send(response);
    }

    int Server::size() {
        return dataset.size();
    }
}
