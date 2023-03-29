#include <vector>
#include <fstream>
#include <ostream>

#include "client.h"

namespace unbalanced_psi {

    Client::Client(std::string filename, u64 server_size)
        : dataset(read_dataset(filename)),
          encrypted(dataset.size()),
          server_size(server_size),
          ios(IOS_THREADS) {
        // randomly sample secret key
        block seed(std::rand()); // TODO: stop using rand()
        PRNG prng(seed);
        key.randomize(prng);
    }

    void Client::offline() {
        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            encrypted[i] = hash_to_group_element(dataset[i]); // h(y)
            encrypted[i] = encrypted[i] * key;                // h(y)^b
        }
    }

    void Client::online() {
        // set up network connections
        auto ip = std::string("127.0.0.1");
        auto port = 1212;

        Session ddh_session(ios, ip + ':' + std::to_string(port), SessionMode::Client, std::string("ddh_session"));
        Channel ddh_channel = ddh_session.addChannel();
        ddh_channel.waitForConnection();

        std::cout << "[client] sending request..." << std::endl;

        vector<u8> request(encrypted[0].sizeBytes() * encrypted.size());
        auto iter = request.data();
        for (Point element : encrypted) {
            element.toBytes(iter);
            iter += element.sizeBytes();
        }
        ddh_channel.send(request);

        vector<u8> response(request.size());
        std::cout << "[client] waiting on response" << std::endl;
        ddh_channel.recv(response);
        std::cout << "[client] response recieved: " << response.size() << std::endl;


        for (auto i = 0; i < encrypted.size(); i++) {
            encrypted[i].fromBytes(response.data() + (i * encrypted[i].sizeBytes()));
        }
    }

    void Client::finalize(std::string filename) {
        std::cout << "[ client ] finalizing..." << std::endl;
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file) {
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
        }

        file.seekg (0, file.end);
        int filesize = file.tellg();
        file.seekg (0, file.beg);

        vector<u8> bytes(filesize);
        file.read((char*) bytes.data(), filesize);

        int found = 0;
        for (u8 *ptr = bytes.data(); ptr < &bytes.back(); ptr += Point::size) {
            std::cout << "[ client ] reading in Point: ";
            std::cout << to_hex(ptr, Point::size) << std::endl;
            Point result;
            result.fromBytes(ptr);

            for (Point element : encrypted) {
                if (element == result) {
                    found++;
                }
            }
        }
        std::cout << "[ client ] found " << std::to_string(found);
        std::cout << " elements in the intersection" << std::endl;
    }

    void Client::to_file(std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) {
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
        }

        vector<u64> queries(dataset.size());

        for (auto i = 0; i < encrypted.size(); i++) {
            // TODO: use correct table size
            queries[i] = Hashtable::hash(dataset[i], server_size);
        }

        file.write((const char*) queries.data(), queries.size() * sizeof(u64));
    }
}
