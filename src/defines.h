#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <future>

#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>

#include <apsi/oprf/ecpoint.h>
#include <gsl/span>

#define INPUT_TYPE u32

namespace unbalanced_psi {

    // result of the oprf
    using hash_type = std::vector<osuCrypto::u8>;

    // datatypes from std
    using std::array;
    using std::tuple;
    using std::vector;
    using std::future;
    using std::string;

    // datatypes from osuCrypto
    using u64   = osuCrypto::u64;
    using u32   = osuCrypto::u32;
    using u8    = osuCrypto::u8;
    using block = osuCrypto::block;

    // random generators
    using PRNG = osuCrypto::PRNG;

    // group element types
    using Point  = apsi::oprf::ECPoint;
    using Number = apsi::oprf::ECPoint::scalar_type;

    // networking
    using IOService   = osuCrypto::IOService;
    using Session     = osuCrypto::Session;
    using SessionMode = osuCrypto::SessionMode;
    using Channel     = osuCrypto::Channel;
}
