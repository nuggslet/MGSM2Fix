/*	see copyright notice in squirrel.h */
#ifndef _SQTABLE_H_
#define _SQTABLE_H_
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_ltable.c.html
*/

#include "sqstring.h"


#define hashptr(p)  ((SQHash)(((SQInteger)p) >> 3))

template <Squirk T>
inline SQHash HashObj(const SQObjectPtr<T> &key)
{
	switch(type(key)) {
		case OT_STRING:		return _string(key)->_hash;
		case OT_FLOAT:		return (SQHash)((SQInteger)_float(key));
		case OT_BOOL: case OT_INTEGER:	return (SQHash)((SQInteger)_integer(key));
		default:			return hashptr(key._unVal.pRefCounted);
	}
}

template <Squirk T>
struct SQTable : public SQDelegable<T>
{
private:
	struct _HashNode
	{
		_HashNode() { next = NULL; }
		SQObjectPtr<T> val;
		SQObjectPtr<T> key;
		_HashNode *next;
	};
	_HashNode *_firstfree;
	_HashNode *_nodes;
	SQInteger _numofnodes;
	SQInteger _usednodes;
	
///////////////////////////
	void AllocNodes(SQInteger nSize);
	void Rehash(bool force);
	SQTable(SQSharedState<T> *ss, SQInteger nInitialSize);
	void _ClearNodes();
public:
	static SQTable<T>* Create(SQSharedState<T> *ss,SQInteger nInitialSize)
	{
		SQTable<T> *newtable = (SQTable<T>*)SQ_MALLOC(sizeof(SQTable<T>));
		new (newtable) SQTable<T>(ss, nInitialSize);
		newtable->_delegate = NULL;
		return newtable;
	}
	void Finalize();
	SQTable<T> *Clone();
	~SQTable()
	{
		SQDelegable<T>::SetDelegate(NULL);
		REMOVE_FROM_CHAIN(&SQDelegable<T>::_sharedstate->_gc_chain, this);
		for (SQInteger i = 0; i < _numofnodes; i++) _nodes[i].~_HashNode();
		SQ_FREE(_nodes, _numofnodes * sizeof(_HashNode));
	}
#ifndef NO_GARBAGE_COLLECTOR 
	void Mark(SQCollectable<T> **chain);
#endif
	inline _HashNode *_Get(const SQObjectPtr<T> &key,SQHash hash)
	{
		_HashNode *n = &_nodes[hash];
		do{
			if(_rawval(n->key) == _rawval(key) && type(n->key) == type(key)){
				return n;
			}
		}while((n = n->next));
		return NULL;
	}
	bool Get(const SQObjectPtr<T> &key,SQObjectPtr<T> &val);
	void Remove(const SQObjectPtr<T> &key);
	bool Set(const SQObjectPtr<T> &key, const SQObjectPtr<T> &val);
	//returns true if a new slot has been created false if it was already present
	bool NewSlot(const SQObjectPtr<T> &key,const SQObjectPtr<T> &val);
	SQInteger Next(bool getweakrefs,const SQObjectPtr<T> &refpos, SQObjectPtr<T> &outkey, SQObjectPtr<T> &outval);
	
	SQInteger CountUsed(){ return _usednodes;}
	void Clear();
	void Release()
	{
		sq_delete(this, SQTable<T>);
	}
	
};

template SQTable<Squirk::Standard>;
template SQTable<Squirk::AlignObject>;

#endif //_SQTABLE_H_
