/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqclass.h"
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

template <Squirk Q>
bool str2num(const SQChar *s,SQObjectPtr<Q> &res)
{
	SQChar *end;
#ifdef _SQ_M2
	if(!scstrncmp(s,_SC("0x"),2) || !scstrncmp(s, _SC("0X"), 2)) {
		s += 2;
		SQInteger r = SQInteger(scstrtol(s,&end,16));
		if(s == end) return false;
		res = r;
		return true;
	}
	else if(!scstrncmp(s,_SC("0b"),2) || !scstrncmp(s, _SC("0B"), 2)) {
		s += 2;
		SQInteger r = SQInteger(scstrtol(s,&end,2));
		if(s == end) return false;
		res = r;
		return true;
	}
	else
#endif
	if(scstrstr(s,_SC("."))){
		SQFloat r = SQFloat(scstrtod(s,&end));
		if(s == end) return false;
		res = r;
		return true;
	}
	else{
		SQInteger r = SQInteger(scstrtol(s,&end,10));
		if(s == end) return false;
		res = r;
		return true;
	}
}

template <Squirk Q>
static SQInteger base_dummy(HSQUIRRELVM<Q> v)
{
	return 0;
}

#ifndef NO_GARBAGE_COLLECTOR
template <Squirk Q>
static SQInteger base_collectgarbage(HSQUIRRELVM<Q> v)
{
	sq_pushinteger(v, sq_collectgarbage(v));
	return 1;
}
#endif

template <Squirk Q>
static SQInteger base_getroottable(HSQUIRRELVM<Q> v)
{
	v->Push(v->_roottable);
	return 1;
}

template <Squirk Q>
static SQInteger base_getconsttable(HSQUIRRELVM<Q> v)
{
	v->Push(_ss(v)->_consts);
	return 1;
}

template <Squirk Q>
static SQInteger base_setroottable(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o=stack_get(v,2);
	if(SQ_FAILED(sq_setroottable(v))) return SQ_ERROR;
	v->Push(o);
	return 1;
}

template <Squirk Q>
static SQInteger base_setconsttable(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o=stack_get(v,2);
	if(SQ_FAILED(sq_setconsttable(v))) return SQ_ERROR;
	v->Push(o);
	return 1;
}

#ifdef _SQ_M2
template <Squirk Q>
static SQInteger base_setexceptionclass(HSQUIRRELVM<Q> v)
{
	return sq_setexceptionclass(v);
}
#endif

template <Squirk Q>
static SQInteger base_seterrorhandler(HSQUIRRELVM<Q> v)
{
	sq_seterrorhandler(v);
	return 0;
}

template <Squirk Q>
static SQInteger base_setdebughook(HSQUIRRELVM<Q> v)
{
	sq_setdebughook(v);
	return 0;
}

template <Squirk Q>
static SQInteger base_enabledebuginfo(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o=stack_get(v,2);
	sq_enabledebuginfo(v,(obj_type(o) != OT_NULL)?1:0);
	return 0;
}

template <Squirk Q>
static SQInteger base_getstackinfos(HSQUIRRELVM<Q> v)
{
	SQInteger level;
	SQStackInfos si;
	SQInteger seq = 0;
	const SQChar *name = NULL;
	sq_getinteger(v, -1, &level);
	if (SQ_SUCCEEDED(sq_stackinfos(v, level, &si)))
	{
		const SQChar *fn = _SC("unknown");
		const SQChar *src = _SC("unknown");
		if(si.funcname)fn = si.funcname;
		if(si.source)src = si.source;
		sq_newtable(v);
		sq_pushstring(v, _SC("func"), -1);
		sq_pushstring(v, fn, -1);
		sq_createslot(v, -3);
		sq_pushstring(v, _SC("src"), -1);
		sq_pushstring(v, src, -1);
		sq_createslot(v, -3);
		sq_pushstring(v, _SC("line"), -1);
		sq_pushinteger(v, si.line);
		sq_createslot(v, -3);
		sq_pushstring(v, _SC("locals"), -1);
		sq_newtable(v);
		seq=0;
		while ((name = sq_getlocal(v, level, seq))) {
			sq_pushstring(v, name, -1);
			sq_push(v, -2);
			sq_createslot(v, -4);
			sq_pop(v, 1);
			seq++;
		}
		sq_createslot(v, -3);
		return 1;
	}

	return 0;
}

template <Squirk Q>
static SQInteger base_assert(HSQUIRRELVM<Q> v)
{
	if(v->IsFalse(stack_get(v,2))){
		return sq_throwerror(v,_SC("assertion failed"));
	}
	return 0;
}

template <Squirk Q>
static SQInteger get_slice_params(HSQUIRRELVM<Q> v,SQInteger &sidx,SQInteger &eidx,SQObjectPtr<Q> &o)
{
	SQInteger top = sq_gettop(v);
	sidx=0;
	eidx=0;
	o=stack_get(v,1);
	SQObjectPtr<Q> &start=stack_get(v,2);
	if(obj_type(start)!=OT_NULL && sq_isnumeric(start)){
		sidx=tointeger(start);
	}
	if(top>2){
		SQObjectPtr<Q> &end=stack_get(v,3);
		if(sq_isnumeric(end)){
			eidx=tointeger(end);
		}
	}
	else {
		eidx = sq_getsize(v,1);
	}
	return 1;
}

template <Squirk Q>
static SQInteger base_print(HSQUIRRELVM<Q> v)
{
	const SQChar *str;
	sq_tostring(v,2);
	sq_getstring(v,-1,&str);
	if(_ss(v)->_printfunc) _ss(v)->_printfunc(v,_SC("%s"),str);
	return 0;
}

template <Squirk Q>
static SQInteger base_compilestring(HSQUIRRELVM<Q> v)
{
	SQInteger nargs=sq_gettop(v);
	const SQChar *src=NULL,*name=_SC("unnamedbuffer");
	SQInteger size;
	sq_getstring(v,2,&src);
	size=sq_getsize(v,2);
	if(nargs>2){
		sq_getstring(v,3,&name);
	}
	if(SQ_SUCCEEDED(sq_compilebuffer(v,src,size,name,SQFalse)))
		return 1;
	else
		return SQ_ERROR;
}

template <Squirk Q>
static SQInteger base_newthread(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &func = stack_get(v,2);
	SQInteger stksize = (_funcproto(_closure(func)->_function)->_stacksize << 1) +2;
	HSQUIRRELVM<Q> newv = sq_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2)? MIN_STACK_OVERHEAD + 2 : stksize);
	sq_move(newv,v,-2);
	return 1;
}

