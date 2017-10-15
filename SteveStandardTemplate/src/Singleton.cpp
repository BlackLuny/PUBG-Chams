#include <Singleton.hpp>

namespace SteveBase {
  // JudySortedArray<void *> singletonMap;
  array<void *, 65536> singletonMap = { };

  void *GetSingletonImpl(interface_hash_t hash) {
    return singletonMap[hash % singletonMap.size()];
  }

  void SetSingletonImpl(interface_hash_t hash, void *ptr) {
    singletonMap[hash % singletonMap.size()] = ptr;
  }
}