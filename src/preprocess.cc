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

    auto hashes = parser.get<u64>("-hashes");
    auto buckets = parser.get<u64>("-buckets");
    auto padding = parser.get<u64>("-pad");

    if (parser.isSet("client") || parser.isSet("-client")) {
        auto input = parser.getOr<std::string>("-db", "out/client.db");

        Cuckoo cuckoo(hashes, buckets);

        vector<INPUT_TYPE> dataset = read_dataset(input);

        Timer timer("[ client ] cuckoo hash", BLUE);
        for (auto element : dataset) {
            cuckoo.insert(element);
        }
        cuckoo.pad(padding);
        timer.stop();

        cuckoo.to_file("out/", "/client.db");

        return 0;
    } else if (parser.isSet("server") || parser.isSet("-server")) {
        auto input = parser.getOr<std::string>("-db", "out/server.db");

        Cuckoo cuckoo(hashes, buckets);

        vector<INPUT_TYPE> dataset = read_dataset(input);

        Timer timer("[ server ] cuckoo hash", RED);
        for (auto element : dataset) {
            cuckoo.insert_all(element);
        }
        cuckoo.pad(padding);
        timer.stop();

        cuckoo.to_file("out/", "/server.db");
        return 0;
    } else {
        std::cerr << "need to specify either --server or --client" << std::endl;
        return 1;
    }
}
