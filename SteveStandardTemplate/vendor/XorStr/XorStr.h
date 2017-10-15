#pragma once

#include <array>
#include <numeric>
#include <string>
#include <utility>

#undef xor

namespace CompileTimeEncryption {

    template<class U>
    constexpr uint32_t fnv1a(const U *input) {
        constexpr uint32_t prime = 16777619;
        uint32_t hash = 2166136261;

        for (int i = 0; input[i] != '\0'; i++) {
            hash ^= input[i];
            hash *= prime;
        }

        return hash;
    }

    template<class T, T val>
    constexpr static T forced_value_v = val;

    // WARNING: do not use __TIMESTAMP__ alone
    // __TIMESTAMP__ will dissolve to the last modification date of this header file!
    // meaning if this file is untouched __TIMESTAMP__ will be too! (it will be a constant)
    // __DATE__ and __TIME__ are more reliable but they are still determinstic, 
    // since their value are based on the compilation date/time, the moment you clicked build
    // and worst if you do not modify any source file both __DATE__ and __TIME__ will persist
    // because the compiler cached it   
    constexpr auto seed = forced_value_v<uint32_t, fnv1a(__DATE__ " " __TIME__ " " __FILE__)>;

    constexpr static uint32_t a = 16807; // 7^5
    constexpr static uint32_t m = std::numeric_limits<int32_t>::max(); // 2^31 - 1

    template <int32_t N>
    constexpr __forceinline uint32_t RandomGenerator() {
        if constexpr (N == 0) {
            return seed;
        } else {
            constexpr uint32_t s = RandomGenerator< N - 1 >();
            constexpr uint32_t lo = a * (s & 0xFFFF); // Multiply lower 16 bits by 16807
            constexpr uint32_t hi = a * (s >> 16); // Multiply higher 16 bits by 16807
            constexpr uint32_t lo2 = lo + ((hi & 0x7FFF) << 16); // Combine lower 15 bits of hi with lo's upper bits
            constexpr uint32_t lo3 = lo2 + hi;
            return lo3 > m ? lo3 - m : lo3;
        }
    }

    template <class T, int32_t N, int32_t M = std::numeric_limits<T>::max()>
    constexpr __forceinline T Random() {
        return static_cast<T>(1 + RandomGenerator<N + 1>() % (M - 1));
    }

    template < class T, size_t N, T K >
    struct XorString {
    private:
        const std::array<T, N> keys;
        std::array<T, N> _encrypted;

        constexpr __forceinline T swapChar(T c) const {
            if constexpr (std::is_same<T, wchar_t>()) {
                return ((c & 0b11111111'00000000) >> 8) | ((c & 0b00000000'11111111) << 8);
            } else {
                return ((c & 0b1111'0000) >> 4) | ((c & 0b0000'1111) << 4);
            }
        }


        // i've added those binary op cause I don't want to see associativity hell
        template<class U, class V>
        constexpr __forceinline U xor (const U a, const V b) const {
            return a ^ b;
        }

        template<class U, class V>
        constexpr __forceinline U add(const U a, const V b) const {
            return a + b;
        }

        template<class U, class V>
        constexpr __forceinline U sub(const U a, const V b) const {
            return a - b;
        }

#define Divisible(a, b) ((a) % (b) == 0)

        template<size_t i>
        constexpr __forceinline T enc(const T c) const {
#if 0
            // xor swapped key and swapped index
            if constexpr (Divisible(i + key, 14)) {
                return xor (xor (c, swapChar(keys[i])), swapChar(i));
            }

            // xor swapped key characters then minus/plus index
            if constexpr (Divisible(key, 13)) {
                return sub(xor (c, swapChar(keys[i])), i);
            }
            if constexpr (Divisible(i + key, 12)) {
                return add(xor (c, swapChar(keys[i])), i);
            }
            // xor swapped key and index
            if constexpr (Divisible(key, 11)) {
                return xor (xor (c, swapChar(keys[i])), i);
            }
            // xor swapped key
            if constexpr (Divisible(i + key, 10)) {
                return xor (c, swapChar(keys[i]));
            }
            // swap character
            if constexpr (Divisible(key, 9)) {
                return swapChar(c);
            }

            // minus/plus index then xor key
            if constexpr (Divisible(i + key, 8)) {
                return xor (sub(c, i), keys[i]);
            }
            if constexpr (Divisible(key, 7)) {
                return xor (add(c, i), keys[i]);
            }

            // xor key then minus/plus index 
            if constexpr (Divisible(i + key, 6)) {
                return sub(xor (c, keys[i]), i);
            }
            if constexpr (Divisible(key, 5)) {
                return add(xor (c, keys[i]), i);

            }

            // xor key and index
            if constexpr (Divisible(i + key, 4)) {
                return xor (xor (c, keys[i]), i);
            }

            // minus/plus index
            if constexpr (Divisible(key, 3)) {
                return sub(c, i);
            }
            if constexpr (Divisible(i + key, 2)) {
                return add(c, i);
            } else {
                // default
                return xor (c, keys[i]);
            }
#endif
            return swapChar(add(xor(swapChar(c), swapChar(keys[i])), K));
        }

