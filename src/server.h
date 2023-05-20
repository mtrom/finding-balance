#pragma once

#include "defines.h"
#include "cuckoo.h"
#include "hashtable.h"
#include "utils.h"


#define SERVER_SEED block(406)

#define SERVER_OFFLINE_INPUT  "out/server.db"
#define SERVER_OFFLINE_OUTPUT "out/server.edb"

#define SERVER_OFFLINE_INPUT_PREFIX "out/"
#define SERVER_OFFLINE_INPUT_SUFFIX "/server.db"
#define SERVER_OFFLINE_OUTPUT_PREFIX "out/"
#define SERVER_OFFLINE_OUTPUT_SUFFIX "/server.edb"

namespace unbalanced_psi {

    using hash_type = vector<u8>;

    class Server {

        // server's dataset
        vector<INPUT_TYPE> dataset;

        // secret key
        Number key;

        // parameters for psi protocol
        PSIParams params;

        public:

        /**
         * setup server with input dataset and parameters
         */
        Server(vector<INPUT_TYPE> dataset, PSIParams& params);

        /**
         * setup server from file with parameters
         *
         * @params <cuckoo_size> number of buckets in the cuckoo table
         * @params <hashtable_size> number of buckets in each hashtable
         * @params <bucket_size> max number of elements in a hashtable bucket
         */
        Server(std::string db_file, PSIParams& params);

        /**
         * encrypt dataset under secret key and prepare hashtable
         *
         * @return for each cuckoo table bucket, return (1) the number of dataset
         *         elements in each bucket of the hashtable, and (2) the bytes
         *         to be used as input for SimplePIR
         */
        vector<Hashtable> offline();

        /**
         * reply to encryption request on client's set
         *
         * @params <channel> communication channel with the client
         * @params <queries> the number of queries to field
         */
        void online(Channel channel);

        /**
         * @return number of elements in the dataset
         */
        int size();

        private:

        /**
         * hash given input to group elements and encrypt under the secret key
         */
        vector<hash_type> encrypt(INPUT_TYPE* elements, int size);
    };
}
