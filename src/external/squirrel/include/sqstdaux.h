/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_AUXLIB_H_
#define _SQSTD_AUXLIB_H_

template <Squirk T>
SQUIRREL_API void sqstd_seterrorhandlers(HSQUIRRELVM<T> v);
template <Squirk T>
SQUIRREL_API void sqstd_printcallstack(HSQUIRRELVM<T> v);

#endif /* _SQSTD_AUXLIB_H_ */
