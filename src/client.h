#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"

#define CLIENT_SEED 18

namespace unbalanced_psi {

    class Client {

        // client's dataset
        std::vector<INPUT_TYPE> dataset;

        // client's encrypted dataset
        vector<Point> encrypted;

        // query indices for pir request
        vector<u64> queries;

        // result of pir protocol as bytes
        vector<u8> answer;

        // initializes the group element operations
        Curve curve;

        // secret key
        Number key;


        public:
        /**
         * @params <db_file> filename for dataset
         */
        Client(std::string db_file);

        /**
         * @params <db_file> filename for dataset
         * @params <answer_file> filename for pir results
         */
        Client(std::string db_file, std::string answer_file);

        /**
         * put together query indices for pir request
         * @params <server_size> number of elements in server's dataset
         */
        void prepare_queries(u64 server_size);

        /**
         * sample secret key and encrypt dataset
         */
        void offline();

        /**
         * send dataset for server's encryption and compare to pir results
         *
         * @params <channel> communication channel with the server
         * @return number of overlapping elements
         */
        u64 online(Channel channel);

        /**
         * write the pir query indices to file
         *
         * @params <filename> filename to write queries to
         */
        void to_file(std::string filename);
    };
}
