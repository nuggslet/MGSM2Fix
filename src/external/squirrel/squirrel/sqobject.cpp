/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqarray.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqfuncproto.h"
#include "sqclass.h"
#include "sqclosure.h"


const SQChar *IdType2Name(SQObjectType type)
{
	switch(_RAW_TYPE(type))
	{
	case _RT_NULL:return _SC("null");
	case _RT_INTEGER:return _SC("integer");
	case _RT_FLOAT:return _SC("float");
	case _RT_BOOL:return _SC("bool");
	case _RT_STRING:return _SC("string");
	case _RT_TABLE:return _SC("table");
	case _RT_ARRAY:return _SC("array");
	case _RT_GENERATOR:return _SC("generator");
	case _RT_CLOSURE:
	case _RT_NATIVECLOSURE:
		return _SC("function");
	case _RT_USERDATA:
	case _RT_USERPOINTER:
		return _SC("userdata");
	case _RT_THREAD: return _SC("thread");
	case _RT_FUNCPROTO: return _SC("function");
	case _RT_CLASS: return _SC("class");
	case _RT_INSTANCE: return _SC("instance");
	case _RT_WEAKREF: return _SC("weakref");
	default:
		return NULL;
	}
}

template <Squirk T>
const SQChar *GetTypeName(const SQObjectPtr<T> &obj1)
{
	return IdType2Name(type(obj1));	
}

template <Squirk T>
SQString<T> *SQString<T>::Create(SQSharedState<T> *ss,const SQChar *s,SQInteger len)
{
	SQString<T> *str=ADD_STRING(ss,s,len);
	str->_sharedstate=ss;
	return str;
}

template <Squirk T>
void SQString<T>::Release()
{
	REMOVE_STRING(_sharedstate,this);
}

template <Squirk T>
SQInteger SQString<T>::Next(const SQObjectPtr<T> &refpos, SQObjectPtr<T> &outkey, SQObjectPtr<T> &outval)
{
	SQInteger idx = (SQInteger)TranslateIndex(refpos);
	while(idx < _len){
		outkey = (SQInteger)idx;
		outval = SQInteger(_val[idx]);
		//return idx for the next iteration
		return ++idx;
	}
	//nothing to iterate anymore
	return -1;
}

template <Squirk T>
SQUnsignedInteger TranslateIndex(const SQObjectPtr<T> &idx)
{
	switch(type(idx)){
		case OT_NULL:
			return 0;
		case OT_INTEGER:
			return (SQUnsignedInteger)_integer(idx);
		default: assert(0); break;
	}
	return 0;
}

template <Squirk T>
SQWeakRef<T> *SQRefCounted<T>::GetWeakRef(SQObjectType type)
{
	if(!_weakref) {
		sq_new(_weakref,SQWeakRef<T>);
		_weakref->_obj._type = type;
		_weakref->_obj._unVal.pRefCounted = this;
	}
	return _weakref;
}

template <Squirk T>
SQRefCounted<T>::~SQRefCounted()
{
	if(_weakref) {
		_weakref->_obj._type = OT_NULL;
		_weakref->_obj._unVal.pRefCounted = NULL;
	}
}

template <Squirk T>
void SQWeakRef<T>::Release() { 
	if(ISREFCOUNTED(_obj._type)) { 
		_obj._unVal.pRefCounted->_weakref = NULL;
	} 
	sq_delete(this,SQWeakRef<T>);
}

template <Squirk T>
bool SQDelegable<T>::GetMetaMethod(SQVM<T> *v,SQMetaMethod mm,SQObjectPtr<T> &res) {
	if(_delegate) {
		return _delegate->Get((*_ss(v)->_metamethods)[mm],res);
	}
	return false;
}

template <Squirk T>
bool SQDelegable<T>::SetDelegate(SQTable<T> *mt)
{
	SQTable<T> *temp = mt;
	if(temp == this) return false;
	while (temp) {
		if (temp->_delegate == this) return false; //cycle detected
		temp = temp->_delegate;
	}
	if (mt)	__ObjAddRef(mt);
	__ObjRelease(_delegate);
	_delegate = mt;
	return true;
}

