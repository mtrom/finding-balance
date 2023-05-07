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

#include <apsi/oprf/ecpoint.h>
#include <apsi/oprf/oprf_sender.h>
#include <gsl/span>

#define PADDING_SEED 1986
#define MAX_THREADS 32

#define _USE_FOUR_Q_ true

#if _USE_FOUR_Q_
#define ENCRYPT(element, key) element.scalar_multiply(key, true)
#else
#define ENCRYPT(element, key) element * key
#endif

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

#if _USE_FOUR_Q_
    using HashedItem = apsi::HashedItem;
    using Point      = apsi::oprf::ECPoint;
    using Number     = apsi::oprf::ECPoint::scalar_type;
#else
    // for ddh
    using Curve  = osuCrypto::REllipticCurve;
    using Point  = osuCrypto::REccPoint;
    using Number = osuCrypto::REccNumber;
#endif

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
