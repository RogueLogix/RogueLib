
#define USING_ROGUELIB_GENERICBINARY

#include <RogueLib/ROBN/AutoSerializable.hpp>

#include <iostream>
#include <chrono>

#define BOOST_TEST_MODULE GenericBinary

#include <boost/test/unit_test.hpp>
#include <random>

using namespace RogueLib::ROBN;

std::default_random_engine generator;

BOOST_AUTO_TEST_CASE(stringEncode) {
    srand(std::chrono::system_clock::now().time_since_epoch().count());
    BOOST_CHECK(toROBN<std::string>("testString")[0] == Type::String);
    for (int i = 0; i < 100; ++i) {
        std::uint16_t strlen = rand();
        std::string str;
        for (int j = 0; j < strlen; ++j) {
            char randChar = 0;
            while (randChar == 0) {
                randChar = rand();
            }
            str += randChar;
        }
        auto strBytes = toROBN(str);
        BOOST_CHECK(strBytes.size() == strlen + 2);
        BOOST_CHECK(strcmp(str.c_str(), (char*) (strBytes.data() + 1)) == 0);
    }
}

BOOST_AUTO_TEST_CASE(stringDecode) {
    srand(std::chrono::system_clock::now().time_since_epoch().count());
    for (int i = 0; i < 100; ++i) {
        std::vector<std::uint8_t> strBytes;
        strBytes.emplace_back(Type::String);
        std::uint16_t strlen = rand();
        strBytes.resize(strlen + 2);
        for (int j = 1; j <= strlen; ++j) {
            char randChar = 0;
            while (randChar == 0) {
                randChar = rand();
            }
            strBytes[j] = randChar;
        }
        strBytes[strlen + 1] = 0;

        std::string str = fromROBN<std::string>(strBytes);

        BOOST_CHECK(str.size() == strlen);
        BOOST_CHECK(strcmp(str.c_str(), (char*) (strBytes.data() + 1)) == 0);
    }
}

BOOST_AUTO_TEST_CASE(boolEncode) {
    auto trueBytes = toROBN(true);
    BOOST_CHECK(trueBytes.size() == 2);
    BOOST_CHECK(trueBytes[0] == Type::Bool);
    BOOST_CHECK(trueBytes[1] == 1);
    auto falseBytes = toROBN(false);
    BOOST_CHECK(falseBytes.size() == 2);
    BOOST_CHECK(falseBytes[0] == Type::Bool);
    BOOST_CHECK(falseBytes[1] == 0);
}

BOOST_AUTO_TEST_CASE(boolDecode) {
    std::vector<std::uint8_t> decodeTestBytes;
    decodeTestBytes.emplace_back(Type::Bool);
    decodeTestBytes.emplace_back(0);
    BOOST_CHECK(!fromROBN<bool>(decodeTestBytes));

    // C++ 20 defines this behavior
    for (std::uint8_t i = 1; i != 0; ++i) {
        decodeTestBytes[1] = i;
        BOOST_CHECK(fromROBN<bool>(decodeTestBytes));
    }
}
//
BOOST_AUTO_TEST_CASE(int8Encode) {
    std::int8_t val = 0;
    do {
        auto bytes = toROBN(val);
        BOOST_CHECK(bytes.size() == 2);
        BOOST_CHECK(bytes[0] == (std::uint8_t(Type::Int8) | std::uint8_t(Endianness::NATIVE)));
        BOOST_CHECK(bytes[1] == std::uint8_t(val));
    } while (val++ != 0);
}

BOOST_AUTO_TEST_CASE(int8Decode) {
    std::vector<std::uint8_t> decodeTestBytes;
    decodeTestBytes.emplace_back(Type::Int8);
    decodeTestBytes.emplace_back(0);
    BOOST_CHECK(!fromROBN<std::int8_t>(decodeTestBytes));

    // C++ 20 defines this behavior
    for (std::uint8_t i = 1; i != 0; ++i) {
        decodeTestBytes[1] = i;
        BOOST_CHECK(std::int8_t(decodeTestBytes[1]) == fromROBN<std::int8_t>(decodeTestBytes));
    }
}

BOOST_AUTO_TEST_CASE(int16Encode) {
    std::int16_t val = 0;
    auto* bytePtr = reinterpret_cast<uint8_t*>(&val);
    do {
        auto bytes = toROBN(val);
        BOOST_CHECK(bytes.size() == 3);
        BOOST_CHECK(bytes[0] == (std::uint8_t(Type::Int16) | std::uint8_t(Endianness::NATIVE)));
        BOOST_CHECK(bytes[1] == bytePtr[0]);
        BOOST_CHECK(bytes[2] == bytePtr[1]);
    } while (++val != 0);
}

