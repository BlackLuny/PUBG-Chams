#pragma once

#include "FunctionalProgramming.hpp"
#include "TypeAliases.hpp"

namespace SteveBase::Misc {
    using namespace std;

    // Not really data strucuture related tho
    template <class T>
    constexpr auto ShallowCopy(T *to, T *from) {
        return (T *)memcpy(to, from, sizeof T);
    }

    template <class T>
    constexpr auto ShallowCopy(T *to) {
        return partial([](T *to, T *from) {
            return ShallowCopy<T>(to, from);
        }, to);
    }

    template <class T>
    constexpr shared_ptr<T> ShallowClone(T *src) {
        shared_ptr<T> mem((T *)malloc(sizeof T), free);
        ShallowCopy(mem.get(), src); // kind of dangerous
        return mem;
    }

    // copypasted: https://stackoverflow.com/a/39018368/3289081
    inline wstring ConvertStringOfBytesToWides(string str) {
        try {
            wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
            return converter.from_bytes(str);
        } catch (range_error&) {
            auto length = str.length();
            wstring result;
            result.reserve(length);
            for (size_t i = 0; i < length; i++) {
                result.push_back(str[i] & 0xFF);
            }
            return result;
        }
    }

    inline string ConvertStringOfWidesToBytes(wstring str) {
        wstring_convert<codecvt_utf8_utf16<wchar_t>> wcu8;
        return wcu8.to_bytes(str);
    }
}