template <Squirk T>
bool SQGenerator<T>::Yield(SQVM<T> *v)
{
	if(_state==eSuspended) { v->Raise_Error(_SC("internal vm error, yielding dead generator"));  return false;}
	if(_state==eDead) { v->Raise_Error(_SC("internal vm error, yielding a dead generator")); return false; }
	SQInteger size = v->_top-v->_stackbase;
	_ci=*v->ci;
	_stack.resize(size);
	for(SQInteger n =0; n<size; n++) {
		_stack._vals[n] = v->_stack[v->_stackbase+n];
		v->_stack[v->_stackbase+n] = _null_<T>;
	}
	SQInteger nvargs = v->ci->_vargs.size;
	SQInteger vargsbase = v->ci->_vargs.base;
	for(SQInteger j = nvargs - 1; j >= 0; j--) {
		_vargsstack.push_back(v->_vargsstack[vargsbase+j]);
	}
	_ci._generator=NULL;
	for(SQInteger i=0;i<_ci._etraps;i++) {
		_etraps.push_back(v->_etraps.top());
		v->_etraps.pop_back();
	}
	_state=eSuspended;
	return true;
}

template <Squirk T>
bool SQGenerator<T>::Resume(SQVM<T> *v,SQInteger target)
{
	SQInteger size=_stack.size();
	if(_state==eDead){ v->Raise_Error(_SC("resuming dead generator")); return false; }
	if(_state==eRunning){ v->Raise_Error(_SC("resuming active generator")); return false; }
	SQInteger prevtop=v->_top-v->_stackbase;
	PUSH_CALLINFO(v,_ci);
	SQInteger oldstackbase=v->_stackbase;
	v->_stackbase = v->_top;
	v->ci->_target = (SQInt32)target;
	v->ci->_generator = this;
	v->ci->_vargs.size = (unsigned short)_vargsstack.size();
	
	for(SQInteger i=0;i<_ci._etraps;i++) {
		v->_etraps.push_back(_etraps.top());
		_etraps.pop_back();
	}
	for(SQInteger n =0; n<size; n++) {
		v->_stack[v->_stackbase+n] = _stack._vals[n];
		_stack._vals[0] = _null_<T>;
	}
	while(_vargsstack.size()) {
		v->_vargsstack.push_back(_vargsstack.back());
		_vargsstack.pop_back();
	}
	v->ci->_vargs.base = (unsigned short)(v->_vargsstack.size() - v->ci->_vargs.size);
	v->_top=v->_stackbase+size;
	v->ci->_prevtop = (SQInt32)prevtop;
	v->ci->_prevstkbase = (SQInt32)(v->_stackbase - oldstackbase);
	_state=eRunning;
	if (type(v->_debughook) != OT_NULL && _rawval(v->_debughook) != _rawval(v->ci->_closure))
		v->CallDebugHook(_SC('c'));

	return true;
}

template <Squirk T>
void SQArray<T>::Extend(const SQArray<T> *a){
	SQInteger xlen;
	if((xlen=a->Size()))
		for(SQInteger i=0;i<xlen;i++)
			Append(a->_values[i]);
}

template <Squirk T>
const SQChar* SQFunctionProto<T>::GetLocal(SQVM<T> *vm,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop)
{
	SQUnsignedInteger nvars=_nlocalvarinfos;
	const SQChar *res=NULL; 
	if(nvars>=nseq){
 		for(SQUnsignedInteger i=0;i<nvars;i++){
			if(_localvarinfos[i]._start_op<=nop && _localvarinfos[i]._end_op>=nop)
			{
				if(nseq==0){
					vm->Push(vm->_stack[stackbase+_localvarinfos[i]._pos]);
					res=_stringval(_localvarinfos[i]._name);
					break;
				}
				nseq--;
			}
		}
	}
	return res;
}

