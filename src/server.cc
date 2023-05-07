#include <cmath>

#include "../CTPL/ctpl.h"

#include "server.h"

namespace unbalanced_psi {

    Server::Server(std::string filename, u64 hsize, u64 bsize) :
        dataset(read_dataset<INPUT_TYPE>(filename)),
        hashtable_size(hsize),
        bucket_size(bsize) { }

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
#if !_USE_FOUR_Q_
                Curve c; // inits relic on the thread
#endif
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

#if _USE_FOUR_Q_
        Point::MakeRandomNonzeroScalar(key);
#else
        // randomly sample secret key
        block seed(SERVER_SEED);
        PRNG prng(seed);
        key.randomize(prng);
#endif

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            Point encrypted = hash_to_group_element(dataset[i]); // h(x)
#if _USE_FOUR_Q_
            encrypted.scalar_multiply(key, true);
#else
            encrypted = encrypted * key;                         // h(x)^a
#endif
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

    void Server::online(Channel channel, u64 queries) {
        // receive client's encrypted dataset
        vector<u8> request;
        channel.recv(request);

        vector<u8> response(request.size());

        // encrypt each point under the server's key
        Point point;
#if _USE_FOUR_Q_
        for (auto i = 0; i < request.size() / Point::save_size; i++) {
            point.load(Point::point_save_span_const_type{
                request.data() + (i * Point::save_size),
                Point::save_size
            });
            point.scalar_multiply(key, true);
            point.save(Point::point_save_span_type{
                response.data() + (i * Point::save_size),
                Point::save_size
            });
        }
#else
        for (auto i = 0; i < request.size() / Point::size; i++) {
            point.fromBytes(request.data() + (i * Point::size));
            point = point * key;
            point.toBytes(response.data() + (i * Point::size));
        }
#endif

        channel.send(response);
    }

    int Server::size() {
        return dataset.size();
    }
}
