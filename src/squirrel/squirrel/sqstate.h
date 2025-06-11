/*	see copyright notice in squirrel.h */
#ifndef _SQSTATE_H_
#define _SQSTATE_H_

#include "squtils.h"
#include "sqobject.h"
template <Squirk Q>
struct SQString;
template <Squirk Q>
struct SQTable;
//max number of character for a printed number
#define NUMBER_MAX_CHAR 50

template <Squirk Q>
struct StringTable
{
	StringTable();
	~StringTable();
	SQString<Q> *Add(const SQChar *,SQInteger len);
	void Remove(SQString<Q> *);
private:
	void Resize(SQInteger size);
	void AllocNodes(SQInteger size);
	SQString<Q> **_strings;
	SQUnsignedInteger _numofslots;
	SQUnsignedInteger _slotused;
};

template <Squirk Q>
struct RefTable {
	struct RefNode {
		SQObjectPtr<Q> obj;
		SQUnsignedInteger refs;
		struct RefNode *next;
	};
	RefTable();
	~RefTable();
	void AddRef(SQObject<Q> &obj);
	SQBool Release(SQObject<Q> &obj);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<Q> **chain);
#endif
	void Finalize();
private:
	RefNode *Get(SQObject<Q> &obj,SQHash &mainpos,RefNode **prev,bool add);
	RefNode *Add(SQHash mainpos,SQObject<Q> &obj);
	void Resize(SQUnsignedInteger size);
	void AllocNodes(SQUnsignedInteger size);
	SQUnsignedInteger _numofslots;
	SQUnsignedInteger _slotused;
	RefNode *_nodes;
	RefNode *_freelist;
	RefNode **_buckets;
};

#define ADD_STRING(ss,str,len) ss->_stringtable->Add(str,len)
#define REMOVE_STRING(ss,bstr) ss->_stringtable->Remove(bstr)

template <Squirk Q>
struct SQObjectPtr;

#if defined(_SQ_M2)
template <Squirk Q>
struct M2SharedStateExt {
	SQObjectPtr<Q> _unknown;
	SQChar *_scratchpad;
	operator SQChar *() const {
		return _scratchpad;
	}
	SQChar *operator=(SQChar *scratchpad) {
		_scratchpad = scratchpad;
		return _scratchpad;
	}
};

template <>
struct M2SharedStateExt<Squirk::AlignObjectShared>
{
	SQObjectPtr<Squirk::Standard> _unknown; // Very demure.
	SQInteger _pad0;
	SQInteger _pad1;
	SQChar *_scratchpad;

	operator SQChar *() const {
		return _scratchpad;
	}
	SQChar *operator=(SQChar *scratchpad) {
		_scratchpad = scratchpad;
		return _scratchpad;
	}
};
#endif

template <Squirk Q>
struct SQSharedState
{
	SQSharedState();
	~SQSharedState();
	void Init();
public:
	SQChar* GetScratchPad(SQInteger size);
	SQInteger GetMetaMethodIdxByName(const SQObjectPtr<Q> &name);
#ifndef NO_GARBAGE_COLLECTOR
	SQInteger CollectGarbage(SQVM<Q> *vm); 
	static void MarkObject(SQObjectPtr<Q> &o,SQCollectable<Q> **chain);
#endif
	SQObjectPtrVec<Q> *_metamethods;
	SQObjectPtr<Q> _metamethodsmap;
	SQObjectPtrVec<Q> *_systemstrings;
	SQObjectPtrVec<Q> *_types;
	StringTable<Q> *_stringtable;
	RefTable<Q> _refs_table;
	SQObjectPtr<Q> _registry;
	SQObjectPtr<Q> _consts;
	SQObjectPtr<Q> _constructoridx;
#ifndef NO_GARBAGE_COLLECTOR
	SQCollectable<Q> *_gc_chain;
#endif
	SQObjectPtr<Q> _root_vm;
	SQObjectPtr<Q> _table_default_delegate;
	static SQRegFunction<Q> _table_default_delegate_funcz[];
	SQObjectPtr<Q> _array_default_delegate;
	static SQRegFunction<Q> _array_default_delegate_funcz[];
	SQObjectPtr<Q> _string_default_delegate;
	static SQRegFunction<Q> _string_default_delegate_funcz[];
	SQObjectPtr<Q> _number_default_delegate;
	static SQRegFunction<Q> _number_default_delegate_funcz[];
	SQObjectPtr<Q> _generator_default_delegate;
	static SQRegFunction<Q> _generator_default_delegate_funcz[];
	SQObjectPtr<Q> _closure_default_delegate;
	static SQRegFunction<Q> _closure_default_delegate_funcz[];
	SQObjectPtr<Q> _thread_default_delegate;
	static SQRegFunction<Q> _thread_default_delegate_funcz[];
	SQObjectPtr<Q> _class_default_delegate;
	static SQRegFunction<Q> _class_default_delegate_funcz[];
	SQObjectPtr<Q> _instance_default_delegate;
	static SQRegFunction<Q> _instance_default_delegate_funcz[];
	SQObjectPtr<Q> _weakref_default_delegate;
	static SQRegFunction<Q> _weakref_default_delegate_funcz[];
	