BOOST_AUTO_TEST_CASE(int16Decode) {
    std::int16_t val = 0;
    do {
        auto bytes = toROBN(val);
        BOOST_CHECK_MESSAGE(val == fromROBN<std::int16_t>(bytes), std::to_string(val));
        bytes[0] ^= Endianness::BIG;
        bytes[1] ^= bytes[2];
        bytes[2] ^= bytes[1];
        bytes[1] ^= bytes[2];
        BOOST_CHECK_MESSAGE(val == fromROBN<std::int16_t>(bytes), std::to_string(val));
    } while (++val != 0);
}

BOOST_AUTO_TEST_CASE(int32Encode) {
    auto runCheck = [](auto val) {
        auto* bytePtr = reinterpret_cast<uint8_t*>(&val);
        auto bytes = toROBN(val);
        BOOST_CHECK(bytes.size() == 5);
        BOOST_CHECK(bytes[0] == (std::uint8_t(Type::Int32) | std::uint8_t(Endianness::NATIVE)));
        BOOST_CHECK(bytes[1] == bytePtr[0]);
        BOOST_CHECK(bytes[2] == bytePtr[1]);
        BOOST_CHECK(bytes[3] == bytePtr[2]);
        BOOST_CHECK(bytes[4] == bytePtr[3]);
    };

    std::uniform_int_distribution<std::int32_t> distribution(1, pow(2, 16));

    for (std::int32_t i = INT32_MIN; i < 0; i += distribution(generator)) {
        runCheck(i);
    }
    for (std::int32_t i = 1; i > 0; i += distribution(generator)) {
        runCheck(i);
    }
}

BOOST_AUTO_TEST_CASE(int32Decode) {
    auto runCheck = [](auto val) {
        auto bytes = toROBN(val);
        BOOST_CHECK_MESSAGE(val == fromROBN<decltype(val)>(bytes), std::to_string(val));
        bytes[0] ^= Endianness::BIG;

        bytes[2] ^= bytes[3];
        bytes[3] ^= bytes[2];
        bytes[2] ^= bytes[3];

        bytes[1] ^= bytes[4];
        bytes[4] ^= bytes[1];
        bytes[1] ^= bytes[4];

        BOOST_CHECK_MESSAGE(val == fromROBN<decltype(val)>(bytes), std::to_string(val));
    };

    std::uniform_int_distribution<std::int32_t> distribution(1, pow(2, 16));

    for (std::int32_t i = INT32_MIN; i < 0; i += distribution(generator)) {
        runCheck(i);
    }
    for (std::int32_t i = 1; i > 0; i += distribution(generator)) {
        runCheck(i);
    }
}

BOOST_AUTO_TEST_CASE(int64Encode) {
    auto runCheck = [](auto val) {
        auto* bytePtr = reinterpret_cast<uint8_t*>(&val);
        auto bytes = toROBN(val);
        BOOST_CHECK(bytes.size() == 9);
        BOOST_CHECK(bytes[0] == (std::uint8_t(Type::Int64) | std::uint8_t(Endianness::NATIVE)));
        BOOST_CHECK(bytes[1] == bytePtr[0]);
        BOOST_CHECK(bytes[2] == bytePtr[1]);
        BOOST_CHECK(bytes[3] == bytePtr[2]);
        BOOST_CHECK(bytes[4] == bytePtr[3]);
        BOOST_CHECK(bytes[5] == bytePtr[4]);
        BOOST_CHECK(bytes[6] == bytePtr[5]);
        BOOST_CHECK(bytes[7] == bytePtr[6]);
        BOOST_CHECK(bytes[8] == bytePtr[7]);
    };

    std::uniform_int_distribution<std::int64_t> distribution(1, pow(2, 48));

    for (std::int64_t i = INT64_MIN; i < 0; i += distribution(generator)) {
        runCheck(i);
    }
    for (std::int64_t i = 1; i > 0; i += distribution(generator)) {
        runCheck(i);
    }
}

BOOST_AUTO_TEST_CASE(int64Decode) {
    auto runCheck = [](auto val) {
        auto bytes = toROBN(val);
        BOOST_CHECK_MESSAGE(val == fromROBN<decltype(val)>(bytes), std::to_string(val));
        bytes[0] ^= Endianness::BIG;

        bytes[1] ^= bytes[8];
        bytes[8] ^= bytes[1];
        bytes[1] ^= bytes[8];

        bytes[2] ^= bytes[7];
        bytes[7] ^= bytes[2];
        bytes[2] ^= bytes[7];

        bytes[3] ^= bytes[6];
        bytes[6] ^= bytes[3];
        bytes[3] ^= bytes[6];

        bytes[4] ^= bytes[5];
        bytes[5] ^= bytes[4];
        bytes[4] ^= bytes[5];

        BOOST_CHECK_MESSAGE(val == fromROBN<decltype(val)>(bytes), std::to_string(val));
    };

    std::uniform_int_distribution<std::int64_t> distribution(1, pow(2, 48));

    for (std::int64_t i = INT64_MIN; i < 0; i += distribution(generator)) {
        runCheck(i);
    }
    for (std::int64_t i = 1; i > 0; i += distribution(generator)) {
        runCheck(i);
    }
}

