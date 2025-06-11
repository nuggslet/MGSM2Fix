/*	see copyright notice in squirrel.h */
#ifndef _SQVM_H_
#define _SQVM_H_

#include "sqopcodes.h"
#include "sqobject.h"
#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 10

#define SQ_SUSPEND_FLAG -666
//base lib
template <Squirk Q>
void sq_base_register(HSQUIRRELVM<Q> v);

struct SQExceptionTrap{
	SQExceptionTrap() {}
	SQExceptionTrap(SQInteger ss, SQInteger stackbase,SQInstruction *ip, SQInteger ex_target){ _stacksize = ss; _stackbase = stackbase; _ip = ip; _extarget = ex_target;}
	SQExceptionTrap(const SQExceptionTrap &et) { (*this) = et;	}
	SQInteger _stackbase;
	SQInteger _stacksize;
	SQInstruction *_ip;
	SQInteger _extarget;
};

#define _INLINE 

#define STK(a) _stack._vals[_stackbase+(a)]
#define TARGET _stack._vals[_stackbase+arg0]

typedef sqvector<SQExceptionTrap> ExceptionsTraps;

struct VarArgs {
	VarArgs() { size = 0; base = 0; }
	unsigned short size;
	unsigned short base;
};

template <Squirk Q>
struct CallInfo {
	//CallInfo() { _generator._type = OT_NULL;}
	SQInstruction *_ip;
	SQObjectPtr<Q> *_literals;
	SQObjectPtr<Q> _closure;
	SQGenerator<Q> *_generator;
	SQInt32 _etraps;
	SQInt32 _prevstkbase;
	SQInt32 _prevtop;
	SQInt32 _target;
	SQInt32 _ncalls;
	SQBool _root;
	VarArgs _vargs;
};

#if defined(_SQ_M2)
template <Squirk Q>
struct M2VMExt {

	SQObjectPtr<Q> _unknown;
	CallInfo<Q>* _callsstack;

	CallInfo<Q>* operator=(CallInfo<Q>* callsstack) {
		_callsstack = callsstack;
		return _callsstack;
	}
	CallInfo<Q>& operator[](std::size_t pos) const {
		return _callsstack[pos];
	}

};

template <>
struct M2VMExt<Squirk::AlignObject>
{
	SQObjectPtr<Squirk::Standard> _unknown; // Very demure.
	SQInteger _pad0;
	SQInteger _pad1;
	CallInfo<Squirk::AlignObject>* _callsstack;

	CallInfo<Squirk::AlignObject>* operator=(CallInfo<Squirk::AlignObject>* callsstack) {
		_callsstack = callsstack;
		return _callsstack;
	}
	CallInfo<Squirk::AlignObject>& operator[](std::size_t pos) const {
		return _callsstack[pos];
	}
};
#endif

