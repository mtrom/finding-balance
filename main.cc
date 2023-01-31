#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include <vector>
using std::vector;

#include "utils.h"


int main(int argc, char** argv) {

  // read in a file and output its contents line by line
  std::ifstream file(argv[1]);

  std::string line;
  while (std::getline(file, line)) {
    std::cout << line;
    std::cout << "\n";
  }

  // create a vector of random values
  int size = 5;

  vector<int> dataset = unbalanced_psi::generate_dataset(size);
  for (int i = 0; i < size; i++) {
    std::cout << std::to_string(dataset[i]) + "\n";
  }
  return 0;
}
