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

#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <byteswap.h>
#include <cstring>
#include <climits>
#include <RogueLib/Exceptions/Exceptions.hpp>

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define X64
#define SUPPORTED_ARCHITECTURE_FOUND
#endif

#if !defined(X64) && defined(_M_I86)
#error 16 bit x86 architecture not supported
#endif

// yup, there are this many different definitions for 32 bit...
#if !defined(X64) && (\
        defined(i386) || defined(__i386) || defined(__i386__) || \
        defined(__IA32__) || defined(_M_IX86) || defined(__X86__) || \
        defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || \
        defined(__INTEL__) || defined(__386))
#error 32 bit x86 architecture not supported
#endif

#if defined(__powerpc64__) && defined(_ARCH_PWR9)
#define POWER9
#define SUPPORTED_ARCHITECTURE_FOUND
#endif

#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_4__)\
 || defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_2__)
#error old ARM architectures not supported
#endif

#ifdef __arm__
#define ARM
#define SUPPORTED_ARCHITECTURE_FOUND
#endif


#if defined(_ILP32) || defined(__ILP32__)
#error ILP32 data model not supported
#endif

#ifndef SUPPORTED_ARCHITECTURE_FOUND
#error Unknown and unsupported archtecture
#endif

// make sure these are the correct sizes
static_assert(CHAR_BIT == 8);
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);

/**
 * FORMAT Description
 *
 * a ROBN blob is made up of consecutive elements with no space between them
 * each element is a type byte, followed by that types data
 * the format for each type's data is as follows
 *
 * Undefined, this should't show up, ever
 * String, a null terminated string, same as the C in memory representation
 * Bool, 8bit int is used for storage, encoded to 1 for true, 0 for false. decoding 0 is false, else is true, same as C++
 *
 * Int8, one byte copied directly to/from C++ representation
 * Int16, two bytes copied to/from C++ representation, endianness may be flipped if required
 * Int32, four bytes copied to/from C++ representation, endianness may be flipped if required
 * Int64, eight bytes copied to/from C++ representation, endianness may be flipped if required
 * Int128, sixteen bytes copied to/from C++ representation, endianness may be flipped if required
 *
 * uInt8, one byte copied directly to/from C++ representation
 * uInt16, two bytes copied to/from C++ representation, endianness may be flipped if required
 * uInt32, four bytes copied to/from C++ representation, endianness may be flipped if required
 * uInt64, eight bytes copied to/from C++ representation, endianness may be flipped if required
 * uInt128, sixteen bytes copied to/from C++ representation, endianness may be flipped if required
 *
 * Float, four bytes copied to/from C++ representation, endianness may be flipped if required
 *          INTERNAL REPRESENTATION IS NOT CHANGED
 * Double, eight bytes copied to/from C++ representation, endianness may be flipped if required
 *          INTERNAL REPRESENTATION IS NOT CHANGED
 *
 * Vector, a length element, usually uInt64, but the decoder can handle any type here, type header required
 *          internal element type header, one byte, may be any ROBN type, vector included
 *          the data for each element, densely packed, each element is assumed to be able to find where
 *          it ends based off its data and the type that it is
 *          EACH ELEMENT MUST BE OF THE SAME TYPE, use sublist for random types
 *
 * Pair, the first element (with type header), the second element (with type header)
 *
 * Map, a length element, usually uInt64, byt the decoder can handle any type here, type header required
 *      a list of pair elements with key/value data elements, full pair element including pair type header
 *
 */

//todo long double?
//todo update this with C++23, so there isn't any more undefined/implementation defined behavior
namespace RogueLib::ROBN {

#define ROGUELIB_UNUSED(x) (void)(x)

    typedef std::byte Byte;

    typedef std::vector<Byte> ROBN;

    namespace NS_ENUM_TYPE {
        /* WARNING: 121 type max (1-122)

         * this is encoded to a single byte,
         * with the leading bit encoding endianness in the binary format (if applicable)
         */
        enum Type {
            Undefined = 0,

