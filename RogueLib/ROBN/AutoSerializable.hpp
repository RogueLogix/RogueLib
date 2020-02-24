#pragma  once

#include "ROBNTranslation.hpp"

#include <map>
#include <functional>

namespace RogueLib::ROBN {
    class AutoSerializable : public Serializable {
        std::map<std::string, std::vector<std::string>> reliance;
        std::map<std::string, std::function<bool()>> requirementChecks;
        std::map<std::string, std::function<ROBN()>> serializationFunctions;
        std::map<std::string, std::function<void(byte*& ptr, const byte* const endPtr,
                                                 Type type)>> deserializationFunctions;
    public:

        template<typename T>
        void registerForSerialization(T& val, std::string name) {
            auto serializationFunc = [&]() {
                return RogueLib::ROBN::toROBN<T>(val);
            };
            auto deserializationFunc = [&](byte*& ptr, const byte* const endPtr, Type type) {
                val = RogueLib::ROBN::fromROBN<T>(ptr, endPtr, type);
            };
            reliance[name] = {};
            serializationFunctions[name] = serializationFunc;
            deserializationFunctions[name] = deserializationFunc;
        }

        void registerRequirementCheck(std::string name, std::function<bool()> isRequired = []() { return true; }) {

        }

        void addReliance(std::string name, std::string relies) {
            reliance[name].emplace_back(relies);
        }

        virtual ROBN toROBN() override {
            ROGUELIB_STACKTRACE
            std::map<std::string, ROBN> objects;

            std::function<void(std::string)> writeObject;
            writeObject = [&](const std::string& name) {
                if (objects.find(name) == objects.end()) {
                    auto relied = reliance[name];
                    for (const auto& item : relied) {
                        writeObject(item);
                    }
                    auto requirementIter = requirementChecks.find(name);
                    if (requirementIter != requirementChecks.end()) {
                        // if its not required (optional) we are going to skip it
                        // well, that is, if we have a requirement check in the first place ofc
                        if (!requirementIter->second()) {
                            return;
                        }
                    }
                }
            };

            for (const auto& item : reliance) {
                writeObject(item.first);
            }

            return RogueLib::ROBN::toROBN(objects);
        }

        virtual void fromROBN(byte*& ptr, const byte* const endPtr, Type type) override {
            ROGUELIB_STACKTRACE
            auto objects = RogueLib::ROBN::fromROBN<std::map<std::string, ROBN>>(ptr, endPtr, type);

            std::function<void(std::string)> readObject;

            readObject = [&](std::string name) {

                for (const auto& relied : reliance[name]) {
                    readObject(relied);
                }

                auto objectsIter = objects.find(name);
                if(objectsIter == objects.end()){
                    auto requirementIter = requirementChecks.find(name);
                    if(requirementIter != requirementChecks.end()){
                        if(requirementIter->second()){
                            // well, shit, we needed this one
                            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary, requirement check failure");
                        }
                    }
                    // didn't need it, aight then.
                    return;
                }
            };

            for (const auto& item : reliance) {
                readObject(item.first);
            }

            if ((ptr + 1) >= endPtr || *ptr != byte{Type::SublistStart}) {
                // TODO throw exception __invalid_argument_incompatible_binary_type
            }
            while (ptr < endPtr && *ptr != byte{Type::SublistEnd}) {
                if (*ptr != byte{Type::String} || ptr++ >= endPtr) {
                    // TODO throw exception __invalid_argument_incompatible_binary_type
                }
                auto name = RogueLib::ROBN::fromROBN<std::string>(ptr, endPtr, Type::String);
                if (ptr == endPtr) {
                    // TODO throw exception __invalid_argument_incompatible_binary_type
                }
                ptr++;
                deserializationFunctions[name](ptr, endPtr, Type(ptr[-1]));
            }
            if (ptr > endPtr || *ptr != byte{Type::SublistEnd}) {
                // TODO throw exception __invalid_argument_incompatible_binary_type
            }
        };

    };

#define ROGUELIB_ROBN_XCAT3_L2(a, b, c) a##b##c
#define ROGUELIB_ROBN_XCAT3(a, b, c) ROGUELIB_ROBN_XCAT3_L2(a, b, c)

#define ROGUELIB_ROBN_SERIALIZABLE_COUNTER(valname, counter)* ROGUELIB_ROBN_XCAT3(__ROGUELIB_ROBN_GB_PTR_, counter, valname) = \
    [&, this]() -> decltype(ROGUELIB_ROBN_XCAT3(__ROGUELIB_ROBN_GB_PTR_, counter, valname)){\
    this->registerForSerialization(valname, #valname);\
    return nullptr;\
}();\
std::remove_pointer<decltype(ROGUELIB_ROBN_XCAT3(__ROGUELIB_ROBN_GB_PTR_, counter, valname))>::type valname

#define ROGUELIB_ROBN_SERIALIZABLE(valname) ROGUELIB_ROBN_SERIALIZABLE_COUNTER(valname, __COUNTER__)

//#define ROGUELIB_ROBN_BEFORE(valname, beforeVal)
//#define ROGUELIB_ROBN_AFTER(valname, afterVal)

#define ROGUELIB_ROBN_CONDITIONALLY_REQUIRED_COUNTER(valname, condition, counter)\
bool ROGUELIB_ROBN_XCAT3(__ROGUELIB_ROBN_REQUIRED_CHECK__, counter, valname)(){\
    return condition;\
}\
/* hey, hey about that function now */\
void* ROGUELIB_ROBN_XCAT3(__ROGUELIB_ROBN_REQUIRED_CHECK_REG_STRAR_PTR__, counter, valname) = [this](){\
    this->addRequired(#valname, [this](){\
        this->ROGUELIB_ROBN_XCAT3(__ROGUELIB_ROBN_REQUIRED_CHECK__, counter, valname)();\
    })\
   return nullptr;\
};\

#define ROGUELIB_ROBN_CONDITIONALLY_REQUIRED(valname, condition)\
ROGUELIB_ROBN_CONDITIONALLY_REQUIRED_COUNTER(valname, condition, __COUNTER__)

#define ROGUELIB_ROBN_CONDITIONALLY_OPTIONAL(valname, condition) ROGUELIB_ROBN_CONDITIONALLY_REQUIRED(valname, !(condition))
#define ROGUELIB_ROBN_REQUIRED(valname) ROGUELIB_ROBN_CONDITIONALLY_REQUIRED(valname, true)
#define ROGUELIB_ROBN_OPTIONAL(valname) ROGUELIB_ROBN_CONDITIONALLY_OPTIONAL(valname, true)

// just shorter redefinitions
#ifdef ROGUELIB_AS_RL
#define RL_GENERICBIANRY_SERIALIZABLE(valname) ROGUELIB_ROBN_SERIALIZABLE(valname)
#endif

#ifdef ROBN_AS_GB
#define ROGUELIB_GB_SERIALIZABLE(valname) ROGUELIB_ROBN_SERIALIZABLE(valname)
#endif

#ifdef ROGUELIB_ROBN_AS_GB
#define GB_SERIALIZABLE(valname) ROGUELIB_ROBN_SERIALIZABLE(valname)
#endif

#ifdef USING_ROGUELIB
#define GENERICBIANRY_SERIALIZABLE(valname) ROGUELIB_ROBN_SERIALIZABLE(valname)
#ifdef USING_ROBN
#define USING_ROGUELIB_ROBN
#endif
#endif

#ifdef USING_ROGUELIB_ROBN
#define SERIALIZABLE(valname) ROGUELIB_ROBN_SERIALIZABLE(valname)
#endif
}