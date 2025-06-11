/*	see copyright notice in squirrel.h */
#ifndef _SQSTDBLOB_H_
#define _SQSTDBLOB_H_

template <Squirk Q>
SQUIRREL_API SQUserPointer sqstd_createblob(HSQUIRRELVM<Q> v, SQInteger size);
template <Squirk Q>
SQUIRREL_API SQRESULT sqstd_getblob(HSQUIRRELVM<Q> v,SQInteger idx,SQUserPointer *ptr);
template <Squirk Q>
SQUIRREL_API SQInteger sqstd_getblobsize(HSQUIRRELVM<Q> v,SQInteger idx);

template <Squirk Q>
SQUIRREL_API SQRESULT sqstd_register_bloblib(HSQUIRRELVM<Q> v);

#endif /*_SQSTDBLOB_H_*/