template <Squirk T>
SQInteger SQFunctionProto<T>::GetLine(SQInstruction *curr)
{
	SQInteger op = (SQInteger)(curr-_instructions);
	SQInteger line=_lineinfos[0]._line;
	for(SQInteger i=1;i<_nlineinfos;i++){
		if(_lineinfos[i]._op>=op)
			return line;
		line=_lineinfos[i]._line;
	}
	return line;
}

#define _CHECK_IO(exp)  { if(!exp)return false; }
template <Squirk T>
bool SafeWrite(HSQUIRRELVM<T> v,SQWRITEFUNC write,SQUserPointer up,SQUserPointer dest,SQInteger size)
{
	if(write(up,dest,size) != size) {
		v->Raise_Error(_SC("io error (write function failure)"));
		return false;
	}
	return true;
}

template <Squirk T>
bool SafeRead(HSQUIRRELVM<T> v,SQWRITEFUNC read,SQUserPointer up,SQUserPointer dest,SQInteger size)
{
	if(size && read(up,dest,size) != size) {
		v->Raise_Error(_SC("io error, read function failure, the origin stream could be corrupted/trucated"));
		return false;
	}
	return true;
}

template <Squirk T>
bool WriteTag(HSQUIRRELVM<T> v,SQWRITEFUNC write,SQUserPointer up,SQInteger tag)
{
	return SafeWrite(v,write,up,&tag,sizeof(tag));
}

template <Squirk T>
bool CheckTag(HSQUIRRELVM<T> v,SQWRITEFUNC read,SQUserPointer up,SQInteger tag)
{
	SQInteger t;
	_CHECK_IO(SafeRead(v,read,up,&t,sizeof(t)));
	if(t != tag){
		v->Raise_Error(_SC("invalid or corrupted closure stream"));
		return false;
	}
	return true;
}

template <Squirk T>
bool WriteObject(HSQUIRRELVM<T> v,SQUserPointer up,SQWRITEFUNC write,SQObjectPtr<T> &o)
{
	_CHECK_IO(SafeWrite(v,write,up,&type(o),sizeof(SQObjectType)));
	switch(type(o)){
	case OT_STRING:
		_CHECK_IO(SafeWrite(v,write,up,&_string(o)->_len,sizeof(SQInteger)));
		_CHECK_IO(SafeWrite(v,write,up,_stringval(o),rsl(_string(o)->_len)));
		break;
	case OT_INTEGER:
		_CHECK_IO(SafeWrite(v,write,up,&_integer(o),sizeof(SQInteger)));break;
	case OT_FLOAT:
		_CHECK_IO(SafeWrite(v,write,up,&_float(o),sizeof(SQFloat)));break;
	case OT_NULL:
		break;
	default:
		v->Raise_Error(_SC("cannot serialize a %s"),GetTypeName(o));
		return false;
	}
	return true;
}

template <Squirk T>
bool ReadObject(HSQUIRRELVM<T> v,SQUserPointer up,SQREADFUNC read,SQObjectPtr<T> &o)
{
	SQObjectType t;
	_CHECK_IO(SafeRead(v,read,up,&t,sizeof(SQObjectType)));
	switch(t){
	case OT_STRING:{
		SQInteger len;
		_CHECK_IO(SafeRead(v,read,up,&len,sizeof(SQInteger)));
		_CHECK_IO(SafeRead(v,read,up,_ss(v)->GetScratchPad(rsl(len)),rsl(len)));
		o=SQString<T>::Create(_ss(v),_ss(v)->GetScratchPad(-1),len);
				   }
		break;
	case OT_INTEGER:{
		SQInteger i;
		_CHECK_IO(SafeRead(v,read,up,&i,sizeof(SQInteger))); o = i; break;
					}
	case OT_FLOAT:{
		SQFloat f;
		_CHECK_IO(SafeRead(v,read,up,&f,sizeof(SQFloat))); o = f; break;
				  }
	case OT_NULL:
		o=_null_<T>;
		break;
	default:
		v->Raise_Error(_SC("cannot serialize a %s"),IdType2Name(t));
		return false;
	}
	return true;
}

