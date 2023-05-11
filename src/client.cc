#include <algorithm>
#include <fstream>
#include <iterator>
#include <ostream>
#include <optional>
#include <vector>

#include "client.h"
#include "cuckoo.h"

#define EMPTY_CUCKOO_BUCKET 0

namespace unbalanced_psi {

    Client::Client(vector<INPUT_TYPE> db, PSIParams& p) :
        dataset(db), params(p) {}

    Client::Client(std::string filename, PSIParams& p) :
        dataset(read_dataset<INPUT_TYPE>(filename)), params(p) {}

    void Client::offline() {
#if _USE_FOUR_Q_
        Point::MakeRandomNonzeroScalar(key);
#else
        // randomly sample secret key
        block seed(CLIENT_SEED);
        PRNG prng(seed);
        key.randomize(prng);
#endif

        Cuckoo cuckoo(params.cuckoo_hashes, params.cuckoo_size);
        if (params.cuckoo_size > 1) {
            for (auto element : dataset) {
                cuckoo.insert(element);
            }
            encrypted.resize(params.cuckoo_size);
        } else {
            encrypted.resize(dataset.size());
        }

        for (auto i = 0; i < encrypted.size(); i++) {
            // if there's an empty cuckoo bucket don't bother creating an element
            if (params.cuckoo_size > 1 && cuckoo.table[i].empty()) {
                encrypted[i] = std::nullopt;
                continue;
            }
            INPUT_TYPE element = params.cuckoo_size > 1 ? cuckoo.table[i][0] : dataset[i];
            auto point = hash_to_group_element(element); // h(y)
#if _USE_FOUR_Q_
            point.scalar_multiply(key, false); // h(y)^b
#else
            point = point * key;               // h(y)^b
#endif
            encrypted[i] = std::optional<Point>{point};
        }
    }

    tuple<vector<u8>, vector<u64>> Client::online(Channel channel) {
        // send encrypted dataset
#if _USE_FOUR_Q_
        vector<u8> request(dataset.size() * Point::save_size);
        auto iter = request.data();
        for (std::optional<Point> element : encrypted) {
            if (!element) { continue; }
            element.value().save(Point::point_save_span_type{iter, Point::save_size});
            iter += Point::save_size;
        }
#else
        vector<u8> request(dataset.size() * Point::size);
        auto iter = request.data();
        for (std::optional<Point> element : encrypted) {
            if (!element) { continue; }
            element.value().toBytes(iter);
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
        auto res_index = 0;
        for (auto i = 0; i < encrypted.size(); i++) {
            // if there's an empty cuckoo bucket it doens't matter what the query / hashes are
            if (!encrypted[i]) { ptr += HASH_3_SIZE; continue; }
            Point point;

#if _USE_FOUR_Q_
            point.load(Point::point_save_span_const_type{
                response.data() + (res_index * Point::save_size),
                Point::save_size
            }); // h(y)^ab
            point.scalar_multiply(inverse, false);
#else
            point.fromBytes(response.data() + (res_index * Point::size)); // h(y)^ab
            point = point * key.inverse();         // h(y)^a
#endif
            hash_group_element(point, HASH_3_SIZE, ptr);          // g(h(y)^a)
            queries[i] = Hashtable::hash(point, params.hashtable_size);

            ptr += HASH_3_SIZE;
            res_index++;
        }

        return std::make_tuple(hashed, queries);
    }
}
