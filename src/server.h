#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"


#define SERVER_SEED 406
#define OFFLINE_THREADS 1


namespace unbalanced_psi {


    class Server {

        // server's dataset
        vector<INPUT_TYPE> dataset;

        // hashtable of encrypted data
        Hashtable hashtable;

        // initializes the group element operations
        Curve curve;

        // seed for randomness in selecting private key
        block seed;

        // secret key
        Number key;


        public:

        /**
         * @params <filename> filename for dataset
         * @params <encrypted> true if the data is already encrypted
         * @params <seed> seed for randomness in selecting private key
         */
        Server(
            std::string db_file,
            bool encrypted = false,
            uint64_t s = uint64_t(SERVER_SEED)
        );

        /**
         * @return number of elements in the dataset
         */
        int size();

        /**
         * encrypt dataset under secret key and prepare hashtable
         */
        void offline();

#if OFFLINE_THREADS != 1
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
