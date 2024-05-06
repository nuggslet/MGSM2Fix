#ifndef _SQ_RDBG_H_
#define _SQ_RDBG_H_

#ifdef _WIN32
#pragma comment(lib, "WSOCK32.LIB")
#endif

template <Squirk T>
struct SQDbgServer;
template <Squirk T>
using HSQREMOTEDBG = SQDbgServer<T>*;

template <Squirk T>
HSQREMOTEDBG<T> sq_rdbg_init(HSQUIRRELVM<T> v,unsigned short port,SQBool autoupdate,SQBool exclusive);
template <Squirk T>
SQRESULT sq_rdbg_waitforconnections(HSQREMOTEDBG<T> rdbg);
template <Squirk T>
SQRESULT sq_rdbg_shutdown(HSQREMOTEDBG<T> rdbg);
template <Squirk T>
SQRESULT sq_rdbg_update(HSQREMOTEDBG<T> rdbg);

#endif //_SQ_RDBG_H_
