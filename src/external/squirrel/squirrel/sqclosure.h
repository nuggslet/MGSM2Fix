/*	see copyright notice in squirrel.h */
#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_

template <Squirk T>
struct SQFunctionProto;

template <Squirk T>
struct SQClosure : public CHAINABLE_OBJ<T>
{
private:
	SQClosure(SQSharedState<T> *ss,SQFunctionProto<T> *func){_function=func; INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
	static SQClosure<T> *Create(SQSharedState<T> *ss,SQFunctionProto<T> *func){
		SQClosure<T> *nc=(SQClosure<T>*)SQ_MALLOC(sizeof(SQClosure<T>));
		new (nc) SQClosure<T>(ss,func);
		return nc;
	}
	void Release(){
		sq_delete(this,SQClosure<T>);
	}
	SQClosure<T> *Clone()
	{
		SQClosure<T> * ret = SQClosure<T>::Create(_opt_ss(this),_funcproto(_function));
		ret->_env = _env;
		ret->_outervalues.copy(_outervalues);
		ret->_defaultparams.copy(_defaultparams);
		return ret;
	}
	~SQClosure()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
	bool Save(SQVM<T> *v,SQUserPointer up,SQWRITEFUNC write);
	static bool Load(SQVM<T> *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr<T> &ret);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> **chain);
	void Finalize(){_outervalues.resize(0); }
#endif
	SQObjectPtr<T> _env;
	SQObjectPtr<T> _function;
	SQObjectPtrVec<T> _outervalues;
	SQObjectPtrVec<T> _defaultparams;
};
//////////////////////////////////////////////
template <Squirk T>
struct SQGenerator : public CHAINABLE_OBJ<T>
{
	enum SQGeneratorState{eRunning,eSuspended,eDead};
private:
	SQGenerator(SQSharedState<T> *ss,SQClosure<T> *closure){_closure=closure;_state=eRunning;_ci._generator=NULL;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
	static SQGenerator<T> *Create(SQSharedState<T> *ss,SQClosure<T> *closure){
		SQGenerator<T> *nc=(SQGenerator<T>*)SQ_MALLOC(sizeof(SQGenerator<T>));
		new (nc) SQGenerator<T>(ss,closure);
		return nc;
	}
	~SQGenerator()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
    void Kill(){
		_state=eDead;
		_stack.resize(0);
		_closure=_null_<T>;}
	void Release(){
		sq_delete(this,SQGenerator<T>);
	}
	bool Yield(SQVM<T> *v);
	bool Resume(SQVM<T> *v,SQInteger target);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> **chain);
	void Finalize(){_stack.resize(0);_closure=_null_<T>;}
#endif
	SQObjectPtr<T> _closure;
	SQObjectPtrVec<T> _stack;
	SQObjectPtrVec<T> _vargsstack;
	typename SQVM<T>::CallInfo _ci;
	ExceptionsTraps _etraps;
	SQGeneratorState _state;
};

template <Squirk T>
struct SQNativeClosure : public CHAINABLE_OBJ<T>
{
private:
	SQNativeClosure(SQSharedState<T> *ss,SQFUNCTION<T> func){_function=func;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);	}
public:
	static SQNativeClosure<T> *Create(SQSharedState<T> *ss,SQFUNCTION<T> func)
	{
		SQNativeClosure<T> *nc=(SQNativeClosure<T>*)SQ_MALLOC(sizeof(SQNativeClosure<T>));
		new (nc) SQNativeClosure<T>(ss,func);
		return nc;
	}
	SQNativeClosure<T> *Clone()
	{
		SQNativeClosure<T> * ret = SQNativeClosure<T>::Create(_opt_ss(this),_function);
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
		sq_delete(this,SQNativeClosure<T>);
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> **chain);
	void Finalize(){_outervalues.resize(0);}
#endif
	SQInteger _nparamscheck;
	SQIntVec _typecheck;
	SQObjectPtrVec<T> _outervalues;
	SQObjectPtr<T> _env;
	SQFUNCTION<T> _function;
	SQObjectPtr<T> _name;
};

template SQClosure<Squirk::Standard>;
template SQClosure<Squirk::AlignObject>;

template SQGenerator<Squirk::Standard>;
template SQGenerator<Squirk::AlignObject>;

template SQNativeClosure<Squirk::Standard>;
template SQNativeClosure<Squirk::AlignObject>;

#endif //_SQCLOSURE_H_
