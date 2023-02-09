#pragma once

namespace unbalanced_psi::client {

  class Client {

    std::vector<int> dataset;

    public:
      // generate or parse database
      // generate DDH key
      // establish network connections
      Client(std::vector<int> *inset); /* hash f, connection details */

      // do we need this? just run generally
      void run();

      // encrypt elements in dataset
      void offline();

      // send elements to server to be exponentiated
      void round_one();

      // do the pir
      void pir();
  };
}
