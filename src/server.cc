#include <cmath>

#include "../CTPL/ctpl.h"

#include "server.h"

namespace unbalanced_psi {

    Server::Server(vector<INPUT_TYPE> db, PSIParams& p) : dataset(db), params(p) { }

    Server::Server(std::string filename, PSIParams& p) :
        dataset(read_dataset<INPUT_TYPE>(filename)), params(p) { }

    vector<tuple<u64, vector<u8>>> Server::offline() {
#if _USE_FOUR_Q_
        Point::MakeRandomNonzeroScalar(key);
#else
        // randomly sample secret key
        block seed(SERVER_SEED);
        PRNG prng(seed);
        key.randomize(prng);
#endif

        Cuckoo cuckoo(params.cuckoo_hashes, params.cuckoo_size);
        cuckoo.insert_all(dataset);
        cuckoo.pad(params.cuckoo_pad);

        vector<tuple<u64, vector<u8>>> output(params.cuckoo_size);
        for (auto i = 0; i < params.cuckoo_size; i++) {
            Hashtable hashtable(params.hashtable_size, params.hashtable_pad);
            for (auto element : cuckoo.table[i]) {
                Point encrypted = hash_to_group_element(element); // h(x)
#if _USE_FOUR_Q_
                encrypted.scalar_multiply(key, true); // h(x)^a
#else
                encrypted = encrypted * key; // h(x)^a
#endif
                hashtable.insert(encrypted);
            }
            hashtable.pad();
            hashtable.shuffle();
            output[i] = std::make_tuple(
                u64(hashtable.max_bucket()),
                hashtable.apply_hash(HASH_3_SIZE)
            );
        }

        return output;
    }

    void Server::online(Channel channel) {
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
