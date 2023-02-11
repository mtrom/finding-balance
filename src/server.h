#pragma once

#include "defines.h"

#define IOS_THREADS 3

namespace unbalanced_psi {


    class Server {

        vector<INPUT_TYPE> dataset;

        // IOService ios(IOS_THREADS);

        public:
        // generate or parse dataset
        // generate DDH key
        // establish network connections
        Server(const vector<INPUT_TYPE>& dataset/* hash fs, connection details, */);

        int size();

        // encrypt elements in dataset
        // create and populate hash table
        void offline();

        // receive client's dataset, encrypt it, and send back
        void listen(/* client's encrypted dataset */); // or `round_one`?

        // do the pir
        void pir();

        private:
        // encrypt an element under our ddh key
        void encrypt(int element);
    };
}
