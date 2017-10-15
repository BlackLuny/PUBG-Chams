#pragma once

#include <cstdint>
#include "TypeAliases.hpp"

namespace SteveBase::Misc {
    /// <ProxiedClass>
    ///	Utility to manipulate runtime C++ objects
    /// Example: class Foo : ProxiedClass {};

    class ProxiedClass {
    public:
        using vfunc_t = uintptr_t;
        using vtable_t = vfunc_t *;

        constexpr vtable_t GetVMT() {
            return *reinterpret_cast<vtable_t*>((uintptr_t)this);
        }

        constexpr vfunc_t GetVFunc(const size_t idx) {
            return GetVMT()[idx];
        }

        template<class Fn>
        constexpr Fn GetVFunc(const size_t i) {
            return (Fn)GetVFunc(i);
        }

        template<class T>
        constexpr T *Pointer(const size_t n = 0) {
            return (T*)((uintptr_t)this + n);
        }

        template<class T>
        constexpr T &Data(const size_t n = 0) {
            return *Pointer<T>(n);
        }

        template<typename Fn>
        constexpr auto CallMemberFuncByAddress(uintptr_t addr) {
            return partial((FuncThisCall<Fn>) addr, this);
        }

        template<typename Fn>
        constexpr auto CallVirt(const size_t idx) {
            return CallMemberFuncByAddress<Fn>(GetVFunc(idx));
        }
    };

#define VFuncClosureBase(fnName, sig, _method) constexpr __forceinline auto fnName sig { return _method <auto sig>

#define NestedDefineVFuncEnd(...) (__VA_ARGS__); }
#define NestedDefineVFuncIndex(idx) (idx) NestedDefineVFuncEnd
#define DefineVFunc(fnName, sig) VFuncClosureBase(fnName, sig, CallVirt) NestedDefineVFuncIndex

#define NestedDefinePatternVFuncPattern(dllName, patternName) (GetPattern(dllName)(patternName)) NestedDefineVFuncEnd
#define DefinePatternVFunc(fnName, sig) VFuncClosureBase(fnName, sig, CallMemberFunc) NestedDefinePatternVFuncPattern

#define NestedDefinePatternVFuncDynamicPattern(dllName, patternName) (PatternManager::GetPatternDynamically(text(dllName), text(patternName))) NestedDefineVFuncEnd
#define DefinePatternVFuncDynamic(fnName, sig) VFuncClosureBase(fnName, sig, CallMemberFunc) NestedDefinePatternVFuncPattern

#define BeginAccessorBase(fnName, sig) \
  __forceinline auto &fnName() { \
        using Fn = auto sig;\
		using T = function<Fn>::result_type;\

#define EndAccessorBase(src) \
        return Data<T>(src); \
  }

#define PODAccessor(fnName, sig) BeginAccessorBase(fnName, sig) EndAccessorBase
#define GenNetVarAccessor(tableName, varName) EndAccessorBase(NetVar(tableName, varName))
#define NetVarAccessor(fnName, sig) BeginAccessorBase(fnName, sig) GenNetVarAccessor
}