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
	{_SC("assert"),base_assert,2, NULL},
	{_SC("print"),base_print,2, NULL},
	{_SC("compilestring"),base_compilestring,-2, _SC(".ss")},
	{_SC("newthread"),base_newthread,2, _SC(".c")},
	{_SC("suspend"),base_suspend,-1, NULL},
	{_SC("array"),base_array,-2, _SC(".n")},
	{_SC("type"),base_type,2, NULL},
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

template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_table_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("t")},
	{_SC("rawget"),table_rawget,2, _SC("t")},
	{_SC("rawset"),table_rawset,3, _SC("t")},
	{_SC("rawdelete"),table_rawdelete,2, _SC("t")},
	{_SC("rawin"),container_rawexists,2, _SC("t")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("clear"),obj_clear,1, _SC(".")},
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
	{_SC("resize"),array_resize,-2, _SC("an")},
	{_SC("reverse"),array_reverse,1, _SC("a")},
	{_SC("sort"),array_sort,-1, _SC("ac")},
	{_SC("slice"),array_slice,-1, _SC("ann")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("clear"),obj_clear,1, _SC(".")},
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
	{_SC("slice"),string_slice,-1, _SC(" s n  n")},
	{_SC("find"),string_find,-2, _SC("s s n ")},
	{_SC("tolower"),string_tolower,1, _SC("s")},
	{_SC("toupper"),string_toupper,1, _SC("s")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{0,0}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
template <Squirk Q>
SQRegFunction<Q> SQSharedState<Q>::_number_default_delegate_funcz[]={
	{_SC("tointeger"),default_delegate_tointeger,1, _SC("n|b")},
	{_SC("tofloat"),default_delegate_tofloat,1, _SC("n|b")},
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
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
	{0,0}
};
