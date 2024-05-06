/*	see copyright notice in squirrel.h */
#ifndef _SQUSERDATA_H_
#define _SQUSERDATA_H_

template <Squirk T>
struct SQUserData : SQDelegable<T>
{
	SQUserData(SQSharedState<T> *ss){ SQDelegable<T>::_delegate = 0; _hook = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain, this); }
	~SQUserData()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
		SQDelegable<T>::SetDelegate(NULL);
	}
	static SQUserData<T>* Create(SQSharedState<T> *ss, SQInteger size)
	{
		SQUserData<T>* ud = (SQUserData<T>*)SQ_MALLOC(sizeof(SQUserData<T>)+(size-1));
		new (ud) SQUserData<T>(ss);
		ud->_size = size;
		ud->_typetag = 0;
		return ud;
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> **chain);
	void Finalize(){SQDelegable<T>::SetDelegate(NULL);}
#endif
	void Release() {
		if (_hook) _hook(_val,_size);
		SQInteger tsize = _size - 1;
		this->~SQUserData();
		SQ_FREE(this, sizeof(SQUserData<T>) + tsize);
	}
		
	SQInteger _size;
	SQRELEASEHOOK _hook;
	SQUserPointer _typetag;
	SQChar _val[1];
};

template SQUserData<Squirk::Standard>;
template SQUserData<Squirk::AlignObject>;

#endif //_SQUSERDATA_H_
