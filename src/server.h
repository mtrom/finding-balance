#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"


namespace unbalanced_psi {


    class Server {

        vector<INPUT_TYPE> dataset;
        Hashtable hashtable;

        Curve curve;
        Number key;

        IOService ios;

        public:
        Server(std::string db_file);

        int size();

        // encrypt dataset
        void offline();

        // reply to encryption request on client set
        void online();

        // write hashtable to file
        void to_file(std::string filename);

        private:
        // encrypt an element under our ddh key
        void encrypt(int element);
    };
}