template <Squirk Q>
static SQInteger base_suspend(HSQUIRRELVM<Q> v)
{
	return sq_suspendvm(v);
}

template <Squirk Q>
static SQInteger base_array(HSQUIRRELVM<Q> v)
{
	SQArray<Q> *a;
	SQObject<Q> &size = stack_get(v,2);
	if(sq_gettop(v) > 2) {
		a = SQArray<Q>::Create(_ss(v),0);
		a->Resize(tointeger(size),stack_get(v,3));
	}
	else {
		a = SQArray<Q>::Create(_ss(v),tointeger(size));
	}
	v->Push(a);
	return 1;
}

template <Squirk Q>
static SQInteger base_type(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o = stack_get(v,2);
	v->Push(SQString<Q>::Create(_ss(v),GetTypeName(o),-1));
	return 1;
}

#ifdef _SQ_M2
template <Squirk Q>
static SQInteger base_exec(HSQUIRRELVM<Q> v)
{
	SQInteger nargs = sq_gettop(v);
	const SQChar *source = NULL;
	sq_getstring(v,2,&source);
	SQInteger contextIdx = nargs >= 3 && sq_gettype(v,3) != OT_NULL ? 3 : 0;
	SQInteger errorIdx = nargs >= 4 && sq_gettype(v,4) != OT_NULL ? 4 : 0;
	return sq_execscript(v,source,sq_getsize(v,2),contextIdx,errorIdx);
}

template <Squirk Q>
static SQInteger base_eval(HSQUIRRELVM<Q> v)
{
	SQInteger nargs = sq_gettop(v);
	sq_tostring(v,2);
	const SQChar *source = NULL;
	sq_getstring(v,-1,&source);
	SQInteger size = sq_getsize(v,-1);
	SQChar *data = sq_getscratchpad(v,(size + 8) * sizeof(SQChar));
	scstrcpy(data,_SC("return "));
	scstrcpy(data + 7,source);
	sq_settop(v,nargs);
	SQInteger contextIdx = nargs >= 3 && sq_gettype(v,3) != OT_NULL ? 3 : 0;
	SQInteger errorIdx = nargs >= 4 && sq_gettype(v,4) != OT_NULL ? 4 : 0;
	return sq_execscript(v,data,size + 7,contextIdx,errorIdx);
}

template <Squirk Q>
static SQInteger base_template(HSQUIRRELVM<Q> v)
{
	SQInteger nargs = sq_gettop(v);
	const SQChar *source = NULL;
	sq_getstring(v,2,&source);
	SQInteger contextIdx = nargs >= 3 && sq_gettype(v,3) != OT_NULL ? 3 : 0;
	SQInteger errorIdx = nargs >= 4 && sq_gettype(v,4) != OT_NULL ? 4 : 0;
	return sq_template(v,source,sq_getsize(v,2),contextIdx,errorIdx);
}
#endif

template <Squirk Q>
static SQRegFunction<Q> base_funcs[]={
	//generic
	{_SC("seterrorhandler"),base_seterrorhandler,2, NULL},
	{_SC("setdebughook"),base_setdebughook,2, NULL},
	{_SC("enabledebuginfo"),base_enabledebuginfo,2, NULL},
	{_SC("getstackinfos"),base_getstackinfos,2, _SC(".n")},
	{_SC("getroottable"),base_getroottable,1, NULL},
	{_SC("setroottable"),base_setroottable,2, NULL},
	{_SC("getconsttable"),base_getconsttable,1, NULL},
	{_SC("setconsttable"),base_setconsttable,2, NULL},
#ifdef _SQ_M2
	{_SC("setexceptionclass"),base_setexceptionclass,2, NULL},
#endif
	{_SC("assert"),base_assert,2, NULL},
	{_SC("print"),base_print,2, NULL},
	{_SC("compilestring"),base_compilestring,-2, _SC(".ss")},
	{_SC("newthread"),base_newthread,2, _SC(".c")},
	{_SC("suspend"),base_suspend,-1, NULL},
	{_SC("array"),base_array,-2, _SC(".n")},
	{_SC("type"),base_type,2, NULL},
#ifdef _SQ_M2
	{_SC("execstring"),base_exec,-2, _SC(".so|x|y|t.")},
	{_SC("evalstring"),base_eval,-2, _SC("..o|x|y|t.")},
	{_SC("template"),base_template,-2, _SC(".so|x|y|t")},
#endif
	{_SC("dummy"),base_dummy,0,NULL},
#ifndef NO_GARBAGE_COLLECTOR
	{_SC("collectgarbage"),base_collectgarbage,1, _SC("t")},
#endif
	{0,0}
};

template <Squirk Q>
void sq_base_register(HSQUIRRELVM<Q> v)
{
	SQInteger i=0;
	sq_pushroottable(v);
	while(base_funcs<Q>[i].name!=0) {
		sq_pushstring(v,base_funcs<Q>[i].name,-1);
		sq_newclosure(v,base_funcs<Q>[i].f,0);
		sq_setnativeclosurename(v,-1,base_funcs<Q>[i].name);
		sq_setparamscheck(v,base_funcs<Q>[i].nparamscheck,base_funcs<Q>[i].typemask);
		sq_createslot(v,-3);
		i++;
	}
	sq_pushstring(v,_SC("_version_"),-1);
	sq_pushstring(v,SQUIRREL_VERSION,-1);
	sq_createslot(v,-3);
	sq_pushstring(v,_SC("_charsize_"),-1);
	sq_pushinteger(v,sizeof(SQChar));
	sq_createslot(v,-3);
	sq_pushstring(v,_SC("_intsize_"),-1);
	sq_pushinteger(v,sizeof(SQInteger));
	sq_createslot(v,-3);
	sq_pushstring(v,_SC("_floatsize_"),-1);
	sq_pushinteger(v,sizeof(SQFloat));
	sq_createslot(v,-3);
	sq_pop(v,1);
}

template void sq_base_register<Squirk::Standard>(HSQUIRRELVM<Squirk::Standard> v);
template void sq_base_register<Squirk::AlignObject>(HSQUIRRELVM<Squirk::AlignObject> v);
template void sq_base_register<Squirk::StandardShared>(HSQUIRRELVM<Squirk::StandardShared> v);
template void sq_base_register<Squirk::AlignObjectShared>(HSQUIRRELVM<Squirk::AlignObjectShared> v);

template <Squirk Q>
static SQInteger default_delegate_len(HSQUIRRELVM<Q> v)
{
	v->Push(SQInteger(sq_getsize(v,1)));
	return 1;
}

