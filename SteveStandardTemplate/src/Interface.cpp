#include <stdafx.hpp>
#include <Interface.hpp>

namespace SteveBase {
    array<void *, 65535> interfaceMap = { nullptr };

    void *GetInterfaceImpl(const interface_hash_t hash) {
        return interfaceMap[hash % interfaceMap.size()];
    }

    void SetInterfaceImpl(const interface_hash_t hash, void *ptr) {
        interfaceMap[hash % interfaceMap.size()] = ptr;
#if DEBUG
        if (ptr != nullptr) {
            LoggerDebug("{0}: 0x{1:X}", GetNameForHash(hash), (uintptr_t)ptr);
        }
#endif
    }

#if DEBUG
    array<string, 65535> reverseHashTable;

    void ConnectHashToName(interface_hash_t typeHash, string name) {
        reverseHashTable[typeHash % reverseHashTable.size()] = name;
    }
    string GetNameForHash(interface_hash_t typeHash) {
        return reverseHashTable[typeHash % reverseHashTable.size()];
    }

#endif
}