/*
Copyright (c) 2003-2009 Alberto Demichelis

This software is provided 'as-is', without any 
express or implied warranty. In no event will the 
authors be held liable for any damages arising from 
the use of this software.

Permission is granted to anyone to use this software 
for any purpose, including commercial applications, 
and to alter it and redistribute it freely, subject 
to the following restrictions:

		1. The origin of this software must not be 
		misrepresented; you must not claim that 
		you wrote the original software. If you 
		use this software in a product, an 
		acknowledgment in the product 
		documentation would be appreciated but is 
		not required.

		2. Altered source versions must be plainly 
		marked as such, and must not be 
		misrepresented as being the original 
		software.

		3. This notice may not be removed or 
		altered from any source distribution.

*/
#ifndef _SQUIRREL_H_
#define _SQUIRREL_H_

#include <stdint.h>

#ifndef SQUIRREL_API
#define SQUIRREL_API extern
#endif

#if (defined(_WIN64) || defined(_LP64))
#define _SQ64
#endif

#define _SQ_M2

#ifdef _SQ_M2
#define SQUIRREL_VERSION_NUMBER 224
#define SCRAT_USE_CXX11_OPTIMIZATIONS
//#define SCRAT_USE_EXCEPTIONS
//#define SCRAT_NO_ERROR_CHECKING
//#define SCRAT_IMPORT
//#define SCRAT_EXPORT
#include "src/squirk.h"
class SQUtils { public: static void Print(const char *fmt, ...); };
#define scprintf(...) SQUtils::Print(__VA_ARGS__)
#endif

#ifdef _SQ_M2
#undef _SQ64

#ifdef _WIN64
	typedef int SQInteger;
	typedef int SQInt32; /*must be 32 bits(also on 64bits processors)*/
	typedef unsigned int SQUnsignedInteger;
	typedef unsigned long long SQHash; /*should be the same size of a pointer*/
#else
	typedef int SQInteger;
	typedef int SQInt32; /*must be 32 bits(also on 64bits processors)*/
	typedef unsigned int SQUnsignedInteger;
	typedef unsigned int SQHash; /*should be the same size of a pointer*/
#endif

#else

#ifdef _SQ64
#ifdef _MSC_VER
	typedef __int64 SQInteger;
	typedef unsigned __int64 SQUnsignedInteger;
	typedef unsigned __int64 SQHash; /*should be the same size of a pointer*/
#else
	typedef long SQInteger;
	typedef unsigned long SQUnsignedInteger;
	typedef unsigned long SQHash; /*should be the same size of a pointer*/
#endif
	typedef int SQInt32;
#else 
	typedef int SQInteger;
	typedef int SQInt32; /*must be 32 bits(also on 64bits processors)*/
	typedef unsigned int SQUnsignedInteger;
	typedef unsigned int SQHash; /*should be the same size of a pointer*/
#endif

#endif

#ifdef SQUSEDOUBLE
typedef double SQFloat;
#else
typedef float SQFloat;
#endif

#if defined(SQUSEDOUBLE) && !defined(_SQ64)
#ifdef _MSC_VER
typedef __int64 SQRawObjectVal; //must be 64bits
#else
typedef long SQRawObjectVal; //must be 64bits
#endif
#define SQ_OBJECT_RAWINIT() { this->_unVal.raw = 0; }
#else
typedef SQUnsignedInteger SQRawObjectVal; //is 32 bits on 32 bits builds and 64 bits otherwise
#define SQ_OBJECT_RAWINIT()
#endif

typedef void* SQUserPointer;
typedef SQUnsignedInteger SQBool;
typedef SQInteger SQRESULT;

#define SQTrue	(1)
#define SQFalse	(0)

template <Squirk Q>
struct SQVM;
template <Squirk Q>
struct SQTable;
template <Squirk Q>
struct SQArray;
template <Squirk Q>
struct SQString;
template <Squirk Q>
struct SQClosure;
template <Squirk Q>
struct SQGenerator;
template <Squirk Q>
struct SQNativeClosure;
template <Squirk Q>
struct SQUserData;
template <Squirk Q>
struct SQFunctionProto;
template <Squirk Q>
struct SQRefCounted;
template <Squirk Q>
struct SQClass;
template <Squirk Q>
struct SQInstance;
template <Squirk Q>
struct SQDelegable;

#ifdef _UNICODE
#define SQUNICODE
#endif

#ifdef SQUNICODE
#if (defined(_MSC_VER) && _MSC_VER >= 1400) // 1400 = VS8

