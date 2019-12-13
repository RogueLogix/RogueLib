
#include <RogueLib/ROBN/ROBNTranslation.hpp>
#include <iostream>
//#include <RogueLib/ROBN/ROBNMap.hpp>
#include <RogueLib/ROBN/ROBNObject.hpp>

using namespace RogueLib::ROBN;

int main() {
    std::cout << __STDCPP_DEFAULT_NEW_ALIGNMENT__ << std::endl;

    __int128 int128;
    auto bytes = toROBN(int128);
    int128 = 0;
    int128 = fromROBN<__int128>(bytes);

    toROBN<std::map<std::vector<std::vector<std::vector<std::int8_t>>>, std::vector<std::vector<std::vector<std::vector<std::int8_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::int16_t>>>, std::vector<std::vector<std::vector<std::vector<std::int16_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::int32_t>>>, std::vector<std::vector<std::vector<std::vector<std::int32_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::int64_t>>>, std::vector<std::vector<std::vector<std::vector<std::int64_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::uint8_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint8_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::uint16_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint16_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::uint32_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint32_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::uint64_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint64_t>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<float>>>, std::vector<std::vector<std::vector<std::vector<float>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<double>>>, std::vector<std::vector<std::vector<std::vector<double>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<__int128>>>, std::vector<std::vector<std::vector<std::vector<__int128>>>>>>({});
    toROBN<std::map<std::vector<std::vector<std::vector<std::string>>>, std::vector<std::vector<std::vector<std::vector<std::string>>>>>>({});

    enum class MyEnum : __int128{
        A,
        B,
        C
    };

    MyEnum a = MyEnum::C;
    bytes = toROBN(a);
    a = MyEnum::B;
    a = fromROBN<MyEnum>(bytes);

    ROBNObject obj;

    std::string str = "yeet";

    obj = str;
    str = "";
    str = std::string(obj);

    std::cout << str << std::endl;

    obj = 12;

    ROBN robn{obj};
}