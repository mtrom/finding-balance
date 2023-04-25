#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"

#define CLIENT_SEED 18

#define CLIENT_QUERY_INPUT  "out/client.db"
#define CLIENT_QUERY_OUTPUT "out/queries.db"

#define CLIENT_QUERY_INPUT_PREFIX "out/"
#define CLIENT_QUERY_INPUT_SUFFIX "/client.db"
#define CLIENT_QUERY_OUTPUT_PREFIX "out/"
#define CLIENT_QUERY_OUTPUT_SUFFIX "/queries.db"

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
         * run client's pir query preparation
         *
         * @params <server_log> log of the server's dataset size
         */
        static void run_prep_queries(u64 server_log);

        /**
         * run many client's pir query preparation for each cuckoo bucket
         *
         * @params <server_log> log of the server's dataset size
         * @params <instances> number of cuckoo buckets being run
         */
        static void run_prep_queries(u64 server_log, u64 instances);

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
         * @return number of overlapping elements and communication size in bytes
         */
        tuple<u64, u64> online(Channel channel);

        /**
         * write the pir query indices to file
         *
         * @params <filename> filename to write queries to
         */
        void to_file(std::string filename);
    };
}
