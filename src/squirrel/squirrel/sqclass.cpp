/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqclass.h"
#include "sqclosure.h"

template <Squirk Q>
SQClass<Q>::SQClass(SQSharedState<Q> *ss,SQClass<Q> *base)
{
	_base = base;
	_typetag = 0;
	_hook = NULL;
	_udsize = 0;
	_metamethods.resize(MT_LAST); //size it to max size
	if(_base) {
		_defaultvalues.copy(base->_defaultvalues);
		_methods.copy(base->_methods);
		_metamethods.copy(base->_metamethods);
		__ObjAddRef(_base);
	}
	_members = base?base->_members->Clone() : SQTable<Q>::Create(ss,0);
	__ObjAddRef(_members);
	_locked = false;
	INIT_CHAIN();
	ADD_TO_CHAIN(&CHAINABLE_OBJ<Q>::_sharedstate->_gc_chain, this);
}

template <Squirk Q>
void SQClass<Q>::Finalize() {
	_attributes = _null_<Q>;
	_defaultvalues.resize(0);
	_methods.resize(0);
	_metamethods.resize(0);
	__ObjRelease(_members);
	if(_base) {
		__ObjRelease(_base);
	}
}

template <Squirk Q>
SQClass<Q>::~SQClass()
{
	REMOVE_FROM_CHAIN(&CHAINABLE_OBJ<Q>::_sharedstate->_gc_chain, this);
	Finalize();
}

template <Squirk Q>
bool SQClass<Q>::NewSlot(SQSharedState<Q> *ss,const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val,bool bstatic)
{
	SQObjectPtr<Q> temp;
	if(_locked) 
		return false; //the class already has an instance so cannot be modified
	if(_members->Get(key,temp) && _isfield(temp)) //overrides the default value
	{
		_defaultvalues[_member_idx(temp)].val = val;
		return true;
	}
	if(obj_type(val) == OT_CLOSURE || obj_type(val) == OT_NATIVECLOSURE || bstatic) {
		SQInteger mmidx;
		if((obj_type(val) == OT_CLOSURE || obj_type(val) == OT_NATIVECLOSURE) && 
			(mmidx = ss->GetMetaMethodIdxByName(key)) != -1) {
			_metamethods[mmidx] = val;
		} 
		else {
			if(obj_type(temp) == OT_NULL) {
				SQClassMember<Q> m;
				m.val = val;
				_members->NewSlot(key,SQObjectPtr<Q>(_make_method_idx(_methods.size())));
				_methods.push_back(m);
			}
			else {
				_methods[_member_idx(temp)].val = val;
			}
		}
		return true;
	}
	SQClassMember<Q> m;
	m.val = val;
	_members->NewSlot(key,SQObjectPtr<Q>(_make_field_idx(_defaultvalues.size())));
	_defaultvalues.push_back(m);
	return true;
}

template <Squirk Q>
SQInstance<Q> *SQClass<Q>::CreateInstance()
{
	if(!_locked) Lock();
	return SQInstance<Q>::Create(_opt_ss(this),this);
}

template <Squirk Q>
SQInteger SQClass<Q>::Next(const SQObjectPtr<Q> &refpos, SQObjectPtr<Q> &outkey, SQObjectPtr<Q> &outval)
{
	SQObjectPtr<Q> oval;
	SQInteger idx = _members->Next(false,refpos,outkey,oval);
	if(idx != -1) {
		if(_ismethod(oval)) {
			outval = _methods[_member_idx(oval)].val;
		}
		else {
			SQObjectPtr<Q> &o = _defaultvalues[_member_idx(oval)].val;
			outval = _realval(o);
		}
	}
	return idx;
}

template <Squirk Q>
bool SQClass<Q>::SetAttributes(const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val)
{
	SQObjectPtr<Q> idx;
	if(_members->Get(key,idx)) {
		if(_isfield(idx))
			_defaultvalues[_member_idx(idx)].attrs = val;
		else
			_methods[_member_idx(idx)].attrs = val;
		return true;
	}
	return false;
}

template <Squirk Q>
bool SQClass<Q>::GetAttributes(const SQObjectPtr<Q> &key,SQObjectPtr<Q> &outval)
{
	SQObjectPtr<Q> idx;
	if(_members->Get(key,idx)) {
		outval = (_isfield(idx)?_defaultvalues[_member_idx(idx)].attrs:_methods[_member_idx(idx)].attrs);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
template <Squirk Q>
void SQInstance<Q>::Init(SQSharedState<Q> *ss)
{
	_userpointer = NULL;
	_hook = NULL;
	__ObjAddRef(_class);
	SQDelegable<Q>::_delegate = _class->_members;
	INIT_CHAIN();
	ADD_TO_CHAIN(&SQDelegable<Q>::_sharedstate->_gc_chain, this);
}

template <Squirk Q>
SQInstance<Q>::SQInstance(SQSharedState<Q> *ss, SQClass<Q> *c, SQInteger memsize)
{
	_memsize = memsize;
	_class = c;
	SQUnsignedInteger nvalues = _class->_defaultvalues.size();
	for(SQUnsignedInteger n = 0; n < nvalues; n++) {
		new (&_values[n]) SQObjectPtr<Q>(_class->_defaultvalues[n].val);
	}
	Init(ss);
}

template <Squirk Q>
SQInstance<Q>::SQInstance(SQSharedState<Q> *ss, SQInstance<Q> *i, SQInteger memsize)
{
	_memsize = memsize;
	_class = i->_class;
	SQUnsignedInteger nvalues = _class->_defaultvalues.size();
	for(SQUnsignedInteger n = 0; n < nvalues; n++) {
		new (&_values[n]) SQObjectPtr<Q>(i->_values[n]);
	}
	Init(ss);
}

template <Squirk Q>
void SQInstance<Q>::Finalize()
{
	SQUnsignedInteger nvalues = _class->_defaultvalues.size();
	__ObjRelease(_class);
	for(SQUnsignedInteger i = 0; i < nvalues; i++) {
		_values[i] = _null_<Q>;
	}
}

template <Squirk Q>
SQInstance<Q>::~SQInstance()
{
	REMOVE_FROM_CHAIN(&SQDelegable<Q>::_sharedstate->_gc_chain, this);
	if(_class){ Finalize(); } //if _class is null it was already finalized by the GC
}

template <Squirk Q>
bool SQInstance<Q>::GetMetaMethod(SQVM<Q> *v,SQMetaMethod mm,SQObjectPtr<Q> &res)
{
	if(obj_type(_class->_metamethods[mm]) != OT_NULL) {
		res = _class->_metamethods[mm];
		return true;
	}
	return false;
}

template <Squirk Q>
bool SQInstance<Q>::InstanceOf(SQClass<Q> *trg)
{
	SQClass<Q> *parent = _class;
	while(parent != NULL) {
		if(parent == trg)
			return true;
		parent = parent->_base;
	}
	return false;
}