            String = 1, // it counts up, i know this, still putting it there manually
            Bool = 2,

            Int8 = 4,
            Int16 = 5,
            Int32 = 6,
            Int64 = 7,
            Int128 = 18,
            BigInt = 21,

            uInt8 = 8,
            uInt16 = 9,
            uInt32 = 10,
            uInt64 = 11,
            uInt128 = 19,

            Float = 12,
            Double = 13,
            LongDouble = 20,
            BigFloat = 22,

            Vector = 15,
            Pair = 16,
            Map = 17,
        };
    }
    typedef NS_ENUM_TYPE::Type Type;

    // consteval
    template<typename T>
    constexpr Type primitiveTypeID() {
        // should be optimized out by compiler

        if (std::is_same<T, std::string>::value) {
            return Type::String;
        }
        if (std::is_same<T, bool>::value) {
            return Type::Bool;
        }

        if (std::is_same<T, std::int8_t>::value) {
            return Type::Int8;
        }
        if (std::is_same<T, std::int16_t>::value) {
            return Type::Int16;
        }
        if (std::is_same<T, std::int32_t>::value) {
            return Type::Int32;
        }
        if (std::is_same<T, std::int64_t>::value) {
            return Type::Int64;
        }
        if (std::is_same<T, __int128>::value) {
            return Type::Int128;
        }

        if (std::is_same<T, std::uint8_t>::value) {
            return Type::uInt8;
        }
        if (std::is_same<T, std::uint16_t>::value) {
            return Type::uInt16;
        }
        if (std::is_same<T, std::uint32_t>::value) {
            return Type::uInt32;
        }
        if (std::is_same<T, std::uint64_t>::value) {
            return Type::uInt64;
        }
        if (std::is_same<T, unsigned __int128>::value) {
            return Type::uInt128;
        }

        if (std::is_same<T, float>::value) {
            return Type::Float;
        }
        if (std::is_same<T, double>::value) {
            return Type::Double;
        }

        return Type::Undefined;
    }

    constexpr size_t primitiveTypeSize(Type type) {
        switch (type) {

            default:
                return 0;
            case Type::Bool:
            case Type::Int8:
            case Type::uInt8:
                return 1;
            case Type::Int16:
            case Type::uInt16:
                return 2;
            case Type::Int32:
            case Type::uInt32:
            case Type::Float:
                return 4;
            case Type::Int64:
            case Type::uInt64:
            case Type::Double:
                return 8;
            case Type::Int128:
            case Type::uInt128:
                return 16;
        }
    }

    namespace NS_ENUM_ENDIANNESS {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || defined(__LITTLE_ENDIAN__)
#define ROBN_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ || defined (__BIG_ENDIAN__)
#define ROBN_BIG_ENDIAN
#else
        // the fuck you doing on a PDP11?
#error Unsupported Endianness
#endif

        enum Endianness {
            LITTLE = 0,
            BIG = 1u << 7u, // first bit in the byte, where i look for it
            NATIVE =
#if defined(ROBN_LITTLE_ENDIAN)
            LITTLE
#elif defined(ROBN_BIG_ENDIAN)
            // while big endian systems should work fine, they have not been tested
            // if you are using this on a big endian system, i recommend you run the test suite which should
            // give you an idea of if it works or not
            // big endian decoding on little endian systems has been tested
#ifndef ROGUELIB_ROBN_ENABLE_BIGENDIAN
#error BIG ENDIAN UNTESTED
#endif
            BIG
#else
#error Unsupported Endianness
#endif
        };
    }
    typedef NS_ENUM_ENDIANNESS::Endianness Endianness;

    constexpr Type removeEndianness(Type type) {
        return static_cast<Type>(type & ~Endianness::BIG);
    }

    constexpr Endianness typeEndianness(Type type) {
        return static_cast<Endianness>(std::uint8_t(type) & std::uint8_t(Endianness::BIG));
    }

