#pragma once

#include <stdafx.hpp>

namespace SteveBase::Manager {
    using namespace std;

    class PatternManager {
    public:
        static void Init();

        static uintptr_t GetPatternImpl(constexpr_hash_t hash);
        // static uintptr_t GetPatternDynamically(string dllName, string pattern);
    };
#define GenGetPattern(patternName) compile_time_hash(patternName))
#define GetPattern(dllName) PatternManager::GetPatternImpl(compile_time_hash(dllName) * GenGetPattern
}