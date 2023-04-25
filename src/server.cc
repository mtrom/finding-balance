#include <cmath>

#include "../CTPL/ctpl.h"

#include "server.h"

namespace unbalanced_psi {

    Server::Server(std::string filename, bool encrypted, uint64_t s) : seed(s) {
        if (!encrypted) {
            dataset = read_dataset(filename);
            auto [table_size, max_bucket] = Hashtable::get_params(u64(dataset.size()));
            hashtable.resize(table_size, max_bucket);
        } else {
            hashtable.from_file(filename);
        }

        // randomly sample secret key
        PRNG prng(seed);
        key.randomize(prng);
    }

    void Server::run_offline() {
        Server server(SERVER_OFFLINE_INPUT);

        Timer timer("[ server ] ddh offline", RED);
        server.offline();
        timer.stop();

        server.to_file(SERVER_OFFLINE_OUTPUT);
    }

    void Server::run_offline(u64 instances) {

        // if there is only one instance, don't bother with the thread pool
        if (instances == 1) {
            return Server::run_offline();
        }

        vector<Server> servers;
        for (auto i = 0; i < instances; i++) {
            servers.push_back(
                Server(
                    SERVER_OFFLINE_INPUT_PREFIX
                    + std::to_string(i)
                    + SERVER_OFFLINE_INPUT_SUFFIX
                )
            );
        }

        ctpl::thread_pool threads(MAX_THREADS);
        vector<std::future<void>> futures(instances);

        Timer timer("[ server ] ddh offline", RED);
        for (auto i = 0; i < instances; i++) {
            futures[i] = threads.push([&servers, i](int) {
                Curve c; // inits relic on the thread
                servers[i].offline();
            });
        }
        for (auto i = 0; i < instances; i++) {
            futures[i].get();
        }
        timer.stop();

        for (auto i = 0; i < instances; i++) {
            servers[i].to_file(
                SERVER_OFFLINE_OUTPUT_PREFIX
                + std::to_string(i)
                + SERVER_OFFLINE_OUTPUT_SUFFIX
            );
        }
    }


#if SERVER_OFFLINE_THREADS == 1
    void Server::offline() {

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            Point encrypted = hash_to_group_element(dataset[i]); // h(x)
            encrypted = encrypted * key;                         // h(x)^a

            hashtable.insert(dataset[i], encrypted);
        }

        // add any required padding and shuffle
        hashtable.pad();
        hashtable.shuffle();
    }
#else
    void Server::offline() {

        // randomly sample secret key
        PRNG prng(seed);
        key.randomize(prng);

        // in multiple threads, build different sections of the hashtable
        std::future<Hashtable> futures[SERVER_OFFLINE_THREADS];
        for (auto i = 0; i < SERVER_OFFLINE_THREADS; i++) {
            futures[i] = std::async(
                &Server::partial_offline,
                this,
                i * (hashtable.buckets() / SERVER_OFFLINE_THREADS),
                (i + 1) * (hashtable.buckets() / SERVER_OFFLINE_THREADS)
            );
        }

        // combine partial hash tables
        for (auto i = 0; i < SERVER_OFFLINE_THREADS; i++) {
            Hashtable other = futures[i].get();
            hashtable.concat(other);
        }
    }

    Hashtable Server::partial_offline(u64 min_bucket, u64 max_bucket) {

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

        return ht;
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

    void Server::to_file(std::string filename) {
        hashtable.to_file(filename);
    }

    int Server::size() {
        return dataset.size();
    }
}