template <Squirk T>
bool SQClosure<T>::Save(SQVM<T> *v,SQUserPointer up,SQWRITEFUNC write)
{
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_HEAD));
	_CHECK_IO(WriteTag(v,write,up,sizeof(SQChar)));
	_CHECK_IO(_funcproto(_function)->Save(v,up,write));
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_TAIL));
	return true;
}

template <Squirk T>
bool SQClosure<T>::Load(SQVM<T> *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr<T> &ret)
{
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_HEAD));
	_CHECK_IO(CheckTag(v,read,up,sizeof(SQChar)));
	SQObjectPtr<T> func;
	_CHECK_IO(SQFunctionProto<T>::Load(v,up,read,func));
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_TAIL));
	ret = SQClosure<T>::Create(_ss(v),_funcproto(func));
	return true;
}

template <Squirk T>
bool SQFunctionProto<T>::Save(SQVM<T> *v,SQUserPointer up,SQWRITEFUNC write)
{
	SQInteger i,nliterals = _nliterals,nparameters = _nparameters;
	SQInteger noutervalues = _noutervalues,nlocalvarinfos = _nlocalvarinfos;
	SQInteger nlineinfos=_nlineinfos,ninstructions = _ninstructions,nfunctions=_nfunctions;
	SQInteger ndefaultparams = _ndefaultparams;
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(WriteObject(v,up,write,_sourcename));
	_CHECK_IO(WriteObject(v,up,write,_name));
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,&nliterals,sizeof(nliterals)));
	_CHECK_IO(SafeWrite(v,write,up,&nparameters,sizeof(nparameters)));
	_CHECK_IO(SafeWrite(v,write,up,&noutervalues,sizeof(noutervalues)));
	_CHECK_IO(SafeWrite(v,write,up,&nlocalvarinfos,sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeWrite(v,write,up,&nlineinfos,sizeof(nlineinfos)));
	_CHECK_IO(SafeWrite(v,write,up,&ndefaultparams,sizeof(ndefaultparams)));
	_CHECK_IO(SafeWrite(v,write,up,&ninstructions,sizeof(ninstructions)));
	_CHECK_IO(SafeWrite(v,write,up,&nfunctions,sizeof(nfunctions)));
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nliterals;i++){
		_CHECK_IO(WriteObject(v,up,write,_literals[i]));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nparameters;i++){
		_CHECK_IO(WriteObject(v,up,write,_parameters[i]));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<noutervalues;i++){
		_CHECK_IO(SafeWrite(v,write,up,&_outervalues[i]._type,sizeof(SQUnsignedInteger)));
		_CHECK_IO(WriteObject(v,up,write,_outervalues[i]._src));
		_CHECK_IO(WriteObject(v,up,write,_outervalues[i]._name));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nlocalvarinfos;i++){
		SQLocalVarInfo<T> &lvi=_localvarinfos[i];
		_CHECK_IO(WriteObject(v,up,write,lvi._name));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._pos,sizeof(SQUnsignedInteger)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._start_op,sizeof(SQUnsignedInteger)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._end_op,sizeof(SQUnsignedInteger)));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_lineinfos,sizeof(SQLineInfo)*nlineinfos));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_defaultparams,sizeof(SQInteger)*ndefaultparams));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_instructions,sizeof(SQInstruction)*ninstructions));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nfunctions;i++){
		_CHECK_IO(_funcproto(_functions[i])->Save(v,up,write));
	}
	_CHECK_IO(SafeWrite(v,write,up,&_stacksize,sizeof(_stacksize)));
	_CHECK_IO(SafeWrite(v,write,up,&_bgenerator,sizeof(_bgenerator)));
	_CHECK_IO(SafeWrite(v,write,up,&_varparams,sizeof(_varparams)));
	return true;
}

