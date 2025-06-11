/*	see copyright notice in squirrel.h */
#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_

template <Squirk Q>
struct SQFunctionProto;

template <Squirk Q>
struct SQClosure : public CHAINABLE_OBJ<Q>
{
private:
	SQClosure(SQSharedState<Q> *ss,SQFunctionProto<Q> *func){_function=func; INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
	static SQClosure<Q> *Create(SQSharedState<Q> *ss,SQFunctionProto<Q> *func){
		SQClosure<Q> *nc=(SQClosure<Q>*)SQ_MALLOC(sizeof(SQClosure<Q>));
		new (nc) SQClosure<Q>(ss,func);
		return nc;
	}
	void Release(){
		sq_delete(this,SQClosure<Q>);
	}
	SQClosure<Q> *Clone()
	{
		SQClosure<Q> * ret = SQClosure<Q>::Create(_opt_ss(this),_funcproto(_function));
		ret->_env = _env;
		ret->_outervalues.copy(_outervalues);
		ret->_defaultparams.copy(_defaultparams);
		return ret;
	}
	~SQClosure()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
	bool Save(SQVM<Q> *v,SQUserPointer up,SQWRITEFUNC write);
	static bool Load(SQVM<Q> *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr<Q> &ret);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> **chain);
	void Finalize(){_outervalues.resize(0); }
#endif
	SQObjectPtr<Q> _env;
	SQObjectPtr<Q> _function;
	SQObjectPtrVec<Q> _outervalues;
	SQObjectPtrVec<Q> _defaultparams;
};
//////////////////////////////////////////////
template <Squirk Q>
struct SQGenerator : public CHAINABLE_OBJ<Q>
{
	enum SQGeneratorState{eRunning,eSuspended,eDead};
private:
	SQGenerator(SQSharedState<Q> *ss,SQClosure<Q> *closure){_closure=closure;_state=eRunning;_ci._generator=NULL;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
	static SQGenerator<Q> *Create(SQSharedState<Q> *ss,SQClosure<Q> *closure){
		SQGenerator<Q> *nc=(SQGenerator<Q>*)SQ_MALLOC(sizeof(SQGenerator<Q>));
		new (nc) SQGenerator<Q>(ss,closure);
		return nc;
	}
	~SQGenerator()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
    void Kill(){
		_state=eDead;
		_stack.resize(0);
		_closure=_null_<Q>;}
	void Release(){
		sq_delete(this,SQGenerator<Q>);
	}
	bool Yield(SQVM<Q> *v);
	bool Resume(SQVM<Q> *v,SQInteger target);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> **chain);
	void Finalize(){_stack.resize(0);_closure=_null_<Q>;}
#endif
	SQObjectPtr<Q> _closure;
	SQObjectPtrVec<Q> _stack;
	SQObjectPtrVec<Q> _vargsstack;
	CallInfo<Q> _ci;
	ExceptionsTraps _etraps;
	SQGeneratorState _state;
};

template <Squirk Q>
struct SQNativeClosure : public CHAINABLE_OBJ<Q>
{
private:
	SQNativeClosure(SQSharedState<Q> *ss,SQFUNCTION<Q> func){_function=func;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);	}
public:
	static SQNativeClosure<Q> *Create(SQSharedState<Q> *ss,SQFUNCTION<Q> func)
	{
		SQNativeClosure<Q> *nc=(SQNativeClosure<Q>*)SQ_MALLOC(sizeof(SQNativeClosure<Q>));
		new (nc) SQNativeClosure<Q>(ss,func);
		return nc;
	}
	SQNativeClosure<Q> *Clone()
	{
		SQNativeClosure<Q> * ret = SQNativeClosure<Q>::Create(_opt_ss(this),_function);
		ret->_env = _env;
		ret->_name = _name;
		ret->_outervalues.copy(_outervalues);
		ret->_typecheck.copy(_typecheck);
		ret->_nparamscheck = _nparamscheck;
		return ret;
	}
	~SQNativeClosure()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
	void Release(){
		sq_delete(this,SQNativeClosure<Q>);
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> **chain);
	void Finalize(){_outervalues.resize(0);}
#endif
	SQInteger _nparamscheck;
	SQIntVec _typecheck;
	SQObjectPtrVec<Q> _outervalues;
	SQObjectPtr<Q> _env;
	SQFUNCTION<Q> _function;
	SQObjectPtr<Q> _name;
};

template SQClosure<Squirk::Standard>;
template SQClosure<Squirk::AlignObject>;
template SQClosure<Squirk::StandardShared>;
template SQClosure<Squirk::AlignObjectShared>;

template SQGenerator<Squirk::Standard>;
template SQGenerator<Squirk::AlignObject>;
template SQGenerator<Squirk::StandardShared>;
template SQGenerator<Squirk::AlignObjectShared>;

template SQNativeClosure<Squirk::Standard>;
template SQNativeClosure<Squirk::AlignObject>;
template SQNativeClosure<Squirk::StandardShared>;
template SQNativeClosure<Squirk::AlignObjectShared>;

#endif //_SQCLOSURE_H_
