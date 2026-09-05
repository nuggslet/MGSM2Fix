#ifndef _SQ_RDBG_H_
#define _SQ_RDBG_H_

#ifdef _WIN32
#pragma comment(lib, "WSOCK32.LIB")
#endif

template <Squirk Q>
struct SQDbgServer;
template <Squirk Q>
using HSQREMOTEDBG = SQDbgServer<Q>*;

template <Squirk Q>
HSQREMOTEDBG<Q> sq_rdbg_init(HSQUIRRELVM<Q> v,unsigned short port,SQBool autoupdate,SQBool exclusive);
template <Squirk Q>
SQRESULT sq_rdbg_waitforconnections(HSQREMOTEDBG<Q> rdbg);
template <Squirk Q>
SQRESULT sq_rdbg_shutdown(HSQREMOTEDBG<Q> rdbg);
template <Squirk Q>
SQRESULT sq_rdbg_update(HSQREMOTEDBG<Q> rdbg);

#endif //_SQ_RDBG_H_
