#include <future>

#include "cuckoo.h"
#include "server.h"
#include "client.h"
#include "utils.h"

namespace unbalanced_psi {

    Cuckoo::Cuckoo() : size(0) {
    }

    Cuckoo::Cuckoo(u64 hashes, u64 buckets) : size(0) {
        resize(hashes, buckets);
    };

    void Cuckoo::resize(u64 h, u64 buckets) {
        hashes = h;
        table.clear();
        table.resize(buckets, vector<INPUT_TYPE>());
    }

    u64 Cuckoo::hash(INPUT_TYPE element, u64 hash_n) {
        block seed(hash_n, element);
        PRNG prng(seed);
        u64 output = prng.get<u64>();
        return output % table.size();
    }

    void Cuckoo::insert(INPUT_TYPE element) {
        u64 i = 0;
        INPUT_TYPE to_insert = element;

        while (true) {
            u64 hash_n = i % hashes;
            u64 index = hash(to_insert, hash_n);
            if (table[index].empty()) {
                table[index].push_back(to_insert);
                break;
            } else {
                auto removed = table[index].back();
                table[index].pop_back();
                table[index].push_back(to_insert);
                to_insert = removed;
            }

            i++;
            if (i > MAX_INSERT) {
                throw std::runtime_error("reached maximum insertion attempts");
            }
        }
        size++;
    }

    void Cuckoo::insert_all(INPUT_TYPE element) {
        vector<u64> indexes;
        for (auto hash_n = 0; hash_n < hashes; hash_n++) {
            u64 index = hash(element, hash_n);

            if (std::find(indexes.begin(), indexes.end(), index) != indexes.end()) {
                continue;
            }

            table[index].push_back(element);
            indexes.push_back(index);
            size++;
        }
    }

    void Cuckoo::insert_all(vector<INPUT_TYPE> elements) {
        // if we aren't cuckoo hashing don't bother with the hashes
        if (table.size() == 1) {
            table[0] = elements;
        } else {
            for (auto element : elements) {
                insert_all(element);
            }
        }
    }

    void Cuckoo::pad(u64 to) {
        if (to == 0) { return; }
        block seed(PADDING_SEED);
        PRNG prng(seed);

        for (u64 index = 0; index < table.size(); index++) {
            if (table[index].size() > to) {
                throw std::runtime_error(
                    "cuckoo bucket already exceeded padding: "
                    + std::to_string(table[index].size()) + " vs. "
                    + std::to_string(to)
                );
            }
            while (table[index].size() < to) {
                table[index].push_back(prng.get<INPUT_TYPE>());
                size++;
            }
        }
    }

    u64 Cuckoo::buckets() {
        return table.size();
    }

