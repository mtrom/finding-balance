#include "utils.h"

#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace unbalanced_psi {


  template<typename T>
    void print(T arg, std::string label) {
      if (!label.empty()) {
        std::cout << label + ": ";
      }
      std::cout << arg;
      std::cout << " (";
      std::cout << typeid(arg).name();
      std::cout << ")\n";
    }

  template<typename T>
    void print(T arg) {
      print(arg, "");
    }

  /**
   * generate a mock dataset
   *
   * @param <size> desired number of elements in the dataset
   * @return random vector of numbers
   */
  vector<u32> generate_dataset(int size) {
    // TODO: how to generate a random seed? chicken and egg...
    block seed(std::rand());
    print(seed, "seed");
    PRNG prng(seed);
    vector<u32> dataset(size);
    prng.get(dataset.data(), dataset.size());
    return dataset;
  }

  /**
   * generate two mock datasets for the server and client
   *
   * @param <server_size> number of elements in the server's dataset
   * @param <client_size> number of elements in the client's dataset
   * @param <overlap>     _minimum_ number of elements shared between the two datasets
   * @return random vector of numbers
   */
  tuple<vector<u32>,vector<u32>> generate_datasets(int server_size, int client_size, int overlap) {
    vector<u32> both   = generate_dataset(overlap);
    vector<u32> server = generate_dataset(server_size - overlap);
    vector<u32> client = generate_dataset(client_size - overlap);

    server.insert(server.end(), both.begin(), both.end());
    client.insert(client.end(), both.begin(), both.end());

    tuple<vector<u32>,vector<u32>> datasets(server, client);
    return datasets;
  }

  /**
   * create a point from a block
   */
  Point to_point(block value, osuCrypto::RandomOracle hash) {
    block hashed;

    hash.Reset();
    hash.Update(value);
    hash.Final(hashed);
    print(hashed, "hashed value");

    Point point;
    point.randomize(hashed);
    print(point, "point value");
    return point;
  }

  void test_encrypt() {

    // PRNG init
    block seed;
    PRNG prng(seed);

    // H() hash function
    osuCrypto::RandomOracle hash(sizeof(block));

    // the generator?
    Curve curve;
    const Point g = curve.getGenerator();

    // Alice's element
    block x(1337);
    print(x, "x");
    Point hx = to_point(x, hash);

    // Bob's element
    block y(1337);
    print(y, "y");
    Point hy = to_point(y, hash);

    // Alice's key
    Number a(prng);
    print(a, "a's key");

    // Bob's key
    Number b(prng);
    print(b, "b's key");

    Point hxa = hx * a;
    Point hxab = hx * b;

    Point hyb = hx * b;
    Point hyba = hx * a;


    print(hxab, "hxab");
    print(hyba, "hyba");

    print(hxab == hyba, "final answer");
  }

}