template <Squirk Q>
static SQInteger default_delegate_tofloat(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o=stack_get(v,1);
	switch(obj_type(o)){
	case OT_STRING:{
		SQObjectPtr<Q> res;
		if(str2num(_stringval(o),res)){
			v->Push(SQObjectPtr<Q>(tofloat(res)));
			break;
		}}
		return sq_throwerror(v, _SC("cannot convert the string"));
		break;
	case OT_INTEGER:case OT_FLOAT:
		v->Push(SQObjectPtr<Q>(tofloat(o)));
		break;
	case OT_BOOL:
		v->Push(SQObjectPtr<Q>((SQFloat)(_integer(o)?1:0)));
		break;
	default:
		v->Push(_null_<Q>);
		break;
	}
	return 1;
}

template <Squirk Q>
static SQInteger default_delegate_tointeger(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o=stack_get(v,1);
	switch(obj_type(o)){
	case OT_STRING:{
		SQObjectPtr<Q> res;
		if(str2num(_stringval(o),res)){
			v->Push(SQObjectPtr<Q>(tointeger(res)));
			break;
		}}
		return sq_throwerror(v, _SC("cannot convert the string"));
		break;
	case OT_INTEGER:case OT_FLOAT:
		v->Push(SQObjectPtr<Q>(tointeger(o)));
		break;
	case OT_BOOL:
		v->Push(SQObjectPtr<Q>(_integer(o)?(SQInteger)1:(SQInteger)0));
		break;
	default:
		v->Push(_null_<Q>);
		break;
	}
	return 1;
}

#ifdef _SQ_M2
template <Squirk Q>
static SQInteger default_delegate_tonumber(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o = stack_get(v,1);
	switch(obj_type(o)) {
	case OT_STRING: {
		SQObjectPtr<Q> result;
		if(str2num(_stringval(o),result)) {
			v->Push(result);
			break;
		}
		v->Push(SQObjectPtr<Q>((SQInteger)0));
		break;
	}
	case OT_INTEGER:
	case OT_FLOAT:
		v->Push(o);
		break;
	case OT_BOOL:
		v->Push(SQObjectPtr<Q>(_integer(o) ? (SQInteger)1 : (SQInteger)0));
		break;
	default:
		v->Push(SQObjectPtr<Q>((SQInteger)0));
		break;
	}
	return 1;
}
#endif

template <Squirk Q>
static SQInteger default_delegate_tostring(HSQUIRRELVM<Q> v)
{
	sq_tostring(v,1);
	return 1;
}

template <Squirk Q>
static SQInteger obj_delegate_weakref(HSQUIRRELVM<Q> v)
{
	sq_weakref(v,1);
	return 1;
}

template <Squirk Q>
static SQInteger obj_clear(HSQUIRRELVM<Q> v)
{
	return sq_clear(v,-1);
}

#ifdef _SQ_M2
template <Squirk Q>
static SQInteger obj_find(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o = stack_get(v,1);
	SQObjectPtr<Q> &value = stack_get(v,2);
	SQObjectPtr<Q> refpos, key, val;
	SQInteger faketojump;
	while(v->FOREACH_OP(o,key,val,refpos,0,666,faketojump) && faketojump != 666) {
		SQInteger res;
		v->ObjCmp(val,value,res);
		if(res == 0) {
			v->Push(key);
			return 1;
		}
	}
	return 0;
}

template <Squirk Q>
static SQInteger obj_findall(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o = stack_get(v,1);
	SQArray<Q> *ret = SQArray<Q>::Create(_ss(v),0);
	SQObjectPtr<Q> &value = stack_get(v,2);
	SQObjectPtr<Q> refpos, key, val;
	SQInteger faketojump;
	while(v->FOREACH_OP(o,key,val,refpos,0,666,faketojump) && faketojump != 666) {
		SQInteger res;
		v->ObjCmp(val,value,res);
		if(res == 0) ret->Append(key);
	}
	v->Push(ret);
	return 1;
}

template <Squirk Q>
static SQInteger obj_includes(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o = stack_get(v,1);
	SQObjectPtr<Q> &value = stack_get(v,2);
	SQObjectPtr<Q> refpos, key, val;
	SQInteger faketojump;
	while(v->FOREACH_OP(o,key,val,refpos,0,666,faketojump) && faketojump != 666) {
		SQInteger res;
		v->ObjCmp(val,value,res);
		if(res == 0) {
			v->Push(true);
			return 1;
		}
	}
	v->Push(false);
	return 1;
}

template <Squirk Q>
static SQInteger obj_includeCount(HSQUIRRELVM<Q> v)
{
	SQInteger count = 0;
	SQObjectPtr<Q> &o = stack_get(v,1);
	SQObjectPtr<Q> &value = stack_get(v,2);
	SQObjectPtr<Q> refpos, key, val;
	SQInteger faketojump;
	while(v->FOREACH_OP(o,key,val,refpos,0,666,faketojump) && faketojump != 666) {
		SQInteger res;
		v->ObjCmp(val,value,res);
		if(res == 0) count++;
	}
	v->Push(count);
	return 1;
}
#endif

template <Squirk Q>
static SQInteger number_delegate_tochar(HSQUIRRELVM<Q> v)
{
	SQObject<Q> &o=stack_get(v,1);
	SQChar c = (SQChar)tointeger(o);
	v->Push(SQString<Q>::Create(_ss(v),(const SQChar *)&c,1));
	return 1;
}


/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

template <Squirk Q>
static SQInteger table_rawdelete(HSQUIRRELVM<Q> v)
{
	if(SQ_FAILED(sq_rawdeleteslot(v,1,SQTrue)))
		return SQ_ERROR;
	return 1;
}

template <Squirk Q>
static SQInteger container_rawexists(HSQUIRRELVM<Q> v)
{
	if(SQ_SUCCEEDED(sq_rawget(v,-2))) {
		sq_pushbool(v,SQTrue);
		return 1;
	}
	sq_pushbool(v,SQFalse);
	return 1;
}

template <Squirk Q>
static SQInteger table_rawset(HSQUIRRELVM<Q> v)
{
	return sq_rawset(v,-3);
}

template <Squirk Q>
static SQInteger table_rawget(HSQUIRRELVM<Q> v)
{
	return SQ_SUCCEEDED(sq_rawget(v,-2))?1:SQ_ERROR;
}

