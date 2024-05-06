#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#  define Sleep sleep
#  include <sys/types.h>
#  include <sys/socket.h>
#endif

#include "sqpcheader.h"
#include <sqstdblob.h>
#include "sqcompiler.h"
#include "sqvm.h"
#include "sqarray.h"
#include "sqtable.h"
#include "sqclass.h"
#include "sqclosure.h"
#include "sqfuncproto.h"
#include "squserdata.h"
#include "sqstring.h"
#include "sqrdbg.h"
#include "sqdbgserver.h"

#ifndef SQUNICODE
#define scstrcpy strcpy
#else
#define scstrcpy wcscpy
#endif
struct XMLEscape{
	const SQChar c;
	const SQChar *esc;
};

#define SQDBG_DEBUG_HOOK _SC("_sqdbg_debug_hook_")
#define SQDBG_ERROR_HANDLER _SC("_sqdbg_error_handler_")

XMLEscape g_escapes[]={
	{_SC('<'),_SC("&lt;")},{'>',_SC("&gt;")},{_SC('&'),_SC("&amp;")},{_SC('\''),_SC("&apos;")},{_SC('\"'),_SC("&quot;")},{_SC('\n'),_SC("&quot;n")},{_SC('\r'),_SC("&quot;r")},{NULL,NULL}
};

const SQChar *IntToString(SQInteger n)
{
	static SQChar temp[256];
	scsprintf(temp,_SC("%d"),n);
	return temp;
}

const SQChar *PtrToString(void *p)
{
	static SQChar temp[256];
	scsprintf(temp,_SC("%p"),p);
	return temp;
}

const SQChar *FloatToString(SQFloat n)
{
	static SQChar temp[256];
	scsprintf(temp, _SC("%f"), n);
	return temp;
}

template <Squirk T>
SQInteger debug_hook(HSQUIRRELVM<T> v, HSQUIRRELVM<T> _v, HSQREMOTEDBG<T> rdbg);
template <Squirk T>
SQInteger error_handler(HSQUIRRELVM<T> v);

template <Squirk T>
SQInteger beginelement(HSQUIRRELVM<T> v)
{
	SQUserPointer up;
	const SQChar *name;
	sq_getuserpointer(v,-1,&up);
	SQDbgServer<T> *self = (SQDbgServer<T>*)up;
	sq_getuserpointer(v,-1,&up);
	sq_getstring(v,2,&name);
	self->BeginElement(name);
	return 0;
}

template <Squirk T>
SQInteger endelement(HSQUIRRELVM<T> v)
{
	SQUserPointer up;
	const SQChar *name;
	sq_getuserpointer(v,-1,&up);
	SQDbgServer<T> *self = (SQDbgServer<T>*)up;
	sq_getuserpointer(v,-1,&up);
	sq_getstring(v,2,&name);
	self->EndElement(name);
	return 0;
}

template <Squirk T>
SQInteger attribute(HSQUIRRELVM<T> v)
{
	SQUserPointer up;
	const SQChar *name,*value;
	sq_getuserpointer(v,-1,&up);
	SQDbgServer<T> *self = (SQDbgServer<T>*)up;
	sq_getuserpointer(v,-1,&up);
	sq_getstring(v,2,&name);
	sq_getstring(v,3,&value);
	self->Attribute(name,value);
	return 0;
}

template <Squirk T>
SQDbgServer<T>::SQDbgServer(HSQUIRRELVM<T> v)
{
	_ready = false;
	//_nestedcalls = 0;
	_autoupdate = false;
	_v = v;
	_state = eDBG_Running;
	_accept = INVALID_SOCKET;
	_endpoint = INVALID_SOCKET;
	//_maxrecursion = 10;
	sq_resetobject(&_debugroot);
}

template <Squirk T>
SQDbgServer<T>::~SQDbgServer()
{
	auto itr = _vmstate.begin();
	while(itr != _vmstate.end()) {
		VMState *vs = itr->second;
		delete vs;
		++itr;
	}
	_vmstate.clear();
	sq_pushobject(_v,_debugroot);
	sq_clear(_v,-1);
	sq_release(_v,&_debugroot);
	if(_accept != INVALID_SOCKET)
		sqdbg_closesocket(_accept);
	if(_endpoint != INVALID_SOCKET)
		sqdbg_closesocket(_endpoint);
}

