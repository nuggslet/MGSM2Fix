/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_AUXLIB_H_
#define _SQSTD_AUXLIB_H_

template <Squirk Q>
SQUIRREL_API void sqstd_seterrorhandlers(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQUIRREL_API void sqstd_printcallstack(HSQUIRRELVM<Q> v);

#endif /* _SQSTD_AUXLIB_H_ */