BOOST_AUTO_TEST_CASE(int128Encode) {
    auto runCheck = [](auto val) {
        auto* bytePtr = reinterpret_cast<uint8_t*>(&val);
        auto bytes = toROBN(val);
        BOOST_CHECK(bytes.size() == 17);
        BOOST_CHECK(bytes[0] == (std::uint8_t(Type::Int128) | std::uint8_t(Endianness::NATIVE)));
        BOOST_CHECK(bytes[1] == bytePtr[0]);
        BOOST_CHECK(bytes[2] == bytePtr[1]);
        BOOST_CHECK(bytes[3] == bytePtr[2]);
        BOOST_CHECK(bytes[4] == bytePtr[3]);
        BOOST_CHECK(bytes[5] == bytePtr[4]);
        BOOST_CHECK(bytes[6] == bytePtr[5]);
        BOOST_CHECK(bytes[7] == bytePtr[6]);
        BOOST_CHECK(bytes[8] == bytePtr[7]);
        BOOST_CHECK(bytes[9] == bytePtr[8]);
        BOOST_CHECK(bytes[10] == bytePtr[9]);
        BOOST_CHECK(bytes[11] == bytePtr[10]);
        BOOST_CHECK(bytes[12] == bytePtr[11]);
        BOOST_CHECK(bytes[13] == bytePtr[12]);
        BOOST_CHECK(bytes[14] == bytePtr[13]);
        BOOST_CHECK(bytes[15] == bytePtr[14]);
        BOOST_CHECK(bytes[16] == bytePtr[15]);
    };

    __int128 randmax = 0;
    randmax |= UINT64_MAX;
    randmax <<= 63;
    randmax |= UINT64_MAX;

    __int128 randmin = 0;
    randmin |= UINT64_MAX;
    randmin <<= 52;
    randmin |= UINT64_MAX;

    std::uniform_int_distribution<__int128> distribution(randmin, randmax);

    __int128 min = 1;
    min <<= 127;

    for (__int128 i = min; i < 0; i += distribution(generator)) {
        runCheck(i);
    }
    for (__int128 i = 1; i > 0; i += distribution(generator)) {
        runCheck(i);
    }
}

BOOST_AUTO_TEST_CASE(int128Decode) {
    auto runCheck = [](auto val) {
        auto bytes = toROBN(val);
        BOOST_CHECK(val == fromROBN<decltype(val)>(bytes));
        bytes[0] ^= Endianness::BIG;

        bytes[1] ^= bytes[16];
        bytes[16] ^= bytes[1];
        bytes[1] ^= bytes[16];

        bytes[2] ^= bytes[15];
        bytes[15] ^= bytes[2];
        bytes[2] ^= bytes[15];

        bytes[3] ^= bytes[14];
        bytes[14] ^= bytes[3];
        bytes[3] ^= bytes[14];

        bytes[4] ^= bytes[13];
        bytes[13] ^= bytes[4];
        bytes[4] ^= bytes[13];

        bytes[5] ^= bytes[12];
        bytes[12] ^= bytes[5];
        bytes[5] ^= bytes[12];

        bytes[6] ^= bytes[11];
        bytes[11] ^= bytes[6];
        bytes[6] ^= bytes[11];

        bytes[7] ^= bytes[10];
        bytes[10] ^= bytes[7];
        bytes[7] ^= bytes[10];

        bytes[8] ^= bytes[9];
        bytes[9] ^= bytes[8];
        bytes[8] ^= bytes[9];

        BOOST_CHECK(val == fromROBN<decltype(val)>(bytes));
    };

    __int128 randmax = 0;
    randmax |= UINT64_MAX;
    randmax <<= 63;
    randmax |= UINT64_MAX;

    __int128 randmin = 0;
    randmin |= UINT64_MAX;
    randmin <<= 52;
    randmin |= UINT64_MAX;

    std::uniform_int_distribution<__int128> distribution(randmin, randmax);

    __int128 min = 1;
    min <<= 127;

    for (__int128 i = min; i < 0; i += distribution(generator)) {
        runCheck(i);
    }
    for (__int128 i = 1; i > 0; i += distribution(generator)) {
        runCheck(i);
    }
}