#include <fstream>
#include <cryptoTools/Common/CLP.h>

#include "cuckoo.h"
#include "utils.h"

using namespace unbalanced_psi;

int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    std::ofstream nullstream;
    std::clog.rdbuf(nullstream.rdbuf());

    auto hashes = parser.get<u64>("-hashes-n");
    auto cuckoo_n = parser.get<u64>("-cuckoo-n");
    auto cuckoo_pad = parser.get<u64>("-cuckoo-pad");
    auto hashtable_size = parser.get<u64>("-bucket-n");

    if (parser.isSet("client") || parser.isSet("-client")) {
        Cuckoo cuckoo(hashes, cuckoo_n);

        vector<INPUT_TYPE> dataset = read_dataset<INPUT_TYPE>(
            parser.getOr<std::string>("-db", "out/client.db")
        );

        Timer timer("[ client ] cuckoo hash", BLUE);
        for (auto element : dataset) {
            cuckoo.insert(element);
        }
        cuckoo.pad(1);
        timer.stop();

        cuckoo.run_client(hashtable_size);

        return 0;
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        Cuckoo cuckoo(hashes, cuckoo_n);

        vector<INPUT_TYPE> dataset = read_dataset<INPUT_TYPE>(
            parser.getOr<std::string>("-db", "out/server.db")
        );

        Timer timer("[ server ] cuckoo hash", RED);
        for (auto element : dataset) {
            cuckoo.insert_all(element);
        }
        cuckoo.pad(cuckoo_pad);
        timer.stop();

        cuckoo.run_server(hashtable_size, parser.get<u64>("-bucket-size"));

        return 0;
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
