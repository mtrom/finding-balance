#pragma once

namespace unbalanced_psi::server {


  class Server {

    std::vector<int> dataset;

    public:
      // generate or parse dataset
      // generate DDH key
      // establish network connections
      Server(std::vector<int> *dataset/* hash fs, connection details, */);

      // encrypt elements in dataset
      // create and populate hash table
      void offline();

      // receive client's dataset, encrypt it, and send back
      void listen(/* client's encrypted dataset */); // or `round_one`?

      // do the pir
      void pir();

    private:
      // encrypt an element under our ddh key
      void encrypt(int element);
  };
}
