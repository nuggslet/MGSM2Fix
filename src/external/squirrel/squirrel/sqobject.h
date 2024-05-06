/*	see copyright notice in squirrel.h */
#ifndef _SQOBJECT_H_
#define _SQOBJECT_H_

#include "squtils.h"

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

template <Squirk T>
struct SQSharedState;

enum SQMetaMethod{
	MT_ADD=0,
	MT_SUB=1,
	MT_MUL=2,
	MT_DIV=3,
	MT_UNM=4,
	MT_MODULO=5,
	MT_SET=6,
	MT_GET=7,
	MT_TYPEOF=8,
	MT_NEXTI=9,
	MT_CMP=10,
	MT_CALL=11,
	MT_CLONED=12,
	MT_NEWSLOT=13,
	MT_DELSLOT=14,
	MT_TOSTRING=15,
	MT_NEWMEMBER=16,
	MT_INHERITED=17,
	MT_LAST = 18
};

#define MM_ADD		_SC("_add")
#define MM_SUB		_SC("_sub")
#define MM_MUL		_SC("_mul")
#define MM_DIV		_SC("_div")
#define MM_UNM		_SC("_unm")
#define MM_MODULO	_SC("_modulo")
#define MM_SET		_SC("_set")
#define MM_GET		_SC("_get")
#define MM_TYPEOF	_SC("_typeof")
#define MM_NEXTI	_SC("_nexti")
#define MM_CMP		_SC("_cmp")
#define MM_CALL		_SC("_call")
#define MM_CLONED	_SC("_cloned")
#define MM_NEWSLOT	_SC("_newslot")
#define MM_DELSLOT	_SC("_delslot")
#define MM_TOSTRING	_SC("_tostring")
#define MM_NEWMEMBER _SC("_newmember")
#define MM_INHERITED _SC("_inherited")

#define MINPOWER2 4

template <Squirk T>
struct SQWeakRef;

template <Squirk T>
struct SQRefCounted
{
	SQRefCounted() { _uiRef = 0; _weakref = NULL; }
	virtual ~SQRefCounted();
	struct SQWeakRef<T> *GetWeakRef(SQObjectType type);
	SQUnsignedInteger _uiRef;
	struct SQWeakRef<T> *_weakref;
	virtual void Release()=0;
};

template <Squirk T>
struct SQWeakRef : SQRefCounted<T>
{
	void Release();
	SQObject<T> _obj;
};

#define _realval(o) (type((o)) != OT_WEAKREF?o:_weakref(o)->_obj)

template <Squirk T>
struct SQObjectPtr;

#define __AddRef(type,unval) if(ISREFCOUNTED(type))	\
		{ \
			unval.pRefCounted->_uiRef++; \
		}  

#define __Release(type,unval) if(ISREFCOUNTED(type) && ((--unval.pRefCounted->_uiRef)<=0))	\
		{	\
			unval.pRefCounted->Release();	\
		}

#define __ObjRelease(obj) { \
	if((obj)) {	\
		(obj)->_uiRef--; \
		if((obj)->_uiRef == 0) \
			(obj)->Release(); \
		(obj) = NULL;	\
	} \
}

#define __ObjAddRef(obj) { \
	(obj)->_uiRef++; \
}

#define type(obj) ((obj)._type)
#define is_delegable(t) (type(t)&SQOBJECT_DELEGABLE)
#define raw_type(obj) _RAW_TYPE((obj)._type)

#define _integer(obj) ((obj)._unVal.nInteger)
#define _float(obj) ((obj)._unVal.fFloat)
#define _string(obj) ((obj)._unVal.pString)
#define _table(obj) ((obj)._unVal.pTable)
#define _array(obj) ((obj)._unVal.pArray)
#define _closure(obj) ((obj)._unVal.pClosure)
#define _generator(obj) ((obj)._unVal.pGenerator)
#define _nativeclosure(obj) ((obj)._unVal.pNativeClosure)
#define _userdata(obj) ((obj)._unVal.pUserData)
#define _userpointer(obj) ((obj)._unVal.pUserPointer)
#define _thread(obj) ((obj)._unVal.pThread)
#define _funcproto(obj) ((obj)._unVal.pFunctionProto)
#define _class(obj) ((obj)._unVal.pClass)
#define _instance(obj) ((obj)._unVal.pInstance)
#define _delegable(obj) ((SQDelegable<T> *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) (obj)._unVal.pUserData->_val

