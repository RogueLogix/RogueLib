
#define USING_ROGUELIB_GENERICBINARY

#include <iostream>
#include <chrono>

#define BOOST_TEST_MODULE GenericBinary

#include <boost/test/unit_test.hpp>
#include <RogueLib/GenericBinary/AutoSerializable.hpp>

using namespace RogueLib::GenericBinary;


BOOST_AUTO_TEST_CASE(stringEncode) {
    srand(std::chrono::system_clock::now().time_since_epoch().count());
    BOOST_CHECK(toBinary<std::string>("testString")[0] == Type::String);
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
        auto strBytes = toBinary(str);
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

        std::string str = fromBinary<std::string>(strBytes);

        BOOST_CHECK(str.size() == strlen);
        BOOST_CHECK(strcmp(str.c_str(), (char*) (strBytes.data() + 1)) == 0);
    }
}

BOOST_AUTO_TEST_CASE(boolEncode) {
    auto trueBytes = toBinary(true);
    BOOST_CHECK(trueBytes.size() == 2);
    BOOST_CHECK(trueBytes[0] == Type::Bool);
    BOOST_CHECK(trueBytes[1] == 1);
    auto falseBytes = toBinary(false);
    BOOST_CHECK(falseBytes.size() == 2);
    BOOST_CHECK(falseBytes[0] == Type::Bool);
    BOOST_CHECK(falseBytes[1] == 0);
}

BOOST_AUTO_TEST_CASE(boolDecode) {
    std::vector<std::uint8_t> decodeTestBytes;
    decodeTestBytes.emplace_back(Type::Bool);
    decodeTestBytes.emplace_back(0);
    BOOST_CHECK(!fromBinary<bool>(decodeTestBytes));

    // C++ 20 defines this behavior
    for (std::uint8_t i = 1; i != 0; ++i) {
        decodeTestBytes[1] = i;
        BOOST_CHECK(fromBinary<bool>(decodeTestBytes));
    }
}
//
BOOST_AUTO_TEST_CASE(int8Encode) {
    std::int8_t val = 0;

}

BOOST_AUTO_TEST_CASE(int8Decode) {
    std::vector<std::uint8_t> decodeTestBytes;
    decodeTestBytes.emplace_back(Type::Bool);
    decodeTestBytes.emplace_back(0);
    BOOST_CHECK(!fromBinary<std::int8_t>(decodeTestBytes));

    // C++ 20 defines this behavior
    for (std::uint8_t i = 1; i != 0; ++i) {
        decodeTestBytes[1] = i;
        BOOST_CHECK(std::int8_t(decodeTestBytes[1]) == fromBinary<std::int8_t>(decodeTestBytes));
    }
}

BOOST_AUTO_TEST_CASE(int16Encode) {

}

BOOST_AUTO_TEST_CASE(int16Decode) {

}