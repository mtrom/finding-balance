#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"

namespace unbalanced_psi {

    class Client {

        std::vector<INPUT_TYPE> dataset;
        vector<Point> encrypted;

        Curve curve;
        Number key;

        IOService ios;

        public:
        // generate or parse database
        // generate DDH key
        // establish network connections
        Client(const vector<INPUT_TYPE>& inputs); /* hash f, connection details */

        // do we need this? just run generally
        void run();

        // encrypt elements in dataset
        void offline();

        // send elements to server to be exponentiated
        void round_one();

        // do the pir
        void pir();
    };
}
