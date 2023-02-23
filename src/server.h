#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"


namespace unbalanced_psi {


    class Server {

        vector<INPUT_TYPE> dataset;
        vector<Point> encrypted;
        Hashtable hashtable;

        Curve curve;
        Number key;

        IOService ios;

        public:
        // generate or parse dataset
        // generate DDH key
        // establish network connections
        Server(const vector<INPUT_TYPE>& dataset/* hash fs, connection details, */);

        int size();

        void run();

        // do the pir
        void pir();

        private:
        // encrypt an element under our ddh key
        void encrypt(int element);
    };
}
