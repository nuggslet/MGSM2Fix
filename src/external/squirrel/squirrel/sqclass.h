/*	see copyright notice in squirrel.h */
#ifndef _SQCLASS_H_
#define _SQCLASS_H_

template <Squirk T>
struct SQInstance;

template <Squirk T>
struct SQClassMember {
	SQClassMember(){}
	SQClassMember(const SQClassMember<T> &o) {
		val = o.val;
		attrs = o.attrs;
	}
	SQObjectPtr<T> val;
	SQObjectPtr<T> attrs;
};

template <Squirk T>
using SQClassMemberVec = sqvector<SQClassMember<T>>;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((SQInteger)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((SQInteger)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

template <Squirk T>
struct SQClass : public CHAINABLE_OBJ<T>
{
	SQClass(SQSharedState<T> *ss,SQClass<T> *base);
public:
	static SQClass* Create(SQSharedState<T> *ss,SQClass<T> *base) {
		SQClass<T> *newclass = (SQClass<T> *)SQ_MALLOC(sizeof(SQClass<T>));
		new (newclass) SQClass<T>(ss, base);
		return newclass;
	}
	~SQClass();
	bool NewSlot(SQSharedState<T> *ss, const SQObjectPtr<T> &key,const SQObjectPtr<T> &val,bool bstatic);
	bool Get(const SQObjectPtr<T> &key,SQObjectPtr<T> &val) {
		if(_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr<T> &o = _defaultvalues[_member_idx(val)].val;
				val = _realval(o);
			}
			else {
				val = _methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool SetAttributes(const SQObjectPtr<T> &key,const SQObjectPtr<T> &val);
	bool GetAttributes(const SQObjectPtr<T> &key,SQObjectPtr<T> &outval);
	void Lock() { _locked = true; if(_base) _base->Lock(); }
	void Release() { 
		if (_hook) { _hook(_typetag,0);}
		sq_delete(this, SQClass<T>);
	}
	void Finalize();
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> ** );
#endif
	SQInteger Next(const SQObjectPtr<T> &refpos, SQObjectPtr<T> &outkey, SQObjectPtr<T> &outval);
	SQInstance<T> *CreateInstance();
	SQTable<T> *_members;
	SQClass<T> *_base;
	SQClassMemberVec<T> _defaultvalues;
	SQClassMemberVec<T> _methods;
	SQObjectPtrVec<T> _metamethods;
	SQObjectPtr<T> _attributes;
	SQUserPointer _typetag;
	SQRELEASEHOOK _hook;
	bool _locked;
	SQInteger _udsize;
};

#define calcinstancesize(_theclass_) \
	(_theclass_->_udsize + sizeof(SQInstance<T>) + (sizeof(SQObjectPtr<T>)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0)))

template <Squirk T>
struct SQInstance : public SQDelegable<T> 
{
	void Init(SQSharedState<T> *ss);
	SQInstance(SQSharedState<T> *ss, SQClass<T> *c, SQInteger memsize);
	SQInstance(SQSharedState<T> *ss, SQInstance<T> *c, SQInteger memsize);
public:
	static SQInstance<T>* Create(SQSharedState<T> *ss,SQClass<T> *theclass) {
		
		SQInteger size = calcinstancesize(theclass);
		SQInstance<T> *newinst = (SQInstance<T> *)SQ_MALLOC(size);
		new (newinst) SQInstance<T>(ss, theclass,size);
		if(theclass->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
		}
		return newinst;
	}
	SQInstance<T> *Clone(SQSharedState<T> *ss)
	{
		SQInteger size = calcinstancesize(_class);
		SQInstance<T> *newinst = (SQInstance<T> *)SQ_MALLOC(size);
		new (newinst) SQInstance<T>(ss, this,size);
		if(_class->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
		}
		return newinst;
	}
	~SQInstance();
	bool Get(const SQObjectPtr<T> &key,SQObjectPtr<T> &val)  {
		if(_class->_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr<T> &o = _values[_member_idx(val)];
				val = _realval(o);
			}
			else {
				val = _class->_methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool Set(const SQObjectPtr<T> &key,const SQObjectPtr<T> &val) {
		SQObjectPtr<T> idx;
		if(_class->_members->Get(key,idx) && _isfield(idx)) {
            _values[_member_idx(idx)] = val;
			return true;
		}
		return false;
	}
	void Release() {
		SQDelegable<T>::_uiRef++;
		if (_hook) { _hook(_userpointer,0);}
		SQDelegable<T>::_uiRef--;
		if(SQDelegable<T>::_uiRef > 0) return;
		SQInteger size = _memsize;
		this->~SQInstance();
		SQ_FREE(this, size);
	}
	void Finalize();
#ifndef NO_GARBAGE_COLLECTOR 
	void Mark(SQCollectable<T> ** );
#endif
	bool InstanceOf(SQClass<T> *trg);
	bool GetMetaMethod(SQVM<T> *v,SQMetaMethod mm,SQObjectPtr<T> &res);

	SQClass<T> *_class;
	SQUserPointer _userpointer;
	SQRELEASEHOOK _hook;
	SQInteger _memsize;
	SQObjectPtr<T> _values[1];
};

template SQClass<Squirk::Standard>;
template SQClass<Squirk::AlignObject>;

template SQInstance<Squirk::Standard>;
template SQInstance<Squirk::AlignObject>;

#endif //_SQCLASS_H_