#ifdef _SQ_M2
template <Squirk Q>
static SQInteger table_keys(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o = stack_get(v,1);
	SQArray<Q> *ret = SQArray<Q>::Create(_ss(v),0);
	SQObjectPtr<Q> refpos, key, val;
	SQInteger faketojump;
	while(v->FOREACH_OP(o,key,val,refpos,0,666,faketojump) && faketojump != 666) {
		ret->Append(key);
	}
	v->Push(ret);
	return 1;
}
#endif

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_table_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("t")},
	{_SC("rawget"),table_rawget,2, _SC("t")},
	{_SC("rawset"),table_rawset,3, _SC("t")},
	{_SC("rawdelete"),table_rawdelete,2, _SC("t")},
	{_SC("rawin"),container_rawexists,2, _SC("t")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{_SC("clear"),obj_clear,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("find"),obj_find,2, _SC("..")},
	{_SC("findall"),obj_findall,2, _SC("..")},
	{_SC("includes"),obj_includes,2, _SC("..")},
	{_SC("includeCount"),obj_includeCount,2, _SC("..")},
	{_SC("keys"),table_keys,1, _SC("t")},
#endif
	{0,0}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

template <Squirk Q>
static SQInteger array_append(HSQUIRRELVM<Q> v)
{
	return sq_arrayappend(v,-2);
}

template <Squirk Q>
static SQInteger array_extend(HSQUIRRELVM<Q> v)
{
	_array(stack_get(v,1))->Extend(_array(stack_get(v,2)));
	return 0;
}

template <Squirk Q>
static SQInteger array_reverse(HSQUIRRELVM<Q> v)
{
	return sq_arrayreverse(v,-1);
}

template <Squirk Q>
static SQInteger array_pop(HSQUIRRELVM<Q> v)
{
	return SQ_SUCCEEDED(sq_arraypop(v,1,SQTrue))?1:SQ_ERROR;
}

template <Squirk Q>
static SQInteger array_top(HSQUIRRELVM<Q> v)
{
	SQObject<Q> &o=stack_get(v,1);
	if(_array(o)->Size()>0){
		v->Push(_array(o)->Top());
		return 1;
	}
	else return sq_throwerror(v,_SC("top() on a empty array"));
}

template <Squirk Q>
static SQInteger array_insert(HSQUIRRELVM<Q> v)
{
	SQObject<Q> &o=stack_get(v,1);
	SQObject<Q> &idx=stack_get(v,2);
	SQObject<Q> &val=stack_get(v,3);
	if(!_array(o)->Insert(tointeger(idx),val))
		return sq_throwerror(v,_SC("index out of range"));
	return 0;
}

template <Squirk Q>
static SQInteger array_remove(HSQUIRRELVM<Q> v)
{
	SQObject<Q> &o = stack_get(v, 1);
	SQObject<Q> &idx = stack_get(v, 2);
	if(!sq_isnumeric(idx)) return sq_throwerror(v, _SC("wrong type"));
	SQObjectPtr<Q> val;
	if(_array(o)->Get(tointeger(idx), val)) {
		_array(o)->Remove(tointeger(idx));
		v->Push(val);
		return 1;
	}
	return sq_throwerror(v, _SC("idx out of range"));
}

#ifdef _SQ_M2
template <Squirk Q>
static SQInteger array_removeValue(HSQUIRRELVM<Q> v)
{
	SQBool all = sq_gettop(v) > 2 ? (tointeger(stack_get(v,3)) ? SQTrue : SQFalse) : SQTrue;
	sq_push(v, 2);
	return sq_arrayremovevalue(v, 1, all);
}
#endif

template <Squirk Q>
static SQInteger array_resize(HSQUIRRELVM<Q> v)
{
	SQObject<Q> &o = stack_get(v, 1);
	SQObject<Q> &nsize = stack_get(v, 2);
	SQObjectPtr<Q> fill;
	if(sq_isnumeric(nsize)) {
		if(sq_gettop(v) > 2)
			fill = stack_get(v, 3);
		_array(o)->Resize(tointeger(nsize),fill);
		return 0;
	}
	return sq_throwerror(v, _SC("size must be a number"));
}


//QSORT ala Sedgewick
template <Squirk Q>
bool _qsort_compare(HSQUIRRELVM<Q> v,SQObjectPtr<Q> &arr,SQObjectPtr<Q> &a,SQObjectPtr<Q> &b,SQInteger func,SQInteger &ret)
{
	if(func < 0) {
		if(!v->ObjCmp(a,b,ret)) return false;
	}
	else {
		SQInteger top = sq_gettop(v);
		sq_push(v, func);
		sq_pushroottable(v);
		v->Push(a);
		v->Push(b);
		if(SQ_FAILED(sq_call(v, 3, SQTrue, SQFalse))) {
			if(!sq_isstring( v->_lasterror)) 
				v->Raise_Error(_SC("compare func failed"));
			return false;
		}
		sq_getinteger(v, -1, &ret);
		sq_settop(v, top);
		return true;
	}
	return true;
}
//QSORT ala Sedgewick
template <Squirk Q>
bool _qsort(HSQUIRRELVM<Q> v,SQObjectPtr<Q> &arr, SQInteger l, SQInteger r,SQInteger func)
{
	SQInteger i, j;
	SQArray<Q> *a=_array(arr);
	SQObjectPtr<Q> pivot,t;
	if( l < r ){
		pivot = a->_values[l];
		i = l; j = r+1;
		while(1){
			SQInteger ret;
			do { 
				++i; 
				if(i > r) break;
				if(!_qsort_compare(v,arr,a->_values[i],pivot,func,ret))
					return false;
			} while( ret <= 0);
			do {
				--j;
				if ( j < 0 ) {
					v->Raise_Error( _SC("Invalid qsort, probably compare function defect") ); 
					return false; 
				}
				if(!_qsort_compare(v,arr,a->_values[j],pivot,func,ret))
					return false;
			}
			while( ret > 0 );
			if( i >= j ) break;
			t = a->_values[i]; a->_values[i] = a->_values[j]; a->_values[j] = t;
		}
		t = a->_values[l]; a->_values[l] = a->_values[j]; a->_values[j] = t;
		if(!_qsort( v, arr, l, j-1,func)) return false;
		if(!_qsort( v, arr, j+1, r,func)) return false;
	}
	return true;
}

template <Squirk Q>
static SQInteger array_sort(HSQUIRRELVM<Q> v)
{
	SQInteger func = -1;
	SQObjectPtr<Q> &o = stack_get(v,1);
	SQObject<Q> &funcobj = stack_get(v,2);
	if(_array(o)->Size() > 1) {
		if(obj_type(funcobj) == OT_CLOSURE || obj_type(funcobj) == OT_NATIVECLOSURE) func = 2;
		if(!_qsort(v, o, 0, _array(o)->Size()-1, func))
			return SQ_ERROR;

	}
	return 0;
}
template <Squirk Q>
static SQInteger array_slice(HSQUIRRELVM<Q> v)
{
	SQInteger sidx,eidx;
	SQObjectPtr<Q> o;
	if(get_slice_params(v,sidx,eidx,o)==-1)return -1;
	SQInteger alen = _array(o)->Size();
	if(sidx < 0)sidx = alen + sidx;
	if(eidx < 0)eidx = alen + eidx;
	if(eidx < sidx)return sq_throwerror(v,_SC("wrong indexes"));
	if(eidx > alen)return sq_throwerror(v,_SC("slice out of range"));
	SQArray<Q> *arr=SQArray<Q>::Create(_ss(v),eidx-sidx);
	SQObjectPtr<Q> t;
	SQInteger count=0;
	for(SQInteger i=sidx;i<eidx;i++){
		_array(o)->Get(i,t);
		arr->Set(count++,t);
	}
	v->Push(arr);
	return 1;
	
}

#ifdef _SQ_M2
template <Squirk Q>
static SQInteger array_splice(HSQUIRRELVM<Q> v)
{
	SQInteger top = sq_gettop(v);
	SQInteger sidx = 0, count = 0;
	SQObjectPtr<Q> o = stack_get(v,1);
	SQInteger alen = _array(o)->Size();
	SQObjectPtr<Q> &start = stack_get(v,2);
	if(obj_type(start) != OT_NULL && sq_isnumeric(start)) sidx = tointeger(start);
	if(sidx < 0) sidx = alen + sidx;
	if(top > 2) {
		SQObjectPtr<Q> &end = stack_get(v,3);
		if(sq_isnumeric(end)) count = tointeger(end);
	} else {
		count = alen - sidx;
	}
	if(count <= 0) return sq_throwerror(v,_SC("wrong indexes"));
	if(sidx + count > alen) return sq_throwerror(v,_SC("slice out of range"));
	SQArray<Q> *arr = SQArray<Q>::Create(_ss(v),count);
	SQObjectPtr<Q> value;
	for(SQInteger i = 1; i <= count; i++) {
		SQInteger idx = sidx + count - i;
		_array(o)->Get(idx,value);
		_array(o)->Remove(idx);
		arr->Set(count-i,value);
	}
	for(SQInteger i = 4; i <= top; i++) {
		SQObject<Q> &valueToInsert = stack_get(v,i);
		_array(o)->Insert(sidx++,valueToInsert);
	}
	v->Push(arr);
	return 1;
}

template <Squirk Q>
static SQInteger array_join(HSQUIRRELVM<Q> v)
{
	SQObject<Q> &o = stack_get(v,1);
	const SQChar *delimiter;
	sq_getstring(v,2,&delimiter);
	SQInteger delimiterLength = (SQInteger)scstrlen(delimiter);
	SQInteger count = _array(o)->Size();
	SQChar *dest = NULL;
	SQInteger allocated = 0;
	for(SQInteger i = 0; i < count; i++) {
		SQObjectPtr<Q> value;
		if(_array(o)->Get(i,value)) {
			SQObjectPtr<Q> result;
			v->ToString(value,result);
			const SQChar *source = _stringval(result);
			SQInteger sourceLength = (SQInteger)scstrlen(source);
			SQInteger separatorLength = allocated ? delimiterLength : 0;
			dest = sq_getscratchpad(v,allocated + separatorLength + sourceLength + 1);
			if(separatorLength) {
				memcpy(&dest[allocated],delimiter,separatorLength * sizeof(SQChar));
				allocated += separatorLength;
			}
			memcpy(&dest[allocated],source,sourceLength * sizeof(SQChar));
			allocated += sourceLength;
			dest[allocated] = 0;
		}
	}
	if(dest) {
		v->Push(SQString<Q>::Create(_ss(v),dest,-1));
	} else {
		v->Push(SQString<Q>::Create(_ss(v),_SC(""),0));
	}
	return 1;
}
#endif

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_array_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("a")},
	{_SC("append"),array_append,2, _SC("a")},
	{_SC("extend"),array_extend,2, _SC("aa")},
	{_SC("push"),array_append,2, _SC("a")},
	{_SC("pop"),array_pop,1, _SC("a")},
	{_SC("top"),array_top,1, _SC("a")},
	{_SC("insert"),array_insert,3, _SC("an")},
	{_SC("remove"),array_remove,2, _SC("an")},
