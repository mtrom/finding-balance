#pragma once

#include <vector>
#include <tuple>

#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Crypto/RCurve.h>
#include <cryptoTools/Crypto/RandomOracle.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Endpoint.h>
#include <cryptoTools/Network/Channel.h>

namespace unbalanced_psi {

    // datatypes from std
    using std::vector;
    using std::tuple;

    // datatypes from osuCrypto
    using i64 = osuCrypto::i64;
    using u64 = osuCrypto::u64;
    using i32 = osuCrypto::i32;
    using u32 = osuCrypto::u32;
    using i16 = osuCrypto::i16;
    using u16 = osuCrypto::u16;
    using i8 = osuCrypto::i8;
    using u8 = osuCrypto::u8;
    using block = osuCrypto::block;

    // random generators
    using PRNG = osuCrypto::PRNG;
    using RandomOracle = osuCrypto::RandomOracle;

    // for ddh
    using Curve  = osuCrypto::REllipticCurve;
    using Point  = osuCrypto::REccPoint;
    using Number = osuCrypto::REccNumber;

    // using IOService = osuCrypto::IOService;
}

#define INPUT_TYPE u32
