#pragma once

#include <vector>

#include "defines.h"

namespace unbalanced_psi {
  vector<INPUT_TYPE> generate_dataset(int size);
  tuple<vector<INPUT_TYPE>,vector<INPUT_TYPE>> generate_datasets(int server_size, int client_size, int overlap);

  Point hash_to_group_element(INPUT_TYPE input);
  vector<Point> hash_to_group_elements(vector<INPUT_TYPE> inputs, RandomOracle oracle);

  void test_encrypt();
}