template <Squirk T>
bool SQDbgServer<T>::Init(HSQUIRRELVM<T> v)
{
	//creates  an environment table for the debugger
	
	sq_newtable(v);
	sq_getstackobj(v,-1,&_debugroot);
	sq_addref(v,&_debugroot);

	//creates a emptyslot to store the watches
	sq_pushstring(v,_SC("watches"),-1);
	sq_pushnull(v);
	sq_newslot(v,-3, SQFalse);

	sq_pushstring(v,_SC("beginelement"),-1);
	sq_pushuserpointer(v,this);
	sq_newclosure(v,beginelement,1);
	sq_setparamscheck(v,2,_SC(".s"));
	sq_newslot(v,-3, SQFalse);

	sq_pushstring(v,_SC("endelement"),-1);
	sq_pushuserpointer(v,this);
	sq_newclosure(v,endelement,1);
	sq_setparamscheck(v,2,_SC(".s"));
	sq_newslot(v,-3, SQFalse);

	sq_pushstring(v,_SC("attribute"),-1);
	sq_pushuserpointer(v,this);
	sq_newclosure(v,attribute,1);
	sq_setparamscheck(v,3,_SC(".ss"));
	sq_newslot(v,-3, SQFalse);

	sq_pop(v,1);

	//stores debug hook and error handler in the registry
	sq_pushregistrytable(v);

	sq_pushstring(v,SQDBG_DEBUG_HOOK,-1);
	sq_pushuserpointer(v,this);
	sq_newclosure(v,(SQFUNCTION<T>)debug_hook<T>,1);
	sq_newslot(v,-3, SQFalse);
	
	sq_pushstring(v,SQDBG_ERROR_HANDLER,-1);
	sq_pushuserpointer(v,this);
	sq_newclosure(v,error_handler,1);
	sq_newslot(v,-3, SQFalse);

	
	sq_pop(v,1);

	//sets the error handlers
	if (_exclusive) SetErrorHandlers(v);
	return true;
}

template <Squirk T>
bool SQDbgServer<T>::ReadMsg()
{
	return false;
}

template <Squirk T>
void SQDbgServer<T>::BusyWait()
{
	while( !ReadMsg() )
		Sleep(0);
}


template <Squirk T>
void SQDbgServer<T>::SendChunk(const SQChar *chunk)
{
	char *buf=NULL;
	int buf_len=0;
#ifdef SQUNICODE
	buf_len=(int)scstrlen(chunk)+1;
	buf=(char *)sq_getscratchpad(_v,(buf_len)*3);
	//wcstombs((char *)buf,chunk,buf_len*3);
	WideCharToMultiByte(CP_UTF8,0,chunk,-1,buf,buf_len*3,NULL,NULL);
#else
	buf_len=(int)scstrlen(chunk);
	buf=(char *)chunk;
#endif
	send(_endpoint,(const char*)buf,(int)strlen((const char *)buf),0);
}

template <Squirk T>
void SQDbgServer<T>::Terminated()
{
	BeginElement(_SC("terminated"));
	EndElement(_SC("terminated"));
	::Sleep(200);
}

template <Squirk T>
VMState *SQDbgServer<T>::GetVMState(HSQUIRRELVM<T> v)
{
	VMState *ret = NULL;
	auto itr = _vmstate.find(v);
	if(itr == _vmstate.end()) {
		ret = new VMState();
		_vmstate.insert(VMStateMap<T>::value_type(v,ret));
	}
	else {
		ret = itr->second;
	}
	return ret;
}

template <Squirk T>
void SQDbgServer<T>::Hook(HSQUIRRELVM<T> v,SQInteger type,SQInteger line,const SQChar *src,const SQChar *func)
{
	_v = v;
	VMState *vs = GetVMState(v);
	switch(_state){
	case eDBG_Running:
		if(type==_SC('l') && _breakpoints.size()) {
			BreakPointSetItor itr = _breakpoints.find(BreakPoint(line,src));
			if(itr != _breakpoints.end()) {
				Break(v,line,src,_SC("breakpoint"));
				BreakExecution();
			}
		}
		break;
	case eDBG_Suspended:
		vs->_nestedcalls=0;
	case eDBG_StepOver:
		switch(type){
		case _SC('l'):
			if(vs->_nestedcalls==0) {
				Break(v,line,src,_SC("step"));
				BreakExecution();
			}
			break;
		case _SC('c'):
			vs->_nestedcalls++;
			break;
		case _SC('r'):
			if(vs->_nestedcalls==0){
				vs->_nestedcalls=0;
				
			}else{
				vs->_nestedcalls--;
			}
			break;
		}
		break;
	case eDBG_StepInto:
		switch(type){
		case _SC('l'):
			vs->_nestedcalls=0;
			Break(v,line,src,_SC("step"));
			BreakExecution();
			break;
		
		}
		break;
	case eDBG_StepReturn:
		switch(type){
		case _SC('l'):
			break;
		case _SC('c'):
			vs->_nestedcalls++;
			break;
		case _SC('r'):
			if(vs->_nestedcalls==0){
				vs->_nestedcalls=0;
				_state=eDBG_StepOver;
			}else{
				vs->_nestedcalls--;
			}
			
			break;
		}
		break;
	case eDBG_Disabled:
		break;
	}
}