template <Squirk T>
bool SQFunctionProto<T>::Load(SQVM<T> *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr<T> &ret)
{
	SQInteger i, nliterals,nparameters;
	SQInteger noutervalues ,nlocalvarinfos ;
	SQInteger nlineinfos,ninstructions ,nfunctions,ndefaultparams ;
	SQObjectPtr<T> sourcename, name;
	SQObjectPtr<T> o;
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(ReadObject(v, up, read, sourcename));
	_CHECK_IO(ReadObject(v, up, read, name));
	
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, &nliterals, sizeof(nliterals)));
	_CHECK_IO(SafeRead(v,read,up, &nparameters, sizeof(nparameters)));
	_CHECK_IO(SafeRead(v,read,up, &noutervalues, sizeof(noutervalues)));
	_CHECK_IO(SafeRead(v,read,up, &nlocalvarinfos, sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeRead(v,read,up, &nlineinfos, sizeof(nlineinfos)));
	_CHECK_IO(SafeRead(v,read,up, &ndefaultparams, sizeof(ndefaultparams)));
	_CHECK_IO(SafeRead(v,read,up, &ninstructions, sizeof(ninstructions)));
	_CHECK_IO(SafeRead(v,read,up, &nfunctions, sizeof(nfunctions)));
	

	SQFunctionProto *f = SQFunctionProto::Create(ninstructions,nliterals,nparameters,
			nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams);
	SQObjectPtr proto = f; //gets a ref in case of failure
	f->_sourcename = sourcename;
	f->_name = name;

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0;i < nliterals; i++){
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_literals[i] = o;
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0; i < nparameters; i++){
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_parameters[i] = o;
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0; i < noutervalues; i++){
		SQUnsignedInteger type;
		SQObjectPtr<T> name;
		_CHECK_IO(SafeRead(v,read,up, &type, sizeof(SQUnsignedInteger)));
		_CHECK_IO(ReadObject(v, up, read, o));
		_CHECK_IO(ReadObject(v, up, read, name));
		f->_outervalues[i] = SQOuterVar(name,o, (SQOuterType)type);
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0; i < nlocalvarinfos; i++){
		SQLocalVarInfo<T> lvi;
		_CHECK_IO(ReadObject(v, up, read, lvi._name));
		_CHECK_IO(SafeRead(v,read,up, &lvi._pos, sizeof(SQUnsignedInteger)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._start_op, sizeof(SQUnsignedInteger)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._end_op, sizeof(SQUnsignedInteger)));
		f->_localvarinfos[i] = lvi;
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_lineinfos, sizeof(SQLineInfo)*nlineinfos));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_defaultparams, sizeof(SQInteger)*ndefaultparams));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_instructions, sizeof(SQInstruction)*ninstructions));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	for(i = 0; i < nfunctions; i++){
		_CHECK_IO(_funcproto(o)->Load(v, up, read, o));
		f->_functions[i] = o;
	}
	_CHECK_IO(SafeRead(v,read,up, &f->_stacksize, sizeof(f->_stacksize)));
	_CHECK_IO(SafeRead(v,read,up, &f->_bgenerator, sizeof(f->_bgenerator)));
	_CHECK_IO(SafeRead(v,read,up, &f->_varparams, sizeof(f->_varparams)));
	
	ret = f;
	return true;
}

#ifndef NO_GARBAGE_COLLECTOR

#define START_MARK() 	if(!(CHAINABLE_OBJ<T>::_uiRef&MARK_FLAG)){ \
		CHAINABLE_OBJ<T>::_uiRef|=MARK_FLAG;

#define END_MARK() SQCollectable<T>::RemoveFromChain(&CHAINABLE_OBJ<T>::_sharedstate->_gc_chain, this); \
		SQCollectable<T>::AddToChain(chain, this); }

