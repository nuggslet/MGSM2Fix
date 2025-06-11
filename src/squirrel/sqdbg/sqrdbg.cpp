#ifdef _WIN32
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <arpa/inet.h>
#  include <sys/time.h>
#  define SOCKET_ERROR (-1)
#  define TIMEVAL struct timeval
#endif
#include <stdio.h>
#include <string.h>

#include "../m2hook.h"
#include "resource.h"

#include <squirrel.h>
#include "sqrdbg.h"
#include "sqdbgserver.h"
template <Squirk Q>
SQInteger debug_hook(HSQUIRRELVM<Q> v, HSQUIRRELVM<Q> _v, HSQREMOTEDBG<Q> rdbg);
template <Squirk Q>
SQInteger error_handler(HSQUIRRELVM<Q> v);

template <Squirk Q>
HSQREMOTEDBG<Q> sq_rdbg_init(HSQUIRRELVM<Q> v,unsigned short port,SQBool autoupdate,SQBool exclusive)
{
	struct sockaddr_in bindaddr;
#ifdef _WIN32
	WSADATA wsadata;
	if (WSAStartup (MAKEWORD(2,2), &wsadata) != 0){
		return NULL;  
	}	
#endif 
	SQDbgServer<Q> *rdbg = new SQDbgServer<Q>(v);
	rdbg->_exclusive = exclusive?true:false;
	rdbg->_autoupdate = autoupdate?true:false;
	rdbg->_accept = socket(AF_INET,SOCK_STREAM,0);
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons(port);
	bindaddr.sin_addr.s_addr = htonl (INADDR_ANY);
	if(::bind(rdbg->_accept,(sockaddr*)&bindaddr,sizeof(bindaddr))==SOCKET_ERROR){
		delete rdbg;
		sq_throwerror(v,_SC("failed to bind the socket"));
		return NULL;
	}
	if(v && !rdbg->Init(v)) {
		delete rdbg;
		sq_throwerror(v,_SC("failed to initialize the debugger"));
		return NULL;
	}
	
    return rdbg;
}

template HSQREMOTEDBG<Squirk::Standard> sq_rdbg_init<Squirk::Standard>(HSQUIRRELVM<Squirk::Standard> v, unsigned short port, SQBool autoupdate, SQBool exclusive);
template HSQREMOTEDBG<Squirk::AlignObject> sq_rdbg_init<Squirk::AlignObject>(HSQUIRRELVM<Squirk::AlignObject> v, unsigned short port, SQBool autoupdate, SQBool exclusive);
template HSQREMOTEDBG<Squirk::StandardShared> sq_rdbg_init<Squirk::StandardShared>(HSQUIRRELVM<Squirk::StandardShared> v, unsigned short port, SQBool autoupdate, SQBool exclusive);
template HSQREMOTEDBG<Squirk::AlignObjectShared> sq_rdbg_init<Squirk::AlignObjectShared>(HSQUIRRELVM<Squirk::AlignObjectShared> v, unsigned short port, SQBool autoupdate, SQBool exclusive);

template <Squirk Q>
SQRESULT sq_rdbg_waitforconnections(HSQREMOTEDBG<Q> rdbg)
{
	const SQChar* serialize_state_nut =
		reinterpret_cast<decltype(serialize_state_nut)>(
			M2Hook::GetInstance(".").ModuleResource(IDR_NUT1, "NUT")
	);
	if(!serialize_state_nut) {
		sq_throwerror(rdbg->_v, _SC("error loading the serialization function"));
	}
	if(SQ_FAILED(sq_compilebuffer(rdbg->_v,serialize_state_nut,(SQInteger)scstrlen(serialize_state_nut),_SC("SERIALIZE_STATE"),SQFalse))) {
		sq_throwerror(rdbg->_v,_SC("error compiling the serialization function"));
	}
	sq_getstackobj(rdbg->_v,-1,&rdbg->_serializefunc);
	sq_addref(rdbg->_v,&rdbg->_serializefunc);
	sq_pop(rdbg->_v,1);

	sockaddr_in cliaddr;
	socklen_t addrlen=sizeof(cliaddr);
	if(listen(rdbg->_accept,0)==SOCKET_ERROR)
		return sq_throwerror(rdbg->_v,_SC("error on listen(socket)"));
	rdbg->_endpoint = accept(rdbg->_accept,(sockaddr*)&cliaddr,&addrlen);
	//do not accept any other connection
	sqdbg_closesocket(rdbg->_accept);
	rdbg->_accept = INVALID_SOCKET;
	if(rdbg->_endpoint==INVALID_SOCKET){
		return sq_throwerror(rdbg->_v,_SC("error accept(socket)"));
	}
	while(!rdbg->_ready){
		sq_rdbg_update(rdbg);
	}
	return SQ_OK;
}

