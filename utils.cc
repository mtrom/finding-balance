#include "utils.h"

using std::vector;

namespace unbalanced_psi {

  /**
   * generate a mock dataset
   *
   * @param <size> desired number of elements in the dataset
   * @return random vector of integers
   */
  vector<int> generate_dataset(int size) {
    vector<int> dataset(size);

    for (int i = 0; i < size; i++) {
      // TODO: rand() is real bad
      dataset[i] = std::rand();
    }

    return dataset;
  }
}
