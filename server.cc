#include <vector>

#include "server.h"

using std::vector;

namespace unbalanced_psi::server {
  Server::Server(vector<int> *inset) {
      dataset = *inset;
  }
}
