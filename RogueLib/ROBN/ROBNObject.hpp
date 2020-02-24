#pragma  once

#include "ROBNTranslation.hpp"

namespace RogueLib::ROBN {
    class ROBNObject : public Serializable {
        ROBN robn;

    public:

        ROBNObject& operator=(const ROBNObject& other) {
            if (&other != this) {
                robn = other.robn;
            }
            return *this;
        }

        template<typename T>
        ROBNObject& operator=(T other) {
            this->robn = RogueLib::ROBN::toROBN(other);
            return *this;
        }

        template<typename T>
        T as() const {
            return RogueLib::ROBN::fromROBN<T>(robn);
        }

        template<typename T>
        explicit operator T()const {
            return as<T>();
        }

        char threeWayComp(const ROBNObject& other) const {
            // smaller robn wins
            if (this->robn.size() < other.robn.size()) {
                return -1;
            }
            if (this->robn.size() > other.robn.size()) {
                return 1;
            }

            // didnt i say that smaller wins?
            for (std::size_t i = 0; i < robn.size(); ++i) {
                if (this->robn[i] < other.robn[i]) {
                    return -1;
                }
                if (this->robn[i] > other.robn[i]) {
                    return 1;
                }
            }
            return 0;
        }

        bool operator<(const ROBNObject& other) const {
            return threeWayComp(other) < 0;
        }

        bool operator>(const ROBNObject& other) const {
            return threeWayComp(other) > 0;
        }

        bool operator<=(const ROBNObject& other) const {
            return threeWayComp(other) <= 0;
        }

        bool operator>=(const ROBNObject& other) const {
            return threeWayComp(other) >= 0;
        }

        bool operator==(const ROBNObject& other) const {
            return threeWayComp(other) == 0;
        }


        bool operator!=(const ROBNObject& other) const {
            return threeWayComp(other) != 0;
        }

        ROBN toROBN() override {
            return RogueLib::ROBN::toROBN(robn);
        }

        void fromROBN(std::byte*& ptr, const std::byte* endPtr, Type type) override {
            robn = RogueLib::ROBN::fromROBN<ROBN>(ptr, endPtr, type);
        }
    };
}