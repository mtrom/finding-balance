#include <cryptoTools/Common/CLP.h>

#include "utils.h"

using namespace unbalanced_psi;


int main(int argc, char *argv[]) {
    osuCrypto::CLP parser;
	parser.parse(argc, argv);

    auto [server, client] = generate_datasets(
        1 << parser.get<int>("-server-log"),
        1 << parser.getOr<int>("-client-log", 0),
        parser.getOr<int>("-overlap", 1)
    );
    write_dataset<INPUT_TYPE>(server, parser.getOr<std::string>("-server-fn", "out/server.db"));
    write_dataset<INPUT_TYPE>(client, parser.getOr<std::string>("-client-fn", "out/client.db"));
}
