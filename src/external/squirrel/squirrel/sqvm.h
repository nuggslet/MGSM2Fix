/*	see copyright notice in squirrel.h */
#ifndef _SQVM_H_
#define _SQVM_H_

#include "sqopcodes.h"
#include "sqobject.h"
#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 10

#define SQ_SUSPEND_FLAG -666
//base lib
template <Squirk T>
void sq_base_register(HSQUIRRELVM<T> v);

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

template <Squirk T>
struct SQVM : public CHAINABLE_OBJ<T>
{
	struct VarArgs {
		VarArgs() { size = 0; base = 0; }
		unsigned short size;
		unsigned short base;
	};

	struct CallInfo{
		//CallInfo() { _generator._type = OT_NULL;}
		SQInstruction *_ip;
		SQObjectPtr<T> *_literals;
		SQObjectPtr<T> _closure;
		SQGenerator<T> *_generator;
		SQInt32 _etraps;
		SQInt32 _prevstkbase;
		SQInt32 _prevtop;
		SQInt32 _target;
		SQInt32 _ncalls;
		SQBool _root;
		VarArgs _vargs;
	};
	
typedef sqvector<CallInfo> CallInfoVec;
public:
	enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_VM, ET_RESUME_THROW_VM };
	SQVM(SQSharedState<T> *ss);
	~SQVM();
	bool Init(SQVM<T> *friendvm, SQInteger stacksize);
	bool Execute(SQObjectPtr<T> &func, SQInteger target, SQInteger nargs, SQInteger stackbase, SQObjectPtr<T> &outres, SQBool raiseerror, ExecutionType et = ET_CALL);
	//starts a native call return when the NATIVE closure returns
	bool CallNative(SQNativeClosure<T> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<T> &retval,bool &suspend);
	//starts a SQUIRREL call in the same "Execution loop"
	bool StartCall(SQClosure<T> *closure, SQInteger target, SQInteger nargs, SQInteger stackbase, bool tailcall);
	bool CreateClassInstance(SQClass<T> *theclass, SQObjectPtr<T> &inst, SQObjectPtr<T> &constructor);
	//call a generic closure pure SQUIRREL or NATIVE
	bool Call(SQObjectPtr<T> &closure, SQInteger nparams, SQInteger stackbase, SQObjectPtr<T> &outres,SQBool raiseerror);
	SQRESULT Suspend();

	void CallDebugHook(SQInteger type,SQInteger forcedline=0);
	void CallErrorHandler(SQObjectPtr<T> &e);
	bool Get(const SQObjectPtr<T> &self, const SQObjectPtr<T> &key, SQObjectPtr<T> &dest, bool raw, bool fetchroot);
	bool FallBackGet(const SQObjectPtr<T> &self,const SQObjectPtr<T> &key,SQObjectPtr<T> &dest,bool raw);
	bool Set(const SQObjectPtr<T> &self, const SQObjectPtr<T> &key, const SQObjectPtr<T> &val, bool fetchroot);
	bool NewSlot(const SQObjectPtr<T> &self, const SQObjectPtr<T> &key, const SQObjectPtr<T> &val,bool bstatic);
	bool DeleteSlot(const SQObjectPtr<T> &self, const SQObjectPtr<T> &key, SQObjectPtr<T> &res);
	bool Clone(const SQObjectPtr<T> &self, SQObjectPtr<T> &target);
	bool ObjCmp(const SQObjectPtr<T> &o1, const SQObjectPtr<T> &o2,SQInteger &res);
	bool StringCat(const SQObjectPtr<T> &str, const SQObjectPtr<T> &obj, SQObjectPtr<T> &dest);
	bool IsEqual(SQObjectPtr<T> &o1,SQObjectPtr<T> &o2,bool &res);
	void ToString(const SQObjectPtr<T> &o,SQObjectPtr<T> &res);
	SQString<T> *PrintObjVal(const SQObject<T> &o);

 
	void Raise_Error(const SQChar *s, ...);
	void Raise_Error(SQObjectPtr<T> &desc);
	void Raise_IdxError(SQObject<T> &o);
	void Raise_CompareError(const SQObject<T> &o1, const SQObject<T> &o2);
	void Raise_ParamTypeError(SQInteger nparam,SQInteger typemask,SQInteger type);

	void TypeOf(const SQObjectPtr<T> &obj1, SQObjectPtr<T> &dest);
	bool CallMetaMethod(SQDelegable<T> *del, SQMetaMethod mm, SQInteger nparams, SQObjectPtr<T> &outres);
	bool ArithMetaMethod(SQInteger op, const SQObjectPtr<T> &o1, const SQObjectPtr<T> &o2, SQObjectPtr<T> &dest);
	bool Return(SQInteger _arg0, SQInteger _arg1, SQObjectPtr<T> &retval);
	//new stuff
	_INLINE bool ARITH_OP(SQUnsignedInteger op,SQObjectPtr<T> &trg,const SQObjectPtr<T> &o1,const SQObjectPtr<T> &o2);
	_INLINE bool BW_OP(SQUnsignedInteger op,SQObjectPtr<T> &trg,const SQObjectPtr<T> &o1,const SQObjectPtr<T> &o2);
	_INLINE bool NEG_OP(SQObjectPtr<T> &trg,const SQObjectPtr<T> &o1);
	_INLINE bool CMP_OP(CmpOP op, const SQObjectPtr<T> &o1,const SQObjectPtr<T> &o2,SQObjectPtr<T> &res);
	bool CLOSURE_OP(SQObjectPtr<T> &target, SQFunctionProto<T> *func);
	bool GETVARGV_OP(SQObjectPtr<T> &target,SQObjectPtr<T> &idx,CallInfo *ci);
	bool CLASS_OP(SQObjectPtr<T> &target,SQInteger base,SQInteger attrs);
	bool GETPARENT_OP(SQObjectPtr<T> &o,SQObjectPtr<T> &target);
	//return true if the loop is finished
	bool FOREACH_OP(SQObjectPtr<T> &o1,SQObjectPtr<T> &o2,SQObjectPtr<T> &o3,SQObjectPtr<T> &o4,SQInteger arg_2,int exitpos,int &jump);
	bool DELEGATE_OP(SQObjectPtr<T> &trg,SQObjectPtr<T> &o1,SQObjectPtr<T> &o2);
	_INLINE bool LOCAL_INC(SQInteger op,SQObjectPtr<T> &target, SQObjectPtr<T> &a, SQObjectPtr<T> &incr);
	_INLINE bool PLOCAL_INC(SQInteger op,SQObjectPtr<T> &target, SQObjectPtr<T> &a, SQObjectPtr<T> &incr);
	_INLINE bool DerefInc(SQInteger op,SQObjectPtr<T> &target, SQObjectPtr<T> &self, SQObjectPtr<T> &key, SQObjectPtr<T> &incr, bool postfix);
	void PopVarArgs(VarArgs &vargs);
	void ClearStack(SQInteger last_top);
