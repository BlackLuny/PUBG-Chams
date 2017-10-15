#pragma once


namespace std {
    struct integer_prehashed {
        constexpr size_t operator()(const constexpr_hash_t& k) const {
            return k;
        }
    };
}

// namespace SteveBase::Misc 
// {
template <typename T>
union FuncSignatureConverter;

template<typename R, typename... Args>
union FuncSignatureConverter<R(Args...)> {
    using type = R(*) (Args...);
};

template <typename T>
using Func = typename FuncSignatureConverter<T>::type;

template <typename T>
union FuncCdeclSignatureConverter;

template<typename R, typename... Args>
union FuncCdeclSignatureConverter<R(Args...)> {
    using type = R(__cdecl *) (Args...);
};

template <typename T>
using FuncCdecl = typename FuncCdeclSignatureConverter<T>::type;

template <typename T>
union FuncStdCallSignatureConverter;

template<typename R, typename... Args>
union FuncStdCallSignatureConverter<R(Args...)> {
    using type = R(__stdcall *) (Args...);
};

template <typename T>
using FuncStdCall = typename FuncStdCallSignatureConverter<T>::type;

template <typename T>
union FuncFastCallSignatureConverter;

template<typename R, typename... Args>
union FuncFastCallSignatureConverter<R(Args...)> {
    using type = R(__fastcall *) (void *, void *, Args...);
};

template <typename T>
using FuncFastCall = typename FuncFastCallSignatureConverter<T>::type;

template <typename T>
union FuncThisCallSignatureConverter;

template<typename R, typename... Args>
union FuncThisCallSignatureConverter<R(Args...)> {
    using type = R(__thiscall *) (void *, Args...);
};

template <typename T>
using FuncThisCall = typename FuncThisCallSignatureConverter<T>::type;

template <typename... Args>
using Action = Func<void(Args...)>;

template <typename... Args>
using ActionCdecl = FuncCdecl<void(Args...)>;

template <typename... Args>
using ActionStdCall = FuncStdCall<void(Args...)>;

template <typename... Args>
using ActionFastCall = FuncFastCall<void(Args...)>;

template <typename... Args>
using ActionThisCall = FuncThisCall<void(Args...)>;

template<class T>
using List = std::vector<T>;

template<class F, class S>
using Pair = std::pair<F, S>;

template<class K, class V, class... Args>
using Dictionary = std::unordered_map<K, V, Args...>;

template<class K, class V, class... Args>
using SortedDictionary = std::map<K, V, Args...>;

template<class T>
using JudyHashArray = Dictionary<constexpr_hash_t, T, std::integer_prehashed>;

template<class T>
using JudySortedArray = SortedDictionary<constexpr_hash_t, T>;

template<class T>
using LazyEvaluated = Func<T()>;
// }