#pragma once

#include <vector>

#include "defines.h"

namespace unbalanced_psi {
  vector<u32> generate_dataset(int size);
  tuple<vector<u32>,vector<u32>> generate_datasets(int server_size, int client_size, int overlap);

  Point to_point(block value, osuCrypto::RandomOracle hash);
  void test_encrypt();
}