template <Squirk T>
void SQVM<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		SQSharedState<T>::MarkObject(_lasterror,chain);
		SQSharedState<T>::MarkObject(_errorhandler,chain);
		SQSharedState<T>::MarkObject(_debughook,chain);
		SQSharedState<T>::MarkObject(_roottable, chain);
		SQSharedState<T>::MarkObject(temp_reg, chain);
		for(SQUnsignedInteger i = 0; i < _stack.size(); i++) SQSharedState<T>::MarkObject(_stack[i], chain);
		for(SQUnsignedInteger j = 0; j < _vargsstack.size(); j++) SQSharedState<T>::MarkObject(_vargsstack[j], chain);
		for(SQInteger k = 0; k < _callsstacksize; k++) SQSharedState<T>::MarkObject(_callsstack[k]._closure, chain);
	END_MARK()
}

template <Squirk T>
void SQArray<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		SQInteger len = _values.size();
		for(SQInteger i = 0;i < len; i++) SQSharedState<T>::MarkObject(_values[i], chain);
	END_MARK()
}
template <Squirk T>
void SQTable<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		if(SQDelegable<T>::_delegate) SQDelegable<T>::_delegate->Mark(chain);
		SQInteger len = _numofnodes;
		for(SQInteger i = 0; i < len; i++){
			SQSharedState<T>::MarkObject(_nodes[i].key, chain);
			SQSharedState<T>::MarkObject(_nodes[i].val, chain);
		}
	END_MARK()
}

template <Squirk T>
void SQClass<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		_members->Mark(chain);
		if(_base) _base->Mark(chain);
		SQSharedState<T>::MarkObject(_attributes, chain);
		for(SQUnsignedInteger i =0; i< _defaultvalues.size(); i++) {
			SQSharedState<T>::MarkObject(_defaultvalues[i].val, chain);
			SQSharedState<T>::MarkObject(_defaultvalues[i].attrs, chain);
		}
		for(SQUnsignedInteger j =0; j< _methods.size(); j++) {
			SQSharedState<T>::MarkObject(_methods[j].val, chain);
			SQSharedState<T>::MarkObject(_methods[j].attrs, chain);
		}
		for(SQUnsignedInteger k =0; k< _metamethods.size(); k++) {
			SQSharedState<T>::MarkObject(_metamethods[k], chain);
		}
	END_MARK()
}

template <Squirk T>
void SQInstance<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		_class->Mark(chain);
		SQUnsignedInteger nvalues = _class->_defaultvalues.size();
		for(SQUnsignedInteger i =0; i< nvalues; i++) {
			SQSharedState<T>::MarkObject(_values[i], chain);
		}
	END_MARK()
}

template <Squirk T>
void SQGenerator<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		for(SQUnsignedInteger i = 0; i < _stack.size(); i++) SQSharedState<T>::MarkObject(_stack[i], chain);
		for(SQUnsignedInteger j = 0; j < _vargsstack.size(); j++) SQSharedState<T>::MarkObject(_vargsstack[j], chain);
		SQSharedState<T>::MarkObject(_closure, chain);
	END_MARK()
}

template <Squirk T>
void SQClosure<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		for(SQUnsignedInteger i = 0; i < _outervalues.size(); i++) SQSharedState<T>::MarkObject(_outervalues[i], chain);
		for(SQUnsignedInteger i = 0; i < _defaultparams.size(); i++) SQSharedState<T>::MarkObject(_defaultparams[i], chain);
	END_MARK()
}

template <Squirk T>
void SQNativeClosure<T>::Mark(SQCollectable<T> **chain)
{
	START_MARK()
		for(SQUnsignedInteger i = 0; i < _outervalues.size(); i++) SQSharedState<T>::MarkObject(_outervalues[i], chain);
	END_MARK()
}

template <Squirk T>
void SQUserData<T>::Mark(SQCollectable<T> **chain){
	START_MARK()
		if(SQDelegable<T>::_delegate) SQDelegable<T>::_delegate->Mark(chain);
	END_MARK()
}

template <Squirk T>
void SQCollectable<T>::UnMark() { SQRefCounted<T>::_uiRef&=~MARK_FLAG; }

#endif