#if defined(wchar_t) //this is if the compiler considers wchar_t as native type
#define wchar_t unsigned short
#endif

#else
typedef unsigned short wchar_t;
#endif

typedef wchar_t SQChar;
#define _SC(a) L##a
#define	scstrcmp	wcscmp
#define scsprintf	swprintf
#define scstrlen	wcslen
#define scstrtod	wcstod
#define scstrtol	wcstol
#define scatoi		_wtoi
#define scstrtoul	wcstoul
#define scvsprintf	vswprintf
#define scstrstr	wcsstr
#define scisspace	iswspace
#define scisdigit	iswdigit
#define scisxdigit	iswxdigit
#define scisalpha	iswalpha
#define sciscntrl	iswcntrl
#define scisalnum	iswalnum
//#define scprintf	wprintf
#define scstrcspn	wcscspn
#define MAX_CHAR 0xFFFF
#else
typedef char SQChar;
#define _SC(a) a
#define	scstrcmp	strcmp
#define scsprintf	sprintf
#define scstrlen	strlen
#define scstrtod	strtod
#define scstrtol	strtol
#define scatoi		atoi
#define scstrtoul	strtoul
#define scvsprintf	vsprintf
#define scstrstr	strstr
#define scisspace	isspace
#define scisdigit	isdigit
#define scisxdigit	isxdigit
#define sciscntrl	iscntrl
#define scisalpha	isalpha
#define scisalnum	isalnum
//#define scprintf	printf
#define scstrcspn	strcspn
#define MAX_CHAR 0xFF
#endif

#define SQUIRREL_VERSION	_SC("Squirrel 2.2.4 stable")
#define SQUIRREL_COPYRIGHT	_SC("Copyright (C) 2003-2009 Alberto Demichelis")
#define SQUIRREL_AUTHOR		_SC("Alberto Demichelis")

#define SQ_VMSTATE_IDLE			0
#define SQ_VMSTATE_RUNNING		1
#define SQ_VMSTATE_SUSPENDED	2

#define SQUIRREL_EOB 0
#define SQ_BYTECODE_STREAM_TAG	0xFAFA

#define SQOBJECT_REF_COUNTED	0x08000000
#define SQOBJECT_NUMERIC		0x04000000
#define SQOBJECT_DELEGABLE		0x02000000
#define SQOBJECT_CANBEFALSE		0x01000000

#define SQ_MATCHTYPEMASKSTRING (-99999)

#define _RT_MASK 0x00FFFFFF
#define _RAW_TYPE(type) (type&_RT_MASK)

#define _RT_NULL			0x00000001
#define _RT_INTEGER			0x00000002
#define _RT_FLOAT			0x00000004
#define _RT_BOOL			0x00000008
#define _RT_STRING			0x00000010
#define _RT_TABLE			0x00000020
#define _RT_ARRAY			0x00000040
#define _RT_USERDATA		0x00000080
#define _RT_CLOSURE			0x00000100
#define _RT_NATIVECLOSURE	0x00000200
#define _RT_GENERATOR		0x00000400
#define _RT_USERPOINTER		0x00000800
#define _RT_THREAD			0x00001000
#define _RT_FUNCPROTO		0x00002000
#define _RT_CLASS			0x00004000
#define _RT_INSTANCE		0x00008000
#define _RT_WEAKREF			0x00010000