#define MSG_ID(x,y) ((y<<8)|x)
//ab Add Breakpoint
//rb Remove Breakpoint
//sp Suspend
template <Squirk T>
void SQDbgServer<T>::ParseMsg(const char *msg)
{
	
	switch(*((unsigned short *)msg)){
		case MSG_ID('a','b'): {
			BreakPoint bp;
			if(ParseBreakpoint(msg+3,bp)){
				AddBreakpoint(bp);
				scprintf(_SC("added bp %d %s\n"),bp._line,bp._src.c_str());
			}
			else
				scprintf(_SC("error parsing add breakpoint"));
							 }
			break;
		case MSG_ID('r','b'): {
			BreakPoint bp;
			if(ParseBreakpoint(msg+3,bp)){
				RemoveBreakpoint(bp);
				scprintf(_SC("removed bp %d %s\n"),bp._line,bp._src.c_str());
			}else
				scprintf(_SC("error parsing remove breakpoint"));
							}
			break;
		case MSG_ID('g','o'):
			if(_state!=eDBG_Running){
				_state = eDBG_Running;
				BeginDocument();
				BeginElement(_SC("resumed"));
				EndElement(_SC("resumed"));
				EndDocument();
				//				Send(_SC("<resumed/>\r\n"));
				scprintf(_SC("go (execution resumed)\n"));
			}
			break;
		case MSG_ID('s', 'p'):
			if (_state != eDBG_Suspended) {
				_state = eDBG_Suspended;
				scprintf(_SC("suspend\n"));
			}
			break;
		case MSG_ID('s', 'o'):
			if (_state == eDBG_Suspended) {
				_state = eDBG_StepOver;
			}
			break;
		case MSG_ID('s', 'i'):
			if (_state == eDBG_Suspended) {
				_state = eDBG_StepInto;
				scprintf(_SC("step into\n"));
			}
			break;
		case MSG_ID('s', 'r'):
			if (_state == eDBG_Suspended) {
				_state = eDBG_StepReturn;
				scprintf(_SC("step return\n"));
			}
			break;
		case MSG_ID('d', 'i'):
			if (_state != eDBG_Disabled) {
				_state = eDBG_Disabled;
				scprintf(_SC("disabled\n"));
			}
			break;
		case MSG_ID('a', 'w'): {
			Watch w;
			if (ParseWatch(msg + 3, w))
			{
				AddWatch(w);
				scprintf(_SC("added watch %d %s\n"), w._id, w._exp.c_str());
				/*if(_state == eDBG_Suspended) {
					Break(_line,_src.c_str(),_break_type.c_str());
				}*/
			}
			else
				scprintf(_SC("error parsing add watch"));
		}
							 break;
		case MSG_ID('r', 'w'): {
			SQInteger id;
			if (ParseRemoveWatch(msg + 3, id))
			{
				RemoveWatch(id);
				scprintf(_SC("added watch %d\n"), id);
			}
			else
				scprintf(_SC("error parsing remove watch"));
		}
							 break;
		case MSG_ID('t', 'r'):
			scprintf(_SC("terminate from user\n"));
			break;
		case MSG_ID('r', 'd'):
			scprintf(_SC("ready\n"));
			_ready = true;
			break;
		case MSG_ID('e', 'v'):
			BeginDocument();
			if (SQ_FAILED(sq_compilebuffer(_v, msg + 3, scstrlen(msg + 3), _SC("REMOTE"), SQFalse)))
				scprintf(_SC("error compiling the remote function"));
			else
			{
				HSQOBJECT<T> func;
				sq_getstackobj(_v, -1, &func);
				sq_addref(_v, &func);
				sq_pop(_v, 1);

				sq_pushobject(_v, func);
				sq_pushroottable(_v);

				BeginElement(_SC("eval"));

				if (SQ_FAILED(sq_call(_v, 1, SQTrue, SQFalse)))
					scprintf(_SC("error calling the remote function"));
				else
				{
					switch (sq_gettype(_v, -1)) {
						case OT_NULL:
							Attribute(_SC("type"), "OT_NULL");
							break;
						case OT_STRING:
						{
							Attribute(_SC("type"), "OT_STRING");
							const SQChar *value;
							sq_getstring(_v, -1, &value);
							Attribute(_SC("value"), value);
						}
						break;
						case OT_FLOAT:
						{
							Attribute(_SC("type"), "OT_FLOAT");
							SQFloat value;
							sq_getfloat(_v, -1, &value);
							Attribute(_SC("value"), FloatToString(value));
						}
						break;
						case OT_INTEGER:
						{
							Attribute(_SC("type"), "OT_INTEGER");
							SQInteger value;
							sq_getinteger(_v, -1, &value);
							Attribute(_SC("value"), IntToString(value));
						}
						break;
						case OT_BOOL:
						{
							Attribute(_SC("type"), "OT_BOOL");
							SQInteger value;
							sq_getinteger(_v, -1, &value);
							Attribute(_SC("value"), value ? "true" : "false");
						}
						break;
						case OT_CLOSURE:
						{
							Attribute(_SC("type"), "OT_CLOSURE");
							HSQOBJECT<T> obj;
							sq_getstackobj(_v, -1, &obj);
							SQClosure<T> *closure = _closure(obj);
							if (closure) {
								SQFunctionProto<T> *proto = _funcproto(closure->_function);
								if (proto && sq_isstring(proto->_name)) {
									Attribute(_SC("name"), _stringval(proto->_name));
								}
							}
						}
						break;
						case OT_NATIVECLOSURE:
						{
							Attribute(_SC("type"), "OT_NATIVECLOSURE");
							HSQOBJECT<T> obj;
							sq_getstackobj(_v, -1, &obj);
							SQNativeClosure<T> *closure = _nativeclosure(obj);
							if (sq_isstring(closure->_name)) {
								Attribute(_SC("name"), _stringval(closure->_name));
							}
							Attribute(_SC("value"), PtrToString(closure->_function));
						}
						break;
						default:
						{
							scprintf(_SC("eval\n"));
							switch (sq_gettype(_v, -1)) {
								case OT_INSTANCE:
									Attribute(_SC("type"), "OT_INSTANCE");
									break;
								case OT_CLASS:
									Attribute(_SC("type"), "OT_CLASS");
									break;
								case OT_TABLE:
									Attribute(_SC("type"), "OT_TABLE");
									break;
								case OT_ARRAY:
									Attribute(_SC("type"), "OT_ARRAY");
									break;
								case OT_USERDATA:
									Attribute(_SC("type"), "OT_USERDATA");
									break;
								case OT_USERPOINTER:
									Attribute(_SC("type"), "OT_USERPOINTER");
									break;
								case OT_GENERATOR:
									Attribute(_SC("type"), "OT_GENERATOR");
									break;
								case OT_THREAD:
									Attribute(_SC("type"), "OT_THREAD");
									break;
								case OT_FUNCPROTO:
									Attribute(_SC("type"), "OT_FUNCPROTO");
									break;
								case OT_WEAKREF:
									Attribute(_SC("type"), "OT_WEAKREF");
									break;
							}
							HSQOBJECT<T> obj;
							sq_getstackobj(_v, -1, &obj);
							Attribute(_SC("value"), PtrToString((void*) obj._unVal.raw));
						}
						break;
					}
				}
				EndElement(_SC("eval"));

				sq_pop(_v, 2);
				sq_release(_v, &func);
			}
			EndDocument();
			break;
		default:
			scprintf(_SC("unknown packet"));

	}
}

