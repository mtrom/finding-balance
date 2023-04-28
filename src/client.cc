#include <algorithm>
#include <fstream>
#include <iterator>
#include <ostream>
#include <vector>

#include "client.h"

namespace unbalanced_psi {

    Client::Client(std::string filename, u64 ht_size) :
        dataset(read_dataset<INPUT_TYPE>(filename)), hashtable_size(ht_size) {}

    void Client::offline() {

        // randomly sample secret key
        block seed(CLIENT_SEED);
        PRNG prng(seed);
        key.randomize(prng);

        encrypted.resize(dataset.size());

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            encrypted[i] = hash_to_group_element(dataset[i]); // h(y)
            encrypted[i] = encrypted[i] * key;                // h(y)^b
        }
    }

    tuple<vector<u8>, vector<u64>, u64> Client::online(Channel channel) {

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

        vector<u8> hashed(encrypted.size() * HASH_3_SIZE);
        auto ptr = hashed.data();

        vector<u64> queries(encrypted.size());

        for (auto i = 0; i < encrypted.size(); i++) {
            encrypted[i].fromBytes(response.data() + (i * Point::size)); // h(y)^ab
            encrypted[i] = encrypted[i] * key.inverse();                 // h(y)^a

            hash_group_element(encrypted[i], HASH_3_SIZE, ptr);          // g(h(y)^a)
            ptr += HASH_3_SIZE;

            queries[i] = Hashtable::hash(encrypted[i], hashtable_size);
        }

        return std::make_tuple(hashed, queries, u64(request.size() + response.size()));
    }
}