#ifdef _SQ_M2
	{_SC("erase"),array_remove,2, _SC("an")},
	{_SC("removeValue"),array_removeValue,2, _SC("a.b")},
#endif
	{_SC("resize"),array_resize,-2, _SC("an")},
	{_SC("reverse"),array_reverse,1, _SC("a")},
	{_SC("sort"),array_sort,-1, _SC("ac")},
	{_SC("slice"),array_slice,-1, _SC("ann")},
#ifdef _SQ_M2
	{_SC("splice"),array_splice,-1, _SC("ann")},
	{_SC("join"),array_join,2, _SC("as")},
#endif
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{_SC("clear"),obj_clear,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("find"),obj_find,2, _SC("..")},
	{_SC("findall"),obj_findall,2, _SC("..")},
	{_SC("includes"),obj_includes,2, _SC("..")},
	{_SC("includeCount"),obj_includeCount,2, _SC("..")},
#endif
	{0,0}
};

//STRING DEFAULT DELEGATE//////////////////////////
template <Squirk Q>
static SQInteger string_slice(HSQUIRRELVM<Q> v)
{
	SQInteger sidx,eidx;
	SQObjectPtr<Q> o;
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,o)))return -1;
	SQInteger slen = _string(o)->_len;
	if(sidx < 0)sidx = slen + sidx;
	if(eidx < 0)eidx = slen + eidx;
	if(eidx < sidx)	return sq_throwerror(v,_SC("wrong indexes"));
	if(eidx > slen)	return sq_throwerror(v,_SC("slice out of range"));
	v->Push(SQString<Q>::Create(_ss(v),&_stringval(o)[sidx],eidx-sidx));
	return 1;
}

template <Squirk Q>
static SQInteger string_find(HSQUIRRELVM<Q> v)
{
	SQInteger top,start_idx=0;
	const SQChar *str,*substr,*ret;
	if(((top=sq_gettop(v))>1) && SQ_SUCCEEDED(sq_getstring(v,1,&str)) && SQ_SUCCEEDED(sq_getstring(v,2,&substr))){
		if(top>2)sq_getinteger(v,3,&start_idx);
		if((sq_getsize(v,1)>start_idx) && (start_idx>=0)){
			ret=scstrstr(&str[start_idx],substr);
			if(ret){
				sq_pushinteger(v,(SQInteger)(ret-str));
				return 1;
			}
		}
		return 0;
	}
	return sq_throwerror(v,_SC("invalid param"));
}

