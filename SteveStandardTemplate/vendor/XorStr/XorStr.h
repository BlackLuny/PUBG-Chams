#pragma once

#include <array>
#include <numeric>
#include <string>
#include <utility>
#include <memory>

#undef xor
#undef max
#undef min

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
    constexpr auto seed = fnv1a(__DATE__ " " __TIME__ " " __FILE__);

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

        template<size_t i>
        constexpr __forceinline T enc(const T c) const {
            return c ^ keys[i];
        }

        __forceinline T dec(const T c, const size_t i) const {
            return c ^ keys[i];
        }

    public:
        template <size_t... Is>
        constexpr __forceinline XorString(const T(&str)[N], std::index_sequence<Is...>) :
        keys {
            Random<T, K + Is>()...
        },
            _encrypted{ enc<Is>(str[Is])... } {

        }

        template <size_t... Is>
        constexpr __forceinline XorString(const std::array<T, N> arr, std::index_sequence<Is...>) :
            keys {
                Random<T, K + Is>()...
            },
            _encrypted{ enc<Is>(arr[Is])... } {

        }

        __forceinline T* decrypt(void) {
            for (size_t i = 0; i < N; ++i) {
                _encrypted[i] = dec(_encrypted[i], i);
            }

            return _encrypted.data();
        }

        __forceinline std::array<T, N> decryptArray(void) {
            for (size_t i = 0; i < N; ++i) {
                _encrypted[i] = dec(_encrypted[i], i);
            }

            return _encrypted;
        }
    };
}

template <class T>
class xor_shared_string : public std::shared_ptr<T> {
    size_t n = 0;

public:
    class iterator {
    public:
        typedef iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;
        iterator(pointer ptr) : ptr_(ptr) {}
        self_type operator++() { self_type i = *this; ++ptr_; return i; }
        self_type operator++(int junk) { ++ptr_; return *this; }
        reference operator*() { return *ptr_; }
        pointer operator->() { return ptr_; }
        bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
        bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
    private:
        pointer ptr_;
    };

    class const_iterator {
    public:
        typedef const_iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef int difference_type;
        typedef std::forward_iterator_tag iterator_category;
        const_iterator(pointer ptr) : ptr_(ptr) {}
        self_type operator++() { self_type i = *this; ++ptr_; return i; }
        self_type operator++(int junk) { ++ptr_; return *this; }
        const reference operator*() { return *ptr_; }
        const pointer operator->() { return ptr_; }
        bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
        bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
    private:
        pointer ptr_;
    };


    xor_shared_string(T *ptr) : std::shared_ptr<T>(ptr), n(strlen(ptr)) {

    }

    xor_shared_string(T *ptr, const size_t N) : std::shared_ptr<T>(ptr), n(N) {

    }

    operator T *() const {
        return get();
    }

    constexpr auto c_str() const {
        return get();
    }

    constexpr auto data() const {
        return c_str();
    }

    constexpr auto length() const {
        return n;
    }

    constexpr auto runtime_length() const {
        return strlen(get());
    }

    constexpr auto size() const {
        return length();
    }

    constexpr auto duplicate() const {
        return xor_shared_string(_strdup(c_str()));
    }
};

#if DEBUG == 0

#ifndef array_encrypt_onetime
#define array_encrypt_onetime(arr) \
    (CompileTimeEncryption::XorString<char, (arr).size(), CompileTimeEncryption::Random<char, __COUNTER__>() >( \
        (arr), std::make_index_sequence<(arr).size()>() \
    ))
#endif

#ifndef text
#define textonce(s) \
    ((CompileTimeEncryption::XorString<char, sizeof(s), CompileTimeEncryption::Random<char, __COUNTER__>() >( \
        s, std::make_index_sequence<(sizeof(s) / sizeof(char))>() \
    )).decrypt())

#define textdup(s) _strdup(textonce(s))
#define textpp(s) (std::string(textonce(s)))
#define text(s) ( xor_shared_string<const char>(textdup(s), sizeof(s) / sizeof(char) - 1) )

#endif

#ifndef textw
#define textwonce(s) \
    ((CompileTimeEncryption::XorString<wchar_t, (sizeof(s) / sizeof(wchar_t)), CompileTimeEncryption::Random<wchar_t, __COUNTER__, 512>() >( \
        s, std::make_index_sequence<(sizeof(s) / sizeof(wchar_t))>() \
    )).decrypt())

#define textwdup(s) _wcsdup(textwonce(s))
#define textwpp(s) (std::wstring( textwonce(s) ))
#define textw(s) ( xor_shared_string<const wchar_t>(textdup(s), sizeof(s) / sizeof(wchar_t) - 1) )

// #define textw(s) (wcsdup(CompileTimeEncryption::XorString<wchar_t, sizeof(s), CompileTimeEncryption::Random<wchar_t, __COUNTER__ + __LINE__ + sizeof(s), 512>() >( s, std::make_index_sequence<sizeof(s)>() ).decrypt()))
#endif

#else

#ifndef text
#define textdup _strdup
#define textonce(s) (s)
#define text(s) ( xor_shared_string<const char>(_strdup(s), (sizeof(s) / sizeof(char)) - 1) )
#endif

#ifndef textw
#define textwdup _wcsdup
#define textwonce(s) (s)
#define textw(s) ( xor_shared_string<const wchar_t>(_wcsdup(s), (sizeof(s) / sizeof(wchar_t)) - 1) )
#endif

#endif