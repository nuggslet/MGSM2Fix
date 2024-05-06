/*	see copyright notice in squirrel.h */
#ifndef _SQSTATE_H_
#define _SQSTATE_H_

#include "squtils.h"
#include "sqobject.h"
template <Squirk T>
struct SQString;
template <Squirk T>
struct SQTable;
//max number of character for a printed number
#define NUMBER_MAX_CHAR 50

template <Squirk T>
struct StringTable
{
	StringTable();
	~StringTable();
	SQString<T> *Add(const SQChar *,SQInteger len);
	void Remove(SQString<T> *);
private:
	void Resize(SQInteger size);
	void AllocNodes(SQInteger size);
	SQString<T> **_strings;
	SQUnsignedInteger _numofslots;
	SQUnsignedInteger _slotused;
};

template <Squirk T>
struct RefTable {
	struct RefNode {
		SQObjectPtr<T> obj;
		SQUnsignedInteger refs;
		struct RefNode *next;
	};
	RefTable();
	~RefTable();
	void AddRef(SQObject<T> &obj);
	SQBool Release(SQObject<T> &obj);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable<T> **chain);
#endif
	void Finalize();
private:
	RefNode *Get(SQObject<T> &obj,SQHash &mainpos,RefNode **prev,bool add);
	RefNode *Add(SQHash mainpos,SQObject<T> &obj);
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

template <Squirk T>
struct SQObjectPtr;

template <Squirk T>
struct SQSharedState
{
	SQSharedState();
	~SQSharedState();
	void Init();
public:
	SQChar* GetScratchPad(SQInteger size);
	SQInteger GetMetaMethodIdxByName(const SQObjectPtr<T> &name);
#ifndef NO_GARBAGE_COLLECTOR
	SQInteger CollectGarbage(SQVM<T> *vm); 
	static void MarkObject(SQObjectPtr<T> &o,SQCollectable<T> **chain);
#endif
	SQObjectPtrVec<T> *_metamethods;
	SQObjectPtr<T> _metamethodsmap;
	SQObjectPtrVec<T> *_systemstrings;
	SQObjectPtrVec<T> *_types;
	StringTable<T> *_stringtable;
	RefTable<T> _refs_table;
	SQObjectPtr<T> _registry;
	SQObjectPtr<T> _consts;
	SQObjectPtr<T> _constructoridx;
#ifndef NO_GARBAGE_COLLECTOR
	SQCollectable<T> *_gc_chain;
#endif
	SQObjectPtr<T> _root_vm;
	SQObjectPtr<T> _table_default_delegate;
	static SQRegFunction<T> _table_default_delegate_funcz[];
	SQObjectPtr<T> _array_default_delegate;
	static SQRegFunction<T> _array_default_delegate_funcz[];
	SQObjectPtr<T> _string_default_delegate;
	static SQRegFunction<T> _string_default_delegate_funcz[];
	SQObjectPtr<T> _number_default_delegate;
	static SQRegFunction<T> _number_default_delegate_funcz[];
	SQObjectPtr<T> _generator_default_delegate;
	static SQRegFunction<T> _generator_default_delegate_funcz[];
	SQObjectPtr<T> _closure_default_delegate;
	static SQRegFunction<T> _closure_default_delegate_funcz[];
	SQObjectPtr<T> _thread_default_delegate;
	static SQRegFunction<T> _thread_default_delegate_funcz[];
	SQObjectPtr<T> _class_default_delegate;
	static SQRegFunction<T> _class_default_delegate_funcz[];
	SQObjectPtr<T> _instance_default_delegate;
	static SQRegFunction<T> _instance_default_delegate_funcz[];
	SQObjectPtr<T> _weakref_default_delegate;
	static SQRegFunction<T> _weakref_default_delegate_funcz[];
	
	SQCOMPILERERROR<T> _compilererrorhandler;
	SQPRINTFUNCTION<T> _printfunc;
	SQPRINTFUNCTION<T> _errorfunc;
	bool _debuginfo;
	bool _notifyallexceptions;

#if defined(_SQ_M2) && defined(_WIN64)
	void *_m2_unknown_0;
	void *_m2_unknown_1;
#endif

#if !defined(_SQ_M2)
private:
#endif
	SQChar *_scratchpad;
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

template StringTable<Squirk::Standard>;
template StringTable<Squirk::AlignObject>;

template RefTable<Squirk::Standard>;
template RefTable<Squirk::AlignObject>;

template <Squirk T>
SQObjectPtr<T> _null_;
template <Squirk T>
SQObjectPtr<T> _true_(true);
template <Squirk T>
SQObjectPtr<T> _false_(false);
template <Squirk T>
SQObjectPtr<T> _one_((SQInteger)1);
template <Squirk T>
SQObjectPtr<T> _minusone_((SQInteger)-1);

template SQObjectPtr<Squirk::Standard> _null_<Squirk::Standard>;
template SQObjectPtr<Squirk::AlignObject> _null_<Squirk::AlignObject>;
template SQObjectPtr<Squirk::Standard> _true_<Squirk::Standard>;
template SQObjectPtr<Squirk::AlignObject> _true_<Squirk::AlignObject>;
template SQObjectPtr<Squirk::Standard> _false_<Squirk::Standard>;
template SQObjectPtr<Squirk::AlignObject> _false_<Squirk::AlignObject>;
template SQObjectPtr<Squirk::Standard> _one_<Squirk::Standard>;
template SQObjectPtr<Squirk::AlignObject> _one_<Squirk::AlignObject>;
template SQObjectPtr<Squirk::Standard> _minusone_<Squirk::Standard>;
template SQObjectPtr<Squirk::AlignObject> _minusone_<Squirk::AlignObject>;

bool CompileTypemask(SQIntVec &res,const SQChar *typemask);

void *sq_vm_malloc(SQUnsignedInteger size);
void *sq_vm_realloc(void *p,SQUnsignedInteger oldsize,SQUnsignedInteger size);
void sq_vm_free(void *p,SQUnsignedInteger size);
#endif //_SQSTATE_H_
