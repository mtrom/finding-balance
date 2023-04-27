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

    Server::Server() {
        // randomly sample secret key
        PRNG prng(SERVER_SEED);
        key.randomize(prng);
    }

    void Server::run_offline(u64 hashtable_size, u64 bucket_size) {
        Server server(SERVER_OFFLINE_INPUT, hashtable_size, bucket_size);

        Timer timer("[ server ] ddh offline", RED);
        vector<u8> output = server.offline();
        timer.stop();

        write_dataset<u8>(output, SERVER_OFFLINE_OUTPUT);
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
        vector<std::future<vector<u8>>> futures(instances);

        Timer timer("[ server ] ddh offline", RED);
        for (auto i = 0; i < instances; i++) {
            futures[i] = threads.push([&servers, i](int) {
                Curve c; // inits relic on the thread
                return servers[i].offline();
            });
        }

        vector<vector<u8>> outputs(instances);
        for (auto i = 0; i < instances; i++) {
            outputs[i] = futures[i].get();
        }
        timer.stop();

        for (auto i = 0; i < instances; i++) {
            write_dataset<u8>(
                outputs[i],
                SERVER_OFFLINE_OUTPUT_PREFIX + std::to_string(i) + SERVER_OFFLINE_OUTPUT_SUFFIX
            );
        }
    }


#if SERVER_OFFLINE_THREADS == 1
    vector<u8> Server::offline() {
        Hashtable hashtable(hashtable_size, bucket_size);

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            Point encrypted = hash_to_group_element(dataset[i]); // h(x)
            encrypted = encrypted * key;                         // h(x)^a

            hashtable.insert(dataset[i], encrypted);
        }

        // add any required padding and shuffle
        hashtable.pad();
        hashtable.shuffle();
        return hashtable.apply_hash(HASH_3_SIZE);
    }
#else
    vector<u8> Server::offline() {

        // in multiple threads, build different sections of the hashtable
        std::future<vector<u8>> futures[SERVER_OFFLINE_THREADS];
        for (auto i = 0; i < SERVER_OFFLINE_THREADS; i++) {
            futures[i] = std::async(
                &Server::partial_offline,
                this,
                i * (hashtable.buckets() / SERVER_OFFLINE_THREADS),
                (i + 1) * (hashtable.buckets() / SERVER_OFFLINE_THREADS)
            );
        }

        // combine partial results
        vector<u8> output;
        for (auto i = 0; i < SERVER_OFFLINE_THREADS; i++) {
            vector<u8> partial = futures[i].get();
            output.insert(output.end(), partial.begin(), partial.end());
        }

        return output;
    }

    vector<u8> Server::partial_offline(u64 min_bucket, u64 max_bucket) {

        // each thread needs to initalize a Curve for relic's sake
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

        // add any required padding and shuffle
        ht.pad(min_bucket, max_bucket);
        ht.shuffle();

        return ht.apply_hash(HASH_3_SIZE);
    }
#endif


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