#define tofloat(num) ((type(num)==OT_INTEGER)?(SQFloat)_integer(num):_float(num))
#define tointeger(num) ((type(num)==OT_FLOAT)?(SQInteger)_float(num):_integer(num))
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

template <Squirk T>
struct SQObjectPtr : public SQObject<T>
{
	SQObjectPtr()
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_NULL;
		this->_unVal.pUserPointer=NULL;
	}
	SQObjectPtr(const SQObjectPtr<T> &o)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=o._type;
		this->_unVal=o._unVal;
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(const SQObject<T> &o)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=o._type;
		this->_unVal=o._unVal;
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQTable<T> *pTable)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_TABLE;
		this->_unVal.pTable=pTable;
		assert(this->_unVal.pTable);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQClass<T> *pClass)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_CLASS;
		this->_unVal.pClass=pClass;
		assert(this->_unVal.pClass);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQInstance<T> *pInstance)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_INSTANCE;
		this->_unVal.pInstance=pInstance;
		assert(this->_unVal.pInstance);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQArray<T> *pArray)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_ARRAY;
		this->_unVal.pArray=pArray;
		assert(this->_unVal.pArray);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQClosure<T> *pClosure)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_CLOSURE;
		this->_unVal.pClosure=pClosure;
		assert(this->_unVal.pClosure);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQGenerator<T> *pGenerator)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_GENERATOR;
		this->_unVal.pGenerator=pGenerator;
		assert(this->_unVal.pGenerator);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQNativeClosure<T> *pNativeClosure)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_NATIVECLOSURE;
		this->_unVal.pNativeClosure=pNativeClosure;
		assert(this->_unVal.pNativeClosure);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQString<T> *pString)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_STRING;
		this->_unVal.pString=pString;
		assert(this->_unVal.pString);
		__AddRef(this->_type, this->_unVal);
	}
	SQObjectPtr(SQUserData<T> *pUserData)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_USERDATA;
		this->_unVal.pUserData=pUserData;
		assert(this->_unVal.pUserData);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQVM<T> *pThread)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_THREAD;
		this->_unVal.pThread=pThread;
		assert(this->_unVal.pThread);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQWeakRef<T> *pWeakRef)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_WEAKREF;
		this->_unVal.pWeakRef=pWeakRef;
		assert(this->_unVal.pWeakRef);
		__AddRef(this->_type, this->_unVal);
	}
	SQObjectPtr(SQFunctionProto<T> *pFunctionProto)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_FUNCPROTO;
		this->_unVal.pFunctionProto=pFunctionProto;
		assert(this->_unVal.pFunctionProto);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQInteger nInteger)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_INTEGER;
		this->_unVal.nInteger=nInteger;
	}
	SQObjectPtr(SQFloat fFloat)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_FLOAT;
		this->_unVal.fFloat=fFloat;
	}
	SQObjectPtr(bool bBool)
	{
		SQ_OBJECT_RAWINIT()
		this->_type = OT_BOOL;
		this->_unVal.nInteger = bBool?1:0;
	}
	SQObjectPtr(SQUserPointer pUserPointer)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_USERPOINTER;
		this->_unVal.pUserPointer=pUserPointer;
	}
	~SQObjectPtr()
	{
		__Release(this->_type,this->_unVal);
	}
	inline void Null()
	{
		SQObjectType tOldType;
		SQObjectValue<T> unOldVal;
		tOldType =this->_type;
		unOldVal =this->_unVal;
		this->_type = OT_NULL;
		this->_unVal.pUserPointer = NULL;
		__Release(tOldType,unOldVal);
	}
	inline SQObjectPtr<T>& operator=(SQInteger i)
	{ 
		__Release(this->_type,this->_unVal);
		this->_unVal.nInteger = i;
		this->_type = OT_INTEGER;
		return *this;
	}
	inline SQObjectPtr<T>& operator=(SQFloat f)
	{ 
		__Release(this->_type,this->_unVal);
		this->_unVal.fFloat = f;
		this->_type = OT_FLOAT;
		return *this;
	}
	inline SQObjectPtr<T>& operator=(const SQObjectPtr<T>& obj)
	{ 
		SQObjectType tOldType;
		SQObjectValue<T> unOldVal;
		tOldType=this->_type;
		unOldVal=this->_unVal;
		this->_unVal = obj._unVal;
		this->_type = obj._type;
		__AddRef(this->_type,this->_unVal);
		__Release(tOldType,unOldVal);
		return *this;
	}
	inline SQObjectPtr<T>& operator=(const SQObject<T>& obj)
	{ 
		SQObjectType tOldType;
		SQObjectValue<T> unOldVal;
		tOldType=this->_type;
		unOldVal=this->_unVal;
		this->_unVal = obj._unVal;
		this->_type = obj._type;
		__AddRef(this->_type,this->_unVal);
		__Release(tOldType,unOldVal);
		return *this;
	}
	private:
		SQObjectPtr(const SQChar *){} //safety
};
/////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_GARBAGE_COLLECTOR
#define MARK_FLAG 0x80000000
template <Squirk T>
struct SQCollectable : public SQRefCounted<T> {
	SQCollectable<T> *_next;
	SQCollectable<T> *_prev;
	SQSharedState<T> *_sharedstate;
	virtual void Release()=0;
	virtual void Mark(SQCollectable<T> **chain)=0;
	void UnMark();
	virtual void Finalize()=0;
	static void AddToChain(SQCollectable<T> **chain,SQCollectable<T> *c);
	static void RemoveFromChain(SQCollectable<T> **chain,SQCollectable<T> *c);
};


