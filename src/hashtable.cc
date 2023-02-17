#include <cmath>

#include "hashtable.h"

#include "cryptoTools/Common/block.h"

namespace unbalanced_psi {

    Hashtable::Hashtable(int size) : hashf(sizeof(int)) {
        table.resize(size, vector<Point>());
    };

    int Hashtable::hash(Point element) {
        // TODO: make a more uniform hash?
        vector<u8> bytes(sizeof(element.sizeBytes()));
        element.toBytes(bytes.data());

        u32 output = 0;

        for (u8 byte : bytes) {
            output = output << 8;
            output += byte;
        }

        return output % table.size();
    }

    void Hashtable::insert(Point element) {
        int index = hash(element);
        table[index].push_back(element);

        if (table[index].size() > log2(table.size())) {
            throw std::overflow_error("more than log2(size) collisions");
        }
    }

    int Hashtable::max_bucket() {
        int collisions = 0;
        for (vector<Point> bucket : table) {
            if (collisions < bucket.size()) { collisions = bucket.size(); }
        }
        return collisions;
    }
}
