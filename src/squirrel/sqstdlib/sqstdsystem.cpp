/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sqstdsystem.h>

#ifdef SQUNICODE
#include <wchar.h>
#define scgetenv _wgetenv
#define scsystem _wsystem
#define scasctime _wasctime
#define scremove _wremove
#define screname _wrename
#else
#define scgetenv getenv
#define scsystem system
#define scasctime asctime
#define scremove remove
#define screname rename
#endif

template <Squirk Q>
static SQInteger _system_getenv(HSQUIRRELVM<Q> v)
{
	const SQChar *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
        sq_pushstring(v,scgetenv(s),-1);
		return 1;
	}
	return 0;
}

template <Squirk Q>
static SQInteger _system_system(HSQUIRRELVM<Q> v)
{
	const SQChar *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
		sq_pushinteger(v,scsystem(s));
		return 1;
	}
	return sq_throwerror(v,_SC("wrong param"));
}

template <Squirk Q>
static SQInteger _system_clock(HSQUIRRELVM<Q> v)
{
	sq_pushfloat(v,((SQFloat)clock())/(SQFloat)CLOCKS_PER_SEC);
	return 1;
}

template <Squirk Q>
static SQInteger _system_time(HSQUIRRELVM<Q> v)
{
	time_t t;
	time(&t);
	sq_pushinteger(v,*((SQInteger *)&t));
	return 1;
}

template <Squirk Q>
static SQInteger _system_remove(HSQUIRRELVM<Q> v)
{
	const SQChar *s;
	sq_getstring(v,2,&s);
	if(scremove(s)==-1)
		return sq_throwerror(v,_SC("remove() failed"));
	return 0;
}

template <Squirk Q>
static SQInteger _system_rename(HSQUIRRELVM<Q> v)
{
	const SQChar *oldn,*newn;
	sq_getstring(v,2,&oldn);
	sq_getstring(v,3,&newn);
	if(screname(oldn,newn)==-1)
		return sq_throwerror(v,_SC("rename() failed"));
	return 0;
}

template <Squirk Q>
static void _set_integer_slot(HSQUIRRELVM<Q> v,const SQChar *name,SQInteger val)
{
	sq_pushstring(v,name,-1);
	sq_pushinteger(v,val);
	sq_rawset(v,-3);
}

template <Squirk Q>
static SQInteger _system_date(HSQUIRRELVM<Q> v)
{
	time_t t;
	SQInteger it;
	SQInteger format = 'l';
	if(sq_gettop(v) > 1) {
		sq_getinteger(v,2,&it);
		t = it;
		if(sq_gettop(v) > 2) {
			sq_getinteger(v,3,(SQInteger*)&format);
		}
	}
	else {
		time(&t);
	}
	tm *date;
    if(format == 'u')
		date = gmtime(&t);
	else
		date = localtime(&t);
	if(!date)
		return sq_throwerror(v,_SC("crt api failure"));
	sq_newtable(v);
	_set_integer_slot(v, _SC("sec"), date->tm_sec);
    _set_integer_slot(v, _SC("min"), date->tm_min);
    _set_integer_slot(v, _SC("hour"), date->tm_hour);
    _set_integer_slot(v, _SC("day"), date->tm_mday);
    _set_integer_slot(v, _SC("month"), date->tm_mon);
    _set_integer_slot(v, _SC("year"), date->tm_year+1900);
    _set_integer_slot(v, _SC("wday"), date->tm_wday);
    _set_integer_slot(v, _SC("yday"), date->tm_yday);
	return 1;
}



#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
template <Squirk Q>
static SQRegFunction<Q> systemlib_funcs[]={
	_DECL_FUNC(getenv,2,_SC(".s")),
	_DECL_FUNC(system,2,_SC(".s")),
	_DECL_FUNC(clock,1,NULL),
	_DECL_FUNC(time,1,NULL),
	_DECL_FUNC(date,-1,_SC(".nn")),
	_DECL_FUNC(remove,2,_SC(".s")),
	_DECL_FUNC(rename,3,_SC(".ss")),
	{0,0}
};

template <Squirk Q>
SQInteger sqstd_register_systemlib(HSQUIRRELVM<Q> v)
{
	SQInteger i=0;
	while(systemlib_funcs<Q>[i].name!=0)
	{
		sq_pushstring(v,systemlib_funcs<Q>[i].name,-1);
		sq_newclosure(v,systemlib_funcs<Q>[i].f,0);
		sq_setparamscheck(v,systemlib_funcs<Q>[i].nparamscheck,systemlib_funcs<Q>[i].typemask);
		sq_setnativeclosurename(v,-1,systemlib_funcs<Q>[i].name);
		sq_createslot(v,-3);
		i++;
	}
	return 1;
}