template <Squirk T>
bool SQDbgServer<T>::ParseBreakpoint(const char *msg,BreakPoint &out)
{
	static char stemp[MAX_BP_PATH];
	static SQChar desttemp[MAX_BP_PATH];
	char *ep=NULL;
	out._line=strtoul(msg,&ep,16);
	if(ep==msg || (*ep)!=':')return false;
	
	char *dest=stemp;
	ep++;
	while((*ep)!='\n' && (*ep)!='\0')
	{
		*dest=tolower(*ep);
		*dest++;*ep++;
	}
	*dest='\0';
	*dest++;
	*dest='\0';
#ifdef SQUNICODE
	int len=(int)strlen(stemp);
	SQChar *p = desttemp;
	size_t destlen = mbstowcs(p,stemp,len);
	p[destlen]=_SC('\0');
	out._src=p;
#else
	out._src=stemp;
#endif
	return true;
}

template <Squirk T>
bool SQDbgServer<T>::ParseWatch(const char *msg,Watch &out)
{
	char *ep=NULL;
	out._id=strtoul(msg,&ep,16);
	if(ep==msg || (*ep)!=':')return false;

	//char *dest=out._src;
	ep++;
	while((*ep)!='\n' && (*ep)!='\0')
	{
		out._exp.append(1,*ep);
		*ep++;
	}
	return true;
}