    template<typename T,
            typename std::enable_if<sizeof(T) == 1 || ((sizeof(T) != 2 && sizeof(T) != 4 && sizeof(T) != 8) &&
                                                       !(std::is_integral<T>::value ||
                                                         std::is_floating_point<T>::value ||
                                                         std::is_same<T, __int128>::value ||
                                                         std::is_same<T, unsigned __int128>::value)), int>::type = 0>
    constexpr T swapEndianness(T val) {
        return val;
    }

    template<typename T, typename std::enable_if<
            (sizeof(T) == 2) && (std::is_integral<T>::value || std::is_floating_point<T>::value), int>::type = 0>
    constexpr T swapEndianness(T val) {
        return bswap_16(val);
    }

    template<typename T, typename std::enable_if<
            (sizeof(T) == 4) && (std::is_integral<T>::value || std::is_floating_point<T>::value), int>::type = 0>
    constexpr T swapEndianness(T val) {
        return bswap_32(val);
    }

    template<typename T, typename std::enable_if<
            (sizeof(T) == 8) && (std::is_integral<T>::value || std::is_floating_point<T>::value), int>::type = 0>
    constexpr T swapEndianness(T val) {
        return bswap_64(val);
    }

    template<typename T, typename std::enable_if<
            std::is_same<T, __int128>::value || std::is_same<T, unsigned __int128>::value, int>::type = 0>
    constexpr T swapEndianness(T val) {
        // 128 bit endianness swap without a native way do to this
        std::uint64_t sections[2] = {0};

        std::memcpy(sections, &val, 16);

        sections[0] = bswap_64(sections[0]);
        sections[1] = bswap_64(sections[1]);
        sections[0] ^= sections[1];
        sections[1] ^= sections[0];
        sections[0] ^= sections[1];

        std::memcpy(&val, sections, 16);

        return val;
    }

    template<typename T>
    constexpr T correctEndianness(T val, Endianness endianness) {
        if (endianness != Endianness::NATIVE) {
            return swapEndianness<T>(val);
        }
        return val;
    }

    class Serializable {
    public:
        virtual ROBN toROBN() = 0;

        virtual void fromROBN(Byte*& ptr, const Byte* endPtr, Type type) = 0;

        [[nodiscard]] virtual bool isFixedBinarySize() const {
            return false;
        }

        [[nodiscard]] virtual std::uint64_t binarySize() const {
            return 0;
        }
    };

    // as fast as it gets for a single value
    template<typename T>
    inline ROBN primitiveToROBN(T val) {
        ROGUELIB_STACKTRACE
        ROBN bytes;
        bytes.resize(sizeof(val) + 1);
        bytes[0] = Byte{primitiveTypeID<T>()};
        std::memcpy(bytes.data() + 1, &val, sizeof(val));
        return bytes;
    }


    // its used in one spot, and thats only hit if its an integral/fp type
    template<typename NT, typename OT, typename std::enable_if<!(
            (std::is_integral<NT>::value || std::is_floating_point<NT>::value) &&
            (std::is_integral<OT>::value || std::is_floating_point<OT>::value)), int>::type = 0>
    std::vector<NT> castVector(std::vector<OT> vector) {
        ROGUELIB_UNUSED(vector);
        return {};
    }

    template<typename NT, typename OT, typename std::enable_if<
            (std::is_integral<NT>::value || std::is_floating_point<NT>::value) &&
            (std::is_integral<OT>::value || std::is_floating_point<OT>::value), int>::type = 0>
    std::vector<NT> castVector(std::vector<OT> vector) {
        std::vector<NT> newVector;
        newVector.resize(vector.size());
        for (std::size_t i = 0; i < vector.size(); ++i) {
            newVector[i] = NT(vector[i]);
        }
        return newVector;
    }