    void Cuckoo::log() {
        for (auto i = 0; i < table.size(); i++) {
            auto bucket = table[i];
            for (INPUT_TYPE element : bucket) {
                std::clog << "[ cuckoo ] bucket (" << i << ")\t:";
                std::clog << element << std::endl;
            }
        }
    }

#if false
    void Cuckoo::run_server(PSIParams& params) {

        // set up a server instance for each bucket
        vector<Server> servers;
        for (auto dataset : table) {
            servers.push_back(Server(dataset, params));
        }

        // set up the network connections
        IOService ios(IOS_THREADS);
        ios.mPrint = false;

        vector<Channel> channels;
        for (auto i = 0; i < servers.size(); i++) {
            Session session(
                ios,
                "127.0.0.1:1212",
                SessionMode::Server,
                ("pirpsi-" + std::to_string(i))
            );
            Channel channel = session.addChannel();
            channels.push_back(channel);
        }

        ctpl::thread_pool threads(MAX_THREADS);

        /////////////////////////////  OFFLINE  //////////////////////////////
        Timer offline_timer("[ server ] ddh offline", RED);

        vector<std::future<tuple<u64, vector<u8>>>> offline_futures(table.size());
        for (auto i = 0; i < servers.size(); i++) {
            offline_futures[i] = threads.push([&servers, i](int) {
                return servers[i].offline();
            });
        }

        for (auto i = 0; i < table.size(); i++) {
            offline_futures[i].wait();
        }
        offline_timer.stop();
        //////////////////////////////////////////////////////////////////////

        //////////////////////////////  ONLINE  //////////////////////////////
        Timer online_timer("[ server ] ddh online", RED);
        vector<std::future<void>> online_futures(table.size());
        for (auto i = 0; i < servers.size(); i++) {
            online_futures[i] = threads.push([&servers, &channels, i](int) {
                servers[i].online(channels[i]);
            });
        }

        for (auto i = 0; i < table.size(); i++) {
            online_futures[i].wait();
        }
        online_timer.stop();
        //////////////////////////////////////////////////////////////////////

        // write databases to file
        for (auto i = 0; i < table.size(); i++) {
            auto [ bucket_size, database ] = offline_futures[i].get();

            // prepend the database with the bucket_size as bytes
            vector<u8> bytes(sizeof(bucket_size));
            std::memcpy(bytes.data(), &bucket_size, sizeof(bucket_size));
            database.insert(database.begin(), bytes.begin(), bytes.end());

            write_dataset<u8>(
                database,
                SERVER_OFFLINE_OUTPUT_PREFIX
                + std::to_string(i)
                + SERVER_OFFLINE_OUTPUT_SUFFIX
            );
        }
    }

    void Cuckoo::run_client(PSIParams& params) {

        // set up a client instance for each bucket
        vector<Client> clients;
        for (auto dataset : table) {
            clients.push_back(Client(dataset, params));
        }

        // set up the network connections
        IOService ios(IOS_THREADS);
        ios.mPrint = false;

        vector<Channel> channels;
        for (auto i = 0; i < clients.size(); i++) {
            Session session(
                ios,
                "127.0.0.1:1212",
                SessionMode::Client,
                "pirpsi-" + std::to_string(i)
            );
            Channel channel = session.addChannel();
            channels.push_back(channel);
        }

        ctpl::thread_pool threads(MAX_THREADS);

        /////////////////////////////  OFFLINE  //////////////////////////////
        Timer offline_timer("[ client ] ddh offline", BLUE);

        vector<std::future<void>> offline_futures(table.size());
        for (auto i = 0; i < clients.size(); i++) {
            offline_futures[i] = threads.push([&clients, i](int) {
                clients[i].offline();
            });
        }

        for (auto i = 0; i < table.size(); i++) {
            offline_futures[i].wait();
        }
        offline_timer.stop();
        //////////////////////////////////////////////////////////////////////

        //////////////////////////////  ONLINE  //////////////////////////////
        Timer online_timer("[ client ] ddh online", BLUE);
        vector<std::future<tuple<vector<u8>, vector<u64>, u64>>> online_futures(table.size());
        for (auto i = 0; i < clients.size(); i++) {
            online_futures[i] = threads.push([&clients, &channels, i](int) {
                return clients[i].online(channels[i]);
            });
        }

        for (auto i = 0; i < table.size(); i++) {
            online_futures[i].wait();
        }
        online_timer.stop();
        //////////////////////////////////////////////////////////////////////

        u64 total_comm(0);

        // write databases to file
        for (auto i = 0; i < table.size(); i++) {
            auto [ hashed, queries, comm ] = online_futures[i].get();

            total_comm += comm;

            write_dataset(
                hashed,
                CLIENT_ONLINE_OUTPUT_PREFIX
                + std::to_string(i)
                + CLIENT_ONLINE_OUTPUT_SUFFIX
            );
            write_dataset(
                queries,
                CLIENT_QUERY_OUTPUT_PREFIX
                + std::to_string(i)
                + CLIENT_QUERY_OUTPUT_SUFFIX
            );
        }

        std::cout << "[  both  ] online comm (bytes)\t: " << total_comm << std::endl;
    }
#endif
}
