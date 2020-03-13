/**
 * Copyright (c) 2020 RogueLogix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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