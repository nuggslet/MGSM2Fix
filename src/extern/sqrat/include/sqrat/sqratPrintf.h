#if !defined(_SCRAT_PRINTF_H_)
#define _SCRAT_PRINTF_H_

#ifdef _SQ_M2
#include <sqstdstring.h>

#include "sqratTypes.h"

namespace Sqrat {

template <Squirk Q, class... Args>
inline SQInteger printf(HSQUIRRELVM<Q> vm, const SQChar* format, const Args&... args) {
    sq_pushstring(vm, format, -1);
    (PushVar(vm, args), ...);
    return sqstd_printf(vm, sizeof...(Args));
}

}
#endif

#endif