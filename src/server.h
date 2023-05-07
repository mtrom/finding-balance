#pragma once

#include "defines.h"
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


    class Server {

        // server's dataset
        vector<INPUT_TYPE> dataset;

#if !_USE_FOUR_Q_
        // initializes the group element operations
        Curve curve;
#endif

        // secret key
        Number key;

        // number of buckets in the hashtable
        u64 hashtable_size;

        // max number of elements in the hashtable
        u64 bucket_size;

        public:

        /**
         * run the server's offline portion of the protocol
         *
         * @params <hashtable_size> number of buckets in the hashtable
         * @params <bucket_size> max number of elements in a hashtable bucket
         */
        static void run_offline(u64 hashtable_size, u64 bucket_size);

        /**
         * run many server offlines for each cuckoo hash bucket
         *
         * @params <instances> number of cuckoo buckets being run
         * @params <hashtable_size> number of buckets in the psi hashtable
         * @params <bucket_size> max number of elements in a psi hashtable bucket
         */
        static void run_offline(u64 instances, u64 hashtable_size, u64 bucket_size);

        /**
         * setup server with input dataset and parameters
         *
         * @params <filename> filename for dataset
         * @params <hashtable_size> number of buckets in the hashtable
         * @params <bucket_size> max number of elements in a hashtable bucket
         */
        Server(std::string db_file, u64 hashtable_size, u64 bucket_size);

        /**
         * encrypt dataset under secret key and prepare hashtable
         *
         * @return size of buckets in database elements and bytes to
         *         be used as input for SimplePIR
         */
        tuple<u64, vector<u8>> offline();

        /**
         * reply to encryption request on client's set
         *
         * @params <channel> communication channel with the client
         * @params <queries> the number of queries to field
         */
        void online(Channel channel, u64 queries);

        /**
         * @return number of elements in the dataset
         */
        int size();

    };
}
