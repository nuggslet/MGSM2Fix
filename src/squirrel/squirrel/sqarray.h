/*	see copyright notice in squirrel.h */
#ifndef _SQARRAY_H_
#define _SQARRAY_H_

template <Squirk Q>
struct SQArray : public CHAINABLE_OBJ<Q>
{
private:
	SQArray(SQSharedState<Q> *ss,SQInteger nsize){_values.resize(nsize); INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
	~SQArray()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
public:
	static SQArray<Q>* Create(SQSharedState<Q> *ss,SQInteger nInitialSize){
		SQArray<Q> *newarray=(SQArray<Q>*)SQ_MALLOC(sizeof(SQArray<Q>));
		new (newarray) SQArray<Q>(ss,nInitialSize);
		return newarray;
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> **chain);
#endif
	void Finalize(){
		_values.resize(0);
	}
	bool Get(const SQInteger nidx,SQObjectPtr<Q> &val)
	{
		if(nidx>=0 && nidx<(SQInteger)_values.size()){
			SQObjectPtr<Q> &o = _values[nidx];
			val = _realval(o);
			return true;
		}
		else return false;
	}
	bool Set(const SQInteger nidx,const SQObjectPtr<Q> &val)
	{
		if(nidx>=0 && nidx<(SQInteger)_values.size()){
			_values[nidx]=val;
			return true;
		}
		else return false;
	}
	SQInteger Next(const SQObjectPtr<Q> &refpos,SQObjectPtr<Q> &outkey,SQObjectPtr<Q> &outval)
	{
		SQUnsignedInteger idx=TranslateIndex(refpos);
		while(idx<_values.size()){
			//first found
			outkey=(SQInteger)idx;
			SQObjectPtr<Q> &o = _values[idx];
			outval = _realval(o);
			//return idx for the next iteration
			return ++idx;
		}
		//nothing to iterate anymore
		return -1;
	}
	SQArray<Q> *Clone(){SQArray<Q> *anew=Create(_opt_ss(this),Size()); anew->_values.copy(_values); return anew; }
	SQInteger Size() const {return _values.size();}
	void Resize(SQInteger size,SQObjectPtr<Q> &fill = _null_<Q>) { _values.resize(size,fill); ShrinkIfNeeded(); }
	void Reserve(SQInteger size) { _values.reserve(size); }
	void Append(const SQObject<Q> &o){_values.push_back(o);}
	void Extend(const SQArray<Q> *a);
	SQObjectPtr<Q> &Top(){return _values.top();}
	void Pop(){_values.pop_back(); ShrinkIfNeeded(); }
	bool Insert(SQInteger idx,const SQObject<Q> &val){
		if(idx < 0 || idx > (SQInteger)_values.size())
			return false;
		_values.insert(idx,val);
		return true;
	}
	void ShrinkIfNeeded() {
		if(_values.size() <= _values.capacity()>>2) //shrink the array
			_values.shrinktofit();
	}
	bool Remove(SQInteger idx){
		if(idx < 0 || idx >= (SQInteger)_values.size())
			return false;
		_values.remove(idx);
		ShrinkIfNeeded();
		return true;
	}
	void Release()
	{
		sq_delete(this,SQArray<Q>);
	}
	SQObjectPtrVec<Q> _values;
};

template SQArray<Squirk::Standard>;
template SQArray<Squirk::AlignObject>;
template SQArray<Squirk::StandardShared>;
template SQArray<Squirk::AlignObjectShared>;

#endif //_SQARRAY_H_