template <Squirk T>
bool SQDbgServer<T>::ParseRemoveWatch(const char *msg,SQInteger &id)
{
	char *ep=NULL;
	id=strtoul(msg,&ep,16);
	if(ep==msg)return false;
	return true;
}

template <Squirk T>
void SQDbgServer<T>::BreakExecution()
{
	_state=eDBG_Suspended;
	while(_state==eDBG_Suspended){
		if(SQ_FAILED(sq_rdbg_update(this)))
			exit(0);
		Sleep(10);
	}
}

//COMMANDS
template <Squirk T>
void SQDbgServer<T>::AddBreakpoint(BreakPoint &bp)
{
	_breakpoints.insert(bp);
	BeginDocument();
		BeginElement(_SC("addbreakpoint"));
			Attribute(_SC("line"),IntToString(bp._line));
			Attribute(_SC("src"),bp._src.c_str());
		EndElement(_SC("addbreakpoint"));
	EndDocument();
}

template <Squirk T>
void SQDbgServer<T>::AddWatch(Watch &w)
{
	_watches.insert(w);
}

template <Squirk T>
void SQDbgServer<T>::RemoveWatch(SQInteger id)
{
	WatchSetItor itor=_watches.find(Watch(id,_SC("")));
	if(itor==_watches.end()){
		BeginDocument();
		BeginElement(_SC("error"));
			Attribute(_SC("desc"),_SC("the watch does not exists"));
		EndElement(_SC("error"));
	EndDocument();
	}
	else{
		_watches.erase(itor);
		scprintf(_SC("removed watch %d\n"),id);
	}
}

template <Squirk T>
void SQDbgServer<T>::RemoveBreakpoint(BreakPoint &bp)
{
	BreakPointSetItor itor=_breakpoints.find(bp);
	if(itor==_breakpoints.end()){
		BeginDocument();
			BeginElement(_SC("break"));
				Attribute(_SC("desc"),_SC("the breakpoint doesn't exists"));
			EndElement(_SC("break"));
		EndDocument();
	}
	else{
		BeginDocument();
			BeginElement(_SC("removebreakpoint"));
				Attribute(_SC("line"),IntToString(bp._line));
				Attribute(_SC("src"),bp._src.c_str());
			EndElement(_SC("removebreakpoint"));
		EndDocument();
		_breakpoints.erase(itor);
	}
}

template <Squirk T>
void SQDbgServer<T>::Break(HSQUIRRELVM<T> v,SQInteger line,const SQChar *src,const SQChar *type,const SQChar *error)
{
	_line = line;
	_src = src;
	_break_type = src;
	if(!error){
		BeginDocument();
			BeginElement(_SC("break"));
				Attribute(_SC("thread"),PtrToString(v));
				Attribute(_SC("line"),IntToString(line));
				Attribute(_SC("src"),src);
				Attribute(_SC("type"),type);
				SerializeState(v);
			EndElement(_SC("break"));
		EndDocument();
	}else{
		BeginDocument();
			BeginElement(_SC("break"));
				Attribute(_SC("thread"),PtrToString(v));
				Attribute(_SC("line"),IntToString(line));
				Attribute(_SC("src"),src);
				Attribute(_SC("type"),type);
				Attribute(_SC("error"),error);
				SerializeState(v);
			EndElement(_SC("break"));
		EndDocument();
	}
}

