#include <algorithm>
#include <fstream>
#include <iterator>
#include <ostream>
#include <optional>
#include <vector>

#include "client.h"
#include "hashtable.h"

#define EMPTY_CUCKOO_BUCKET 0

namespace unbalanced_psi {

    Client::Client(vector<INPUT_TYPE> db, PSIParams& p) :
        dataset(db), params(p) {}

    Client::Client(std::string filename, PSIParams& p) :
        dataset(read_dataset<INPUT_TYPE>(filename)), params(p) {}

    void Client::offline() {
        // sample a random secret key
        Point::MakeRandomNonzeroScalar(key);

        // calculate the encrypted group element for each input
        for (auto i = 0; i < dataset.size(); i++) {
            auto point = hash_to_group_element(dataset[i]); // h(y)
            point.scalar_multiply(key, false);              // h(y)^b
            encrypted.push_back(point);
        }
    }

    tuple<vector<hash_type>, vector<u64>> Client::online(Channel channel) {

        // send encrypted dataset
        vector<u8> request(encrypted.size() * Point::save_size);
        auto iter = request.data();
        for (Point element : encrypted) {
            element.save(Point::point_save_span_type{iter, Point::save_size});
            iter += Point::save_size;
        }
        channel.send(request);

        // read in doubly-encrypted dataset and replace our `encrypted` with it
        vector<u8> response(request.size());
        channel.recv(response);

        // for decrypting with our secret key
        Number inverse;
        Point::InvertScalar(key, inverse);

        // calculate oprf result and query pairs
        vector<hash_type> results;
        vector<u64> queries;
        for (auto i = 0; i < response.size(); i += Point::save_size) {
            // oprf result
            hash_type result(HASH_3_SIZE);

            // parse the group element, decrypt, and hash
            Point point;
            point.load(Point::point_save_span_const_type{           // h(y)^ab
                response.data() + i,
                Point::save_size
            });
            point.scalar_multiply(inverse, false);                  // h(y)^a
            hash_group_element(point, HASH_3_SIZE, result.data());  // g(h(y)^a)

            // index to use as pir query
            queries.push_back(Hashtable::hash(result, params.hashtable_size));
            results.push_back(result);
        }

        if (params.cuckoo_size == 1) {
            return std::make_tuple(results, queries);
        } else {
            CuckooVector cuckoo(params);
            for (auto i = 0; i < results.size(); i++) {
                cuckoo.insert(results[i], queries[i]);
            }
            return cuckoo.split();
        }
    }
}
