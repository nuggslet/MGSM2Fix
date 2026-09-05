/*	see copyright notice in squirrel.h */
#ifndef _SQUSERDATA_H_
#define _SQUSERDATA_H_

template <Squirk Q>
struct SQUserData : SQDelegable<Q>
{
	SQUserData(SQSharedState<Q> *ss){ SQDelegable<Q>::_delegate = 0; _hook = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain, this); }
	~SQUserData()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
		SQDelegable<Q>::SetDelegate(NULL);
	}
	static SQUserData<Q>* Create(SQSharedState<Q> *ss, SQInteger size)
	{
		SQUserData<Q>* ud = (SQUserData<Q>*)SQ_MALLOC(sizeof(SQUserData<Q>)+(size-1));
		new (ud) SQUserData<Q>(ss);
		ud->_size = size;
		ud->_typetag = 0;
		return ud;
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> **chain);
	void Finalize(){SQDelegable<Q>::SetDelegate(NULL);}
#endif
	void Release() {
		if (_hook) _hook(_val,_size);
		SQInteger tsize = _size - 1;
		this->~SQUserData();
		SQ_FREE(this, sizeof(SQUserData<Q>) + tsize);
	}
		
	SQInteger _size;
	SQRELEASEHOOK _hook;
	SQUserPointer _typetag;
	SQChar _val[1];
};

template SQUserData<Squirk::Standard>;
template SQUserData<Squirk::AlignObject>;
template SQUserData<Squirk::StandardShared>;
template SQUserData<Squirk::AlignObjectShared>;

#endif //_SQUSERDATA_H_
