/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_STREAM_H_
#define _SQSTD_STREAM_H_

template <Squirk T>
SQInteger _stream_readblob(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_readline(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_readn(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_writeblob(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_writen(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_seek(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_tell(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_len(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_eos(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _stream_flush(HSQUIRRELVM<T> v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}
template <Squirk T>
SQRESULT declare_stream(HSQUIRRELVM<T> v, const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,SQRegFunction<T> *methods,SQRegFunction<T> *globals);
#endif /*_SQSTD_STREAM_H_*/
