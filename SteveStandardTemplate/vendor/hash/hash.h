#pragma once

#include <cstdint>

using constexpr_hash_t = uint32_t;

template<class U = char>
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

#define run_time_fnv1a_hash fnv1a
#define compile_time_fnv1a_hash(s) forced_value_v<constexpr_hash_t, fnv1a(s)>


#define run_time_hash run_time_fnv1a_hash
#define compile_time_hash compile_time_fnv1a_hash

template<class T>
constexpr constexpr_hash_t MakeTypeHash() {
#ifdef _MSC_VER
    return compile_time_hash(__FUNCDNAME__);
#else
    return compile_time_hash(__PRETTY_FUNCTION__);
#endif
}
