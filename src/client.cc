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

#if _USE_FOUR_Q_
        Point::MakeRandomNonzeroScalar(key);
#else
        // randomly sample secret key
        block seed(CLIENT_SEED);
        PRNG prng(seed);
        key.randomize(prng);
#endif

        encrypted.resize(dataset.size());

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            encrypted[i] = hash_to_group_element(dataset[i]); // h(y)
#if _USE_FOUR_Q_
            encrypted[i].scalar_multiply(key, false);
#else
            encrypted[i] = encrypted[i] * key;        // h(y)^b
#endif
        }
    }

    tuple<vector<u8>, vector<u64>, u64> Client::online(Channel channel) {

        // send encrypted dataset
#if _USE_FOUR_Q_
        vector<u8> request(encrypted.size() * Point::save_size);
        auto iter = request.data();
        for (Point element : encrypted) {
            element.save(Point::point_save_span_type{iter, Point::save_size});
            iter += Point::save_size;
        }
#else
        vector<u8> request(encrypted.size() * Point::size);
        auto iter = request.data();
        for (Point element : encrypted) {
            element.toBytes(iter);
            iter += Point::size;
        }
#endif
        channel.send(request);

        // read in doubly-encrypted dataset and replace our `encrypted` with it
        vector<u8> response(request.size());
        channel.recv(response);

        vector<u8> hashed(encrypted.size() * HASH_3_SIZE);
        auto ptr = hashed.data();

        vector<u64> queries(encrypted.size());

#if _USE_FOUR_Q_
        Number inverse;
        Point::InvertScalar(key, inverse);
#endif
        for (auto i = 0; i < encrypted.size(); i++) {
#if _USE_FOUR_Q_
            encrypted[i].load(Point::point_save_span_const_type{
                response.data() + (i * Point::save_size),
                Point::save_size
            }); // h(y)^ab
            encrypted[i].scalar_multiply(inverse, false);
#else
            encrypted[i].fromBytes(response.data() + (i * Point::size)); // h(y)^ab
            encrypted[i] = encrypted[i] * key.inverse();                 // h(y)^a
#endif
            hash_group_element(encrypted[i], HASH_3_SIZE, ptr);          // g(h(y)^a)
            ptr += HASH_3_SIZE;

            queries[i] = Hashtable::hash(encrypted[i], hashtable_size);
        }

#if _USE_FOUR_Q_
        // uploaded and downloaded each point
        u64 comm = Point::save_size * encrypted.size() * 2;
#else
        u64 comm = u64(request.size() + response.size());
#endif
        return std::make_tuple(hashed, queries, comm);
    }
}