#define ADD_TO_CHAIN(chain,obj) this->AddToChain(chain,obj)
#define REMOVE_FROM_CHAIN(chain,obj) {if(!(this->_uiRef&MARK_FLAG)) this->RemoveFromChain(chain,obj);}
#define CHAINABLE_OBJ SQCollectable
#define INIT_CHAIN() {this->_next=NULL;this->_prev=NULL;this->_sharedstate=ss;}
#else

#define ADD_TO_CHAIN(chain,obj) ((void)0)
#define REMOVE_FROM_CHAIN(chain,obj) ((void)0)
#define CHAINABLE_OBJ SQRefCounted
#define INIT_CHAIN() ((void)0)
#endif

template <Squirk T>
struct SQDelegable : public CHAINABLE_OBJ<T> {
	bool SetDelegate(SQTable<T> *m);
	virtual bool GetMetaMethod(SQVM<T> *v,SQMetaMethod mm,SQObjectPtr<T> &res);
	SQTable<T> *_delegate;
};

template <Squirk T>
SQUnsignedInteger TranslateIndex(const SQObjectPtr<T> &idx);
template <Squirk T>
using SQObjectPtrVec = sqvector<SQObjectPtr<T>>;
typedef sqvector<SQInteger> SQIntVec;
template <Squirk T>
const SQChar *GetTypeName(const SQObjectPtr<T> &obj1);
const SQChar *IdType2Name(SQObjectType type);

template SQRefCounted<Squirk::Standard>;
template SQRefCounted<Squirk::AlignObject>;

template SQWeakRef<Squirk::Standard>;
template SQWeakRef<Squirk::AlignObject>;

template SQObjectPtr<Squirk::Standard>;
template SQObjectPtr<Squirk::AlignObject>;

template SQCollectable<Squirk::Standard>;
template SQCollectable<Squirk::AlignObject>;

template SQDelegable<Squirk::Standard>;
template SQDelegable<Squirk::AlignObject>;

#endif //_SQOBJECT_H_
