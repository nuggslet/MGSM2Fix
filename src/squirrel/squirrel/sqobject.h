/*	see copyright notice in squirrel.h */
#ifndef _SQOBJECT_H_
#define _SQOBJECT_H_

#include "squtils.h"

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

template <Squirk Q>
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

template <Squirk Q>
struct SQWeakRef;

template <Squirk Q>
struct SQRefCounted
{
	SQRefCounted() { _uiRef = 0; _weakref = NULL; }
	virtual ~SQRefCounted();
	struct SQWeakRef<Q> *GetWeakRef(SQObjectType type);
	SQUnsignedInteger _uiRef;
	struct SQWeakRef<Q> *_weakref;
	virtual void Release()=0;
};

template <Squirk Q>
struct SQWeakRef : SQRefCounted<Q>
{
	void Release();
	SQObject<Q> _obj;
};

#define _realval(o) (obj_type((o)) != OT_WEAKREF?o:_weakref(o)->_obj)

template <Squirk Q>
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

#define obj_type(obj) ((obj)._type)
#define is_delegable(t) (obj_type(t)&SQOBJECT_DELEGABLE)
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
#define _delegable(obj) ((SQDelegable<Q> *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) (obj)._unVal.pUserData->_val

#define tofloat(num) ((obj_type(num)==OT_INTEGER)?(SQFloat)_integer(num):_float(num))
#define tointeger(num) ((obj_type(num)==OT_FLOAT)?(SQInteger)_float(num):_integer(num))
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

template <Squirk Q>
struct SQObjectPtr : public SQObject<Q>
{
	SQObjectPtr()
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_NULL;
		this->_unVal.pUserPointer=NULL;
	}
	SQObjectPtr(const SQObjectPtr<Q> &o)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=o._type;
		this->_unVal=o._unVal;
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(const SQObject<Q> &o)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=o._type;
		this->_unVal=o._unVal;
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQTable<Q> *pTable)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_TABLE;
		this->_unVal.pTable=pTable;
		assert(this->_unVal.pTable);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQClass<Q> *pClass)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_CLASS;
		this->_unVal.pClass=pClass;
		assert(this->_unVal.pClass);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQInstance<Q> *pInstance)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_INSTANCE;
		this->_unVal.pInstance=pInstance;
		assert(this->_unVal.pInstance);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQArray<Q> *pArray)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_ARRAY;
		this->_unVal.pArray=pArray;
		assert(this->_unVal.pArray);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQClosure<Q> *pClosure)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_CLOSURE;
		this->_unVal.pClosure=pClosure;
		assert(this->_unVal.pClosure);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQGenerator<Q> *pGenerator)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_GENERATOR;
		this->_unVal.pGenerator=pGenerator;
		assert(this->_unVal.pGenerator);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQNativeClosure<Q> *pNativeClosure)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_NATIVECLOSURE;
		this->_unVal.pNativeClosure=pNativeClosure;
		assert(this->_unVal.pNativeClosure);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQString<Q> *pString)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_STRING;
		this->_unVal.pString=pString;
		assert(this->_unVal.pString);
		__AddRef(this->_type, this->_unVal);
	}
	SQObjectPtr(SQUserData<Q> *pUserData)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_USERDATA;
		this->_unVal.pUserData=pUserData;
		assert(this->_unVal.pUserData);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQVM<Q> *pThread)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_THREAD;
		this->_unVal.pThread=pThread;
		assert(this->_unVal.pThread);
		__AddRef(this->_type,this->_unVal);
	}
	SQObjectPtr(SQWeakRef<Q> *pWeakRef)
	{
		SQ_OBJECT_RAWINIT()
		this->_type=OT_WEAKREF;
		this->_unVal.pWeakRef=pWeakRef;
		assert(this->_unVal.pWeakRef);
		__AddRef(this->_type, this->_unVal);
	}
	SQObjectPtr(SQFunctionProto<Q> *pFunctionProto)
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
		SQObjectValue<Q> unOldVal;
		tOldType =this->_type;
		unOldVal =this->_unVal;
		this->_type = OT_NULL;
		this->_unVal.pUserPointer = NULL;
		__Release(tOldType,unOldVal);
	}
	inline SQObjectPtr<Q>& operator=(SQInteger i)
	{ 
		__Release(this->_type,this->_unVal);
		this->_unVal.nInteger = i;
		this->_type = OT_INTEGER;
		return *this;
	}
	inline SQObjectPtr<Q>& operator=(SQFloat f)
	{ 
		__Release(this->_type,this->_unVal);
		this->_unVal.fFloat = f;
		this->_type = OT_FLOAT;
		return *this;
	}
	inline SQObjectPtr<Q>& operator=(const SQObjectPtr<Q>& obj)
	{ 
		SQObjectType tOldType;
		SQObjectValue<Q> unOldVal;
		tOldType=this->_type;
		unOldVal=this->_unVal;
		this->_unVal = obj._unVal;
		this->_type = obj._type;
		__AddRef(this->_type,this->_unVal);
		__Release(tOldType,unOldVal);
		return *this;
	}
	inline SQObjectPtr<Q>& operator=(const SQObject<Q>& obj)
	{ 
		SQObjectType tOldType;
		SQObjectValue<Q> unOldVal;
		tOldType=this->_type;
		unOldVal=this->_unVal;
		this->_unVal = obj._unVal;
		this->_type = obj._type;
		__AddRef(this->_type,this->_unVal);
		__Release(tOldType,unOldVal);
		return *this;
	}
