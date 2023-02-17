#include "server.h"

namespace unbalanced_psi {

    Server::Server(const vector<INPUT_TYPE>& inputs)
        : dataset(inputs), encrypted(dataset.size()), hashtable(dataset.size()) {

        // randomly sample secret key
        block seed(std::rand()); // TODO: stop using rand()
        PRNG prng(seed);
        key.randomize(prng);

        // hash elements in dataset
        for (auto i = 0; i < dataset.size(); i++) {
            encrypted[i] = hash_to_group_element(dataset[i]); // h(x)
            encrypted[i] = encrypted[i] * key;                // h(x)^a

            hashtable.insert(encrypted[i]);
        }

        std::cout << hashtable.max_bucket() << std::endl;

    }

    int Server::size() {
        return dataset.size();
    }
}
