#if !defined(_SCRAT_PROXY_METHODS_H_)
#define _SCRAT_PROXY_METHODS_H_

#ifdef _SQ_M2
#include <type_traits>
#include <utility>

#include "sqratTypes.h"

namespace Sqrat {

template <Squirk Q, class C, class Method>
struct SqProxy;

template <Squirk Q, class C, class R, class... Args>
struct SqProxy<Q, C, R (*)(C*, Args...)> {
    using Method = R (*)(C*, Args...);

    template <size_t... Indices>
    static SQInteger Call(HSQUIRRELVM<Q> vm, Method method, C* instance, std::index_sequence<Indices...>) {
        if constexpr (std::is_void_v<R>) {
            method(instance, Var<Args, Q>(vm, static_cast<SQInteger>(Indices) + 2).value...);
            return 0;
        } else {
            PushVar(vm, method(instance, Var<Args, Q>(vm, static_cast<SQInteger>(Indices) + 2).value...));
            return 1;
        }
    }

    static SQInteger Func(HSQUIRRELVM<Q> vm) {
        Method* method = NULL;
        if (SQ_FAILED(sq_getuserdata(vm, -1, reinterpret_cast<SQUserPointer*>(&method), NULL)) || !method) {
            return sq_throwerror(vm, _SC("invalid proxy function"));
        }
        C* instance = ClassType<C, Q>::GetInstance(vm, 1);
        if (!instance) return sq_throwerror(vm, _SC("invalid instance"));
        return Call(vm, *method, instance, std::index_sequence_for<Args...>{});
    }
};

template <Squirk Q, class C, class R, class... Args>
inline SQFUNCTION<Q> SqProxyFunc(R (*)(C*, Args...)) {
    return &SqProxy<Q, C, R (*)(C*, Args...)>::Func;
}

}
#endif

#endif