#ifdef _SQ_M2
	inline bool operator==(const SQObjectPtr<Q>& obj) const
	{
		return this->_unVal.raw == obj._unVal.raw && this->_type == obj._type;
	}
#endif
	private:
		SQObjectPtr(const SQChar *){} //safety
};
/////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_GARBAGE_COLLECTOR
#define MARK_FLAG 0x80000000
template <Squirk Q>
struct SQCollectable : public SQRefCounted<Q> {
	SQCollectable<Q> *_next;
	SQCollectable<Q> *_prev;
	SQSharedState<Q> *_sharedstate;
	virtual void Release()=0;
	virtual void Mark(SQCollectable<Q> **chain)=0;
	void UnMark();
	virtual void Finalize()=0;
	static void AddToChain(SQCollectable<Q> **chain,SQCollectable<Q> *c);
	static void RemoveFromChain(SQCollectable<Q> **chain,SQCollectable<Q> *c);
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

template <Squirk Q>
struct SQDelegable : public CHAINABLE_OBJ<Q> {
	bool SetDelegate(SQTable<Q> *m);
	virtual bool GetMetaMethod(SQVM<Q> *v,SQMetaMethod mm,SQObjectPtr<Q> &res);
	SQTable<Q> *_delegate;
};

template <Squirk Q>
SQUnsignedInteger TranslateIndex(const SQObjectPtr<Q> &idx);
template <Squirk Q>
using SQObjectPtrVec = sqvector<SQObjectPtr<Q>>;
typedef sqvector<SQInteger> SQIntVec;
template <Squirk Q>
const SQChar *GetTypeName(const SQObjectPtr<Q> &obj1);
const SQChar *IdType2Name(SQObjectType type);

template SQRefCounted<Squirk::Standard>;
template SQRefCounted<Squirk::AlignObject>;
template SQRefCounted<Squirk::StandardShared>;
template SQRefCounted<Squirk::AlignObjectShared>;

template SQWeakRef<Squirk::Standard>;
template SQWeakRef<Squirk::AlignObject>;
template SQWeakRef<Squirk::StandardShared>;
template SQWeakRef<Squirk::AlignObjectShared>;

template SQObjectPtr<Squirk::Standard>;
template SQObjectPtr<Squirk::AlignObject>;
template SQObjectPtr<Squirk::StandardShared>;
template SQObjectPtr<Squirk::AlignObjectShared>;

template SQCollectable<Squirk::Standard>;
template SQCollectable<Squirk::AlignObject>;
template SQCollectable<Squirk::StandardShared>;
template SQCollectable<Squirk::AlignObjectShared>;

template SQDelegable<Squirk::Standard>;
template SQDelegable<Squirk::AlignObject>;
template SQDelegable<Squirk::StandardShared>;
template SQDelegable<Squirk::AlignObjectShared>;

#endif //_SQOBJECT_H_
