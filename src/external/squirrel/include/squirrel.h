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

#include <loguru.hpp>
#define scprintf(...) LOG_F(INFO, __VA_ARGS__)

#ifndef SQUIRREL_API
#define SQUIRREL_API extern
#endif

enum class Squirk {
	Standard,
	AlignObject,
};

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

template <Squirk T>
struct SQVM;
template <Squirk T>
struct SQTable;
template <Squirk T>
struct SQArray;
template <Squirk T>
struct SQString;
template <Squirk T>
struct SQClosure;
template <Squirk T>
struct SQGenerator;
template <Squirk T>
struct SQNativeClosure;
template <Squirk T>
struct SQUserData;
template <Squirk T>
struct SQFunctionProto;
template <Squirk T>
struct SQRefCounted;
template <Squirk T>
struct SQClass;
template <Squirk T>
struct SQInstance;
template <Squirk T>
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

template <Squirk T>
struct SQWeakRef;

template <Squirk T>
union tagSQObjectValue
{
	struct SQTable<T> *pTable;
	struct SQArray<T> *pArray;
	struct SQClosure<T> *pClosure;
	struct SQGenerator<T> *pGenerator;
	struct SQNativeClosure<T> *pNativeClosure;
	struct SQString<T> *pString;
	struct SQUserData<T> *pUserData;
	SQInteger nInteger;
	SQFloat fFloat;
	SQUserPointer pUserPointer;
	struct SQFunctionProto<T> *pFunctionProto;
	struct SQRefCounted<T> *pRefCounted;
	struct SQDelegable<T> *pDelegable;
	struct SQVM<T> *pThread;
	struct SQClass<T> *pClass;
	struct SQInstance<T> *pInstance;
	struct SQWeakRef<T> *pWeakRef;
	SQRawObjectVal raw;
};
template <Squirk T>
using SQObjectValue = tagSQObjectValue<T>;

template <Squirk T>
struct tagSQObject
{
	SQObjectType _type;
	SQObjectValue<T> _unVal;
};
template <>
struct __declspec(align(sizeof(int64_t))) tagSQObject<Squirk::AlignObject>
{
	SQObjectType _type;
	SQObjectValue<Squirk::AlignObject> __declspec(align(sizeof(int64_t))) _unVal;
};
template <Squirk T>
using SQObject = tagSQObject<T>;
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

template <Squirk T>
using HSQUIRRELVM = SQVM<T>*;

template <Squirk T>
using HSQOBJECT = SQObject<T>;
typedef SQMemberHandle HSQMEMBERHANDLE;
template <Squirk T>
using SQFUNCTION = SQInteger(*)(HSQUIRRELVM<T>);
typedef SQInteger (*SQRELEASEHOOK)(SQUserPointer,SQInteger size);
template <Squirk T>
using SQCOMPILERERROR = void(*)(HSQUIRRELVM<T>, const SQChar * /*desc*/, const SQChar * /*source*/, SQInteger /*line*/, SQInteger /*column*/);
template <Squirk T>
using SQPRINTFUNCTION = void(*)(HSQUIRRELVM<T>, const SQChar *, ...);

typedef SQInteger (*SQWRITEFUNC)(SQUserPointer,SQUserPointer,SQInteger);
typedef SQInteger (*SQREADFUNC)(SQUserPointer,SQUserPointer,SQInteger);

typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer);

template <Squirk T>
struct tagSQRegFunction{
	const SQChar *name;
	SQFUNCTION<T> f;
	SQInteger nparamscheck;
	const SQChar *typemask;
};
template <Squirk T>
using SQRegFunction = tagSQRegFunction<T>;

typedef struct tagSQFunctionInfo {
	SQUserPointer funcid;
	const SQChar *name;
	const SQChar *source;
} SQFunctionInfo;

