#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"

namespace unbalanced_psi {

    class Client {

        std::vector<INPUT_TYPE> dataset;
        vector<Point> encrypted;

        // size of server's database
        u64 server_size;

        Curve curve;
        Number key;

        IOService ios;

        public:
        Client(std::string db_file, u64 server_size);

        // put together request
        void offline();

        // encrypt dataset and compare
        void online();

        // compare pir results to online
        void finalize(std::string filename);

        // write the query indexes to file
        void to_file(std::string filename);
    };
}
