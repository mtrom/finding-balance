#include "server.h"

namespace unbalanced_psi {

    Server::Server(vector<INPUT_TYPE> db, PSIParams& p) : dataset(db), params(p) { }

    Server::Server(std::string filename, PSIParams& p) :
        dataset(read_dataset<INPUT_TYPE>(filename)), params(p) { }

    vector<Hashtable> Server::offline() {
        // sample a random secret key
        Point::MakeRandomNonzeroScalar(key);


        Timer timer("[ server ] encrypt & hash", GREEN);
        // calculate the encrypted hash for each input
        vector<hash_type> output;
        if (params.threads == 1) {
            output = encrypt(dataset.data(), dataset.size());
        } else {
            vector<future<vector<hash_type>>> futures(params.threads);

            // encrypt elements in seperate threads
            int batch = dataset.size() / params.threads + (dataset.size() % params.threads != 0);
            for (auto i = 0; i < params.threads; i++) {
                futures[i] = std::async(
                    std::launch::async,
                    &Server::encrypt,
                    this,
                    dataset.data() + (i * batch),
                    i + 1 < params.threads ? batch : dataset.size() - (i * batch)
                );
            }

            for (auto i = 0; i < params.threads; i++) {
                auto partial = futures[i].get();
                output.insert(output.end(), partial.begin(), partial.end());
            }
        }
        timer.stop();

        timer = Timer("[ server ] organize", GREEN);
        if (params.cuckoo_size == 1) {
            Hashtable hashtable(params.hashtable_size);
            for (auto i = 0; i < output.size(); i++) {
                hashtable.insert(output[i]);
            }
            hashtable.pad();
            timer.stop();
            return vector<Hashtable>{hashtable};
        } else {
            CuckooTable cuckoo(params);
            for (auto i = 0; i < output.size(); i++) {
                cuckoo.insert(output[i]);
            }
            cuckoo.pad();
            timer.stop();
            return cuckoo.table;
        }
    }

    vector<hash_type> Server::encrypt(INPUT_TYPE* elements, int size) {
        vector<hash_type> encrypted;
        for (auto i = 0; i < size; i++) {
            hash_type hashed(HASH_3_SIZE);
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
