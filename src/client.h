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
        Client(std::string db_file);

        // put together request
        void offline();

        // encrypt dataset and compare
        void online();

        // compare pir results to online
        void finalize();

        // write the query indexes to file
        void to_file(std::string filename);
    };
}
