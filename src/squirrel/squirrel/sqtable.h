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

template <Squirk Q>
inline SQHash HashObj(const SQObjectPtr<Q> &key)
{
	switch(obj_type(key)) {
		case OT_STRING:		return _string(key)->_hash;
		case OT_FLOAT:		return (SQHash)((SQInteger)_float(key));
		case OT_BOOL: case OT_INTEGER:	return (SQHash)((SQInteger)_integer(key));
		default:			return hashptr(key._unVal.pRefCounted);
	}
}

template <Squirk Q>
struct SQTable : public SQDelegable<Q>
{
private:
	struct _HashNode
	{
		_HashNode() { next = NULL; }
		SQObjectPtr<Q> val;
		SQObjectPtr<Q> key;
		_HashNode *next;
	};
	_HashNode *_firstfree;
	_HashNode *_nodes;
	SQInteger _numofnodes;
	SQInteger _usednodes;
	
///////////////////////////
	void AllocNodes(SQInteger nSize);
	void Rehash(bool force);
	SQTable(SQSharedState<Q> *ss, SQInteger nInitialSize);
	void _ClearNodes();
public:
	static SQTable<Q>* Create(SQSharedState<Q> *ss,SQInteger nInitialSize)
	{
		SQTable<Q> *newtable = (SQTable<Q>*)SQ_MALLOC(sizeof(SQTable<Q>));
		new (newtable) SQTable<Q>(ss, nInitialSize);
		newtable->_delegate = NULL;
		return newtable;
	}
	void Finalize();
	SQTable<Q> *Clone();
	~SQTable()
	{
		SQDelegable<Q>::SetDelegate(NULL);
		REMOVE_FROM_CHAIN(&SQDelegable<Q>::_sharedstate->_gc_chain, this);
		for (SQInteger i = 0; i < _numofnodes; i++) _nodes[i].~_HashNode();
		SQ_FREE(_nodes, _numofnodes * sizeof(_HashNode));
	}
#ifndef NO_GARBAGE_COLLECTOR 
	void Mark(SQCollectable<Q> **chain);
#endif
	inline _HashNode *_Get(const SQObjectPtr<Q> &key,SQHash hash)
	{
		_HashNode *n = &_nodes[hash];
		do{
			if(_rawval(n->key) == _rawval(key) && obj_type(n->key) == obj_type(key)){
				return n;
			}
		}while((n = n->next));
		return NULL;
	}
	bool Get(const SQObjectPtr<Q> &key,SQObjectPtr<Q> &val);
	void Remove(const SQObjectPtr<Q> &key);
	bool Set(const SQObjectPtr<Q> &key, const SQObjectPtr<Q> &val);
	//returns true if a new slot has been created false if it was already present
	bool NewSlot(const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val);
	SQInteger Next(bool getweakrefs,const SQObjectPtr<Q> &refpos, SQObjectPtr<Q> &outkey, SQObjectPtr<Q> &outval);
	
	SQInteger CountUsed(){ return _usednodes;}
	void Clear();
	void Release()
	{
		sq_delete(this, SQTable<Q>);
	}
	
};

template SQTable<Squirk::Standard>;
template SQTable<Squirk::AlignObject>;
template SQTable<Squirk::StandardShared>;
template SQTable<Squirk::AlignObjectShared>;

#endif //_SQTABLE_H_
