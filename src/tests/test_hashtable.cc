#include <stdlib.h>

#include "../defines.h"
#include "../hashtable.h"
#include "../utils.h"

using namespace unbalanced_psi;

int test_tofile() {
    Curve curve;

    Hashtable table("bin/server.db");
    return table.buckets() == 10 && table.size == 10;
}


int main() {
    return test_tofile();
}
