#include <cmath>

#include "../CTPL/ctpl.h"

#include "server.h"

namespace unbalanced_psi {

    Server::Server(vector<INPUT_TYPE> db, PSIParams& p) : dataset(db), params(p) { }

    Server::Server(std::string filename, PSIParams& p) :
        dataset(read_dataset<INPUT_TYPE>(filename)), params(p) { }

    vector<Hashtable> Server::offline() {
        Point::MakeRandomNonzeroScalar(key);

        if (params.cuckoo_size > 1 && params.threads > 1) {
            return offline(params.threads, params.cuckoo_size);
        } else if (params.cuckoo_size == 1 && params.threads > 1) {
            vector<Hashtable> output;
            output.push_back(offline(params.threads));
            return output;
        } else if (params.cuckoo_size > 1 && params.threads == 1) {
            return offline(params.cuckoo_size);
        } else {
            vector<vector<u8>> hashes = encrypt(dataset.data(), dataset.size());

            Hashtable hashtable(params.hashtable_size);

            for (auto hash : hashes) {
                hashtable.insert(hash);
            }

            hashtable.pad();

            return vector<Hashtable>{hashtable};
        }
    }

    Hashtable Server::offline(int threads) {

        ctpl::thread_pool tpool(threads);
        vector<std::future<vector<vector<u8>>>> futures(threads);

        // encrypt elements in seperate threads
        int batch = dataset.size() / threads + (dataset.size() % threads != 0);
        for (auto i = 0; i < threads; i++) {
            INPUT_TYPE* elements = dataset.data() + (i * batch);
            int size = i + 1 < threads ? batch : dataset.size() - (i * batch);
            futures[i] = tpool.push([this, elements, size](int) {
                return this->encrypt(elements, size);
            });
        }

        Hashtable hashtable(params.hashtable_size);

        // gather encrypted elements and insert into the hashtable
        for (auto i = 0; i < threads; i++) {
            vector<vector<u8>> hashes = futures[i].get();
            for (auto j = 0; j < hashes.size(); j++) {
                hashtable.insert(hashes[j]);
            }
        }

        hashtable.pad();

        return hashtable;
    }

    /**
     * run offline with cuckoo hashing on one thread
     */
    vector<Hashtable> Server::offline(u64 cuckoo_size) {
        // set up cuckoo hashtable
        Cuckoo cuckoo(params.cuckoo_hashes, params.cuckoo_size);
        cuckoo.insert_all(dataset);
        cuckoo.pad(params.cuckoo_pad);

        vector<Hashtable> output;

        for (auto i = 0; i < params.cuckoo_size; i++) {
            output.push_back(get_offline_output(&cuckoo.table[i]));
        }

        return output;
    }

    vector<Hashtable> Server::offline(int threads, u64 cuckoo_size) {
        // set up cuckoo hashtable
        Cuckoo cuckoo(params.cuckoo_hashes, params.cuckoo_size);
        cuckoo.insert_all(dataset);
        cuckoo.pad(params.cuckoo_pad);

        ctpl::thread_pool tpool(threads);
        vector<std::future<Hashtable>> futures(params.cuckoo_size);

        for (auto i = 0; i < params.cuckoo_size; i++) {
            auto bucket = &cuckoo.table[i];
            futures[i] = tpool.push([this, bucket](int) {
                return this->get_offline_output(bucket);
            });
        }

        vector<Hashtable> output;
        for (auto i = 0; i < params.cuckoo_size; i++) {
            output.push_back(futures[i].get());
        }

        return output;
    }

    Hashtable Server::get_offline_output(vector<INPUT_TYPE>* elements) {
        Hashtable hashtable(params.hashtable_size);
        vector<vector<u8>> hashes = encrypt(elements->data(), elements->size());

        for (auto i = 0; i < hashes.size(); i++) {
            hashtable.insert(hashes[i]);
        }

        hashtable.pad();

        return hashtable;
    }

    vector<vector<u8>> Server::encrypt(INPUT_TYPE* elements, int size) {
        vector<vector<u8>> encrypted;
        for (auto i = 0; i < size; i++) {
            vector<u8> hashed(HASH_3_SIZE);
            auto point = hash_to_group_element(*(elements + i));     // h(x)
            point.scalar_multiply(key, true);                        // h(x)^a
            hash_group_element(point, hashed.size(), hashed.data()); // g(h(x)^a)
            encrypted.push_back(hashed);
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
        channel.send(response);
    }

    int Server::size() {
        return dataset.size();
    }
}
