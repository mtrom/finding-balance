#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"

#define CLIENT_SEED 18

#define CLIENT_INPUT  "out/client.db"

#define CLIENT_QUERY_OUTPUT "out/queries.db"
#define CLIENT_ONLINE_OUTPUT "out/client.edb"

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

        // size of the server's hashtable
        u64 hashtable_size;

        // initializes the group element operations
        Curve curve;

        // secret key
        Number key;

        public:

        /**
         * @params <db_file> filename for dataset
         * @params <hashtable_size> size of server's hashtable
         */
        Client(std::string db_file, u64 hashtable_size);

        /**
         * sample secret key and encrypt dataset
         */
        void offline();

        /**
         * send dataset for server's encryption and compare to pir results
         *
         * @params <channel> communication channel with the server
         * @return result of the oprf, query inputs to pir, & comm size in bytes
         */
        tuple<vector<u8>, vector<u64>, u64> online(Channel channel);
    };
}
