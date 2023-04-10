#pragma once

#include <vector>
#include <tuple>

#include <cryptoTools/Common/TestCollection.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Crypto/RCurve.h>
#include <cryptoTools/Crypto/RandomOracle.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/Endpoint.h>
#include <cryptoTools/Network/IOService.h>

#define PADDING_SEED 1986
#define IOS_THREADS 3

namespace unbalanced_psi {

    // datatypes from std
    using std::array;
    using std::tuple;
    using std::vector;

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

    // networking
    using IOService   = osuCrypto::IOService;
    using Session     = osuCrypto::Session;
    using SessionMode = osuCrypto::SessionMode;
    using Channel     = osuCrypto::Channel;

    // for testing
    using TestCollection = osuCrypto::TestCollection;
    using UnitTestFail   = osuCrypto::UnitTestFail;
}

#define INPUT_TYPE u32
#define MOCK_ELEMENT INPUT_TYPE(-1)