typedef enum tagSQObjectType{
	OT_NULL =			(_RT_NULL|SQOBJECT_CANBEFALSE),
	OT_INTEGER =		(_RT_INTEGER|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_FLOAT =			(_RT_FLOAT|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_BOOL =			(_RT_BOOL|SQOBJECT_CANBEFALSE),
	OT_STRING =			(_RT_STRING|SQOBJECT_REF_COUNTED),
	OT_TABLE =			(_RT_TABLE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_ARRAY =			(_RT_ARRAY|SQOBJECT_REF_COUNTED),
	OT_USERDATA =		(_RT_USERDATA|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_CLOSURE =		(_RT_CLOSURE|SQOBJECT_REF_COUNTED),
	OT_NATIVECLOSURE =	(_RT_NATIVECLOSURE|SQOBJECT_REF_COUNTED),
	OT_GENERATOR =		(_RT_GENERATOR|SQOBJECT_REF_COUNTED),
	OT_USERPOINTER =	_RT_USERPOINTER,
	OT_THREAD =			(_RT_THREAD|SQOBJECT_REF_COUNTED) ,
	OT_FUNCPROTO =		(_RT_FUNCPROTO|SQOBJECT_REF_COUNTED), //internal usage only
	OT_CLASS =			(_RT_CLASS|SQOBJECT_REF_COUNTED),
	OT_INSTANCE =		(_RT_INSTANCE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_WEAKREF =		(_RT_WEAKREF|SQOBJECT_REF_COUNTED)
}SQObjectType;

#define ISREFCOUNTED(t) (t&SQOBJECT_REF_COUNTED)

template <Squirk Q>
struct SQWeakRef;

template <Squirk Q>
union tagSQObjectValue
{
	struct SQTable<Q> *pTable;
	struct SQArray<Q> *pArray;
	struct SQClosure<Q> *pClosure;
	struct SQGenerator<Q> *pGenerator;
	struct SQNativeClosure<Q> *pNativeClosure;
	struct SQString<Q> *pString;
	struct SQUserData<Q> *pUserData;
	SQInteger nInteger;
	SQFloat fFloat;
	SQUserPointer pUserPointer;
	struct SQFunctionProto<Q> *pFunctionProto;
	struct SQRefCounted<Q> *pRefCounted;
	struct SQDelegable<Q> *pDelegable;
	struct SQVM<Q> *pThread;
	struct SQClass<Q> *pClass;
	struct SQInstance<Q> *pInstance;
	struct SQWeakRef<Q> *pWeakRef;
	SQRawObjectVal raw;
};
template <Squirk Q>
using SQObjectValue = tagSQObjectValue<Q>;

template <Squirk Q>
struct tagSQObject
{
	SQObjectType _type;
	SQObjectValue<Q> _unVal;
};
template <>
struct __declspec(align(sizeof(int64_t))) tagSQObject<Squirk::AlignObject>
{
	SQObjectType _type;
	SQObjectValue<Squirk::AlignObject> __declspec(align(sizeof(int64_t))) _unVal;
};
template <>
struct __declspec(align(sizeof(int64_t))) tagSQObject<Squirk::AlignObjectShared>
{
	SQObjectType _type;
	SQObjectValue<Squirk::AlignObjectShared> __declspec(align(sizeof(int64_t))) _unVal;
};
template <Squirk Q>
using SQObject = tagSQObject<Q>;
template SQObject<Squirk::Standard>;

typedef struct  tagSQMemberHandle {
	SQBool _static;
	SQInteger _index;
}SQMemberHandle;

typedef struct tagSQStackInfos{
	const SQChar* funcname;
	const SQChar* source;
	SQInteger line;
}SQStackInfos;

template <Squirk Q>
using HSQUIRRELVM = SQVM<Q>*;

template <Squirk Q>
using HSQOBJECT = SQObject<Q>;
typedef SQMemberHandle HSQMEMBERHANDLE;
template <Squirk Q>
using SQFUNCTION = SQInteger(*)(HSQUIRRELVM<Q>);
typedef SQInteger (*SQRELEASEHOOK)(SQUserPointer,SQInteger size);
template <Squirk Q>
using SQCOMPILERERROR = void(*)(HSQUIRRELVM<Q>, const SQChar * /*desc*/, const SQChar * /*source*/, SQInteger /*line*/, SQInteger /*column*/);
template <Squirk Q>
using SQPRINTFUNCTION = void(*)(HSQUIRRELVM<Q>, const SQChar *, ...);

typedef SQInteger (*SQWRITEFUNC)(SQUserPointer,SQUserPointer,SQInteger);
typedef SQInteger (*SQREADFUNC)(SQUserPointer,SQUserPointer,SQInteger);

typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer);

template <Squirk Q>
struct tagSQRegFunction{
	const SQChar *name;
	SQFUNCTION<Q> f;
	SQInteger nparamscheck;
	const SQChar *typemask;
};
template <Squirk Q>
using SQRegFunction = tagSQRegFunction<Q>;

typedef struct tagSQFunctionInfo {
	SQUserPointer funcid;
	const SQChar *name;
	const SQChar *source;
} SQFunctionInfo;

/*vm*/
template <Squirk Q> SQUIRREL_API HSQUIRRELVM<Q> sq_open(SQInteger initialstacksize);
template <Squirk Q> SQUIRREL_API HSQUIRRELVM<Q> sq_newthread(HSQUIRRELVM<Q> friendvm, SQInteger initialstacksize);
template <Squirk Q> SQUIRREL_API void sq_seterrorhandler(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_close(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_setforeignptr(HSQUIRRELVM<Q> v, SQUserPointer p);
template <Squirk Q> SQUIRREL_API SQUserPointer sq_getforeignptr(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_setprintfunc(HSQUIRRELVM<Q> v, SQPRINTFUNCTION<Q> printfunc);
template <Squirk Q> SQUIRREL_API SQPRINTFUNCTION<Q> sq_getprintfunc(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API SQRESULT sq_suspendvm(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API SQRESULT sq_wakeupvm(HSQUIRRELVM<Q> v, SQBool resumedret, SQBool retval, SQBool raiseerror, SQBool throwerror);
template <Squirk Q> SQUIRREL_API SQInteger sq_getvmstate(HSQUIRRELVM<Q> v);

/*compiler*/
template <Squirk Q> SQUIRREL_API SQRESULT sq_compile(HSQUIRRELVM<Q> v, SQLEXREADFUNC read, SQUserPointer p, const SQChar *sourcename, SQBool raiseerror);
template <Squirk Q> SQUIRREL_API SQRESULT sq_compilebuffer(HSQUIRRELVM<Q> v, const SQChar *s, SQInteger size, const SQChar *sourcename, SQBool raiseerror);
template <Squirk Q> SQUIRREL_API void sq_enabledebuginfo(HSQUIRRELVM<Q> v, SQBool enable);
template <Squirk Q> SQUIRREL_API void sq_notifyallexceptions(HSQUIRRELVM<Q> v, SQBool enable);
template <Squirk Q> SQUIRREL_API void sq_setcompilererrorhandler(HSQUIRRELVM<Q> v, SQCOMPILERERROR<Q> f);

/*stack operations*/
template <Squirk Q> SQUIRREL_API void sq_push(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API void sq_pop(HSQUIRRELVM<Q> v, SQInteger nelemstopop);
template <Squirk Q> SQUIRREL_API void sq_poptop(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_remove(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQInteger sq_gettop(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_settop(HSQUIRRELVM<Q> v, SQInteger newtop);
template <Squirk Q> SQUIRREL_API void sq_reservestack(HSQUIRRELVM<Q> v, SQInteger nsize);
template <Squirk Q> SQUIRREL_API SQInteger sq_cmp(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_move(HSQUIRRELVM<Q> dest, HSQUIRRELVM<Q> src, SQInteger idx);

/*object creation handling*/
template <Squirk Q> SQUIRREL_API SQUserPointer sq_newuserdata(HSQUIRRELVM<Q> v, SQUnsignedInteger size);
template <Squirk Q> SQUIRREL_API void sq_newtable(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_newarray(HSQUIRRELVM<Q> v, SQInteger size);
template <Squirk Q> SQUIRREL_API void sq_newclosure(HSQUIRRELVM<Q> v, SQFUNCTION<Q> func, SQUnsignedInteger nfreevars);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setparamscheck(HSQUIRRELVM<Q> v, SQInteger nparamscheck, const SQChar *typemask);
template <Squirk Q> SQUIRREL_API SQRESULT sq_bindenv(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API void sq_pushstring(HSQUIRRELVM<Q> v, const SQChar *s, SQInteger len);
template <Squirk Q> SQUIRREL_API void sq_pushfloat(HSQUIRRELVM<Q> v, SQFloat f);
template <Squirk Q> SQUIRREL_API void sq_pushinteger(HSQUIRRELVM<Q> v, SQInteger n);
template <Squirk Q> SQUIRREL_API void sq_pushbool(HSQUIRRELVM<Q> v, SQBool b);
template <Squirk Q> SQUIRREL_API void sq_pushuserpointer(HSQUIRRELVM<Q> v, SQUserPointer p);
template <Squirk Q> SQUIRREL_API void sq_pushnull(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API SQObjectType sq_gettype(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQInteger sq_getsize(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getbase(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQBool sq_instanceof(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_tostring(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API void sq_tobool(HSQUIRRELVM<Q> v, SQInteger idx, SQBool *b);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getstring(HSQUIRRELVM<Q> v, SQInteger idx, const SQChar **c);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getinteger(HSQUIRRELVM<Q> v, SQInteger idx, SQInteger *i);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getfloat(HSQUIRRELVM<Q> v, SQInteger idx, SQFloat *f);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getbool(HSQUIRRELVM<Q> v, SQInteger idx, SQBool *b);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getthread(HSQUIRRELVM<Q> v, SQInteger idx, HSQUIRRELVM<Q> *thread);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getuserpointer(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer *p);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getuserdata(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer *p, SQUserPointer *typetag);
template <Squirk Q> SQUIRREL_API SQRESULT sq_settypetag(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer typetag);
template <Squirk Q> SQUIRREL_API SQRESULT sq_gettypetag(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer *typetag);
template <Squirk Q> SQUIRREL_API void sq_setreleasehook(HSQUIRRELVM<Q> v, SQInteger idx, SQRELEASEHOOK hook);
template <Squirk Q> SQUIRREL_API SQChar *sq_getscratchpad(HSQUIRRELVM<Q> v, SQInteger minsize);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getfunctioninfo(HSQUIRRELVM<Q> v, SQInteger idx, SQFunctionInfo *fi);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getclosureinfo(HSQUIRRELVM<Q> v, SQInteger idx, SQUnsignedInteger *nparams, SQUnsignedInteger *nfreevars);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setnativeclosurename(HSQUIRRELVM<Q> v, SQInteger idx, const SQChar *name);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setinstanceup(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer p);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getinstanceup(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer *p, SQUserPointer typetag);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setclassudsize(HSQUIRRELVM<Q> v, SQInteger idx, SQInteger udsize);
template <Squirk Q> SQUIRREL_API SQRESULT sq_newclass(HSQUIRRELVM<Q> v, SQBool hasbase);
template <Squirk Q> SQUIRREL_API SQRESULT sq_createinstance(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setattributes(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getattributes(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getclass(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API void sq_weakref(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getdefaultdelegate(HSQUIRRELVM<Q> v, SQObjectType t);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getmemberhandle(HSQUIRRELVM<Q> v, SQInteger idx, HSQMEMBERHANDLE *handle);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getbyhandle(HSQUIRRELVM<Q> v, SQInteger idx, const HSQMEMBERHANDLE *handle);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setbyhandle(HSQUIRRELVM<Q> v, SQInteger idx, const HSQMEMBERHANDLE *handle);

/*object manipulation*/
template <Squirk Q> SQUIRREL_API void sq_pushroottable(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_pushregistrytable(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_pushconsttable(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setroottable(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setconsttable(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API SQRESULT sq_newslot(HSQUIRRELVM<Q> v, SQInteger idx, SQBool bstatic);
template <Squirk Q> SQUIRREL_API SQRESULT sq_deleteslot(HSQUIRRELVM<Q> v, SQInteger idx, SQBool pushval);
template <Squirk Q> SQUIRREL_API SQRESULT sq_set(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_get(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_rawget(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_rawset(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_rawdeleteslot(HSQUIRRELVM<Q> v, SQInteger idx, SQBool pushval);
template <Squirk Q> SQUIRREL_API SQRESULT sq_arrayappend(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_arraypop(HSQUIRRELVM<Q> v, SQInteger idx, SQBool pushval);
template <Squirk Q> SQUIRREL_API SQRESULT sq_arrayresize(HSQUIRRELVM<Q> v, SQInteger idx, SQInteger newsize);
template <Squirk Q> SQUIRREL_API SQRESULT sq_arrayreverse(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_arrayremove(HSQUIRRELVM<Q> v, SQInteger idx, SQInteger itemidx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_arrayinsert(HSQUIRRELVM<Q> v, SQInteger idx, SQInteger destpos);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setdelegate(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getdelegate(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_clone(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_setfreevariable(HSQUIRRELVM<Q> v, SQInteger idx, SQUnsignedInteger nval);
template <Squirk Q> SQUIRREL_API SQRESULT sq_next(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getweakrefval(HSQUIRRELVM<Q> v, SQInteger idx);
template <Squirk Q> SQUIRREL_API SQRESULT sq_clear(HSQUIRRELVM<Q> v, SQInteger idx);

/*calls*/
template <Squirk Q> SQUIRREL_API SQRESULT sq_call(HSQUIRRELVM<Q> v, SQInteger params, SQBool retval, SQBool raiseerror);
template <Squirk Q> SQUIRREL_API SQRESULT sq_resume(HSQUIRRELVM<Q> v, SQBool retval, SQBool raiseerror);
template <Squirk Q> SQUIRREL_API const SQChar *sq_getlocal(HSQUIRRELVM<Q> v, SQUnsignedInteger level, SQUnsignedInteger idx);
template <Squirk Q> SQUIRREL_API const SQChar *sq_getfreevariable(HSQUIRRELVM<Q> v, SQInteger idx, SQUnsignedInteger nval);
template <Squirk Q> SQUIRREL_API SQRESULT sq_throwerror(HSQUIRRELVM<Q> v, const SQChar *err);
template <Squirk Q> SQUIRREL_API void sq_reseterror(HSQUIRRELVM<Q> v);
template <Squirk Q> SQUIRREL_API void sq_getlasterror(HSQUIRRELVM<Q> v);

/*raw object handling*/
template <Squirk Q> SQUIRREL_API SQRESULT sq_getstackobj(HSQUIRRELVM<Q> v, SQInteger idx, HSQOBJECT<Q> *po);
template <Squirk Q> SQUIRREL_API void sq_pushobject(HSQUIRRELVM<Q> v, HSQOBJECT<Q> obj);
template <Squirk Q> SQUIRREL_API void sq_addref(HSQUIRRELVM<Q> v, HSQOBJECT<Q> *po);
template <Squirk Q> SQUIRREL_API SQBool sq_release(HSQUIRRELVM<Q> v, HSQOBJECT<Q> *po);
template <Squirk Q> SQUIRREL_API void sq_resetobject(HSQOBJECT<Q> *po);
template <Squirk Q> SQUIRREL_API const SQChar *sq_objtostring(HSQOBJECT<Q> *o);
template <Squirk Q> SQUIRREL_API SQBool sq_objtobool(HSQOBJECT<Q> *o);
template <Squirk Q> SQUIRREL_API SQInteger sq_objtointeger(HSQOBJECT<Q> *o);
template <Squirk Q> SQUIRREL_API SQFloat sq_objtofloat(HSQOBJECT<Q> *o);
template <Squirk Q> SQUIRREL_API SQRESULT sq_getobjtypetag(HSQOBJECT<Q> *o, SQUserPointer * typetag);

/*GC*/
template <Squirk Q> SQUIRREL_API SQInteger sq_collectgarbage(HSQUIRRELVM<Q> v);

/*serialization*/
template <Squirk Q> SQUIRREL_API SQRESULT sq_writeclosure(HSQUIRRELVM<Q> vm, SQWRITEFUNC writef, SQUserPointer up);
template <Squirk Q> SQUIRREL_API SQRESULT sq_readclosure(HSQUIRRELVM<Q> vm, SQREADFUNC readf, SQUserPointer up);

/*debug*/
template <Squirk Q> SQUIRREL_API SQRESULT sq_stackinfos(HSQUIRRELVM<Q> v, SQInteger level, SQStackInfos *si);
template <Squirk Q> SQUIRREL_API void sq_setdebughook(HSQUIRRELVM<Q> v);

/*mem allocation*/
SQUIRREL_API void *sq_malloc(SQUnsignedInteger size);
SQUIRREL_API void *sq_realloc(void* p, SQUnsignedInteger oldsize, SQUnsignedInteger newsize);
SQUIRREL_API void sq_free(void *p, SQUnsignedInteger size);

/*UTILITY MACRO*/
#define sq_isnumeric(o) ((o)._type&SQOBJECT_NUMERIC)
#define sq_istable(o) ((o)._type==OT_TABLE)
#define sq_isarray(o) ((o)._type==OT_ARRAY)
#define sq_isfunction(o) ((o)._type==OT_FUNCPROTO)
#define sq_isclosure(o) ((o)._type==OT_CLOSURE)
#define sq_isgenerator(o) ((o)._type==OT_GENERATOR)
#define sq_isnativeclosure(o) ((o)._type==OT_NATIVECLOSURE)
#define sq_isstring(o) ((o)._type==OT_STRING)
#define sq_isinteger(o) ((o)._type==OT_INTEGER)
#define sq_isfloat(o) ((o)._type==OT_FLOAT)
#define sq_isuserpointer(o) ((o)._type==OT_USERPOINTER)
#define sq_isuserdata(o) ((o)._type==OT_USERDATA)
#define sq_isthread(o) ((o)._type==OT_THREAD)
#define sq_isnull(o) ((o)._type==OT_NULL)
#define sq_isclass(o) ((o)._type==OT_CLASS)
#define sq_isinstance(o) ((o)._type==OT_INSTANCE)
#define sq_isbool(o) ((o)._type==OT_BOOL)
#define sq_isweakref(o) ((o)._type==OT_WEAKREF)
#define sq_type(o) ((o)._type)

/* deprecated */
#define sq_createslot(v,n) sq_newslot(v,n,SQFalse)

#define SQ_OK (0)
#define SQ_ERROR (-1)

#define SQ_FAILED(res) (res<0)
#define SQ_SUCCEEDED(res) (res>=0)

#endif /*_SQUIRREL_H_*/