        // commented return in the select block: original cipher

        __forceinline T dec(const T c, const int i) const {
#if 0
            // xor swapped key and swapped index
            if (Divisible(i + keys[i], 14)) {
                return xor (xor (c, swapChar(i)), swapChar(keys[i]));
                // xor(xor(c, swapChar(keys[i])), i);
            }

            // xor swapped key characters then minus/plus index
            if (Divisible(keys[i], 13)) {
                return xor (add(c, i), swapChar(keys[i]));
                // sub(xor(c, swapChar(keys[i])), i);
            }
            if (Divisible(i + keys[i], 12)) {
                return xor (sub(c, i), swapChar(keys[i]));
                // add(xor(c, swapChar(keys[i])), i);
            }

            // xor swapped key and index
            if (Divisible(keys[i], 11)) {
                return xor (xor (c, i), swapChar(keys[i]));
                // xor(xor(c, swapChar(keys[i])), i);
            }

            // xor swapped key
            if (Divisible(i + keys[i], 10)) {
                return xor (c, swapChar(keys[i]));
                // xor(c, swapChar(keys[i]));
            }

            // swap character
            if (Divisible(keys[i], 9)) {
                return swapChar(c);
                // swapChar(c);
            }

            // minus/plus index then xor key
            if (Divisible(i + keys[i], 8)) {
                return add(xor (c, keys[i]), i);
                // xor(sub(c, i), keys[i]);
            }
            if (Divisible(keys[i], 7)) {
                return sub(xor (c, keys[i]), i);
                // xor(add(c, i), keys[i]);
            }

            // xor key then minus/plus index 
            if (Divisible(i + keys[i], 6)) {
                return xor (add(c, i), keys[i]);
                // sub(xor (c, keys[i]), i);
            }
            if (Divisible(keys[i], 5)) {
                return xor (sub(c, i), keys[i]);
                // add(xor (c, keys[i]), i);
            }

            // xor key and index
            if (Divisible(i + keys[i], 4)) {
                return xor (xor (c, i), keys[i]);
                // xor(xor(c ^ keys[i]), i);
            }

            // minus/plus index
            if (Divisible(keys[i], 3)) {
                return add(c, i);
                // sub(c, i)
            }
            if (Divisible(i + keys[i], 2)) {
                return sub(c, i);
                // add(c, i)
            } else {
                return xor (c, keys[i]);
            }
#endif
            return swapChar(xor(sub(swapChar(c), K), swapChar(keys[i])));
            // return swapChar(add(xor (swapChar(c), swapChar(keys[i])), K));
        }

#undef Divisible
    public:
        template <size_t... Is>
        constexpr __forceinline XorString(const T (&str)[N], std::index_sequence<Is...>) :
        keys{
            Random<T, K + Is>()...
        },
        _encrypted{ enc<Is>(str[Is])... } {

        }

        __forceinline T* decrypt(void) {
            for (size_t i = 0; i < N; ++i) {
                _encrypted[i] = dec(_encrypted[i], i);
            }
            _encrypted[N - 1] = '\0';

            return _encrypted.data();
        }
    };
}

#ifndef DEBUG

#ifndef text
#define text(s) (std::string(CompileTimeEncryption::XorString<char, sizeof(s), CompileTimeEncryption::Random<char, __COUNTER__ + __LINE__ + sizeof(s)>() >( s, std::make_index_sequence<sizeof(s) - 1>() ).decrypt()))
#endif

#ifndef textw
#define textw(s) (std::wstring(CompileTimeEncryption::XorString<wchar_t, sizeof(s), CompileTimeEncryption::Random<wchar_t, __COUNTER__ + __LINE__ + sizeof(s), 512>() >( s, std::make_index_sequence<sizeof(s) - 1>() ).decrypt()))
#endif

#else

#ifndef text
#define text(s) (std::string(s))
#endif

#ifndef textw
#define textw(s) (std::wstring(s))
#endif

#endif