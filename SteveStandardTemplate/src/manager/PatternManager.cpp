#include <stdafx.hpp>
#include <manager/PatternManager.hpp>
#include <utility/SystemUtility.hpp>
#include <utility/Logger.hpp>

namespace SteveBase::Manager {
    using namespace Utility;
  
    using PatternOffsetPair = Pair<const char *, int>;
    using PatternNamePatternOffsetPair = Pair<constexpr_hash_t, PatternOffsetPair>;
    using PatternList = List<PatternNamePatternOffsetPair>;
    using ScanModuleEntry = Pair<const char *, PatternList>;

    inline PatternNamePatternOffsetPair MakePattern(const constexpr_hash_t hash, const char *pat, const int offset = 0) {
        return { hash,{ pat, offset } };
    }

#define GenPattern(pat, ...) textdup(pat), __VA_ARGS__ )

#if DEBUG == 1
    JudySortedArray<const char *> reverseHashTableForPatterns;

    inline constexpr_hash_t PutReverHashTableEntry(const constexpr_hash_t hash, const char *str) {
        reverseHashTableForPatterns[hash] = str;
        return hash;
    }

#define Pattern(s) MakePattern( PutReverHashTableEntry(compile_time_hash(s), textdup(s)), GenPattern
#else
#define Pattern(s) MakePattern( compile_time_hash(s), GenPattern
#endif

    struct ScanModule : ScanModuleEntry {
        ScanModule(const char *name, PatternList &&patternList) {
            first = name;
            second = patternList;
        }
    };

    ScanModule patterns[] = {
        ScanModule {
            textdup("TslGame.exe"), {
                Pattern("RHIEndDrawingViewport")("48 89 5C 24 ? 55 56 57 41 56 41 57 48 83 EC 40 FF 81 ? ? ? ?"),
            }
        },
    };

    JudySortedArray<uintptr_t> resolvedPattern;

    uintptr_t PatternManager::GetPatternImpl(const constexpr_hash_t hash) {
        return resolvedPattern[hash];
    }

    /*
    uintptr_t PatternManager::GetPatternDynamically(string dllName, string pattern) {
        const auto hash = run_time_hash(dllName.data()) * run_time_hash(pattern.data());
        if (GetPatternImpl(hash) == 0U) {
            const auto pat = SystemUtility::FindModulePattern(dllName, pattern);
            resolvedPattern[hash] = (uintptr_t)pat;
        }
        return GetPatternImpl(hash);
    }
    */

    void PatternManager::Init() {
        for (auto &it1 : patterns) {
            const auto &dllName = it1.first;
            const auto dllHash = run_time_hash(dllName);

            for (auto &it2 : it1.second) {
                const constexpr_hash_t &nameHash = it2.first;
                const auto &pattern = it2.second.first;
                const int &offset = it2.second.second;
                const auto key = dllHash * nameHash;
                const auto result = SystemUtility::FindModulePattern(dllName, pattern);

                if (result) {
                    resolvedPattern[key] = (uintptr_t)(result + offset);
                    LoggerDebug("Pattern Found, From \"{0}\", Named {1}, Address Is 0x{2:X}...", dllName, reverseHashTableForPatterns[nameHash], (uintptr_t)resolvedPattern[key]);
                } else {
                    LoggerDebug("Pattern Not Found, From \"{0}\", Named {1}", dllName, reverseHashTableForPatterns[nameHash]);
                }
            }
        }
    }
}