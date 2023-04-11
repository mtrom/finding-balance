#pragma once

#include "defines.h"
#include "hashtable.h"
#include "utils.h"


#define OFFLINE_THREADS 4


namespace unbalanced_psi {


    class Server {

        vector<INPUT_TYPE> dataset;
        Hashtable hashtable;

        Curve curve;
        Number key;

        IOService ios;

        public:

        Server(
            std::string db_file,
            bool encrypted = false,
            uint64_t seed = uint64_t(406)
        );

        int size();

        // encrypt dataset
        void offline();

#if OFFLINE_THREADS != 1
        Hashtable partial_offline(u64 min_bucket, u64 max_bucket);
#endif

        // reply to encryption request on client set
        void online();

        // write hashtable to file
        void to_file(std::string filename);

        private:
        // encrypt an element under our ddh key
        void encrypt(int element);
    };
}