template SQRESULT sq_rdbg_waitforconnections<Squirk::Standard>(HSQREMOTEDBG<Squirk::Standard> rdbg);
template SQRESULT sq_rdbg_waitforconnections<Squirk::AlignObject>(HSQREMOTEDBG<Squirk::AlignObject> rdbg);
template SQRESULT sq_rdbg_waitforconnections<Squirk::StandardShared>(HSQREMOTEDBG<Squirk::StandardShared> rdbg);
template SQRESULT sq_rdbg_waitforconnections<Squirk::AlignObjectShared>(HSQREMOTEDBG<Squirk::AlignObjectShared> rdbg);

template <Squirk Q>
SQRESULT sq_rdbg_update(HSQREMOTEDBG<Q> rdbg)
{
	TIMEVAL time;
	time.tv_sec=0;
	time.tv_usec=0;
	fd_set read_flags;
    FD_ZERO(&read_flags);
	FD_SET(rdbg->_endpoint, &read_flags);
	select(NULL/*ignored*/, &read_flags, NULL, NULL, &time);

	if(FD_ISSET(rdbg->_endpoint,&read_flags)){
		char temp[1024];
		int size=0;
		char c,prev=NULL;
		memset(&temp,0,sizeof(temp));
		int res;
		FD_CLR(rdbg->_endpoint, &read_flags);
		while((res = recv(rdbg->_endpoint,&c,1,0))>0){
			
			if(c=='\n')break;
			if(c!='\r'){
				temp[size]=c;
				prev=c;
				size++;
			}
			if(size >= sizeof(temp)-2) break;
		}
		switch(res){

		case 0:
			return sq_throwerror(rdbg->_v,_SC("disconnected"));
		case SOCKET_ERROR:
			return sq_throwerror(rdbg->_v,_SC("socket error"));
        }
		
		temp[size]=NULL;
		temp[size+1]=NULL;
		rdbg->ParseMsg(temp);
	}
	return SQ_OK;
}

template SQRESULT sq_rdbg_update<Squirk::Standard>(HSQREMOTEDBG<Squirk::Standard> rdbg);
template SQRESULT sq_rdbg_update<Squirk::AlignObject>(HSQREMOTEDBG<Squirk::AlignObject> rdbg);
template SQRESULT sq_rdbg_update<Squirk::StandardShared>(HSQREMOTEDBG<Squirk::StandardShared> rdbg);
template SQRESULT sq_rdbg_update<Squirk::AlignObjectShared>(HSQREMOTEDBG<Squirk::AlignObjectShared> rdbg);

template <Squirk Q>
SQInteger debug_hook(HSQUIRRELVM<Q> v, HSQUIRRELVM<Q> _v, HSQREMOTEDBG<Q> rdbg)
{
	if (v) _v = v;

	SQUserPointer up;
	SQInteger event_type,line;
	const SQChar *src,*func;
	sq_getinteger(_v,2,&event_type);
	sq_getstring(_v,3,&src);
	sq_getinteger(_v,4,&line);
	sq_getstring(_v,5,&func);
	if (v) {
		sq_getuserpointer(_v, -1, &up);
		rdbg = (HSQREMOTEDBG<Q>)up;
	}
	rdbg->Hook(_v,event_type,line,src,func);
	if(rdbg->_autoupdate) {
		if(SQ_FAILED(sq_rdbg_update(rdbg)))
			return sq_throwerror(_v,_SC("socket failed"));
	}
	return 0;
}

