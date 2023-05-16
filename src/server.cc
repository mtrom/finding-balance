#include <cmath>

#include "../CTPL/ctpl.h"

#include "server.h"

namespace unbalanced_psi {

    using OFFLINE_TUPLE = tuple<u64, vector<u8>>;

    Server::Server(vector<INPUT_TYPE> db, PSIParams& p) : dataset(db), params(p) { }

    Server::Server(std::string filename, PSIParams& p) :
        dataset(read_dataset<INPUT_TYPE>(filename)), params(p) { }

    vector<OFFLINE_TUPLE> Server::offline() {
#if _USE_FOUR_Q_
        Point::MakeRandomNonzeroScalar(key);
#else
        // randomly sample secret key
        block seed(SERVER_SEED);
        PRNG prng(seed);
        key.randomize(prng);
#endif

        if (params.cuckoo_size > 1 && params.threads > 1) {
            return offline(params.threads, params.cuckoo_size);
        } else if (params.cuckoo_size == 1 && params.threads > 1) {
            vector<OFFLINE_TUPLE> output(1);
            output[0] = offline(params.threads);
            return output;
        } else if (params.cuckoo_size > 1 && params.threads == 1) {
            return offline(params.cuckoo_size);
        } else {
            vector<Point> encrypted = encrypt(dataset.data(), dataset.size());

            Hashtable hashtable(params.hashtable_size, params.hashtable_pad);

            for (auto point : encrypted) {
                hashtable.insert(point);
            }

            hashtable.pad();
            if (!params.dynamic_padding()) { hashtable.shuffle(); }

            return vector<OFFLINE_TUPLE>{std::make_tuple(
                u64(hashtable.max_bucket),
                hashtable.apply_hash(HASH_3_SIZE)
            )};
        }
    }

    OFFLINE_TUPLE Server::offline(int threads) {

        ctpl::thread_pool tpool(threads);
        vector<std::future<vector<Point>>> futures(threads);

        // encrypt elements in seperate threads
        int batch = dataset.size() / threads + (dataset.size() % threads != 0);
        for (auto i = 0; i < threads; i++) {
            INPUT_TYPE* elements = dataset.data() + (i * batch);
            int size = i + 1 < threads ? batch : dataset.size() - (i * batch);
            futures[i] = tpool.push([this, elements, size](int) {
                return this->encrypt(elements, size);
            });
        }

        Hashtable hashtable(params.hashtable_size, params.hashtable_pad);

        // gather encrypted elements and insert into the hashtable
        for (auto i = 0; i < threads; i++) {
            vector<Point> encrypted = futures[i].get();
            for (auto point : encrypted) {
                hashtable.insert(point);
            }
        }

        auto pad_to = hashtable.max_bucket;
        hashtable.pad(threads);
        if (!params.dynamic_padding()) { hashtable.shuffle(); }

        return std::make_tuple(
            u64(hashtable.max_bucket),
            hashtable.apply_hash(HASH_3_SIZE)
        );
    }

    /**
     * run offline with cuckoo hashing on one thread
     */
    vector<OFFLINE_TUPLE> Server::offline(u64 cuckoo_size) {
        // set up cuckoo hashtable
        Cuckoo cuckoo(params.cuckoo_hashes, params.cuckoo_size);
        cuckoo.insert_all(dataset);
        cuckoo.pad(params.cuckoo_pad);

        vector<OFFLINE_TUPLE> output(params.cuckoo_size);

        for (auto i = 0; i < params.cuckoo_size; i++) {
            output[i] = get_offline_output(&cuckoo.table[i]);
        }

        return output;
    }

    vector<OFFLINE_TUPLE> Server::offline(int threads, u64 cuckoo_size) {
        // set up cuckoo hashtable
        Cuckoo cuckoo(params.cuckoo_hashes, params.cuckoo_size);
        cuckoo.insert_all(dataset);
        cuckoo.pad(params.cuckoo_pad);

        ctpl::thread_pool tpool(threads);
        vector<std::future<OFFLINE_TUPLE>> futures(params.cuckoo_size);

        for (auto i = 0; i < params.cuckoo_size; i++) {
            auto bucket = &cuckoo.table[i];
            futures[i] = tpool.push([this, bucket](int) {
                return this->get_offline_output(bucket);
            });
        }

        vector<OFFLINE_TUPLE> output(params.cuckoo_size);
        for (auto i = 0; i < params.cuckoo_size; i++) {
            output[i] = futures[i].get();
        }

        return output;
    }

    OFFLINE_TUPLE Server::get_offline_output(vector<INPUT_TYPE>* elements) {
        Hashtable hashtable(params.hashtable_size, params.hashtable_pad);
        vector<Point> encrypted = encrypt(elements->data(), elements->size());

        for (auto point : encrypted) {
            hashtable.insert(point);
        }

        hashtable.pad();
        if (!params.dynamic_padding()) { hashtable.shuffle(); }

        return std::make_tuple(
            u64(hashtable.max_bucket),
            hashtable.apply_hash(HASH_3_SIZE)
        );
    }

    vector<Point> Server::encrypt(INPUT_TYPE* elements, int size) {
        vector<Point> encrypted(size);
        for (auto i = 0; i < size; i++) {
            encrypted[i] = hash_to_group_element(*(elements + i)); // h(x)
#if _USE_FOUR_Q_
            encrypted[i].scalar_multiply(key, true);             // h(x)^a
#else
            encrypted[i] = encrypted[i] * key;                          // h(x)^a
#endif
        }
        return encrypted;
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
