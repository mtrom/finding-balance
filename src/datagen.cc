#include <cryptoTools/Common/CLP.h>

#include "utils.h"

using namespace unbalanced_psi;


int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    auto [server, client] = generate_datasets(
        1 << parser.get<int>("-server-log"),
        1 << parser.get<int>("-client-log"),
        parser.get<int>("-overlap")
    );
    write_dataset<INPUT_TYPE>(server, parser.getOr<std::string>("-server-fn", "out/server.db"));
    write_dataset<INPUT_TYPE>(client, parser.getOr<std::string>("-client-fn", "out/client.db"));
}