    // i support casting here, i cant really make it much better
    // TODO: string interpretation?
    template<typename T, typename std::enable_if<
            std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type = 0>
    inline T fromROBN(Byte*& ptr, const Byte* endPtr, Type type) {
        ROGUELIB_STACKTRACE
        switch (removeEndianness(type)) {
            default:
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            case Type::Bool: {
                if (ptr < endPtr) {
//                    ptr++;
//                    return std::to_integer<bool>(ptr[-1]);
                    return fromROBN < bool > (ptr, endPtr, Type::uInt8);
                }
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
            case Type::Int8: {
                std::int8_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, 1);
                ptr++;
                return tempval;
            }
            case Type::Int16: {
                std::int16_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::Int32: {
                std::int32_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::Int64: {
                std::int64_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::Int128: {
                __int128 tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::uInt8: {
                std::uint8_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, 1);
                ptr++;
                return tempval;
            }
            case Type::uInt16: {
                std::uint16_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::uInt32: {
                std::uint32_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::uInt64: {
                std::uint64_t tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::uInt128: {
                unsigned __int128 tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::Float: {
                float tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
            case Type::Double: {
                double tempval;
                if ((ptr + sizeof(tempval)) > endPtr) {
                    throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                }
                std::memcpy(&tempval, ptr, sizeof(tempval));
                ptr += sizeof(tempval);
                return correctEndianness(tempval, typeEndianness(type));
            }
        }
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    inline T fromROBN(Byte*& ptr, const Byte* endPtr, Type type) {
        return static_cast<T>(fromROBN < std::underlying_type_t<T> >
                              (ptr, endPtr, type));
    }

    // consteval
    template<typename T>
    constexpr bool isFixedBinarySize() {
        if (std::is_integral<T>::value || std::is_floating_point<T>::value) {
            return true;
        }
        if (std::is_base_of<Serializable, T>::value) {
            T t;
            auto* serializable = (Serializable*) (&t);
            return serializable->isFixedBinarySize();
        }
        return false;
    }

    // consteval
    template<typename T>
    constexpr std::uint64_t typeBinarySize() {
        if (!isFixedBinarySize<T>()) {
            return 0;
        }
        std::uint64_t size = primitiveTypeSize(primitiveTypeID<T>());
        if (size) {
            return size;
        }
        if (std::is_base_of<Serializable, T>::value) {
            T t;
            auto* serializable = (Serializable*) (&t);
            return serializable->binarySize();
        }
        return 0;
    }

    template<typename T>
    inline T returnTypeBS(std::string arg) {
        ROGUELIB_UNUSED(arg);
        return {};
    }

    template<>
    inline std::string returnTypeBS<std::string>(std::string arg) {
        return arg;
    }


    template<typename T, typename std::enable_if_t<std::is_same<std::string, T>::value, int> = 0>
    inline T fromROBN(Byte*& ptr, const Byte* const endPtr, Type type) {
        ROGUELIB_STACKTRACE
        if (type == Type::String) {
            auto length = strnlen((const char*) (ptr), std::size_t(endPtr - ptr));
            std::string str{(const char*) (ptr), length};
            return returnTypeBS<T>(str);
        }
        throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
    }

    template<typename T, typename std::enable_if_t<std::is_base_of<Serializable, T>::value, int> = 0>
    inline T fromROBN(Byte*& ptr, const Byte* const endPtr, Type type) {
        ROGUELIB_STACKTRACE
        T t{};
        auto* tPtr = (Serializable*) &t;
        tPtr->fromROBN(ptr, endPtr, type);
        return t;
    }

    template<typename>
    struct is_std_vector : std::false_type {
    };

    template<typename T>
    struct is_std_vector<std::vector<T>> : std::true_type {
    };

    template<typename T, typename A>
    struct is_std_vector<std::vector<T, A>> : std::true_type {
    };


    template<typename V, typename std::enable_if_t<std::is_same<V, std::vector<bool>>::value, int> = 0>
    V fromROBN(Byte*& ptr, const Byte* const endPtr, Type type) {
        ROGUELIB_STACKTRACE
        std::vector<bool> vector;
        auto checkPtr = [&](std::uint64_t neededBytes) {
            if ((ptr + neededBytes) > endPtr) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
        };

        if (type != Type::Vector) {
            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
        }
        checkPtr(1);
        Type lengthType = static_cast<Type>(*ptr++);
        auto length = fromROBN < std::uint64_t > (ptr, endPtr, lengthType);
        checkPtr(1);
        Type valType = static_cast<Type>(*ptr++);
        vector.resize(length);

        if (primitiveTypeSize(valType) == 0) {
            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
        }

        for (std::uint64_t i = 0; i < length; ++i) {
            vector[i] = RogueLib::ROBN::fromROBN<bool>(ptr, endPtr, type);
        }

        return vector;
    }

    template<typename V, typename std::enable_if_t<
            is_std_vector<V>::value && !std::is_same<V, std::vector<bool>>::value, int> = 0>
    V fromROBN(Byte*& ptr, const Byte* const endPtr, Type type) {
        ROGUELIB_STACKTRACE

        typedef typename V::value_type T;

        auto checkPtr = [&](std::uint64_t neededBytes) {
            if ((ptr + neededBytes) > endPtr) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
        };


        auto* startPtr = ptr;


        if (type != Type::Vector) {
            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
        }
//            auto length = BinaryConversion<std::uint64_t>::fromROBN(ptr, endPtr);

        if (std::is_integral<T>::value || std::is_floating_point<T>::value) {
            checkPtr(1);
            Type lengthType = static_cast<Type>(*ptr++);
            auto length = fromROBN < std::uint64_t > (ptr, endPtr, lengthType);
            checkPtr(1);
            Type valType = static_cast<Type>(*ptr++);
            std::vector<T> vector;
            vector.resize(length);
            // if its the same size, then i can do a memory copy
            if (removeEndianness(valType) == primitiveTypeID<T>()) {
                checkPtr(length * sizeof(T));
                // if its identical to the host representation then its only a memory copy
                if (sizeof(T) == 1 || typeEndianness(valType) == Endianness::NATIVE) {
                    std::memcpy(vector.data(), ptr, length * sizeof(T));
                } else {
                    // fuck, i need to swap the endianness,

                    // with clang 11 using -march=native
                    // and a release mode -O3 -march=natvei libc++
                    // this is faster than using intrinsics
                    for (std::size_t i = 0; i < vector.size(); ++i) {
                        T t;
                        std::memcpy(&t, ptr, sizeof(T));
                        t = swapEndianness(t);
                        vector[i] = t;
                        ptr += sizeof(T);
                    }
                }
                return vector;
            } else {
                // well, shit, its a different type

                // yes this is slow, there isn't much i can do about that
                switch (removeEndianness(type)) {
                    default: {
                        throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
                    }
                    case Type::Bool: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<bool>>(startPtr, endPtr, type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::Int8: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::int8_t>>(startPtr, endPtr,
                                                                                   type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::Int16: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::int16_t>>(startPtr, endPtr,
                                                                                    type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::Int32: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::int32_t>>(startPtr, endPtr,
                                                                                    type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::Int64: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::int64_t>>(startPtr, endPtr,
                                                                                    type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::uInt8: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::uint8_t>>(startPtr, endPtr,
                                                                                    type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::uInt16: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::uint16_t>>(startPtr, endPtr,
                                                                                     type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::uInt32: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::uint32_t>>(startPtr, endPtr,
                                                                                     type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::uInt64: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<std::uint64_t>>(startPtr, endPtr,
                                                                                     type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::Float: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<float>>(startPtr, endPtr, type));
                        ptr = startPtr;
                        return returnVector;
                    }
                    case Type::Double: {
                        auto returnVector = castVector<T>(
                                RogueLib::ROBN::fromROBN<std::vector<double>>(startPtr, endPtr, type));
                        ptr = startPtr;
                        return returnVector;
                    }
                }
            }
        } else {
            // so, its a non-integer type.
            checkPtr(1);
            Type lengthType = static_cast<Type>(*ptr++);
            auto length = fromROBN < std::uint64_t > (ptr, endPtr, lengthType);
            checkPtr(1);
            Type valType = static_cast<Type>(*ptr++);
            std::vector<T> vector;
            vector.resize(length);

            for (std::uint64_t i = 0; i < length; ++i) {
                vector[i] = RogueLib::ROBN::fromROBN<T>(ptr, endPtr, valType);
            }

            return vector;
        }
    }


    template<typename>
    struct is_std_pair : std::false_type {
    };

    template<typename T, typename A>
    struct is_std_pair<std::pair<T, A>> : std::true_type {
    };

    template<typename P, typename std::enable_if_t<is_std_pair<P>::value, int> = 0>
    inline P fromROBN(Byte*& ptr, const Byte* const endPtr, Type type) {
        ROGUELIB_STACKTRACE
        typedef typename P::first_type FT;
        typedef typename P::second_type ST;

        if (type != Type::Pair || ptr >= endPtr) {
            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
        }
        P pair{};
        Type firstType = static_cast<Type>(*ptr);
        pair.first = fromROBN < FT > (ptr, endPtr, firstType);
        if (ptr >= endPtr) {
            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
        }
        Type secondType = static_cast<Type>(*ptr);
        pair.second = fromROBN < ST > (ptr, endPtr, secondType);
        return pair;
    }

    template<typename>
    struct is_std_map : std::false_type {
    };

    template<typename T, typename A>
    struct is_std_map<std::map<T, A>> : std::true_type {
    };

    template<typename M, typename std::enable_if_t<is_std_map<M>::value, int> = 0>
    inline M fromROBN(Byte*& ptr, const Byte* const endPtr, Type type) {
        ROGUELIB_STACKTRACE
        typedef typename M::key_type KT;
        typedef typename M::mapped_type MT;

        if (type != Type::Map || ptr >= endPtr) {
            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
        }

        M map{};
        Type lengthType = static_cast<Type>(*ptr++);
        auto length = fromROBN < std::uint64_t > (ptr, endPtr, lengthType);
        for (std::uint64_t i = 0; i < length; ++i) {
            if (ptr >= endPtr || *(ptr++) != Byte{Type::Pair}) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
            map.insert(fromROBN < std::pair<KT, MT>>
            (ptr, endPtr, Type::Pair));
        }

        return map;
    }

    // i cant do *function* partial specialization
    // but i can classes.......
    template<typename T>
    class BinaryConversion {
    public:
        static ROBN toROBN(T val) {
            ROGUELIB_STACKTRACE
            if (std::is_integral<T>::value || std::is_floating_point<T>::value) {
                return primitiveToROBN(val);
            }
            if (std::is_same<std::string, T>::value) {
                ROBN bytes;
                auto* valPtr = (std::string*) &val;
                bytes.resize(valPtr->size() + 2);
                bytes[0] = Byte{Type::String};
                std::memcpy(bytes.data() + 1, valPtr->data(), valPtr->size());
                *(bytes.end() - 1) = Byte{0}; // null termination
                return bytes;
            }
            throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible type");
        }

        static T fromROBN(Byte*& ptr, const Byte* const endPtr) {
            ROGUELIB_STACKTRACE
            if (ptr > endPtr) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
            Type type = static_cast<Type>(*ptr++);
            return RogueLib::ROBN::fromROBN<T>(ptr, endPtr, type);
        }
    };

    // special case for boolean because it can be stored as packed values, so .data() doesnt work
    template<typename A>
    class BinaryConversion<std::vector<bool, A>> {
    public:
        static ROBN toROBN(std::vector<bool, A> val) {
            ROGUELIB_STACKTRACE
            // vectors of bools are really weird, because they *can* be compacted
            // because i dont use it much, i just encode each bool as a byte

            // most of this is duplicate from the normal vector code

            if (val.size() == 0) {
                // empty vector
                ROBN bytes;
                bytes.resize(11);
                bytes[0] = Byte{Type::Vector};
                bytes[1] = Byte(Type::uInt64) |
                           Byte(Endianness::NATIVE); // endianness doesnt matter because its zero
                memset(bytes.data() + 2, 0, 9);
                return bytes;
            }

            ROBN bytes;
            // the size is defined already
            bytes.resize(11 + (val.size()));
            auto* dataPtr = bytes.data();

            dataPtr[0] = Byte{Type::Vector};
            dataPtr[1] = Byte(Type::uInt64) | Byte(Endianness::NATIVE);
            dataPtr += 2;
            std::uint64_t length = val.size();
            std::memcpy(dataPtr, &length, 8);
            dataPtr += 8;
            *dataPtr = Byte{primitiveTypeID<bool>()};
            dataPtr++;
            // ok, header is done, now copy in the values
            for (int i = 0; i < val.size(); ++i) {
                *dataPtr++ = val[i]; // no, there isn't a better way to access a vector of bools
            }
            return bytes;
        }

        static std::vector<bool, A> fromROBN(Byte*& ptr, const Byte* const endPtr) {
            ROGUELIB_STACKTRACE
            if ((ptr + 1) > endPtr) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
            Type type = static_cast<Type>(*ptr++);

            return RogueLib::ROBN::fromROBN<std::vector<bool>>(ptr, endPtr, type);
        }
    };

    template<typename T, typename A>
    class BinaryConversion<std::vector<T, A>> {
    public:
        static ROBN toROBN(std::vector<T, A>& val) {
            ROGUELIB_STACKTRACE
            if (val.size() == 0) {
                // empty vector
                ROBN bytes;
                bytes.resize(11);
                bytes[0] = Byte{Type::Vector};
                bytes[1] = Byte(Type::uInt64) |
                           Byte(Endianness::NATIVE); // endianness doesnt matter because its zero
                for (int i = 2; i < 11; ++i) {
                    bytes[i] = Byte{0};
                }
                return bytes;
            }

            if (std::is_integral<T>::value || std::is_floating_point<T>::value) {
                ROBN bytes;
                // the size is defined already
                bytes.resize(11 + (val.size() * sizeof(T)));
                auto* dataPtr = bytes.data();

                dataPtr[0] = Byte{Type::Vector};
                dataPtr[1] = Byte(Type::uInt64) | Byte(Endianness::NATIVE);
                dataPtr += 2;
                std::uint64_t length = val.size();
                std::memcpy(dataPtr, &length, 8);
                dataPtr += 8;
                *dataPtr = Byte{primitiveTypeID<T>()};
                dataPtr++;
                // ok, header is done, now copy in the values
                std::memcpy(dataPtr, val.data(), val.size() * sizeof(T));
                return bytes;
            } else {

                // i cant get it to go faster...

                ROBN bytes;

//                if (is_std_vector<T>::value) {
//                    // if its a vector, then i may be able to deduce its size
//                    bytes.resize(findVectorTreeSize(val));
//                }

                // ok, so, lets see if the size is fixed so i can do pre-allocation
                if (isFixedBinarySize < T > ()) {
                    bytes.reserve(11 + typeBinarySize<T>() * val.size());
                }

                auto* dataPtr = bytes.data();
                std::uint64_t usedBytes = bytes.size();
                auto requestBytes = [&](std::uint64_t requestedBytes) {
                    if (usedBytes + requestedBytes > bytes.size()) {
                        bytes.resize(usedBytes + requestedBytes);
                        dataPtr = bytes.data() + usedBytes;
                        usedBytes += requestedBytes;
                    }
                };

                requestBytes(11);

                dataPtr[0] = Byte{Type::Vector};
                dataPtr[1] = Byte(Type::uInt64) | Byte(Endianness::NATIVE);
                dataPtr += 2;
                std::uint64_t length = bytes.size();
                std::memcpy(dataPtr, &length, 8);
                dataPtr += 8;
                *dataPtr = Byte{primitiveTypeID<T>()};


                std::vector<ROBN> subROBN;
                subROBN.reserve(length);

                std::uint64_t byteLength = 0;

                for (auto& element: val) {
                    auto vec = BinaryConversion<T>::toROBN(element);
                    byteLength += vec.size();
                    subROBN.emplace_back(vec);
                }

                bytes.reserve(bytes.size() + byteLength);

                bool first = true;
                for (const auto& vec : subROBN) {
                    bytes.insert(bytes.end(), vec.begin() + (first ? 0 : 1), vec.end());
                    first = false;
                }
                return bytes;
            }
        }

        static std::vector<T, A> fromROBN(Byte*& ptr, const Byte* const endPtr) {
            ROGUELIB_STACKTRACE
            if ((ptr + 1) > endPtr) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
            Type type = static_cast<Type>(*ptr++);

            return RogueLib::ROBN::fromROBN<std::vector<T, A>>(ptr, endPtr, type);
        }
    };

    template<typename T, typename A>
    class BinaryConversion<std::pair<T, A>> {
    public:
        static ROBN toROBN(std::pair<T, A> val) {
            ROGUELIB_STACKTRACE
            auto firstBytes = BinaryConversion<T>::toROBN(val.first);
            auto secondBytes = BinaryConversion<A>::toROBN(val.second);

            ROBN bytes;
            bytes.resize(firstBytes.size() + secondBytes.size() + 1);
            auto data = bytes.data();
            data[0] = Byte{Type::Pair};
            data++;
            std::memcpy(data, firstBytes.data(), firstBytes.size());
            data += firstBytes.size();
            std::memcpy(data, secondBytes.data(), secondBytes.size());
            return bytes;
        }

        static std::pair<T, A> fromROBN(Byte*& ptr, const Byte* const endPtr) {
            ROGUELIB_STACKTRACE
            if (ptr >= endPtr) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
            Type type = static_cast<Type>(*ptr++);
            return RogueLib::ROBN::fromROBN<std::pair<T, A>>(ptr, endPtr, type);
        }
    };


    template<typename T, typename A>
    class BinaryConversion<std::map<T, A>> {
    public:
        static ROBN toROBN(std::map<T, A> val) {
            ROGUELIB_STACKTRACE
            ROBN bytes;

            bytes.emplace_back(Byte{Type::Map});

            auto lengthBytes = BinaryConversion<std::uint64_t>::toROBN(val.size());
            bytes.insert(bytes.end(), lengthBytes.begin(), lengthBytes.end());

            for (auto elementPair : val) {
                auto pairBytes = BinaryConversion<std::pair<T, A>>::toROBN(elementPair);
                bytes.insert(bytes.end(), pairBytes.begin(), pairBytes.end());
            }

            return bytes;
        }

        static std::map<T, A> fromROBN(Byte*& ptr, const Byte* const endPtr) {
            ROGUELIB_STACKTRACE
            if (ptr >= endPtr) {
                throw Exceptions::InvalidArgument(ROGUELIB_EXCEPTION_INFO, "Incompatible binary");
            }
            Type type = static_cast<Type>(*ptr++);
            return RogueLib::ROBN::fromROBN<std::map<T, A>>(ptr, endPtr, type);
        }
    };

    template<typename T, typename std::enable_if_t<!(std::is_base_of<Serializable, T>::value ||
                                                     std::is_enum<T>::value), int> = 0>
    ROBN toROBN(T val) {
        ROGUELIB_STACKTRACE
        return BinaryConversion<T>::toROBN(val);
    }

    template<typename T>
    ROBN toROBN(std::vector<T>& val) {
        ROGUELIB_STACKTRACE
        return BinaryConversion<std::vector<T>>::toROBN(val);
    }

    template<typename T, typename std::enable_if_t<std::is_base_of<Serializable, T>::value, int> = 0>
    ROBN toROBN(T& val) {
        ROGUELIB_STACKTRACE
        return BinaryConversion<T>::toROBN(val);
    }

    template<typename T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
    ROBN toROBN(T val) {
        ROGUELIB_STACKTRACE
        typedef typename std::underlying_type<T>::type UnderlyingType;
        return BinaryConversion<UnderlyingType>::toROBN(static_cast<UnderlyingType>(val));
    }

    template<typename T>
    T fromROBN(ROBN bytes) {
        ROGUELIB_STACKTRACE
        auto* start = (Byte*) bytes.data();
        auto* end = (Byte*) (bytes.data() + bytes.size());
        return BinaryConversion<T>::fromROBN(start, end);
    }
}