/*vm*/
template <Squirk T> SQUIRREL_API HSQUIRRELVM<T> sq_open(SQInteger initialstacksize);
template <Squirk T> SQUIRREL_API HSQUIRRELVM<T> sq_newthread(HSQUIRRELVM<T> friendvm, SQInteger initialstacksize);
template <Squirk T> SQUIRREL_API void sq_seterrorhandler(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_close(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_setforeignptr(HSQUIRRELVM<T> v, SQUserPointer p);
template <Squirk T> SQUIRREL_API SQUserPointer sq_getforeignptr(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_setprintfunc(HSQUIRRELVM<T> v, SQPRINTFUNCTION<T> printfunc);
template <Squirk T> SQUIRREL_API SQPRINTFUNCTION<T> sq_getprintfunc(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API SQRESULT sq_suspendvm(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API SQRESULT sq_wakeupvm(HSQUIRRELVM<T> v, SQBool resumedret, SQBool retval, SQBool raiseerror, SQBool throwerror);
template <Squirk T> SQUIRREL_API SQInteger sq_getvmstate(HSQUIRRELVM<T> v);

/*compiler*/
template <Squirk T> SQUIRREL_API SQRESULT sq_compile(HSQUIRRELVM<T> v, SQLEXREADFUNC read, SQUserPointer p, const SQChar *sourcename, SQBool raiseerror);
template <Squirk T> SQUIRREL_API SQRESULT sq_compilebuffer(HSQUIRRELVM<T> v, const SQChar *s, SQInteger size, const SQChar *sourcename, SQBool raiseerror);
template <Squirk T> SQUIRREL_API void sq_enabledebuginfo(HSQUIRRELVM<T> v, SQBool enable);
template <Squirk T> SQUIRREL_API void sq_notifyallexceptions(HSQUIRRELVM<T> v, SQBool enable);
template <Squirk T> SQUIRREL_API void sq_setcompilererrorhandler(HSQUIRRELVM<T> v, SQCOMPILERERROR<T> f);

/*stack operations*/
template <Squirk T> SQUIRREL_API void sq_push(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API void sq_pop(HSQUIRRELVM<T> v, SQInteger nelemstopop);
template <Squirk T> SQUIRREL_API void sq_poptop(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_remove(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQInteger sq_gettop(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_settop(HSQUIRRELVM<T> v, SQInteger newtop);
template <Squirk T> SQUIRREL_API void sq_reservestack(HSQUIRRELVM<T> v, SQInteger nsize);
template <Squirk T> SQUIRREL_API SQInteger sq_cmp(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_move(HSQUIRRELVM<T> dest, HSQUIRRELVM<T> src, SQInteger idx);

/*object creation handling*/
template <Squirk T> SQUIRREL_API SQUserPointer sq_newuserdata(HSQUIRRELVM<T> v, SQUnsignedInteger size);
template <Squirk T> SQUIRREL_API void sq_newtable(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_newarray(HSQUIRRELVM<T> v, SQInteger size);
template <Squirk T> SQUIRREL_API void sq_newclosure(HSQUIRRELVM<T> v, SQFUNCTION<T> func, SQUnsignedInteger nfreevars);
template <Squirk T> SQUIRREL_API SQRESULT sq_setparamscheck(HSQUIRRELVM<T> v, SQInteger nparamscheck, const SQChar *typemask);
template <Squirk T> SQUIRREL_API SQRESULT sq_bindenv(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API void sq_pushstring(HSQUIRRELVM<T> v, const SQChar *s, SQInteger len);
template <Squirk T> SQUIRREL_API void sq_pushfloat(HSQUIRRELVM<T> v, SQFloat f);
template <Squirk T> SQUIRREL_API void sq_pushinteger(HSQUIRRELVM<T> v, SQInteger n);
template <Squirk T> SQUIRREL_API void sq_pushbool(HSQUIRRELVM<T> v, SQBool b);
template <Squirk T> SQUIRREL_API void sq_pushuserpointer(HSQUIRRELVM<T> v, SQUserPointer p);
template <Squirk T> SQUIRREL_API void sq_pushnull(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API SQObjectType sq_gettype(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQInteger sq_getsize(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_getbase(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQBool sq_instanceof(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_tostring(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API void sq_tobool(HSQUIRRELVM<T> v, SQInteger idx, SQBool *b);
template <Squirk T> SQUIRREL_API SQRESULT sq_getstring(HSQUIRRELVM<T> v, SQInteger idx, const SQChar **c);
template <Squirk T> SQUIRREL_API SQRESULT sq_getinteger(HSQUIRRELVM<T> v, SQInteger idx, SQInteger *i);
template <Squirk T> SQUIRREL_API SQRESULT sq_getfloat(HSQUIRRELVM<T> v, SQInteger idx, SQFloat *f);
template <Squirk T> SQUIRREL_API SQRESULT sq_getbool(HSQUIRRELVM<T> v, SQInteger idx, SQBool *b);
template <Squirk T> SQUIRREL_API SQRESULT sq_getthread(HSQUIRRELVM<T> v, SQInteger idx, HSQUIRRELVM<T> *thread);
template <Squirk T> SQUIRREL_API SQRESULT sq_getuserpointer(HSQUIRRELVM<T> v, SQInteger idx, SQUserPointer *p);
template <Squirk T> SQUIRREL_API SQRESULT sq_getuserdata(HSQUIRRELVM<T> v, SQInteger idx, SQUserPointer *p, SQUserPointer *typetag);
template <Squirk T> SQUIRREL_API SQRESULT sq_settypetag(HSQUIRRELVM<T> v, SQInteger idx, SQUserPointer typetag);
template <Squirk T> SQUIRREL_API SQRESULT sq_gettypetag(HSQUIRRELVM<T> v, SQInteger idx, SQUserPointer *typetag);
template <Squirk T> SQUIRREL_API void sq_setreleasehook(HSQUIRRELVM<T> v, SQInteger idx, SQRELEASEHOOK hook);
template <Squirk T> SQUIRREL_API SQChar *sq_getscratchpad(HSQUIRRELVM<T> v, SQInteger minsize);
template <Squirk T> SQUIRREL_API SQRESULT sq_getfunctioninfo(HSQUIRRELVM<T> v, SQInteger idx, SQFunctionInfo *fi);
template <Squirk T> SQUIRREL_API SQRESULT sq_getclosureinfo(HSQUIRRELVM<T> v, SQInteger idx, SQUnsignedInteger *nparams, SQUnsignedInteger *nfreevars);
template <Squirk T> SQUIRREL_API SQRESULT sq_setnativeclosurename(HSQUIRRELVM<T> v, SQInteger idx, const SQChar *name);
template <Squirk T> SQUIRREL_API SQRESULT sq_setinstanceup(HSQUIRRELVM<T> v, SQInteger idx, SQUserPointer p);
template <Squirk T> SQUIRREL_API SQRESULT sq_getinstanceup(HSQUIRRELVM<T> v, SQInteger idx, SQUserPointer *p, SQUserPointer typetag);
template <Squirk T> SQUIRREL_API SQRESULT sq_setclassudsize(HSQUIRRELVM<T> v, SQInteger idx, SQInteger udsize);
template <Squirk T> SQUIRREL_API SQRESULT sq_newclass(HSQUIRRELVM<T> v, SQBool hasbase);
template <Squirk T> SQUIRREL_API SQRESULT sq_createinstance(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_setattributes(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_getattributes(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_getclass(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API void sq_weakref(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_getdefaultdelegate(HSQUIRRELVM<T> v, SQObjectType t);
template <Squirk T> SQUIRREL_API SQRESULT sq_getmemberhandle(HSQUIRRELVM<T> v, SQInteger idx, HSQMEMBERHANDLE *handle);
template <Squirk T> SQUIRREL_API SQRESULT sq_getbyhandle(HSQUIRRELVM<T> v, SQInteger idx, const HSQMEMBERHANDLE *handle);
template <Squirk T> SQUIRREL_API SQRESULT sq_setbyhandle(HSQUIRRELVM<T> v, SQInteger idx, const HSQMEMBERHANDLE *handle);

/*object manipulation*/
template <Squirk T> SQUIRREL_API void sq_pushroottable(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_pushregistrytable(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_pushconsttable(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API SQRESULT sq_setroottable(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API SQRESULT sq_setconsttable(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API SQRESULT sq_newslot(HSQUIRRELVM<T> v, SQInteger idx, SQBool bstatic);
template <Squirk T> SQUIRREL_API SQRESULT sq_deleteslot(HSQUIRRELVM<T> v, SQInteger idx, SQBool pushval);
template <Squirk T> SQUIRREL_API SQRESULT sq_set(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_get(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_rawget(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_rawset(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_rawdeleteslot(HSQUIRRELVM<T> v, SQInteger idx, SQBool pushval);
template <Squirk T> SQUIRREL_API SQRESULT sq_arrayappend(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_arraypop(HSQUIRRELVM<T> v, SQInteger idx, SQBool pushval);
template <Squirk T> SQUIRREL_API SQRESULT sq_arrayresize(HSQUIRRELVM<T> v, SQInteger idx, SQInteger newsize);
template <Squirk T> SQUIRREL_API SQRESULT sq_arrayreverse(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_arrayremove(HSQUIRRELVM<T> v, SQInteger idx, SQInteger itemidx);
template <Squirk T> SQUIRREL_API SQRESULT sq_arrayinsert(HSQUIRRELVM<T> v, SQInteger idx, SQInteger destpos);
template <Squirk T> SQUIRREL_API SQRESULT sq_setdelegate(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_getdelegate(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_clone(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_setfreevariable(HSQUIRRELVM<T> v, SQInteger idx, SQUnsignedInteger nval);
template <Squirk T> SQUIRREL_API SQRESULT sq_next(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_getweakrefval(HSQUIRRELVM<T> v, SQInteger idx);
template <Squirk T> SQUIRREL_API SQRESULT sq_clear(HSQUIRRELVM<T> v, SQInteger idx);

/*calls*/
template <Squirk T> SQUIRREL_API SQRESULT sq_call(HSQUIRRELVM<T> v, SQInteger params, SQBool retval, SQBool raiseerror);
template <Squirk T> SQUIRREL_API SQRESULT sq_resume(HSQUIRRELVM<T> v, SQBool retval, SQBool raiseerror);
template <Squirk T> SQUIRREL_API const SQChar *sq_getlocal(HSQUIRRELVM<T> v, SQUnsignedInteger level, SQUnsignedInteger idx);
template <Squirk T> SQUIRREL_API const SQChar *sq_getfreevariable(HSQUIRRELVM<T> v, SQInteger idx, SQUnsignedInteger nval);
template <Squirk T> SQUIRREL_API SQRESULT sq_throwerror(HSQUIRRELVM<T> v, const SQChar *err);
template <Squirk T> SQUIRREL_API void sq_reseterror(HSQUIRRELVM<T> v);
template <Squirk T> SQUIRREL_API void sq_getlasterror(HSQUIRRELVM<T> v);

/*raw object handling*/
template <Squirk T> SQUIRREL_API SQRESULT sq_getstackobj(HSQUIRRELVM<T> v, SQInteger idx, HSQOBJECT<T> *po);
template <Squirk T> SQUIRREL_API void sq_pushobject(HSQUIRRELVM<T> v, HSQOBJECT<T> obj);
template <Squirk T> SQUIRREL_API void sq_addref(HSQUIRRELVM<T> v, HSQOBJECT<T> *po);
template <Squirk T> SQUIRREL_API SQBool sq_release(HSQUIRRELVM<T> v, HSQOBJECT<T> *po);
template <Squirk T> SQUIRREL_API void sq_resetobject(HSQOBJECT<T> *po);
template <Squirk T> SQUIRREL_API const SQChar *sq_objtostring(HSQOBJECT<T> *o);
template <Squirk T> SQUIRREL_API SQBool sq_objtobool(HSQOBJECT<T> *o);
template <Squirk T> SQUIRREL_API SQInteger sq_objtointeger(HSQOBJECT<T> *o);
template <Squirk T> SQUIRREL_API SQFloat sq_objtofloat(HSQOBJECT<T> *o);
template <Squirk T> SQUIRREL_API SQRESULT sq_getobjtypetag(HSQOBJECT<T> *o, SQUserPointer * typetag);

/*GC*/
template <Squirk T> SQUIRREL_API SQInteger sq_collectgarbage(HSQUIRRELVM<T> v);

/*serialization*/
template <Squirk T> SQUIRREL_API SQRESULT sq_writeclosure(HSQUIRRELVM<T> vm, SQWRITEFUNC writef, SQUserPointer up);
template <Squirk T> SQUIRREL_API SQRESULT sq_readclosure(HSQUIRRELVM<T> vm, SQREADFUNC readf, SQUserPointer up);

/*debug*/
template <Squirk T> SQUIRREL_API SQRESULT sq_stackinfos(HSQUIRRELVM<T> v, SQInteger level, SQStackInfos *si);
template <Squirk T> SQUIRREL_API void sq_setdebughook(HSQUIRRELVM<T> v);

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