#ifdef _DEBUG_DUMP
	void dumpstack(SQInteger stackbase=-1, bool dumpall = false);
#endif

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> **chain);
#endif
	void Finalize();
	void GrowCallStack() {
		SQInteger newsize = _alloccallsstacksize*2;
		_callstackdata.resize(newsize);
		_callsstack = &_callstackdata[0];
		_alloccallsstacksize = newsize;
	}
	void Release(){ sq_delete(this,SQVM<T>); } //does nothing
////////////////////////////////////////////////////////////////////////////
	//stack functions for the api
	void Remove(SQInteger n);

	bool IsFalse(SQObjectPtr<T> &o);
	
	void Pop();
	void Pop(SQInteger n);
	void Push(const SQObjectPtr<T> &o);
	SQObjectPtr<T> &Top();
	SQObjectPtr<T> &PopGet();
	SQObjectPtr<T> &GetUp(SQInteger n);
	SQObjectPtr<T> &GetAt(SQInteger n);

	SQObjectPtrVec<T> _stack;
	SQObjectPtrVec<T> _vargsstack;
	SQInteger _top;
	SQInteger _stackbase;
	SQObjectPtr<T> _roottable;
	SQObjectPtr<T> _lasterror;
	SQObjectPtr<T> _errorhandler;

	SQObjectPtr<T> _debughook;

	SQObjectPtr<T> temp_reg;

#if defined(_SQ_M2) && !defined(_WIN64)
	SQObjectPtr<T> _m2_unknown; // ???
#endif

	CallInfo* _callsstack;
	SQInteger _callsstacksize;
	SQInteger _alloccallsstacksize;
	sqvector<CallInfo>  _callstackdata;

	ExceptionsTraps _etraps;
	CallInfo *ci;
	void *_foreignptr;
	//VMs sharing the same state
	SQSharedState<T> *_sharedstate;
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

template <Squirk T>
inline SQObjectPtr<T> &stack_get(HSQUIRRELVM<T> v,SQInteger idx){return ((idx>=0)?(v->GetAt(idx+v->_stackbase-1)):(v->GetUp(idx)));}

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

#endif //_SQVM_H_