	SQCOMPILERERROR<Q> _compilererrorhandler;
	SQPRINTFUNCTION<Q> _printfunc;
	SQPRINTFUNCTION<Q> _errorfunc;
	bool _debuginfo;
	bool _notifyallexceptions;

#if !defined(_SQ_M2)
private:
#endif
#if defined(_SQ_M2)
	std::conditional_t<
		Q == Squirk::StandardShared ||
		Q == Squirk::AlignObjectShared,
		M2SharedStateExt<Q>, SQChar *>
		_scratchpad;
#else
	SQChar *_scratchpad;
#endif
	SQInteger _scratchpadsize;
};

#define _sp(s) (_sharedstate->GetScratchPad(s))
#define _spval (_sharedstate->GetScratchPad(-1))

#define _table_ddel		_table(_sharedstate->_table_default_delegate) 
#define _array_ddel		_table(_sharedstate->_array_default_delegate) 
#define _string_ddel	_table(_sharedstate->_string_default_delegate) 
#define _number_ddel	_table(_sharedstate->_number_default_delegate) 
#define _generator_ddel	_table(_sharedstate->_generator_default_delegate) 
#define _closure_ddel	_table(_sharedstate->_closure_default_delegate) 
#define _thread_ddel	_table(_sharedstate->_thread_default_delegate) 
#define _class_ddel		_table(_sharedstate->_class_default_delegate) 
#define _instance_ddel	_table(_sharedstate->_instance_default_delegate) 
#define _weakref_ddel	_table(_sharedstate->_weakref_default_delegate) 

#ifdef SQUNICODE //rsl REAL STRING LEN
#define rsl(l) ((l)<<1)
#else
#define rsl(l) (l)
#endif

template SQSharedState<Squirk::Standard>;
template SQSharedState<Squirk::AlignObject>;
template SQSharedState<Squirk::StandardShared>;
template SQSharedState<Squirk::AlignObjectShared>;

template StringTable<Squirk::Standard>;
template StringTable<Squirk::AlignObject>;
template StringTable<Squirk::StandardShared>;
template StringTable<Squirk::AlignObjectShared>;

template RefTable<Squirk::Standard>;
template RefTable<Squirk::AlignObject>;
template RefTable<Squirk::StandardShared>;
template RefTable<Squirk::AlignObjectShared>;

namespace
{
	template <Squirk Q>
	SQObjectPtr<Q> _null_;
	template <Squirk Q>
	SQObjectPtr<Q> _true_{ true };
	template <Squirk Q>
	SQObjectPtr<Q> _false_{ false };
	template <Squirk Q>
	SQObjectPtr<Q> _one_{ (SQInteger)1 };
	template <Squirk Q>
	SQObjectPtr<Q> _minusone_{ (SQInteger)-1 };
}

bool CompileTypemask(SQIntVec &res,const SQChar *typemask);

void *sq_vm_malloc(SQUnsignedInteger size);
void *sq_vm_realloc(void *p,SQUnsignedInteger oldsize,SQUnsignedInteger size);
void sq_vm_free(void *p,SQUnsignedInteger size);
#endif //_SQSTATE_H_
