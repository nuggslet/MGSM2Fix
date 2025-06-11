/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_STREAM_H_
#define _SQSTD_STREAM_H_

template <Squirk Q>
SQInteger _stream_readblob(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_readline(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_readn(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_writeblob(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_writen(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_seek(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_tell(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_len(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_eos(HSQUIRRELVM<Q> v);
template <Squirk Q>
SQInteger _stream_flush(HSQUIRRELVM<Q> v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}
template <Squirk Q>
SQRESULT declare_stream(HSQUIRRELVM<Q> v, const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,SQRegFunction<Q> *methods,SQRegFunction<Q> *globals);
#endif /*_SQSTD_STREAM_H_*/
