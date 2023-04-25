#include <vector>
#include <fstream>
#include <ostream>

#include "client.h"

namespace unbalanced_psi {

    Client::Client(std::string filename)
        : dataset(read_dataset(filename)),
          encrypted(dataset.size()),
          queries(dataset.size()) { }

    Client::Client(std::string db_file, std::string answer_file) :
        dataset(read_dataset(db_file)),
        encrypted(dataset.size()),
        queries(dataset.size())
    {
        // read in results from pir
        std::ifstream file(answer_file, std::ios::in | std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot open " + answer_file);
        }

        file.seekg (0, file.end);
        int filesize = file.tellg();
        file.seekg (0, file.beg);

        answer.resize(filesize);
        file.read((char*) answer.data(), filesize);
    }

    void Client::run_prep_queries(u64 server_log) {
        Client client(CLIENT_QUERY_INPUT);

        Timer timer("[ client ] ddh offline (1)", BLUE);
        client.prepare_queries(1 << server_log);
        timer.stop();

        client.to_file(CLIENT_QUERY_OUTPUT);
    }

    void Client::run_prep_queries(u64 server_log, u64 instances) {
        // if only one instance just do it directly
        if (instances == 1) {
            return Client::run_prep_queries(server_log);
        }

        vector<Client> clients;
        for (auto i = 0; i < instances; i++) {
            clients.push_back(
                Client(
                    CLIENT_QUERY_INPUT_PREFIX
                    + std::to_string(i)
                    + CLIENT_QUERY_INPUT_SUFFIX
                )
            );
        }

        // since this is such a small operation, multithreading
        // won't yield much improvement
        Timer timer("[ client ] ddh offline (1)", BLUE);
        for (auto i = 0; i < instances; i++) {
            clients[i].prepare_queries(1 << server_log);
        }
        timer.stop();

        for (auto i = 0; i < instances; i++) {
            clients[i].to_file(
                CLIENT_QUERY_OUTPUT_PREFIX
                + std::to_string(i)
                + CLIENT_QUERY_OUTPUT_SUFFIX
            );
        }
    }


    void Client::prepare_queries(u64 server_size) {

        // determine which columns to query
        for (auto i = 0; i < encrypted.size(); i++) {
            queries[i] = Hashtable::hash(dataset[i], server_size);
        }

    }

    void Client::offline() {

        // randomly sample secret key
        block seed(CLIENT_SEED);
        PRNG prng(seed);
        key.randomize(prng);

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            encrypted[i] = hash_to_group_element(dataset[i]); // h(y)
            encrypted[i] = encrypted[i] * key;                // h(y)^b
        }
    }

    tuple<u64, u64> Client::online(Channel channel) {

        // send encrypted dataset
        vector<u8> request(encrypted.size() * Point::size);
        auto iter = request.data();
        for (Point element : encrypted) {
            element.toBytes(iter);
            iter += Point::size;
        }
        channel.send(request);

        // read in doubly-encrypted dataset and replace our `encrypted` with it
        vector<u8> response(request.size());
        channel.recv(response);

        for (auto i = 0; i < encrypted.size(); i++) {
            encrypted[i].fromBytes(response.data() + (i * Point::size));
        }

        // look through pir results and determine overlap
        int found = 0;
        for (u8 *ptr = answer.data(); ptr < &answer.back(); ptr += Point::size) {
            Point result;
            result.fromBytes(ptr); // h(x)^a
            result = result * key; // h(x)^ab

            for (Point element : encrypted) {
                if (element == result) {
                    found++;
                }
            }
        }

        return std::make_tuple(found, u64(request.size() + response.size()));
    }

    void Client::to_file(std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot open " + filename);
        }

        file.write((const char*) queries.data(), queries.size() * sizeof(u64));
    }
}