template <Squirk Q>
struct SQVM : public CHAINABLE_OBJ<Q>
{	
typedef sqvector<CallInfo<Q>> CallInfoVec;
public:
	enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_VM, ET_RESUME_THROW_VM };
	SQVM(SQSharedState<Q> *ss);
	~SQVM();
	bool Init(SQVM<Q> *friendvm, SQInteger stacksize);
	bool Execute(SQObjectPtr<Q> &func, SQInteger target, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &outres, SQBool raiseerror, ExecutionType et = ET_CALL);
	//starts a native call return when the NATIVE closure returns
	bool CallNative(SQNativeClosure<Q> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &retval,bool &suspend);
	//starts a SQUIRREL call in the same "Execution loop"
	bool StartCall(SQClosure<Q> *closure, SQInteger target, SQInteger nargs, SQInteger stackbase, bool tailcall);
	bool CreateClassInstance(SQClass<Q> *theclass, SQObjectPtr<Q> &inst, SQObjectPtr<Q> &constructor);
	//call a generic closure pure SQUIRREL or NATIVE
	bool Call(SQObjectPtr<Q> &closure, SQInteger nparams, SQInteger stackbase, SQObjectPtr<Q> &outres,SQBool raiseerror);
	SQRESULT Suspend();

	void CallDebugHook(SQInteger type,SQInteger forcedline=0);
	void CallErrorHandler(SQObjectPtr<Q> &e);
	bool Get(const SQObjectPtr<Q> &self, const SQObjectPtr<Q> &key, SQObjectPtr<Q> &dest, bool raw, bool fetchroot);
	bool FallBackGet(const SQObjectPtr<Q> &self,const SQObjectPtr<Q> &key,SQObjectPtr<Q> &dest,bool raw);
	bool Set(const SQObjectPtr<Q> &self, const SQObjectPtr<Q> &key, const SQObjectPtr<Q> &val, bool fetchroot);
	bool NewSlot(const SQObjectPtr<Q> &self, const SQObjectPtr<Q> &key, const SQObjectPtr<Q> &val,bool bstatic);
	bool DeleteSlot(const SQObjectPtr<Q> &self, const SQObjectPtr<Q> &key, SQObjectPtr<Q> &res);
	bool Clone(const SQObjectPtr<Q> &self, SQObjectPtr<Q> &target);
	bool ObjCmp(const SQObjectPtr<Q> &o1, const SQObjectPtr<Q> &o2,SQInteger &res);
	bool StringCat(const SQObjectPtr<Q> &str, const SQObjectPtr<Q> &obj, SQObjectPtr<Q> &dest);
	bool IsEqual(SQObjectPtr<Q> &o1,SQObjectPtr<Q> &o2,bool &res);
	void ToString(const SQObjectPtr<Q> &o,SQObjectPtr<Q> &res);
	SQString<Q> *PrintObjVal(const SQObject<Q> &o);

 
	void Raise_Error(const SQChar *s, ...);
	void Raise_Error(SQObjectPtr<Q> &desc);
	void Raise_IdxError(SQObject<Q> &o);
	void Raise_CompareError(const SQObject<Q> &o1, const SQObject<Q> &o2);
	void Raise_ParamTypeError(SQInteger nparam,SQInteger typemask,SQInteger type);

	void TypeOf(const SQObjectPtr<Q> &obj1, SQObjectPtr<Q> &dest);
	bool CallMetaMethod(SQDelegable<Q> *del, SQMetaMethod mm, SQInteger nparams, SQObjectPtr<Q> &outres);
	bool ArithMetaMethod(SQInteger op, const SQObjectPtr<Q> &o1, const SQObjectPtr<Q> &o2, SQObjectPtr<Q> &dest);
	bool Return(SQInteger _arg0, SQInteger _arg1, SQObjectPtr<Q> &retval);
	//new stuff
	_INLINE bool ARITH_OP(SQUnsignedInteger op,SQObjectPtr<Q> &trg,const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2);
	_INLINE bool BW_OP(SQUnsignedInteger op,SQObjectPtr<Q> &trg,const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2);
	_INLINE bool NEG_OP(SQObjectPtr<Q> &trg,const SQObjectPtr<Q> &o1);
	_INLINE bool CMP_OP(CmpOP op, const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2,SQObjectPtr<Q> &res);
	bool CLOSURE_OP(SQObjectPtr<Q> &target, SQFunctionProto<Q> *func);
	bool GETVARGV_OP(SQObjectPtr<Q> &target,SQObjectPtr<Q> &idx,CallInfo<Q> *ci);
	bool CLASS_OP(SQObjectPtr<Q> &target,SQInteger base,SQInteger attrs);
	bool GETPARENT_OP(SQObjectPtr<Q> &o,SQObjectPtr<Q> &target);
	//return true if the loop is finished
	bool FOREACH_OP(SQObjectPtr<Q> &o1,SQObjectPtr<Q> &o2,SQObjectPtr<Q> &o3,SQObjectPtr<Q> &o4,SQInteger arg_2,int exitpos,int &jump);
	bool DELEGATE_OP(SQObjectPtr<Q> &trg,SQObjectPtr<Q> &o1,SQObjectPtr<Q> &o2);
	_INLINE bool LOCAL_INC(SQInteger op,SQObjectPtr<Q> &target, SQObjectPtr<Q> &a, SQObjectPtr<Q> &incr);
	_INLINE bool PLOCAL_INC(SQInteger op,SQObjectPtr<Q> &target, SQObjectPtr<Q> &a, SQObjectPtr<Q> &incr);
	_INLINE bool DerefInc(SQInteger op,SQObjectPtr<Q> &target, SQObjectPtr<Q> &self, SQObjectPtr<Q> &key, SQObjectPtr<Q> &incr, bool postfix);
	void PopVarArgs(VarArgs &vargs);
	void ClearStack(SQInteger last_top);
