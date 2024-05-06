/*	see copyright notice in squirrel.h */
#ifndef _SQARRAY_H_
#define _SQARRAY_H_

template <Squirk T>
struct SQArray : public CHAINABLE_OBJ<T>
{
private:
	SQArray(SQSharedState<T> *ss,SQInteger nsize){_values.resize(nsize); INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
	~SQArray()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
public:
	static SQArray<T>* Create(SQSharedState<T> *ss,SQInteger nInitialSize){
		SQArray<T> *newarray=(SQArray<T>*)SQ_MALLOC(sizeof(SQArray<T>));
		new (newarray) SQArray<T>(ss,nInitialSize);
		return newarray;
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> **chain);
#endif
	void Finalize(){
		_values.resize(0);
	}
	bool Get(const SQInteger nidx,SQObjectPtr<T> &val)
	{
		if(nidx>=0 && nidx<(SQInteger)_values.size()){
			SQObjectPtr<T> &o = _values[nidx];
			val = _realval(o);
			return true;
		}
		else return false;
	}
	bool Set(const SQInteger nidx,const SQObjectPtr<T> &val)
	{
		if(nidx>=0 && nidx<(SQInteger)_values.size()){
			_values[nidx]=val;
			return true;
		}
		else return false;
	}
	SQInteger Next(const SQObjectPtr<T> &refpos,SQObjectPtr<T> &outkey,SQObjectPtr<T> &outval)
	{
		SQUnsignedInteger idx=TranslateIndex(refpos);
		while(idx<_values.size()){
			//first found
			outkey=(SQInteger)idx;
			SQObjectPtr<T> &o = _values[idx];
			outval = _realval(o);
			//return idx for the next iteration
			return ++idx;
		}
		//nothing to iterate anymore
		return -1;
	}
	SQArray<T> *Clone(){SQArray<T> *anew=Create(_opt_ss(this),Size()); anew->_values.copy(_values); return anew; }
	SQInteger Size() const {return _values.size();}
	void Resize(SQInteger size,SQObjectPtr<T> &fill = _null_<T>) { _values.resize(size,fill); ShrinkIfNeeded(); }
	void Reserve(SQInteger size) { _values.reserve(size); }
	void Append(const SQObject<T> &o){_values.push_back(o);}
	void Extend(const SQArray<T> *a);
	SQObjectPtr<T> &Top(){return _values.top();}
	void Pop(){_values.pop_back(); ShrinkIfNeeded(); }
	bool Insert(SQInteger idx,const SQObject<T> &val){
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
		sq_delete(this,SQArray<T>);
	}
	SQObjectPtrVec<T> _values;
};

template SQArray<Squirk::Standard>;
template SQArray<Squirk::AlignObject>;

#endif //_SQARRAY_H_