#ifdef _SQ_M2
static const SQChar *scstrrstr(const SQChar *string, const SQChar *pattern)
{
	const SQChar *last = NULL;
	for(const SQChar *position = string; (position = scstrstr(position,pattern)); position++) {
		last = position;
		if(*position == '\0') return last;
	}
	return last;
}

template <Squirk Q>
static SQInteger string_rfind(HSQUIRRELVM<Q> v)
{
	SQInteger top, startIdx = 0;
	const SQChar *str, *substr, *result;
	if(((top = sq_gettop(v)) > 1) && SQ_SUCCEEDED(sq_getstring(v,1,&str)) && SQ_SUCCEEDED(sq_getstring(v,2,&substr))) {
		if(top > 2) sq_getinteger(v,3,&startIdx);
		if(sq_getsize(v,1) >= startIdx && startIdx >= 0) {
			result = scstrrstr(&str[startIdx],substr);
			if(result) {
				sq_pushinteger(v,(SQInteger)(result-str));
				return 1;
			}
		}
		return 0;
	}
	return sq_throwerror(v,_SC("invalid param"));
}

template <Squirk Q>
static SQInteger string_substr(HSQUIRRELVM<Q> v)
{
	SQInteger top = sq_gettop(v);
	SQObjectPtr<Q> object = stack_get(v,1);
	SQInteger length = _string(object)->_len;
	SQInteger start = top > 1 ? tointeger(stack_get(v,2)) : 0;
	if(start < 0) start = length + start;
	SQInteger end = top > 2 ? start + tointeger(stack_get(v,3)) : length;
	if(start < 0 || end < start) return sq_throwerror(v,_SC("wrong indexes"));
	if(end > length) return sq_throwerror(v,_SC("slice out of range"));
	v->Push(SQString<Q>::Create(_ss(v),&_stringval(object)[start],end-start));
	return 1;
}

template <Squirk Q>
static SQInteger string_charAt(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> object = stack_get(v,1);
	SQInteger index = tointeger(stack_get(v,2));
	SQInteger length = _string(object)->_len;
	if(index < 0) index = length + index;
	if(index < 0 || index >= length) return sq_throwerror(v,_SC("slice out of range"));
	v->Push(SQString<Q>::Create(_ss(v),&_stringval(object)[index],1));
	return 1;
}

template <Squirk Q>
static SQInteger string_replace(HSQUIRRELVM<Q> v)
{
	SQObject<Q> str = stack_get(v,1);
	SQObject<Q> from = stack_get(v,2);
	SQObject<Q> to = stack_get(v,3);
	const SQChar *strData = _stringval(str);
	const SQChar *fromData = _stringval(from);
	const SQChar *toData = _stringval(to);
	SQInteger strLength = _string(str)->_len;
	SQInteger fromLength = _string(from)->_len;
	SQInteger toLength = _string(to)->_len;
	if(fromLength == 0) return sq_throwerror(v,_SC("empty search string"));
	const SQChar *position;
	const SQChar *start = strData;
	while((position = scstrstr(start,fromData)) != NULL) {
		start = position + fromLength;
		strLength += toLength - fromLength;
	}
	SQChar *result = _ss(v)->GetScratchPad(rsl(strLength + 1));
	start = strData;
	SQChar *output = result;
	while((position = scstrstr(start,fromData)) != NULL) {
		while(start < position) *output++ = *start++;
		start += fromLength;
		for(SQInteger i = 0; i < toLength; i++) *output++ = toData[i];
	}
	while((*output++ = *start++) != '\0') {}
	v->Push(SQString<Q>::Create(_ss(v),result,strLength));
	return 1;
}

template <Squirk Q>
static SQInteger string_split(HSQUIRRELVM<Q> v)
{
	SQObject<Q> str = stack_get(v,1);
	SQObject<Q> delimiter = stack_get(v,2);
	const SQChar *source = _stringval(str);
	const SQChar *delimiterData = _stringval(delimiter);
	SQInteger delimiterLength = _string(delimiter)->_len;
	if(delimiterLength == 0) return sq_throwerror(v,_SC("empty delimiter"));
	SQArray<Q> *result = SQArray<Q>::Create(_ss(v),0);
	const SQChar *position;
	while((position = scstrstr(source,delimiterData)) != NULL) {
		SQInteger length = (SQInteger)(position-source);
		result->Append(SQObjectPtr<Q>(SQString<Q>::Create(_ss(v),source,length)));
		source = position + delimiterLength;
	}
	result->Append(SQObjectPtr<Q>(SQString<Q>::Create(_ss(v),source,(SQInteger)scstrlen(source))));
	v->Push(result);
	return 1;
}

static SQInteger string_mbcharlen(const SQChar *str, SQInteger index)
{
#if defined(SQUNICODE)
	return str[index] ? 1 : 0;
#elif defined(USESJIS)
	unsigned char ch = (unsigned char)str[index];
	return ch ? (((((ch ^ 0x20) - 0xa1) & 0xff) < 0x3c) ? 2 : 1) : 0;
#else
	unsigned char ch = (unsigned char)str[index];
	if(!ch) return 0;
	if((ch & 0x80) == 0) return 1;
	if((ch & 0xe0) == 0xc0) return 2;
	if((ch & 0xf0) == 0xe0) return 3;
	if((ch & 0xf8) == 0xf0) return 4;
	if((ch & 0xfc) == 0xf8) return 5;
	if((ch & 0xfe) == 0xfc) return 6;
	if((ch & 0xc0) == 0x80) {
		SQInteger count = 0;
		do { index++; count++; ch = (unsigned char)str[index]; } while(ch && (ch & 0xc0) == 0x80);
		return count;
	}
	return 1;
#endif
}

static SQInteger string_mbcount(const SQChar *str, SQInteger length)
{
	SQInteger count = 0, index = 0, charLength;
	while(index < length && (charLength = string_mbcharlen(str,index)) > 0) {
		count++;
		index += charLength;
	}
	return count;
}

template <Squirk Q>
static SQInteger string_mbnext(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &str = stack_get(v,1);
	SQInteger index = sq_gettop(v) > 1 ? tointeger(stack_get(v,2)) : 0;
	v->Push(SQObjectPtr<Q>(index >= 0 && index < _string(str)->_len ? string_mbcharlen(_stringval(str),index) : 0));
	return 1;
}

template <Squirk Q>
static SQInteger string_mblen(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &str = stack_get(v,1);
	v->Push(SQObjectPtr<Q>(string_mbcount(_stringval(str),_string(str)->_len)));
	return 1;
}