#ifdef _DEBUG_DUMP
	void dumpstack(SQInteger stackbase=-1, bool dumpall = false);
#endif

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> **chain);
#endif
	void Finalize();
	void GrowCallStack() {
		SQInteger newsize = _alloccallsstacksize*2;
		_callstackdata.resize(newsize);
		_callsstack = &_callstackdata[0];
		_alloccallsstacksize = newsize;
	}
	void Release(){ sq_delete(this,SQVM<Q>); } //does nothing
////////////////////////////////////////////////////////////////////////////
	//stack functions for the api
	void Remove(SQInteger n);

	bool IsFalse(SQObjectPtr<Q> &o);
	
	void Pop();
	void Pop(SQInteger n);
	void Push(const SQObjectPtr<Q> &o);
	SQObjectPtr<Q> &Top();
	SQObjectPtr<Q> &PopGet();
	SQObjectPtr<Q> &GetUp(SQInteger n);
	SQObjectPtr<Q> &GetAt(SQInteger n);

	SQObjectPtrVec<Q> _stack;
	SQObjectPtrVec<Q> _vargsstack;
	SQInteger _top;
	SQInteger _stackbase;
	SQObjectPtr<Q> _roottable;
	SQObjectPtr<Q> _lasterror;
	SQObjectPtr<Q> _errorhandler;

	SQObjectPtr<Q> _debughook;

	SQObjectPtr<Q> temp_reg;

#if defined(_SQ_M2)
	std::conditional_t<
		Q == Squirk::Standard ||
		Q == Squirk::AlignObject,
		M2VMExt<Q>, CallInfo<Q>*>
		_callsstack;
#else
	CallInfo<Q>* _callsstack;
#endif

	SQInteger _callsstacksize;
	SQInteger _alloccallsstacksize;
	sqvector<CallInfo<Q>>  _callstackdata;

	ExceptionsTraps _etraps;
	CallInfo<Q> *ci;
	void *_foreignptr;
	//VMs sharing the same state
	SQSharedState<Q> *_sharedstate;
	SQInteger _nnativecalls;
	//suspend infos
	SQBool _suspended;
	SQBool _suspended_root;
	SQInteger _suspended_target;
	SQInteger _suspended_traps;
	VarArgs _suspend_varargs;
};

struct AutoDec{
	AutoDec(SQInteger *n) { _n = n; }
	~AutoDec() { (*_n)--; }
	SQInteger *_n;
};

template <Squirk Q>
inline SQObjectPtr<Q> &stack_get(HSQUIRRELVM<Q> v,SQInteger idx){return ((idx>=0)?(v->GetAt(idx+v->_stackbase-1)):(v->GetUp(idx)));}

#define _ss(_vm_) (_vm_)->_sharedstate

#ifndef NO_GARBAGE_COLLECTOR
#define _opt_ss(_vm_) (_vm_)->_sharedstate
#else
#define _opt_ss(_vm_) NULL
#endif

#define PUSH_CALLINFO(v,nci){ \
	if(v->_callsstacksize == v->_alloccallsstacksize) { \
		v->GrowCallStack(); \
	} \
	v->ci = &v->_callsstack[v->_callsstacksize]; \
	*(v->ci) = nci; \
	v->_callsstacksize++; \
}

#define POP_CALLINFO(v){ \
	v->_callsstacksize--; \
	v->ci->_closure.Null(); \
	if(v->_callsstacksize)	\
		v->ci = &v->_callsstack[v->_callsstacksize-1] ; \
	else	\
		v->ci = NULL; \
}

template SQVM<Squirk::Standard>;
template SQVM<Squirk::AlignObject>;
template SQVM<Squirk::StandardShared>;
template SQVM<Squirk::AlignObjectShared>;

#endif //_SQVM_H_