template <Squirk T>
void SQDbgServer<T>::SerializeState(HSQUIRRELVM<T> v)
{
	if (_exclusive) {
		sq_pushnull(v);
		sq_setdebughook(v);
		sq_pushnull(v);
		sq_seterrorhandler(v);
	}
	sq_pushobject(v,_serializefunc);
	sq_pushobject(v,_debugroot);
	sq_pushstring(v,_SC("watches"),-1);
	sq_newtable(v);
	for(WatchSetItor i=_watches.begin(); i!=_watches.end(); ++i)
	{
		sq_pushinteger(v,i->_id);
		sq_pushstring(v,i->_exp.c_str(),(SQInteger)i->_exp.length());
		sq_createslot(v,-3);
	}
	sq_rawset(v,-3);
	if(SQ_SUCCEEDED(sq_call(v,1,SQTrue,SQFalse))){
		//if(SQ_SUCCEEDED(sqstd_getblob(v,-1,(SQUserPointer*)&sz)))
			//SendChunk(sz);
	}
	sq_pop(v,2);
	
	if (_exclusive) SetErrorHandlers(v);
}

template <Squirk T>
void SQDbgServer<T>::SetErrorHandlers(HSQUIRRELVM<T> v)
{
	sq_pushregistrytable(v);
	sq_pushstring(v,SQDBG_DEBUG_HOOK,-1);
	sq_rawget(v,-2);
	sq_setdebughook(v);
	sq_pushstring(v,SQDBG_ERROR_HANDLER,-1);
	sq_rawget(v,-2);
	sq_seterrorhandler(v);
	sq_pop(v,1);
}

template <Squirk T>
void SQDbgServer<T>::BeginDocument()
{ 
	_xmlcurrentement = -1; 
	SendChunk(_SC("<?xml version='1.0' encoding='utf-8'?>"));
}

template <Squirk T>
void SQDbgServer<T>::BeginElement(const SQChar *name)
{
	_xmlcurrentement++;
	XMLElementState *self = &xmlstate[_xmlcurrentement];
	scstrcpy(self->name,name);
	self->haschildren = false;
	if(_xmlcurrentement > 0) {
		XMLElementState *parent = &xmlstate[_xmlcurrentement-1];
		if(!parent->haschildren) {
			SendChunk(_SC(">")); // closes the parent tag
			parent->haschildren = true;
		}
	}
	_scratchstring.resize(2+scstrlen(name));
	scsprintf(&_scratchstring[0],_SC("<%s"),name);
	SendChunk(&_scratchstring[0]);
}

template <Squirk T>
void SQDbgServer<T>::Attribute(const SQChar *name,const SQChar *value)
{
	XMLElementState *self = &xmlstate[_xmlcurrentement];
	assert(!self->haschildren); //cannot have attributes if already has children
	const SQChar *escval = escape_xml(value);
	_scratchstring.resize(10+scstrlen(name)+scstrlen(escval));
	scsprintf(&_scratchstring[0],_SC(" %s=\"%s\""),name,escval);
	SendChunk(&_scratchstring[0]);
}

template <Squirk T>
void SQDbgServer<T>::EndElement(const SQChar *name)
{
	XMLElementState *self = &xmlstate[_xmlcurrentement];
	assert(scstrcmp(self->name,name) == 0);
	if(self->haschildren) {
		_scratchstring.resize(10+scstrlen(name));
		scsprintf(&_scratchstring[0],_SC("</%s>"),name);
		SendChunk(&_scratchstring[0]);
		
	}
	else {
		SendChunk(_SC("/>"));
	}
	_xmlcurrentement--;
}

template <Squirk T>
void SQDbgServer<T>::EndDocument()
{
	SendChunk(_SC("\r\n"));
}

//this can be done much better/faster(do we need that?)
template <Squirk T>
const SQChar *SQDbgServer<T>::escape_xml(const SQChar *s)
{
	SQChar *temp=sq_getscratchpad(_v,((SQInteger)scstrlen(s)*6) + sizeof(SQChar));
	SQChar *dest=temp;
	while(*s!=_SC('\0')){
		
		const SQChar *escape = NULL;
		switch(*s) {
			case _SC('<'): escape = _SC("&lt;"); break;
			case _SC('>'): escape = _SC("&gt;"); break;
			case _SC('&'): escape = _SC("&amp;"); break;
			case _SC('\''): escape = _SC("&apos;"); break;
			case _SC('\"'): escape = _SC("&quot;"); break;
			case _SC('\n'): escape = _SC("\\n"); break;
			case _SC('\r'): escape = _SC("\\r"); break;
		}
		if(escape) {
			scstrcpy(dest,escape);
			dest += scstrlen(escape);
		}
		else {
			*dest=*s;*dest++;
		}
		*s++;
	}
	*dest=_SC('\0');
	return temp;
	
}