template <Squirk Q>
static SQInteger string_mbsubstr(HSQUIRRELVM<Q> v)
{
	SQInteger top = sq_gettop(v);
	SQObjectPtr<Q> &str = stack_get(v,1);
	const SQChar *data = _stringval(str);
	SQInteger byteLength = _string(str)->_len;
	SQInteger charCount = string_mbcount(data,byteLength);
	SQInteger startChar = top > 1 ? tointeger(stack_get(v,2)) : 0;
	if(startChar < 0) startChar = charCount + startChar;
	SQInteger endChar = top > 2 ? startChar + tointeger(stack_get(v,3)) : charCount;
	if(startChar < 0 || endChar < startChar) return sq_throwerror(v,_SC("wrong indexes"));
	if(endChar > charCount) return sq_throwerror(v,_SC("slice out of range"));
	SQInteger byteStart = 0, byteEnd = 0, charIndex = 0, charLength;
	while(charIndex < startChar && (charLength = string_mbcharlen(data,byteStart)) > 0) { charIndex++; byteStart += charLength; }
	byteEnd = byteStart;
	while(charIndex < endChar && (charLength = string_mbcharlen(data,byteEnd)) > 0) { charIndex++; byteEnd += charLength; }
	v->Push(SQString<Q>::Create(_ss(v),&data[byteStart],byteEnd-byteStart));
	return 1;
}
#endif

#define STRING_TOFUNCZ(func) static SQInteger string_##func(HSQUIRRELVM<Q> v) \
{ \
	SQObject<Q> str=stack_get(v,1); \
	SQInteger len=_string(str)->_len; \
	const SQChar *sThis=_stringval(str); \
	SQChar *sNew=(_ss(v)->GetScratchPad(rsl(len))); \
	for(SQInteger i=0;i<len;i++) sNew[i]=func(sThis[i]); \
	v->Push(SQString<Q>::Create(_ss(v),sNew,len)); \
	return 1; \
}

template <Squirk Q>
STRING_TOFUNCZ(tolower)
template <Squirk Q>
STRING_TOFUNCZ(toupper)

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_string_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("s")},
	{_SC("tointeger"),default_delegate_tointeger,1, _SC("s")},
	{_SC("tofloat"),default_delegate_tofloat,1, _SC("s")},
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{_SC("slice"),string_slice,-1, _SC(" s n  n")},
	{_SC("find"),string_find,-2, _SC("s s n ")},
#ifdef _SQ_M2
	{_SC("rfind"),string_rfind,-2, _SC("ssn")},
