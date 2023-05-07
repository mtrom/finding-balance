#include <apsi/oprf/ecpoint.h>
#include <apsi/oprf/oprf_sender.h>
#include <apsi/util/utils.h>
#include <gsl/span>

#include "test_apsi.h"
#include "../utils.h"

namespace unbalanced_psi {

    void test_encryption() {

        INPUT_TYPE server(1234);
        INPUT_TYPE client(1234);

        Number s_key;
        Number c_key;

        Point s_point = hash_to_group_element(server);
        Point c_point = hash_to_group_element(client);

        s_point.scalar_multiply(s_key, true);

        c_point.scalar_multiply(c_key, false);
        c_point.scalar_multiply(s_key, true);

        Number inverse;
        Point::InvertScalar(c_key, inverse);
        c_point.scalar_multiply(inverse, false);

        vector<u8> s_bytes(HASH_3_SIZE);
        vector<u8> c_bytes(HASH_3_SIZE);

        hash_group_element(s_point, HASH_3_SIZE, s_bytes.data());
        hash_group_element(c_point, HASH_3_SIZE, c_bytes.data());

        if (s_bytes != c_bytes) {
            throw UnitTestFail("wrong");
        }
    }
}
