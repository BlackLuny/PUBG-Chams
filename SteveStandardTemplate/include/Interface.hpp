#pragma once

#include <stdafx.hpp>
#include <utility/Logger.hpp>

namespace SteveBase {
    using namespace std;
    using namespace Utility;
    /// <Get/SetInterface>
    /// Interface central without the use of typeid
    /// This is created due to the concern of type expose side effect
    /// made by typeid.
    /// 
    /// Originally we could use GetInterface<T> and SetInterface<T>
    /// to have type safe interface container,
    /// but it is later revealed the types can be found in IDA
    /// which could lead to posibillity of exploiation by anti-cheat software.
    ///
    /// Now with the marco hack we could have type safe spec while encrypting the type
    /// information as well, but the pointers have to be handled with tricks

    using interface_hash_t = constexpr_hash_t;

#if DEBUG
    void ConnectHashToName(interface_hash_t typeHash, string name);
    string GetNameForHash(interface_hash_t typeHash);
#endif

    template<class T>
    constexpr interface_hash_t GetHashForType() {
        constexpr auto hash = MakeTypeHash<T>();
#if DEBUG
        ConnectHashToName(hash, Misc::GetTypeName<T>());
#endif
        return hash;
    }

    void *GetInterfaceImpl(const interface_hash_t hash);
    void SetInterfaceImpl(const interface_hash_t hash, void *ptr);

    template<class T>
    constexpr T *GetInterface() {
        return (T *)GetInterfaceImpl(GetHashForType<T>());
    }

    template<class T>
    constexpr T GetInterface() {
        return (T)GetInterfaceImpl(GetHashForType<T>());
    }

    template<class T>
    constexpr T *Get() {
        return GetInterface<T>();
    }

    template<class T>
    constexpr void SetInterface(T *ptr) {
        SetInterfaceImpl(GetHashForType<T>(), ptr);
    }

    template<class T>
    constexpr void SetInterface(T ptr) {
        SetInterfaceImpl(GetHashForType<T>(), ptr);
    }
}