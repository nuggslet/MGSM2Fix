/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqclass.h"
#include "sqclosure.h"

template <Squirk T>
SQClass<T>::SQClass(SQSharedState<T> *ss,SQClass<T> *base)
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
	_members = base?base->_members->Clone() : SQTable<T>::Create(ss,0);
	__ObjAddRef(_members);
	_locked = false;
	INIT_CHAIN();
	ADD_TO_CHAIN(&CHAINABLE_OBJ<T>::_sharedstate->_gc_chain, this);
}

template <Squirk T>
void SQClass<T>::Finalize() {
	_attributes = _null_<T>;
	_defaultvalues.resize(0);
	_methods.resize(0);
	_metamethods.resize(0);
	__ObjRelease(_members);
	if(_base) {
		__ObjRelease(_base);
	}
}

template <Squirk T>
SQClass<T>::~SQClass()
{
	REMOVE_FROM_CHAIN(&CHAINABLE_OBJ<T>::_sharedstate->_gc_chain, this);
	Finalize();
}

template <Squirk T>
bool SQClass<T>::NewSlot(SQSharedState<T> *ss,const SQObjectPtr<T> &key,const SQObjectPtr<T> &val,bool bstatic)
{
	SQObjectPtr<T> temp;
	if(_locked) 
		return false; //the class already has an instance so cannot be modified
	if(_members->Get(key,temp) && _isfield(temp)) //overrides the default value
	{
		_defaultvalues[_member_idx(temp)].val = val;
		return true;
	}
	if(type(val) == OT_CLOSURE || type(val) == OT_NATIVECLOSURE || bstatic) {
		SQInteger mmidx;
		if((type(val) == OT_CLOSURE || type(val) == OT_NATIVECLOSURE) && 
			(mmidx = ss->GetMetaMethodIdxByName(key)) != -1) {
			_metamethods[mmidx] = val;
		} 
		else {
			if(type(temp) == OT_NULL) {
				SQClassMember<T> m;
				m.val = val;
				_members->NewSlot(key,SQObjectPtr<T>(_make_method_idx(_methods.size())));
				_methods.push_back(m);
			}
			else {
				_methods[_member_idx(temp)].val = val;
			}
		}
		return true;
	}
	SQClassMember<T> m;
	m.val = val;
	_members->NewSlot(key,SQObjectPtr<T>(_make_field_idx(_defaultvalues.size())));
	_defaultvalues.push_back(m);
	return true;
}

template <Squirk T>
SQInstance<T> *SQClass<T>::CreateInstance()
{
	if(!_locked) Lock();
	return SQInstance<T>::Create(_opt_ss(this),this);
}

template <Squirk T>
SQInteger SQClass<T>::Next(const SQObjectPtr<T> &refpos, SQObjectPtr<T> &outkey, SQObjectPtr<T> &outval)
{
	SQObjectPtr<T> oval;
	SQInteger idx = _members->Next(false,refpos,outkey,oval);
	if(idx != -1) {
		if(_ismethod(oval)) {
			outval = _methods[_member_idx(oval)].val;
		}
		else {
			SQObjectPtr<T> &o = _defaultvalues[_member_idx(oval)].val;
			outval = _realval(o);
		}
	}
	return idx;
}

template <Squirk T>
bool SQClass<T>::SetAttributes(const SQObjectPtr<T> &key,const SQObjectPtr<T> &val)
{
	SQObjectPtr<T> idx;
	if(_members->Get(key,idx)) {
		if(_isfield(idx))
			_defaultvalues[_member_idx(idx)].attrs = val;
		else
			_methods[_member_idx(idx)].attrs = val;
		return true;
	}
	return false;
}

template <Squirk T>
bool SQClass<T>::GetAttributes(const SQObjectPtr<T> &key,SQObjectPtr<T> &outval)
{
	SQObjectPtr<T> idx;
	if(_members->Get(key,idx)) {
		outval = (_isfield(idx)?_defaultvalues[_member_idx(idx)].attrs:_methods[_member_idx(idx)].attrs);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
template <Squirk T>
void SQInstance<T>::Init(SQSharedState<T> *ss)
{
	_userpointer = NULL;
	_hook = NULL;
	__ObjAddRef(_class);
	SQDelegable<T>::_delegate = _class->_members;
	INIT_CHAIN();
	ADD_TO_CHAIN(&SQDelegable<T>::_sharedstate->_gc_chain, this);
}

template <Squirk T>
SQInstance<T>::SQInstance(SQSharedState<T> *ss, SQClass<T> *c, SQInteger memsize)
{
	_memsize = memsize;
	_class = c;
	SQUnsignedInteger nvalues = _class->_defaultvalues.size();
	for(SQUnsignedInteger n = 0; n < nvalues; n++) {
		new (&_values[n]) SQObjectPtr<T>(_class->_defaultvalues[n].val);
	}
	Init(ss);
}

template <Squirk T>
SQInstance<T>::SQInstance(SQSharedState<T> *ss, SQInstance<T> *i, SQInteger memsize)
{
	_memsize = memsize;
	_class = i->_class;
	SQUnsignedInteger nvalues = _class->_defaultvalues.size();
	for(SQUnsignedInteger n = 0; n < nvalues; n++) {
		new (&_values[n]) SQObjectPtr<T>(i->_values[n]);
	}
	Init(ss);
}

template <Squirk T>
void SQInstance<T>::Finalize()
{
	SQUnsignedInteger nvalues = _class->_defaultvalues.size();
	__ObjRelease(_class);
	for(SQUnsignedInteger i = 0; i < nvalues; i++) {
		_values[i] = _null_<T>;
	}
}

template <Squirk T>
SQInstance<T>::~SQInstance()
{
	REMOVE_FROM_CHAIN(&SQDelegable<T>::_sharedstate->_gc_chain, this);
	if(_class){ Finalize(); } //if _class is null it was already finalized by the GC
}

template <Squirk T>
bool SQInstance<T>::GetMetaMethod(SQVM<T> *v,SQMetaMethod mm,SQObjectPtr<T> &res)
{
	if(type(_class->_metamethods[mm]) != OT_NULL) {
		res = _class->_metamethods[mm];
		return true;
	}
	return false;
}

template <Squirk T>
bool SQInstance<T>::InstanceOf(SQClass<T> *trg)
{
	SQClass<T> *parent = _class;
	while(parent != NULL) {
		if(parent == trg)
			return true;
		parent = parent->_base;
	}
	return false;
}
