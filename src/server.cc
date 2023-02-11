#include <vector>

#include "server.h"

namespace unbalanced_psi {

  Server::Server(const vector<INPUT_TYPE>& dataset) {
      this->dataset = dataset;
  }

  int Server::size() {
    return dataset.size();
  }
}
