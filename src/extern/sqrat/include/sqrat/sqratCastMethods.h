#if !defined(_SCRAT_CASTMEMBER_METHODS_H_)
#define _SCRAT_CASTMEMBER_METHODS_H_

#ifdef _SQ_M2
#include <type_traits>
#include <utility>

#include "sqratTypes.h"

namespace Sqrat {

template <Squirk Q, class SrcT, class Method>
struct SqCastMember;

template <Squirk Q, class SrcT, class C, class R, class... Args>
struct SqCastMember<Q, SrcT, R (C::*)(Args...)> {
    using Method = R (C::*)(Args...);

    template <size_t... Indices>
    static SQInteger Call(HSQUIRRELVM<Q> vm, Method method, C* instance, std::index_sequence<Indices...>) {
        if constexpr (std::is_void_v<R>) {
            (instance->*method)(Var<Args, Q>(vm, static_cast<SQInteger>(Indices) + 2).value...);
            return 0;
        } else {
            PushVar(vm, (instance->*method)(Var<Args, Q>(vm, static_cast<SQInteger>(Indices) + 2).value...));
            return 1;
        }
    }

    static SQInteger Func(HSQUIRRELVM<Q> vm) {
        Method* method = NULL;
        if (SQ_FAILED(sq_getuserdata(vm, -1, reinterpret_cast<SQUserPointer*>(&method), NULL)) || !method) {
            return sq_throwerror(vm, _SC("invalid cast function"));
        }
        SrcT* source = ClassType<SrcT, Q>::GetInstance(vm, 1);
        if (!source) return sq_throwerror(vm, _SC("invalid instance"));
        C* instance = static_cast<C*>(*source);
        if (!instance) return sq_throwerror(vm, _SC("invalid instance"));
        return Call(vm, *method, instance, std::index_sequence_for<Args...>{});
    }
};

template <Squirk Q, class SrcT, class C, class R, class... Args>
struct SqCastMember<Q, SrcT, R (C::*)(Args...) const> {
    using Method = R (C::*)(Args...) const;

    template <size_t... Indices>
    static SQInteger Call(HSQUIRRELVM<Q> vm, Method method, const C* instance, std::index_sequence<Indices...>) {
        if constexpr (std::is_void_v<R>) {
            (instance->*method)(Var<Args, Q>(vm, static_cast<SQInteger>(Indices) + 2).value...);
            return 0;
        } else {
            PushVar(vm, (instance->*method)(Var<Args, Q>(vm, static_cast<SQInteger>(Indices) + 2).value...));
            return 1;
        }
    }

    static SQInteger Func(HSQUIRRELVM<Q> vm) {
        Method* method = NULL;
        if (SQ_FAILED(sq_getuserdata(vm, -1, reinterpret_cast<SQUserPointer*>(&method), NULL)) || !method) {
            return sq_throwerror(vm, _SC("invalid cast function"));
        }
        SrcT* source = ClassType<SrcT, Q>::GetInstance(vm, 1);
        if (!source) return sq_throwerror(vm, _SC("invalid instance"));
        const C* instance = static_cast<const C*>(*source);
        if (!instance) return sq_throwerror(vm, _SC("invalid instance"));
        return Call(vm, *method, instance, std::index_sequence_for<Args...>{});
    }
};

template <Squirk Q, class SrcT, class C, class R, class... Args>
inline SQFUNCTION<Q> SqCastMemberFunc(R (C::*)(Args...)) {
    return &SqCastMember<Q, SrcT, R (C::*)(Args...)>::Func;
}

template <Squirk Q, class SrcT, class C, class R, class... Args>
inline SQFUNCTION<Q> SqCastMemberFunc(R (C::*)(Args...) const) {
    return &SqCastMember<Q, SrcT, R (C::*)(Args...) const>::Func;
}

}
#endif

#endif