#pragma  once

#include <compare>

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

        std::strong_ordering operator<=>(
        const ROBNObject& other
        ) const {
            // smaller robn wins
            if (this->robn.size() < other.robn.size()) {
                return std::strong_ordering::less;
            }
            if (this->robn.size() > other.robn.size()) {
                return std::strong_ordering::greater;
            }

            // didnt i say that smaller wins?
            for (int i = 0; i < robn.size(); ++i) {
                if (this->robn[i] < other.robn[i]) {
                    return std::strong_ordering::less;
                }
                if (this->robn[i] > other.robn[i]) {
                    return std::strong_ordering::greater;
                }
            }
            return std::strong_ordering::equal;
        }

        bool operator<(const ROBNObject& other) const {
            return std::is_lt(*this <=> other);
        }

        bool operator>(const ROBNObject& other) const {
            return std::is_gt(*this <=> other);
        }

        bool operator<=(const ROBNObject& other) const {
            return std::is_lteq(*this <=> other);
        }

        bool operator>=(const ROBNObject& other) const {
            return std::is_gteq(*this <=> other);
        }

        bool operator==(const ROBNObject& other) const {
            return std::is_eq(*this <=> other);
        }


        bool operator!=(const ROBNObject& other) const {
            return std::is_neq(*this <=> other);
        }

        ROBN toROBN() override {
            return RogueLib::ROBN::toROBN(robn);
        }

        void fromROBN(std::uint8_t*& ptr, const std::uint8_t* endPtr, Type type) override {
            robn = RogueLib::ROBN::fromROBN<ROBN>(ptr, endPtr, type);
        }
    };
}