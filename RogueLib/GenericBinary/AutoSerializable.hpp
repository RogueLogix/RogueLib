#pragma  once

#include "BinaryTranslation.hpp"

#include <map>
#include <functional>

namespace RogueLib::GenericBinary {
    class AutoSerializable : public Serializable {
        std::vector<std::pair<std::string, std::function<std::vector<std::uint8_t>()>>> serializationFunctions;
        std::map<std::string, std::function<void(std::uint8_t*& ptr, const std::uint8_t* const endPtr,
                                                 Type type)>> deserializationFunctions;
    public:

        template<typename T>
        void registerForSerialization(T& val, std::string name) {
            auto serializationFunc = [&]() {
                return RogueLib::GenericBinary::toBinary<T>(val);
            };
            auto deserializationFunc = [&](std::uint8_t*& ptr, const std::uint8_t* const endPtr, Type type) {
                val = RogueLib::GenericBinary::fromBinary<T>(ptr, endPtr, type);
            };
            serializationFunctions.emplace_back(name, serializationFunc);
            deserializationFunctions[name] = deserializationFunc;
        }

        virtual std::vector<std::uint8_t> toBinary() override {
            std::vector<std::uint8_t> bytes;
            bytes.emplace_back(Type::SublistStart);
            {
                for (auto& element : serializationFunctions) {
                    auto nameBytes = RogueLib::GenericBinary::toBinary(element.first);
                    auto elementBytes = element.second();
                    bytes.insert(bytes.end(), nameBytes.begin(), nameBytes.end());
                    bytes.insert(bytes.end(), elementBytes.begin(), elementBytes.end());
                }
            }
            bytes.emplace_back(Type::SublistEnd);
            return bytes;
        }

        virtual void fromBinary(std::uint8_t*& ptr, const std::uint8_t* const endPtr, Type type) override {
            if ((ptr + 1) >= endPtr || *ptr != Type::SublistStart) {
                // TODO throw exception __invalid_argument_incompatible_binary_type
            }
            while (ptr < endPtr && *ptr != Type::SublistEnd) {
                if (*ptr != Type::String || ptr++ >= endPtr) {
                    // TODO throw exception __invalid_argument_incompatible_binary_type
                }
                auto name = RogueLib::GenericBinary::fromBinary<std::string>(ptr, endPtr, Type::String);
                if (ptr == endPtr) {
                    // TODO throw exception __invalid_argument_incompatible_binary_type
                }
                ptr++;
                deserializationFunctions[name](ptr, endPtr, Type(ptr[-1]));
            }
            if (ptr > endPtr || *ptr != Type::SublistEnd) {
                // TODO throw exception __invalid_argument_incompatible_binary_type
            }
        };

    };

#define ROGUELIB_GENERICBINARY_XCAT3_L2(a, b, c) a##b##c
#define ROGUELIB_GENERICBINARY_XCAT3(a, b, c) ROGUELIB_GENERICBINARY_XCAT3_L2(a, b, c)

#define ROGUELIB_GENERICBINARY_SERIALIZABLE_COUNTER(valname, counter)* ROGUELIB_GENERICBINARY_XCAT3(__ROGUELIB_GENERICBINARY_GB_PTR_, counter, valname) = \
    [&, this]() -> decltype(ROGUELIB_GENERICBINARY_XCAT3(__ROGUELIB_GENERICBINARY_GB_PTR_, counter, valname)){\
    this->registerForSerialization(valname, #valname);\
    return nullptr;\
}();\
std::remove_pointer<decltype(ROGUELIB_GENERICBINARY_XCAT3(__ROGUELIB_GENERICBINARY_GB_PTR_, counter, valname))>::type valname

#define ROGUELIB_GENERICBINARY_SERIALIZABLE(valname) ROGUELIB_GENERICBINARY_SERIALIZABLE_COUNTER(valname, __COUNTER__)




// just shorter redefinitions
#ifdef ROGUELIB_AS_RL
#define RL_GENERICBIANRY_SERIALIZABLE(valname) ROGUELIB_GENERICBINARY_SERIALIZABLE(valname)
#endif

#ifdef GENERICBINARY_AS_GB
#define ROGUELIB_GB_SERIALIZABLE(valname) ROGUELIB_GENERICBINARY_SERIALIZABLE(valname)
#endif

#ifdef ROGUELIB_GENERICBINARY_AS_GB
#define GB_SERIALIZABLE(valname) ROGUELIB_GENERICBINARY_SERIALIZABLE(valname)
#endif

#ifdef USING_ROGUELIB
#define GENERICBIANRY_SERIALIZABLE(valname) ROGUELIB_GENERICBINARY_SERIALIZABLE(valname)
#ifdef USING_GENERICBINARY
#define USING_ROGUELIB_GENERICBINARY
#endif
#endif

#ifdef USING_ROGUELIB_GENERICBINARY
#define SERIALIZABLE(valname) ROGUELIB_GENERICBINARY_SERIALIZABLE(valname)
#endif
}