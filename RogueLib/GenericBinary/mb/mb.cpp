
#include <RogueLib/GenericBinary/BinaryTranslation.hpp>

using namespace RogueLib::GenericBinary;

int main() {
    toBinary<std::map<std::vector<std::vector<std::vector<std::int8_t>>>, std::vector<std::vector<std::vector<std::vector<std::int8_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<std::int16_t>>>, std::vector<std::vector<std::vector<std::vector<std::int16_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<std::int32_t>>>, std::vector<std::vector<std::vector<std::vector<std::int32_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<std::int64_t>>>, std::vector<std::vector<std::vector<std::vector<std::int64_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<std::uint8_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint8_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<std::uint16_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint16_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<std::uint32_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint32_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<std::uint64_t>>>, std::vector<std::vector<std::vector<std::vector<std::uint64_t>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<float>>>, std::vector<std::vector<std::vector<std::vector<float>>>>>>({});
    toBinary<std::map<std::vector<std::vector<std::vector<double>>>, std::vector<std::vector<std::vector<std::vector<double>>>>>>({});
}