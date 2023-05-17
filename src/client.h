#pragma once

#include <optional>

#include "defines.h"
#include "hashtable.h"
#include "utils.h"

#define CLIENT_SEED 18

#define CLIENT_INPUT  "out/client.db"

#define CLIENT_QUERY_OUTPUT "out/queries.db"
#define CLIENT_ONLINE_OUTPUT "out/client.edb"

#define CLIENT_QUERY_OUTPUT_PREFIX "out/"
#define CLIENT_QUERY_OUTPUT_SUFFIX "/queries.db"

#define CLIENT_ONLINE_OUTPUT_PREFIX "out/"
#define CLIENT_ONLINE_OUTPUT_SUFFIX "/client.edb"

namespace unbalanced_psi {

    class Client {

        // client's dataset
        std::vector<INPUT_TYPE> dataset;

        // client's encrypted dataset
        vector<std::optional<Point>> encrypted;

        // cuckoo table
        std::unordered_map<INPUT_TYPE, int> cuckoo_indexes;

#if !_USE_FOUR_Q_
        // initializes the group element operations
        Curve curve;
#endif
        // secret key
        Number key;

        // parameters for psi protocol
        PSIParams params;

        public:

        /**
         * setup server with input dataset and parameters
         *
         * @params <hashtable_size> size of server's hashtable
         */
        Client(vector<INPUT_TYPE> dataset, PSIParams& params);

        /**
         * setup server from file
         */
        Client(std::string db_file, PSIParams& params);

        /**
         * sample secret key and encrypt dataset
         */
        void offline();

        /**
         * send dataset for server's encryption and compare to pir results
         *
         * @params <channel> communication channel with the server
         * @return result of the oprf and query inputs to pir
         */
        tuple<vector<u8>, vector<u64>> online(Channel channel);
    };
}
