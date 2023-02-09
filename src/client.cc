#include <vector>

#include "client.h"

using std::vector;

namespace unbalanced_psi::client {
  Client::Client(vector<int> *inset) {
      dataset = *inset;
  }
}
