/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <stdarg.h>
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"

template <Squirk T>
SQRESULT sq_getfunctioninfo(HSQUIRRELVM<T> v,SQInteger level,SQFunctionInfo *fi)
{
	SQInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		auto &ci = v->_callsstack[cssize-level-1];
		if(sq_isclosure(ci._closure)) {
			SQClosure<T> *c = _closure(ci._closure);
			SQFunctionProto<T> *proto = _funcproto(c->_function);
			fi->funcid = proto;
			fi->name = type(proto->_name) == OT_STRING?_stringval(proto->_name):_SC("unknown");
			fi->source = type(proto->_name) == OT_STRING?_stringval(proto->_sourcename):_SC("unknown");
			return SQ_OK;
		}
	}
	return sq_throwerror(v,_SC("the object is not a closure"));
}

template SQRESULT sq_getfunctioninfo<Squirk::Standard>(HSQUIRRELVM<Squirk::Standard> v, SQInteger idx, SQFunctionInfo *fi);
template SQRESULT sq_getfunctioninfo<Squirk::AlignObject>(HSQUIRRELVM<Squirk::AlignObject> v, SQInteger idx, SQFunctionInfo *fi);

template <Squirk T>
SQRESULT sq_stackinfos(HSQUIRRELVM<T> v, SQInteger level, SQStackInfos *si)
{
	SQInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		memset(si, 0, sizeof(SQStackInfos));
		auto &ci = v->_callsstack[cssize-level-1];
		switch (type(ci._closure)) {
		case OT_CLOSURE:{
			SQFunctionProto<T> *func = _funcproto(_closure(ci._closure)->_function);
			if (type(func->_name) == OT_STRING)
				si->funcname = _stringval(func->_name);
			if (type(func->_sourcename) == OT_STRING)
				si->source = _stringval(func->_sourcename);
			si->line = func->GetLine(ci._ip);
						}
			break;
		case OT_NATIVECLOSURE:
			si->source = _SC("NATIVE");
			si->funcname = _SC("unknown");
			if(type(_nativeclosure(ci._closure)->_name) == OT_STRING)
				si->funcname = _stringval(_nativeclosure(ci._closure)->_name);
			si->line = -1;
			break;
		default: break; //shutup compiler
		}
		return SQ_OK;
	}
	return SQ_ERROR;
}

template SQRESULT sq_stackinfos<Squirk::Standard>(HSQUIRRELVM<Squirk::Standard> v, SQInteger level, SQStackInfos *si);
template SQRESULT sq_stackinfos<Squirk::AlignObject>(HSQUIRRELVM<Squirk::AlignObject> v, SQInteger level, SQStackInfos *si);

template <Squirk T>
void SQVM<T>::Raise_Error(const SQChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	scvsprintf(_sp(rsl((SQInteger)scstrlen(s)+(NUMBER_MAX_CHAR*2))), s, vl);
	va_end(vl);
	_lasterror = SQString<T>::Create(_ss(this),_spval,-1);
}

template <Squirk T>
void SQVM<T>::Raise_Error(SQObjectPtr<T> &desc)
{
	_lasterror = desc;
}

template <Squirk T>
SQString<T> *SQVM<T>::PrintObjVal(const SQObject<T> &o)
{
	switch(type(o)) {
	case OT_STRING: return _string(o);
	case OT_INTEGER:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)), _SC("%d"), _integer(o));
		return SQString<T>::Create(_ss(this), _spval);
		break;
	case OT_FLOAT:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)), _SC("%.14g"), _float(o));
		return SQString<T>::Create(_ss(this), _spval);
		break;
	default:
		return SQString<T>::Create(_ss(this), GetTypeName<T>(o));
	}
}

template <Squirk T>
void SQVM<T>::Raise_IdxError(SQObject<T> &o)
{
	SQObjectPtr oval = PrintObjVal(o);
	Raise_Error(_SC("the index '%.50s' does not exist"), _stringval(oval));
}

template <Squirk T>
void SQVM<T>::Raise_CompareError(const SQObject<T> &o1, const SQObject<T> &o2)
{
	SQObjectPtr oval1 = PrintObjVal(o1), oval2 = PrintObjVal(o2);
	Raise_Error(_SC("comparsion between '%.50s' and '%.50s'"), _stringval(oval1), _stringval(oval2));
}

template <Squirk T>
void SQVM<T>::Raise_ParamTypeError(SQInteger nparam,SQInteger typemask,SQInteger type)
{
	SQObjectPtr<T> exptypes = SQString<T>::Create(_ss(this), _SC(""), -1);
	SQInteger found = 0;	
	for(SQInteger i=0; i<16; i++)
	{
		SQInteger mask = 0x00000001 << i;
		if(typemask & (mask)) {
			if(found>0) StringCat(exptypes,SQString<T>::Create(_ss(this), _SC("|"), -1), exptypes);
			found ++;
			StringCat(exptypes,SQString<T>::Create(_ss(this), IdType2Name((SQObjectType)mask), -1), exptypes);
		}
	}
	Raise_Error(_SC("parameter %d has an invalid type '%s' ; expected: '%s'"), nparam, IdType2Name((SQObjectType)type), _stringval(exptypes));
}
