/*	see copyright notice in squirrel.h */
#ifndef _SQCLASS_H_
#define _SQCLASS_H_

template <Squirk Q>
struct SQInstance;

template <Squirk Q>
struct SQClassMember {
	SQClassMember(){}
	SQClassMember(const SQClassMember<Q> &o) {
		val = o.val;
		attrs = o.attrs;
	}
	SQObjectPtr<Q> val;
	SQObjectPtr<Q> attrs;
};

template <Squirk Q>
using SQClassMemberVec = sqvector<SQClassMember<Q>>;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((SQInteger)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((SQInteger)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

template <Squirk Q>
struct SQClass : public CHAINABLE_OBJ<Q>
{
	SQClass(SQSharedState<Q> *ss,SQClass<Q> *base);
public:
	static SQClass* Create(SQSharedState<Q> *ss,SQClass<Q> *base) {
		SQClass<Q> *newclass = (SQClass<Q> *)SQ_MALLOC(sizeof(SQClass<Q>));
		new (newclass) SQClass<Q>(ss, base);
		return newclass;
	}
	~SQClass();
	bool NewSlot(SQSharedState<Q> *ss, const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val,bool bstatic);
	bool Get(const SQObjectPtr<Q> &key,SQObjectPtr<Q> &val) {
		if(_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr<Q> &o = _defaultvalues[_member_idx(val)].val;
				val = _realval(o);
			}
			else {
				val = _methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool SetAttributes(const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val);
	bool GetAttributes(const SQObjectPtr<Q> &key,SQObjectPtr<Q> &outval);
	void Lock() { _locked = true; if(_base) _base->Lock(); }
	void Release() { 
		if (_hook) { _hook(_typetag,0);}
		sq_delete(this, SQClass<Q>);
	}
	void Finalize();
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> ** );
#endif
	SQInteger Next(const SQObjectPtr<Q> &refpos, SQObjectPtr<Q> &outkey, SQObjectPtr<Q> &outval);
	SQInstance<Q> *CreateInstance();
	SQTable<Q> *_members;
	SQClass<Q> *_base;
	SQClassMemberVec<Q> _defaultvalues;
	SQClassMemberVec<Q> _methods;
	SQObjectPtrVec<Q> _metamethods;
	SQObjectPtr<Q> _attributes;
	SQUserPointer _typetag;
	SQRELEASEHOOK _hook;
	bool _locked;
	SQInteger _udsize;
};

#define calcinstancesize(_theclass_) \
	(_theclass_->_udsize + sizeof(SQInstance<Q>) + (sizeof(SQObjectPtr<Q>)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0)))

template <Squirk Q>
struct SQInstance : public SQDelegable<Q> 
{
	void Init(SQSharedState<Q> *ss);
	SQInstance(SQSharedState<Q> *ss, SQClass<Q> *c, SQInteger memsize);
	SQInstance(SQSharedState<Q> *ss, SQInstance<Q> *c, SQInteger memsize);
public:
	static SQInstance<Q>* Create(SQSharedState<Q> *ss,SQClass<Q> *theclass) {
		
		SQInteger size = calcinstancesize(theclass);
		SQInstance<Q> *newinst = (SQInstance<Q> *)SQ_MALLOC(size);
		new (newinst) SQInstance<Q>(ss, theclass,size);
		if(theclass->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
		}
		return newinst;
	}
	SQInstance<Q> *Clone(SQSharedState<Q> *ss)
	{
		SQInteger size = calcinstancesize(_class);
		SQInstance<Q> *newinst = (SQInstance<Q> *)SQ_MALLOC(size);
		new (newinst) SQInstance<Q>(ss, this,size);
		if(_class->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
		}
		return newinst;
	}
	~SQInstance();
	bool Get(const SQObjectPtr<Q> &key,SQObjectPtr<Q> &val)  {
		if(_class->_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr<Q> &o = _values[_member_idx(val)];
				val = _realval(o);
			}
			else {
				val = _class->_methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool Set(const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val) {
		SQObjectPtr<Q> idx;
		if(_class->_members->Get(key,idx) && _isfield(idx)) {
            _values[_member_idx(idx)] = val;
			return true;
		}
		return false;
	}
	void Release() {
		SQDelegable<Q>::_uiRef++;
		if (_hook) { _hook(_userpointer,0);}
		SQDelegable<Q>::_uiRef--;
		if(SQDelegable<Q>::_uiRef > 0) return;
		SQInteger size = _memsize;
		this->~SQInstance();
		SQ_FREE(this, size);
	}
	void Finalize();
#ifndef NO_GARBAGE_COLLECTOR 
	void Mark(SQCollectable<Q> ** );
#endif
	bool InstanceOf(SQClass<Q> *trg);
	bool GetMetaMethod(SQVM<Q> *v,SQMetaMethod mm,SQObjectPtr<Q> &res);

	SQClass<Q> *_class;
	SQUserPointer _userpointer;
	SQRELEASEHOOK _hook;
	SQInteger _memsize;
	SQObjectPtr<Q> _values[1];
};

template SQClass<Squirk::Standard>;
template SQClass<Squirk::AlignObject>;
template SQClass<Squirk::StandardShared>;
template SQClass<Squirk::AlignObjectShared>;

template SQInstance<Squirk::Standard>;
template SQInstance<Squirk::AlignObject>;
template SQInstance<Squirk::StandardShared>;
template SQInstance<Squirk::AlignObjectShared>;

#endif //_SQCLASS_H_
