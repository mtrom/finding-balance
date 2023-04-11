#include <vector>
#include <fstream>
#include <ostream>

#include "client.h"

namespace unbalanced_psi {

    Client::Client(std::string filename)
        : dataset(read_dataset(filename)),
          encrypted(dataset.size()),
          queries(dataset.size()),
          ios(IOS_THREADS) {
        // randomly sample secret key
        block seed(std::rand()); // TODO: stop using rand()
        PRNG prng(seed);
        key.randomize(prng);
    }

    void Client::offline(u64 server_size) {
        // determine which columns to query
        for (auto i = 0; i < encrypted.size(); i++) {
            std::clog << "[ client ] query for " << dataset[i];
            queries[i] = Hashtable::hash(dataset[i], server_size);
            std::clog << " is " << queries[i] << std::endl;
        }

    }

    void Client::online() {
        // set up network connections
        auto ip = std::string("127.0.0.1");
        auto port = 1212;

        Session ddh_session(ios, ip + ':' + std::to_string(port), SessionMode::Client, std::string("ddh_session"));
        Channel ddh_channel = ddh_session.addChannel();
        ddh_channel.waitForConnection();

        Timer timer("[ client ] online: \t");

        // TODO: should this be offline as well? two offlines?
        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            encrypted[i] = hash_to_group_element(dataset[i]); // h(y)
            std::clog << "[ client ] h(y): " << to_hex(encrypted[i]) << std::endl;
            encrypted[i] = encrypted[i] * key;                // h(y)^b
            std::clog << "[ client ] h(y)^b: " << to_hex(encrypted[i]) << std::endl;
        }


        std::clog << "[client] sending request..." << std::endl;

        vector<u8> request(encrypted.size() * Point::size);
        auto iter = request.data();
        for (Point element : encrypted) {
            element.toBytes(iter);
            iter += Point::size;
        }
        ddh_channel.send(request);

        vector<u8> response(request.size());
        std::clog << "[client] waiting on response" << std::endl;
        ddh_channel.recv(response);
        std::clog << "[client] response recieved: " << response.size() << std::endl;


        for (auto i = 0; i < encrypted.size(); i++) {
            std::clog << "[ client ] received h(y)^ab: ";
            std::clog << to_hex(response.data() + (i * Point::size), Point::size) << std::endl;
            encrypted[i].fromBytes(response.data() + (i * Point::size));
        }
        timer.stop();
    }

    void Client::finalize(std::string filename) {
        std::clog << "[ client ] finalizing..." << std::endl;
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file) {
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
        }

        file.seekg (0, file.end);
        int filesize = file.tellg();
        file.seekg (0, file.beg);

        vector<u8> bytes(filesize);
        file.read((char*) bytes.data(), filesize);

        Timer timer("[ client ] finalize:\t");

        int found = 0;
        for (u8 *ptr = bytes.data(); ptr < &bytes.back(); ptr += Point::size) {
            std::clog << "[ client ] reading in Point: ";
            std::clog << to_hex(ptr, Point::size) << std::endl;
            Point result;
            result.fromBytes(ptr); // h(x)^a
            result = result * key; // h(x)^ab
            std::clog << "            (and encrypted): ";
            std::clog << to_hex(result) << std::endl;

            for (Point element : encrypted) {
                if (element == result) {
                    std::clog << "[ client ] found: " << to_hex(element) << std::endl;
                    found++;
                }
            }
        }

        timer.stop();

        std::cout << "[ client ] found " << std::to_string(found);
        std::cout << " elements in the intersection" << std::endl;
    }

    void Client::to_file(std::string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) {
            throw std::filesystem::filesystem_error("cannot open " + filename, std::error_code());
        }

        file.write((const char*) queries.data(), queries.size() * sizeof(u64));
    }
}
