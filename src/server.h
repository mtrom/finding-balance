#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"


#define SERVER_SEED block(406)
#define SERVER_OFFLINE_THREADS 1

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

        // hashtable of encrypted data
        Hashtable hashtable;

        // initializes the group element operations
        Curve curve;

        // secret key
        Number key;


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
         * @params <hashtable_size> number of buckets in the hashtable
         * @params <bucket_size> max number of elements in a hashtable bucket
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
         * setup a basic server with just the private key setup
         */
        Server();

        /**
         * @return number of elements in the dataset
         */
        int size();

        /**
         * encrypt dataset under secret key and prepare hashtable
         */
        void offline();

#if SERVER_OFFLINE_THREADS != 1
        /**
         * prepare a partial hashset
         *
         * @params <min_bucket> minimum bucket to populate, inclusive
         * @params <max_bucket> maximum bucket to populate, exclusive
         * @return hashtable with elements and padding from min to max bucket
         *         and empty in all other buckets
         */
        Hashtable partial_offline(u64 min_bucket, u64 max_bucket);
#endif

        /**
         * reply to encryption request on client's set
         *
         * @params <channel> communication channel with the client
         */
        void online(Channel channel);

        /**
         * write the encrypted hashtable to a file
         *
         * @params <filename> filename to write encrypted dataset to
         */
        void to_file(std::string filename);
    };
}