#endif
	{_SC("tolower"),string_tolower,1, _SC("s")},
	{_SC("toupper"),string_toupper,1, _SC("s")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
#ifdef _SQ_M2
	{_SC("substr"),string_substr,-1, _SC("snn")},
	{_SC("charAt"),string_charAt,2, _SC("sn")},
	{_SC("replace"),string_replace,3, _SC("sss")},
	{_SC("split"),string_split,2, _SC("ss")},
	{_SC("mbnext"),string_mbnext,-1, _SC("sn")},
	{_SC("mblen"),string_mblen,1, _SC("s")},
	{_SC("mbsubstr"),string_mbsubstr,-1, _SC("snn")},
#endif
	{0,0}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_number_default_delegate_funcz[]={
	{_SC("tointeger"),default_delegate_tointeger,1, _SC("n|b")},
	{_SC("tofloat"),default_delegate_tofloat,1, _SC("n|b")},
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{_SC("tochar"),number_delegate_tochar,1, _SC("n|b")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{0,0}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
template <Squirk Q>
static SQInteger closure_pcall(HSQUIRRELVM<Q> v)
{
	return SQ_SUCCEEDED(sq_call(v,sq_gettop(v)-1,SQTrue,SQFalse))?1:SQ_ERROR;
}

template <Squirk Q>
static SQInteger closure_call(HSQUIRRELVM<Q> v)
{
	return SQ_SUCCEEDED(sq_call(v,sq_gettop(v)-1,SQTrue,SQTrue))?1:SQ_ERROR;
}

template <Squirk Q>
static SQInteger _closure_acall(HSQUIRRELVM<Q> v,SQBool raiseerror)
{
	SQArray<Q> *aparams=_array(stack_get(v,2));
	SQInteger nparams=aparams->Size();
	v->Push(stack_get(v,1));
	for(SQInteger i=0;i<nparams;i++)v->Push(aparams->_values[i]);
	return SQ_SUCCEEDED(sq_call(v,nparams,SQTrue,raiseerror))?1:SQ_ERROR;
}

template <Squirk Q>
static SQInteger closure_acall(HSQUIRRELVM<Q> v)
{
	return _closure_acall(v,SQTrue);
}

template <Squirk Q>
static SQInteger closure_pacall(HSQUIRRELVM<Q> v)
{
	return _closure_acall(v,SQFalse);
}

template <Squirk Q>
static SQInteger closure_bindenv(HSQUIRRELVM<Q> v)
{
	if(SQ_FAILED(sq_bindenv(v,1)))
		return SQ_ERROR;
	return 1;
}

template <Squirk Q>
static SQInteger closure_getinfos(HSQUIRRELVM<Q> v) {
	SQObject<Q> o = stack_get(v,1);
	SQTable<Q> *res = SQTable<Q>::Create(_ss(v),4);
	if(obj_type(o) == OT_CLOSURE) {
		SQFunctionProto<Q> *f = _funcproto(_closure(o)->_function);
		SQInteger nparams = f->_nparameters + (f->_varparams?1:0);
		SQObjectPtr<Q> params = SQArray<Q>::Create(_ss(v),nparams);
		for(SQInteger n = 0; n<f->_nparameters; n++) {
			_array(params)->Set((SQInteger)n,f->_parameters[n]);
		}
		if(f->_varparams) {
			_array(params)->Set(nparams-1,SQString<Q>::Create(_ss(v),_SC("..."),-1));
		}
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("native"),-1),false);
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("name"),-1),f->_name);
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("src"),-1),f->_sourcename);
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("parameters"),-1),params);
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("varargs"),-1),f->_varparams);
	}
	else { //OT_NATIVECLOSURE 
		SQNativeClosure<Q> *nc = _nativeclosure(o);
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("native"),-1),true);
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("name"),-1),nc->_name);
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("paramscheck"),-1),nc->_nparamscheck);
		SQObjectPtr<Q> typecheck;
		if(nc->_typecheck.size() > 0) {
			typecheck =
				SQArray<Q>::Create(_ss(v), nc->_typecheck.size());
			for(SQUnsignedInteger n = 0; n<nc->_typecheck.size(); n++) {
					_array(typecheck)->Set((SQInteger)n,nc->_typecheck[n]);
			}
		}
		res->NewSlot(SQString<Q>::Create(_ss(v),_SC("typecheck"),-1),typecheck);
	}
	v->Push(res);
	return 1;
}

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_closure_default_delegate_funcz[]={
	{_SC("call"),closure_call,-1, _SC("c")},
	{_SC("pcall"),closure_pcall,-1, _SC("c")},
	{_SC("acall"),closure_acall,2, _SC("ca")},
	{_SC("pacall"),closure_pacall,2, _SC("ca")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{_SC("bindenv"),closure_bindenv,2, _SC("c x|y|t")},
	{_SC("getinfos"),closure_getinfos,1, _SC("c")},
	{0,0}
};

//GENERATOR DEFAULT DELEGATE
template <Squirk Q>
static SQInteger generator_getstatus(HSQUIRRELVM<Q> v)
{
	SQObject<Q> &o=stack_get(v,1);
	switch(_generator(o)->_state){
		case SQGenerator<Q>::eSuspended:v->Push(SQString<Q>::Create(_ss(v),_SC("suspended")));break;
		case SQGenerator<Q>::eRunning:v->Push(SQString<Q>::Create(_ss(v),_SC("running")));break;
		case SQGenerator<Q>::eDead:v->Push(SQString<Q>::Create(_ss(v),_SC("dead")));break;
	}
	return 1;
}

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_generator_default_delegate_funcz[]={
	{_SC("getstatus"),generator_getstatus,1, _SC("g")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{0,0}
};

//THREAD DEFAULT DELEGATE
template <Squirk Q>
static SQInteger thread_call(HSQUIRRELVM<Q> v)
{
	
	SQObjectPtr o = stack_get(v,1);
	if(obj_type(o) == OT_THREAD) {
		SQInteger nparams = sq_gettop(v);
		_thread(o)->Push(_thread(o)->_roottable);
		for(SQInteger i = 2; i<(nparams+1); i++)
			sq_move(_thread(o),v,i);
		if(SQ_SUCCEEDED(sq_call(_thread(o),nparams,SQTrue,SQFalse))) {
			sq_move(v,_thread(o),-1);
			sq_pop(_thread(o),1);
			return 1;
		}
		v->_lasterror = _thread(o)->_lasterror;
		return SQ_ERROR;
	}
	return sq_throwerror(v,_SC("wrong parameter"));
}

template <Squirk Q>
static SQInteger thread_wakeup(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> o = stack_get(v,1);
	if(obj_type(o) == OT_THREAD) {
		SQVM<Q> *thread = _thread(o);
		SQInteger state = sq_getvmstate(thread);
		if(state != SQ_VMSTATE_SUSPENDED) {
			switch(state) {
				case SQ_VMSTATE_IDLE:
					return sq_throwerror(v,_SC("cannot wakeup a idle thread"));
				break;
				case SQ_VMSTATE_RUNNING:
					return sq_throwerror(v,_SC("cannot wakeup a running thread"));
				break;
			}
		}
			
		SQInteger wakeupret = sq_gettop(v)>1?1:0;
		if(wakeupret) {
			sq_move(thread,v,2);
		}
		if(SQ_SUCCEEDED(sq_wakeupvm(thread,wakeupret,SQTrue,SQTrue,SQFalse))) {
			sq_move(v,thread,-1);
			sq_pop(thread,1); //pop retval
			if(sq_getvmstate(thread) == SQ_VMSTATE_IDLE) {
				sq_settop(thread,1); //pop roottable
			}
			return 1;
		}
		sq_settop(thread,1);
		v->_lasterror = thread->_lasterror;
		return SQ_ERROR;
	}
	return sq_throwerror(v,_SC("wrong parameter"));
}

template <Squirk Q>
static SQInteger thread_getstatus(HSQUIRRELVM<Q> v)
{
	SQObjectPtr<Q> &o = stack_get(v,1);
	switch(sq_getvmstate(_thread(o))) {
		case SQ_VMSTATE_IDLE:
			sq_pushstring(v,_SC("idle"),-1);
		break;
		case SQ_VMSTATE_RUNNING:
			sq_pushstring(v,_SC("running"),-1);
		break;
		case SQ_VMSTATE_SUSPENDED:
			sq_pushstring(v,_SC("suspended"),-1);
		break;
		default:
			return sq_throwerror(v,_SC("internal VM error"));
	}
	return 1;
}

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_thread_default_delegate_funcz[] = {
	{_SC("call"), thread_call, -1, _SC("v")},
	{_SC("wakeup"), thread_wakeup, -1, _SC("v")},
	{_SC("getstatus"), thread_getstatus, 1, _SC("v")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{0,0},
};

template <Squirk Q>
static SQInteger class_getattributes(HSQUIRRELVM<Q> v)
{
	if(SQ_SUCCEEDED(sq_getattributes(v,-2)))
		return 1;
	return SQ_ERROR;
}

template <Squirk Q>
static SQInteger class_setattributes(HSQUIRRELVM<Q> v)
{
	if(SQ_SUCCEEDED(sq_setattributes(v,-3)))
		return 1;
	return SQ_ERROR;
}

template <Squirk Q>
static SQInteger class_instance(HSQUIRRELVM<Q> v)
{
	if(SQ_SUCCEEDED(sq_createinstance(v,-1)))
		return 1;
	return SQ_ERROR;
}

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_class_default_delegate_funcz[] = {
	{_SC("getattributes"), class_getattributes, 2, _SC("y.")},
	{_SC("setattributes"), class_setattributes, 3, _SC("y..")},
	{_SC("rawin"),container_rawexists,2, _SC("y")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{_SC("instance"),class_instance,1, _SC("y")},
	{0,0}
};

template <Squirk Q>
static SQInteger instance_getclass(HSQUIRRELVM<Q> v)
{
	if(SQ_SUCCEEDED(sq_getclass(v,1)))
		return 1;
	return SQ_ERROR;
}

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_instance_default_delegate_funcz[] = {
	{_SC("getclass"), instance_getclass, 1, _SC("x")},
	{_SC("rawin"),container_rawexists,2, _SC("x")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{0,0}
};

template <Squirk Q>
static SQInteger weakref_ref(HSQUIRRELVM<Q> v)
{
	if(SQ_FAILED(sq_getweakrefval(v,1)))
		return SQ_ERROR;
	return 1;
}

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_weakref_default_delegate_funcz[] = {
	{_SC("ref"),weakref_ref,1, _SC("r")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
#ifdef _SQ_M2
	{_SC("tonumber"),default_delegate_tonumber,1, _SC(".")},
#endif
	{0,0}
};
