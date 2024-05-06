/*	see copyright notice in squirrel.h */
#ifndef _SQSTDBLOB_H_
#define _SQSTDBLOB_H_

template <Squirk T>
SQUIRREL_API SQUserPointer sqstd_createblob(HSQUIRRELVM<T> v, SQInteger size);
template <Squirk T>
SQUIRREL_API SQRESULT sqstd_getblob(HSQUIRRELVM<T> v,SQInteger idx,SQUserPointer *ptr);
template <Squirk T>
SQUIRREL_API SQInteger sqstd_getblobsize(HSQUIRRELVM<T> v,SQInteger idx);

template <Squirk T>
SQUIRREL_API SQRESULT sqstd_register_bloblib(HSQUIRRELVM<T> v);

#endif /*_SQSTDBLOB_H_*/