template SQInteger debug_hook<Squirk::Standard>(HSQUIRRELVM<Squirk::Standard> v, HSQUIRRELVM<Squirk::Standard> _v, HSQREMOTEDBG<Squirk::Standard> rdbg);
template SQInteger debug_hook<Squirk::AlignObject>(HSQUIRRELVM<Squirk::AlignObject> v, HSQUIRRELVM<Squirk::AlignObject> _v, HSQREMOTEDBG<Squirk::AlignObject> rdbg);
template SQInteger debug_hook<Squirk::StandardShared>(HSQUIRRELVM<Squirk::StandardShared> v, HSQUIRRELVM<Squirk::StandardShared> _v, HSQREMOTEDBG<Squirk::StandardShared> rdbg);
template SQInteger debug_hook<Squirk::AlignObjectShared>(HSQUIRRELVM<Squirk::AlignObjectShared> v, HSQUIRRELVM<Squirk::AlignObjectShared> _v, HSQREMOTEDBG<Squirk::AlignObjectShared> rdbg);

template <Squirk Q>
SQInteger error_handler(HSQUIRRELVM<Q> v)
{
	SQUserPointer up;
	const SQChar *sErr=NULL;
	const SQChar *fn=_SC("unknown");
	const SQChar *src=_SC("unknown");
	SQInteger line=-1;
	SQStackInfos si;
	sq_getuserpointer(v,-1,&up);
	HSQREMOTEDBG<Q> rdbg=(HSQREMOTEDBG<Q>)up;
	if(SQ_SUCCEEDED(sq_stackinfos(v,1,&si)))
	{
		if(si.funcname)fn=si.funcname;
		if(si.source)src=si.source;
		line=si.line;
		scprintf(_SC("*FUNCTION [%s] %s line [%d]\n"),fn,src,si.line);
	}
	if(sq_gettop(v)>=1){
		if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))	{
			scprintf(_SC("\nAN ERROR HAS OCCURED [%s]\n"),sErr);
			rdbg->Break(v,si.line,src,_SC("error"),sErr);
		}
		else{
			scprintf(_SC("\nAN ERROR HAS OCCURED [unknown]\n"));
			rdbg->Break(v,si.line,src,_SC("error"),_SC("unknown"));
		}
	}
	rdbg->BreakExecution();
	return 0;
}

template SQInteger error_handler<Squirk::Standard>(HSQUIRRELVM<Squirk::Standard> rdbg);
template SQInteger error_handler<Squirk::AlignObject>(HSQUIRRELVM<Squirk::AlignObject> rdbg);
template SQInteger error_handler<Squirk::StandardShared>(HSQUIRRELVM<Squirk::StandardShared> rdbg);
template SQInteger error_handler<Squirk::AlignObjectShared>(HSQUIRRELVM<Squirk::AlignObjectShared> rdbg);

template <Squirk Q>
SQRESULT sq_rdbg_shutdown(HSQREMOTEDBG<Q> rdbg)
{
	delete rdbg;
#ifdef _WIN32
	WSACleanup();
#endif
	return SQ_OK;
}

template SQRESULT sq_rdbg_shutdown<Squirk::Standard>(HSQREMOTEDBG<Squirk::Standard> rdbg);
template SQRESULT sq_rdbg_shutdown<Squirk::AlignObject>(HSQREMOTEDBG<Squirk::AlignObject> rdbg);
template SQRESULT sq_rdbg_shutdown<Squirk::StandardShared>(HSQREMOTEDBG<Squirk::StandardShared> rdbg);
template SQRESULT sq_rdbg_shutdown<Squirk::AlignObjectShared>(HSQREMOTEDBG<Squirk::AlignObjectShared> rdbg);
