/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <math.h>
#include <stdlib.h>
#include "sqopcodes.h"
#include "sqfuncproto.h"
#include "sqvm.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqarray.h"
#include "sqclass.h"

#define TOP() (_stack._vals[_top-1])

#define CLEARSTACK(_last_top) { if((_last_top) >= _top) ClearStack(_last_top); }
template <Squirk Q>
void SQVM<Q>::ClearStack(SQInteger last_top)
{
	SQObjectType tOldType;
	SQObjectValue<Q> unOldVal;
	while (last_top >= _top) {
		SQObjectPtr<Q> &o = _stack._vals[last_top--];
		tOldType = o._type;
		unOldVal = o._unVal;
		o._type = OT_NULL;
		o._unVal.pUserPointer = NULL;
		__Release(tOldType,unOldVal);
	}
}

template <Squirk Q>
bool SQVM<Q>::BW_OP(SQUnsignedInteger op,SQObjectPtr<Q> &trg,const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2)
{
	SQInteger res;
	SQInteger i1 = _integer(o1), i2 = _integer(o2);
	if((obj_type(o1)==OT_INTEGER) && (obj_type(o2)==OT_INTEGER))
	{
		switch(op) {
			case BW_AND:	res = i1 & i2; break;
			case BW_OR:		res = i1 | i2; break;
			case BW_XOR:	res = i1 ^ i2; break;
			case BW_SHIFTL:	res = i1 << i2; break;
			case BW_SHIFTR:	res = i1 >> i2; break;
			case BW_USHIFTR:res = (SQInteger)(*((SQUnsignedInteger*)&i1) >> i2); break;
			default: { Raise_Error(_SC("internal vm error bitwise op failed")); return false; }
		}
	} 
	else { Raise_Error(_SC("bitwise op between '%s' and '%s'"),GetTypeName(o1),GetTypeName(o2)); return false;}
	trg = res;
	return true;
}

template <Squirk Q>
bool SQVM<Q>::ARITH_OP(SQUnsignedInteger op,SQObjectPtr<Q> &trg,const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2)
{
	if(sq_isnumeric(o1) && sq_isnumeric(o2)) {
			if((obj_type(o1)==OT_INTEGER) && (obj_type(o2)==OT_INTEGER)) {
				SQInteger res, i1 = _integer(o1), i2 = _integer(o2);
				switch(op) {
				case '+': res = i1 + i2; break;
				case '-': res = i1 - i2; break;
				case '/': if(i2 == 0) { Raise_Error(_SC("division by zero")); return false; }
					res = i1 / i2; 
					break;
				case '*': res = i1 * i2; break;
				case '%': if(i2 == 0) { Raise_Error(_SC("modulo by zero")); return false; }
					res = i1 % i2; 
					break;
				default: res = 0xDEADBEEF;
				}
				trg = res;
			}else{
				SQFloat res, f1 = tofloat(o1), f2 = tofloat(o2);
				switch(op) {
				case '+': res = f1 + f2; break;
				case '-': res = f1 - f2; break;
				case '/': res = f1 / f2; break;
				case '*': res = f1 * f2; break;
				case '%': res = SQFloat(fmod((double)f1,(double)f2)); break;
				default: res = 0x0f;
				}
				trg = res;
			}	
		} else {
			if(op == '+' &&	(obj_type(o1) == OT_STRING || obj_type(o2) == OT_STRING)){
					if(!StringCat(o1, o2, trg)) return false;
			}
			else if(!ArithMetaMethod(op,o1,o2,trg)) { 
				Raise_Error(_SC("arith op %c on between '%s' and '%s'"),op,GetTypeName(o1),GetTypeName(o2)); return false; 
			}
		}
		return true;
}

template <Squirk Q>
SQVM<Q>::SQVM(SQSharedState<Q> *ss)
{
	_sharedstate=ss;
	_suspended = SQFalse;
	_suspended_target=-1;
	_suspended_root = SQFalse;
	_suspended_traps=-1;
	_foreignptr=NULL;
	_nnativecalls=0;
	_lasterror = _null_<Q>;
	_errorhandler = _null_<Q>;
	_debughook = _null_<Q>;
	if constexpr (std::is_same_v<decltype(_callsstack), M2VMExt<Q>>) {
		_callsstack._unknown = _null_<Squirk::Standard>; // ???
	}
	ci = NULL;
	INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
}

template <Squirk Q>
void SQVM<Q>::Finalize()
{
	_roottable = _null_<Q>;
	_lasterror = _null_<Q>;
	_errorhandler = _null_<Q>;
	_debughook = _null_<Q>;
	temp_reg = _null_<Q>;
	_callstackdata.resize(0);
	SQInteger size=_stack.size();
	for(SQInteger i=0;i<size;i++)
		_stack[i]=_null_<Q>;
}

template <Squirk Q>
SQVM<Q>::~SQVM()
{
	Finalize();
	//sq_free(_callsstack,_alloccallsstacksize*sizeof(CallInfo));
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

template <Squirk Q>
bool SQVM<Q>::ArithMetaMethod(SQInteger op,const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2,SQObjectPtr<Q> &dest)
{
	SQMetaMethod mm;
	switch(op){
		case _SC('+'): mm=MT_ADD; break;
		case _SC('-'): mm=MT_SUB; break;
		case _SC('/'): mm=MT_DIV; break;
		case _SC('*'): mm=MT_MUL; break;
		case _SC('%'): mm=MT_MODULO; break;
		default: mm = MT_ADD; assert(0); break; //shutup compiler
	}
	if(is_delegable(o1) && _delegable(o1)->_delegate) {
		Push(o1);Push(o2);
		return CallMetaMethod(_delegable(o1),mm,2,dest);
	}
	return false;
}

template <Squirk Q>
bool SQVM<Q>::NEG_OP(SQObjectPtr<Q> &trg,const SQObjectPtr<Q> &o)
{
	
	switch(obj_type(o)) {
	case OT_INTEGER:
		trg = -_integer(o);
		return true;
	case OT_FLOAT:
		trg = -_float(o);
		return true;
	case OT_TABLE:
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o)->_delegate) {
			Push(o);
			if(CallMetaMethod(_delegable(o), MT_UNM, 1, temp_reg)) {
				trg = temp_reg;
				return true;
			}
		}
	default:break; //shutup compiler
	}
	Raise_Error(_SC("attempt to negate a %s"), GetTypeName(o));
	return false;
}

#define _RET_SUCCEED(exp) { result = (exp); return true; } 
template <Squirk Q>
bool SQVM<Q>::ObjCmp(const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2,SQInteger &result)
{
	if(obj_type(o1)==obj_type(o2)){
		if(_userpointer(o1)==_userpointer(o2))_RET_SUCCEED(0);
		SQObjectPtr<Q> res;
		switch(obj_type(o1)){
		case OT_STRING:
			_RET_SUCCEED(scstrcmp(_stringval(o1),_stringval(o2)));
		case OT_INTEGER:
			_RET_SUCCEED(_integer(o1)-_integer(o2));
		case OT_FLOAT:
			_RET_SUCCEED((_float(o1)<_float(o2))?-1:1);
		case OT_TABLE:
		case OT_USERDATA:
		case OT_INSTANCE:
			if(_delegable(o1)->_delegate) {
				Push(o1);Push(o2);
				if(CallMetaMethod(_delegable(o1),MT_CMP,2,res)) break;
			}
			//continues through (no break needed)
		default: 
			_RET_SUCCEED( _userpointer(o1) < _userpointer(o2)?-1:1 );
		}
		if(obj_type(res)!=OT_INTEGER) { Raise_CompareError(o1,o2); return false; }
			_RET_SUCCEED(_integer(res));
		
	}
	else{
		if(sq_isnumeric(o1) && sq_isnumeric(o2)){
			if((obj_type(o1)==OT_INTEGER) && (obj_type(o2)==OT_FLOAT)) { 
				if( _integer(o1)==_float(o2) ) { _RET_SUCCEED(0); }
				else if( _integer(o1)<_float(o2) ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
			else{
				if( _float(o1)==_integer(o2) ) { _RET_SUCCEED(0); }
				else if( _float(o1)<_integer(o2) ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
		}
		else if(obj_type(o1)==OT_NULL) {_RET_SUCCEED(-1);}
		else if(obj_type(o2)==OT_NULL) {_RET_SUCCEED(1);}
		else { Raise_CompareError(o1,o2); return false; }
		
	}
	assert(0);
	_RET_SUCCEED(0); //cannot happen
}

template <Squirk Q>
bool SQVM<Q>::CMP_OP(CmpOP op, const SQObjectPtr<Q> &o1,const SQObjectPtr<Q> &o2,SQObjectPtr<Q> &res)
{
	SQInteger r;
	if(ObjCmp(o1,o2,r)) {
		switch(op) {
			case CMP_G: res = (r > 0)?_true_<Q>:_false_<Q>; return true;
			case CMP_GE: res = (r >= 0)?_true_<Q>:_false_<Q>; return true;
			case CMP_L: res = (r < 0)?_true_<Q>:_false_<Q>; return true;
			case CMP_LE: res = (r <= 0)?_true_<Q>:_false_<Q>; return true;
			
		}
		assert(0);
	}
	return false;
}

template <Squirk Q>
void SQVM<Q>::ToString(const SQObjectPtr<Q> &o,SQObjectPtr<Q> &res)
{
	switch(obj_type(o)) {
	case OT_STRING:
		res = o;
		return;
	case OT_FLOAT:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)),_SC("%g"),_float(o));
		break;
	case OT_INTEGER:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)),_SC("%d"),_integer(o));
		break;
	case OT_BOOL:
		scsprintf(_sp(rsl(6)),_integer(o)?_SC("true"):_SC("false"));
		break;
	case OT_TABLE:
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o)->_delegate) {
			Push(o);
			if(CallMetaMethod(_delegable(o),MT_TOSTRING,1,res)) {
				if(obj_type(res) == OT_STRING)
					return;
				//else keeps going to the default
			}
		}
	default:
		scsprintf(_sp(rsl(sizeof(void*)+20)),_SC("(%s : 0x%p)"),GetTypeName(o),(void*)_rawval(o));
	}
	res = SQString<Q>::Create(_ss(this),_spval);
}

template <Squirk Q>
bool SQVM<Q>::StringCat(const SQObjectPtr<Q> &str,const SQObjectPtr<Q> &obj,SQObjectPtr<Q> &dest)
{
	SQObjectPtr<Q> a, b;
	ToString(str, a);
	ToString(obj, b);
	SQInteger l = _string(a)->_len , ol = _string(b)->_len;
	SQChar *s = _sp(rsl(l + ol + 1));
	memcpy(s, _stringval(a), rsl(l)); 
	memcpy(s + l, _stringval(b), rsl(ol));
	dest = SQString<Q>::Create(_ss(this), _spval, l + ol);
	return true;
}

template <Squirk Q>
void SQVM<Q>::TypeOf(const SQObjectPtr<Q> &obj1,SQObjectPtr<Q> &dest)
{
	if(is_delegable(obj1) && _delegable(obj1)->_delegate) {
		Push(obj1);
		if(CallMetaMethod(_delegable(obj1),MT_TYPEOF,1,dest))
			return;
	}
	dest = SQString<Q>::Create(_ss(this),GetTypeName(obj1));
}

template <Squirk Q>
bool SQVM<Q>::Init(SQVM<Q> *friendvm, SQInteger stacksize)
{
	_stack.resize(stacksize);
	_alloccallsstacksize = 4;
	_callstackdata.resize(_alloccallsstacksize);
	_callsstacksize = 0;
	_callsstack = &_callstackdata[0];
	_stackbase = 0;
	_top = 0;
	if(!friendvm) 
		_roottable = SQTable<Q>::Create(_ss(this), 0);
	else {
		_roottable = friendvm->_roottable;
		_errorhandler = friendvm->_errorhandler;
		_debughook = friendvm->_debughook;
	}
	
	sq_base_register(this);
	return true;
}

extern SQInstructionDesc g_InstrDesc[];

template <Squirk Q>
bool SQVM<Q>::StartCall(SQClosure<Q> *closure,SQInteger target,SQInteger args,SQInteger stackbase,bool tailcall)
{
	SQFunctionProto<Q> *func = _funcproto(closure->_function);
	
	const SQInteger paramssize = func->_nparameters;
	const SQInteger newtop = stackbase + func->_stacksize;
	SQInteger nargs = args;
	if (paramssize != nargs) {
		SQInteger ndef = func->_ndefaultparams;
		SQInteger diff;
		if(ndef && nargs < paramssize && (diff = paramssize - nargs) <= ndef) {
			for(SQInteger n = ndef - diff; n < ndef; n++) {
				_stack._vals[stackbase + (nargs++)] = closure->_defaultparams[n];
			}
		}
		else if(func->_varparams)
		{
			if (nargs < paramssize) {
				Raise_Error(_SC("wrong number of parameters"));
				return false;
			}
			for(SQInteger n = 0; n < nargs - paramssize; n++) {
				_vargsstack.push_back(_stack._vals[stackbase+paramssize+n]);
				_stack._vals[stackbase+paramssize+n] = _null_<Q>;
			}
		}
		else {
			Raise_Error(_SC("wrong number of parameters"));
			return false;
		}
	}

	if(obj_type(closure->_env) == OT_WEAKREF) {
		_stack._vals[stackbase] = _weakref(closure->_env)->_obj;
	}

	if (!tailcall) {
		CallInfo<Q> lc;
		lc._generator = NULL;
		lc._etraps = 0;
		lc._prevstkbase = (SQInt32) ( stackbase - _stackbase );
		lc._target = (SQInt32) target;
		lc._prevtop = (SQInt32) (_top - _stackbase);
		lc._ncalls = 1;
		lc._root = SQFalse;
		PUSH_CALLINFO(this, lc);
	}
	else {
		ci->_ncalls++;
	}
	ci->_vargs.size = (SQInt32)(nargs - paramssize);
	ci->_vargs.base = (SQInt32)(_vargsstack.size()-(ci->_vargs.size));
	ci->_closure = closure;
	ci->_literals = func->_literals;
	ci->_ip = func->_instructions;
	//grows the stack if needed
	if (((SQUnsignedInteger)newtop + (func->_stacksize<<1)) > _stack.size()) {
		_stack.resize(_stack.size() + (func->_stacksize<<1));
	}
		
	_top = newtop;
	_stackbase = stackbase;
	if (obj_type(_debughook) != OT_NULL && _rawval(_debughook) != _rawval(ci->_closure))
		CallDebugHook(_SC('c'));
	return true;
}

template <Squirk Q>
bool SQVM<Q>::Return(SQInteger _arg0, SQInteger _arg1, SQObjectPtr<Q> &retval)
{
	if (obj_type(_debughook) != OT_NULL && _rawval(_debughook) != _rawval(ci->_closure))
		for(SQInteger i=0;i<ci->_ncalls;i++)
			CallDebugHook(_SC('r'));
			
	SQBool broot = ci->_root;
	SQInteger last_top = _top;
	SQInteger target = ci->_target;
	SQInteger oldstackbase = _stackbase;
	_stackbase -= ci->_prevstkbase;
	_top = _stackbase + ci->_prevtop;
	if(ci->_vargs.size) PopVarArgs(ci->_vargs);
	POP_CALLINFO(this);
	if (broot) {
		if (_arg0 != MAX_FUNC_STACKSIZE) retval = _stack._vals[oldstackbase+_arg1];
		else retval = _null_<Q>;
	}
	else {
		if(target != -1) { //-1 is when a class contructor ret value has to be ignored
			if (_arg0 != MAX_FUNC_STACKSIZE)
				STK(target) = _stack._vals[oldstackbase+_arg1];
			else
				STK(target) = _null_<Q>;
		}
	}

	CLEARSTACK(last_top);
	assert(oldstackbase >= _stackbase); 
	return broot?true:false;
}

#define _RET_ON_FAIL(exp) { if(!exp) return false; }

template <Squirk Q>
bool SQVM<Q>::LOCAL_INC(SQInteger op,SQObjectPtr<Q> &target, SQObjectPtr<Q> &a, SQObjectPtr<Q> &incr)
{
	_RET_ON_FAIL(ARITH_OP( op , target, a, incr));
	a = target;
	return true;
}

template <Squirk Q>
bool SQVM<Q>::PLOCAL_INC(SQInteger op,SQObjectPtr<Q> &target, SQObjectPtr<Q> &a, SQObjectPtr<Q> &incr)
{
 	SQObjectPtr<Q> trg;
	_RET_ON_FAIL(ARITH_OP( op , trg, a, incr));
	target = a;
	a = trg;
	return true;
}

template <Squirk Q>
bool SQVM<Q>::DerefInc(SQInteger op,SQObjectPtr<Q> &target, SQObjectPtr<Q> &self, SQObjectPtr<Q> &key, SQObjectPtr<Q> &incr, bool postfix)
{
	SQObjectPtr<Q> tmp, tself = self, tkey = key;
	if (!Get(tself, tkey, tmp, false, true)) { Raise_IdxError(tkey); return false; }
	_RET_ON_FAIL(ARITH_OP( op , target, tmp, incr))
	Set(tself, tkey, target,true);
	if (postfix) target = tmp;
	return true;
}

#define arg0 (_i_._arg0)
#define arg1 (_i_._arg1)
#define sarg1 (*((SQInt32 *)&_i_._arg1))
#define arg2 (_i_._arg2)
#define arg3 (_i_._arg3)
#define sarg3 ((SQInteger)*((signed char *)&_i_._arg3))

template <Squirk Q>
SQRESULT SQVM<Q>::Suspend()
{
	if (_suspended)
		return sq_throwerror(this, _SC("cannot suspend an already suspended vm"));
	if (_nnativecalls!=2)
		return sq_throwerror(this, _SC("cannot suspend through native calls/metamethods"));
	return SQ_SUSPEND_FLAG;
}

template <Squirk Q>
void SQVM<Q>::PopVarArgs(VarArgs &vargs)
{
	for(SQInteger n = 0; n< vargs.size; n++)
		_vargsstack.pop_back();
}

#define _FINISH(howmuchtojump) {jump = howmuchtojump; return true; }
template <Squirk Q>
bool SQVM<Q>::FOREACH_OP(SQObjectPtr<Q> &o1,SQObjectPtr<Q> &o2,SQObjectPtr<Q>
&o3,SQObjectPtr<Q> &o4,SQInteger arg_2,int exitpos,int &jump)
{
	SQInteger nrefidx;
	switch(obj_type(o1)) {
	case OT_TABLE:
		if((nrefidx = _table(o1)->Next(false,o4, o2, o3)) == -1) _FINISH(exitpos);
		o4 = (SQInteger)nrefidx; _FINISH(1);
	case OT_ARRAY:
		if((nrefidx = _array(o1)->Next(o4, o2, o3)) == -1) _FINISH(exitpos);
		o4 = (SQInteger) nrefidx; _FINISH(1);
	case OT_STRING:
		if((nrefidx = _string(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
		o4 = (SQInteger)nrefidx; _FINISH(1);
	case OT_CLASS:
		if((nrefidx = _class(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
		o4 = (SQInteger)nrefidx; _FINISH(1);
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o1)->_delegate) {
			SQObjectPtr<Q> itr;
			Push(o1);
			Push(o4);
			if(CallMetaMethod(_delegable(o1), MT_NEXTI, 2, itr)){
				o4 = o2 = itr;
				if(obj_type(itr) == OT_NULL) _FINISH(exitpos);
				if(!Get(o1, itr, o3, false,false)) {
					Raise_Error(_SC("_nexti returned an invalid idx"));
					return false;
				}
				_FINISH(1);
			}
			Raise_Error(_SC("_nexti failed"));
			return false;
		}
		break;
	case OT_GENERATOR:
		if(_generator(o1)->_state == SQGenerator<Q>::eDead) _FINISH(exitpos);
		if(_generator(o1)->_state == SQGenerator<Q>::eSuspended) {
			SQInteger idx = 0;
			if(obj_type(o4) == OT_INTEGER) {
				idx = _integer(o4) + 1;
			}
			o2 = idx;
			o4 = idx;
			_generator(o1)->Resume(this, arg_2+1);
			_FINISH(0);
		}
	default: 
		Raise_Error(_SC("cannot iterate %s"), GetTypeName(o1));
	}
	return false; //cannot be hit(just to avoid warnings)
}

template <Squirk Q>
bool SQVM<Q>::DELEGATE_OP(SQObjectPtr<Q> &trg,SQObjectPtr<Q> &o1,SQObjectPtr<Q> &o2)
{
	if(obj_type(o1) != OT_TABLE) { Raise_Error(_SC("delegating a '%s'"), GetTypeName(o1)); return false; }
	switch(obj_type(o2)) {
	case OT_TABLE:
		if(!_table(o1)->SetDelegate(_table(o2))){
			Raise_Error(_SC("delegate cycle detected"));
			return false;
		}
		break;
	case OT_NULL:
		_table(o1)->SetDelegate(NULL);
		break;
	default:
		Raise_Error(_SC("using '%s' as delegate"), GetTypeName(o2));
		return false;
		break;
	}
	trg = o1;
	return true;
}
#define COND_LITERAL (arg3!=0?ci->_literals[arg1]:STK(arg1))

#define _GUARD(exp) { if(!exp) { Raise_Error(_lasterror); SQ_THROW();} }

#define SQ_THROW() { goto exception_trap; }

template <Squirk Q>
bool SQVM<Q>::CLOSURE_OP(SQObjectPtr<Q> &target, SQFunctionProto<Q> *func)
{
	SQInteger nouters;
	SQClosure<Q> *closure = SQClosure<Q>::Create(_ss(this), func);
	if((nouters = func->_noutervalues)) {
		closure->_outervalues.reserve(nouters);
		for(SQInteger i = 0; i<nouters; i++) {
			SQOuterVar<Q> &v = func->_outervalues[i];
			switch(v._type){
			case otSYMBOL:
				closure->_outervalues.push_back(_null_<Q>);
				if(!Get(_stack._vals[_stackbase]/*STK(0)*/, v._src, closure->_outervalues.top(), false,true))
				{Raise_IdxError(v._src); return false; }
				break;
			case otLOCAL:
				closure->_outervalues.push_back(_stack._vals[_stackbase+_integer(v._src)]);
				break;
			case otOUTER:
				closure->_outervalues.push_back(_closure(ci->_closure)->_outervalues[_integer(v._src)]);
				break;
			}
		}
	}
	SQInteger ndefparams;
	if((ndefparams = func->_ndefaultparams)) {
		closure->_defaultparams.reserve(ndefparams);
		for(SQInteger i = 0; i < ndefparams; i++) {
			SQInteger spos = func->_defaultparams[i];
			closure->_defaultparams.push_back(_stack._vals[_stackbase + spos]);
		}
	}
	target = closure;
	return true;

}

template <Squirk Q>
bool SQVM<Q>::GETVARGV_OP(SQObjectPtr<Q> &target,SQObjectPtr<Q> &index,CallInfo<Q> *ci)
{
	if(ci->_vargs.size == 0) {
		Raise_Error(_SC("the function doesn't have var args"));
		return false;
	}
	if(!sq_isnumeric(index)){
		Raise_Error(_SC("indexing 'vargv' with %s"),GetTypeName(index));
		return false;
	}
	SQInteger idx = tointeger(index);
	if(idx < 0 || idx >= ci->_vargs.size){ Raise_Error(_SC("vargv index out of range")); return false; }
	target = _vargsstack[ci->_vargs.base+idx];
	return true;
}

template <Squirk Q>
bool SQVM<Q>::CLASS_OP(SQObjectPtr<Q> &target,SQInteger baseclass,SQInteger attributes)
{
	SQClass<Q> *base = NULL;
	SQObjectPtr<Q> attrs;
	if(baseclass != -1) {
		if(obj_type(_stack._vals[_stackbase+baseclass]) != OT_CLASS) { Raise_Error(_SC("trying to inherit from a %s"),GetTypeName(_stack._vals[_stackbase+baseclass])); return false; }
		base = _class(_stack._vals[_stackbase + baseclass]);
	}
	if(attributes != MAX_FUNC_STACKSIZE) {
		attrs = _stack._vals[_stackbase+attributes];
	}
	target = SQClass<Q>::Create(_ss(this),base);
	if(obj_type(_class(target)->_metamethods[MT_INHERITED]) != OT_NULL) {
		int nparams = 2;
		SQObjectPtr<Q> ret;
		Push(target); Push(attrs);
		Call(_class(target)->_metamethods[MT_INHERITED],nparams,_top - nparams, ret, false);
		Pop(nparams);
	}
	_class(target)->_attributes = attrs;
	return true;
}

template <Squirk Q>
bool SQVM<Q>::IsEqual(SQObjectPtr<Q> &o1,SQObjectPtr<Q> &o2,bool &res)
{
	if(obj_type(o1) == obj_type(o2)) {
		res = ((_userpointer(o1) == _userpointer(o2)?true:false));
	}
	else {
		if(sq_isnumeric(o1) && sq_isnumeric(o2)) {
			SQInteger cmpres;
			if(!ObjCmp(o1, o2,cmpres)) return false;
			res = (cmpres == 0);
		}
		else {
			res = false;
		}
	}
	return true;
}

template <Squirk Q>
bool SQVM<Q>::IsFalse(SQObjectPtr<Q> &o)
{
	if((obj_type(o) & SQOBJECT_CANBEFALSE) && ( (obj_type(o) == OT_FLOAT) && (_float(o) == SQFloat(0.0)) )
		|| (_integer(o) == 0) ) { //OT_NULL|OT_INTEGER|OT_BOOL
		return true;
	}
	return false;
}

template <Squirk Q>
bool SQVM<Q>::GETPARENT_OP(SQObjectPtr<Q> &o,SQObjectPtr<Q> &target)
{
	switch(obj_type(o)) {
		case OT_TABLE: target = _table(o)->_delegate?SQObjectPtr(_table(o)->_delegate):_null_<Q>;
			break;
		case OT_CLASS: target = _class(o)->_base?_class(o)->_base:_null_<Q>;
			break;
		default:
			Raise_Error(_SC("the %s type doesn't have a parent slot"), GetTypeName(o));
			return false;
	}
	return true;
}

template <Squirk Q>
bool SQVM<Q>::Execute(SQObjectPtr<Q> &closure, SQInteger target, SQInteger nargs, SQInteger stackbase,SQObjectPtr<Q> &outres, SQBool raiseerror,ExecutionType et)
{
	if ((_nnativecalls + 1) > MAX_NATIVE_CALLS) { Raise_Error(_SC("Native stack overflow")); return false; }
	_nnativecalls++;
	AutoDec ad(&_nnativecalls);
	SQInteger traps = 0;
	//temp_reg vars for OP_CALL
	SQInteger ct_target;
	SQInteger ct_stackbase;
	bool ct_tailcall; 

	switch(et) {
		case ET_CALL: {
			SQInteger last_top = _top;
			temp_reg = closure;
			if(!StartCall(_closure(temp_reg), _top - nargs, nargs, stackbase, false)) { 
				//call the handler if there are no calls in the stack, if not relies on the previous node
				if(ci == NULL) CallErrorHandler(_lasterror);
				return false;
			}
			if (_funcproto(_closure(temp_reg)->_function)->_bgenerator) {
				SQFunctionProto<Q> *f = _funcproto(_closure(temp_reg)->_function);
				SQGenerator<Q> *gen = SQGenerator<Q>::Create(_ss(this), _closure(temp_reg));
				_GUARD(gen->Yield(this));
				Return(1, ci->_target, temp_reg);
				outres = gen;
				CLEARSTACK(last_top);
				return true;
			}
			ci->_root = SQTrue;
					  }
			break;
		case ET_RESUME_GENERATOR: _generator(closure)->Resume(this, target); ci->_root = SQTrue; traps += ci->_etraps; break;
		case ET_RESUME_VM:
		case ET_RESUME_THROW_VM:
			traps = _suspended_traps;
			ci->_root = _suspended_root;
			ci->_vargs = _suspend_varargs;
			_suspended = SQFalse;
			if(et  == ET_RESUME_THROW_VM) { SQ_THROW(); }
			break;
	}
	
exception_restore:
	//
	{
		for(;;)
		{
			const SQInstruction &_i_ = *ci->_ip++;
#ifdef _DEBUG_DUMP
			dumpstack(_stackbase);
#endif
			//scprintf("\n[%d] %s %d %d %d %d\n",ci->_ip-ci->_iv->_vals,g_InstrDesc[_i_.op].name,arg0,arg1,arg2,arg3);
			switch(_i_.op)
			{
			case _OP_LINE:
				if(obj_type(_debughook) != OT_NULL && _rawval(_debughook) != _rawval(ci->_closure))
					CallDebugHook(_SC('l'),arg1);
				continue;
			case _OP_LOAD: TARGET = ci->_literals[arg1]; continue;
			case _OP_LOADINT: TARGET = (SQInteger)arg1; continue;
			case _OP_LOADFLOAT: TARGET = *((SQFloat *)&arg1); continue;
			case _OP_DLOAD: TARGET = ci->_literals[arg1]; STK(arg2) = ci->_literals[arg3];continue;
			case _OP_TAILCALL:
				temp_reg = STK(arg1);
				if (obj_type(temp_reg) == OT_CLOSURE && !_funcproto(_closure(temp_reg)->_function)->_bgenerator){ 
					ct_tailcall = true;
					if(ci->_vargs.size) PopVarArgs(ci->_vargs);
					for (SQInteger i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
					ct_target = ci->_target;
					ct_stackbase = _stackbase;
					goto common_call;
				}
			case _OP_CALL: {
					ct_tailcall = false;
					ct_target = arg0;
					temp_reg = STK(arg1);
					ct_stackbase = _stackbase+arg2;

common_call:
					SQObjectPtr clo = temp_reg;
					SQInteger last_top = _top;
					switch (obj_type(clo)) {
					case OT_CLOSURE:{
						_GUARD(StartCall(_closure(clo), ct_target, arg3, ct_stackbase, ct_tailcall));
						if (_funcproto(_closure(clo)->_function)->_bgenerator) {
							SQGenerator<Q> *gen = SQGenerator<Q>::Create(_ss(this), _closure(clo));
							_GUARD(gen->Yield(this));
							Return(1, ct_target, clo);
							STK(ct_target) = gen;
							CLEARSTACK(last_top);
							continue;
						}
						}
						continue;
					case OT_NATIVECLOSURE: {
						bool suspend;
						_GUARD(CallNative(_nativeclosure(clo), arg3, ct_stackbase, clo,suspend));
						if(suspend){
							_suspended = SQTrue;
							_suspended_target = ct_target;
							_suspended_root = ci->_root;
							_suspended_traps = traps;
							_suspend_varargs = ci->_vargs;
							outres = clo;
							return true;
						}
						if(ct_target != -1) { //skip return value for constructors
							STK(ct_target) = clo;
						}
										   }
						continue;
					case OT_CLASS:{
						SQObjectPtr<Q> inst;
						_GUARD(CreateClassInstance(_class(clo),inst,temp_reg));
						STK(ct_target) = inst;
						ct_target = -1; //fakes return value target so that is not overwritten by the constructor
						if(obj_type(temp_reg) != OT_NULL) {
							_stack._vals[ct_stackbase] = inst;
							goto common_call; //hard core spaghetti code(reissues the OP_CALL to invoke the constructor)
						}
						}
						break;
					case OT_TABLE:
					case OT_USERDATA:
					case OT_INSTANCE:
						{
						Push(clo);
						for (SQInteger i = 0; i < arg3; i++) Push(STK(arg2 + i));
						if (_delegable(clo) && CallMetaMethod(_delegable(clo), MT_CALL, arg3+1, clo)){
							STK(ct_target) = clo;
							break;
						}
						Raise_Error(_SC("attempt to call '%s'"), GetTypeName(clo));
						SQ_THROW();
					  }
					default:
						Raise_Error(_SC("attempt to call '%s'"), GetTypeName(clo));
						SQ_THROW();
					}
				}
				  continue;
			case _OP_PREPCALL:
			case _OP_PREPCALLK:
				{
					SQObjectPtr<Q> &key = _i_.op == _OP_PREPCALLK?(ci->_literals)[arg1]:STK(arg1);
					SQObjectPtr<Q> &o = STK(arg2);
					if (!Get(o, key, temp_reg,false,true)) {
						if(obj_type(o) == OT_CLASS) { //hack?
							if(_class_ddel->Get(key,temp_reg)) {
								STK(arg3) = o;
								TARGET = temp_reg;
								continue;
							}
						}
						{ Raise_IdxError(key); SQ_THROW();}
					}

					STK(arg3) = obj_type(o) == OT_CLASS?STK(0):o;
					TARGET = temp_reg;
				}
				continue;
			case _OP_GETK:
				if (!Get(STK(arg2), ci->_literals[arg1], temp_reg, false,true)) { Raise_IdxError(ci->_literals[arg1]); SQ_THROW();}
				TARGET = temp_reg;
				continue;
			case _OP_MOVE: TARGET = STK(arg1); continue;
			case _OP_NEWSLOT:
				_GUARD(NewSlot(STK(arg1), STK(arg2), STK(arg3),false));
				if(arg0 != arg3) TARGET = STK(arg3);
				continue;
			case _OP_DELETE: _GUARD(DeleteSlot(STK(arg1), STK(arg2), TARGET)); continue;
			case _OP_SET:
				if (!Set(STK(arg1), STK(arg2), STK(arg3),true)) { Raise_IdxError(STK(arg2)); SQ_THROW(); }
				if (arg0 != arg3) TARGET = STK(arg3);
				continue;
			case _OP_GET:
				if (!Get(STK(arg1), STK(arg2), temp_reg, false,true)) { Raise_IdxError(STK(arg2)); SQ_THROW(); }
				TARGET = temp_reg;
				continue;
			case _OP_EQ:{
				bool res;
				if(!IsEqual(STK(arg2),COND_LITERAL,res)) { SQ_THROW(); }
				TARGET = res?_true_<Q>:_false_<Q>;
				}continue;
			case _OP_NE:{ 
				bool res;
				if(!IsEqual(STK(arg2),COND_LITERAL,res)) { SQ_THROW(); }
				TARGET = (!res)?_true_<Q>:_false_<Q>;
				} continue;
			case _OP_ARITH: _GUARD(ARITH_OP( arg3 , temp_reg, STK(arg2), STK(arg1))); TARGET = temp_reg; continue;
			case _OP_BITW:	_GUARD(BW_OP( arg3,TARGET,STK(arg2),STK(arg1))); continue;
			case _OP_RETURN:
				if(ci->_generator) {
					ci->_generator->Kill();
				}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps==0);
					outres = temp_reg;
					return true;
				}
				continue;
			case _OP_LOADNULLS:{ for(SQInt32 n=0; n < arg1; n++) STK(arg0+n) = _null_<Q>; }continue;
			case _OP_LOADROOTTABLE:	TARGET = _roottable; continue;
			case _OP_LOADBOOL: TARGET = arg1?_true_<Q>:_false_<Q>; continue;
			case _OP_DMOVE: STK(arg0) = STK(arg1); STK(arg2) = STK(arg3); continue;
			case _OP_JMP: ci->_ip += (sarg1); continue;
			case _OP_JNZ: if(!IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
			case _OP_JZ: if(IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
			case _OP_LOADFREEVAR: TARGET = _closure(ci->_closure)->_outervalues[arg1]; continue;
			case _OP_VARGC: TARGET = SQInteger(ci->_vargs.size); continue;
			case _OP_GETVARGV: 
				if(!GETVARGV_OP(TARGET,STK(arg1),ci)) { SQ_THROW(); } 
				continue;
			case _OP_NEWTABLE: TARGET = SQTable<Q>::Create(_ss(this), arg1); continue;
			case _OP_NEWARRAY: TARGET = SQArray<Q>::Create(_ss(this), 0); _array(TARGET)->Reserve(arg1); continue;
			case _OP_APPENDARRAY: _array(STK(arg0))->Append(COND_LITERAL);	continue;
			case _OP_GETPARENT: _GUARD(GETPARENT_OP(STK(arg1),TARGET)); continue;
			case _OP_COMPARITH: _GUARD(DerefInc(arg3, TARGET, STK((((SQUnsignedInteger)arg1&0xFFFF0000)>>16)), STK(arg2), STK(arg1&0x0000FFFF), false)); continue;
			case _OP_COMPARITHL: _GUARD(LOCAL_INC(arg3, TARGET, STK(arg1), STK(arg2))); continue;
			case _OP_INC: {SQObjectPtr<Q> o(sarg3); _GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, false));} continue;
			case _OP_INCL: {SQObjectPtr<Q> o(sarg3); _GUARD(LOCAL_INC('+',TARGET, STK(arg1), o));} continue;
			case _OP_PINC: {SQObjectPtr<Q> o(sarg3); _GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, true));} continue;
			case _OP_PINCL:	{SQObjectPtr<Q> o(sarg3); _GUARD(PLOCAL_INC('+',TARGET, STK(arg1), o));} continue;
			case _OP_CMP:	_GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg1),TARGET))	continue;
			case _OP_EXISTS: TARGET = Get(STK(arg1), STK(arg2), temp_reg, true,false)?_true_<Q>:_false_<Q>;continue;
			case _OP_INSTANCEOF: 
				if(obj_type(STK(arg1)) != OT_CLASS || obj_type(STK(arg2)) != OT_INSTANCE)
				{Raise_Error(_SC("cannot apply instanceof between a %s and a %s"),GetTypeName(STK(arg1)),GetTypeName(STK(arg2))); SQ_THROW();}
				TARGET = _instance(STK(arg2))->InstanceOf(_class(STK(arg1)))?_true_<Q>:_false_<Q>;
				continue;
			case _OP_AND: 
				if(IsFalse(STK(arg2))) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_OR:
				if(!IsFalse(STK(arg2))) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_NEG: _GUARD(NEG_OP(TARGET,STK(arg1))); continue;
			case _OP_NOT: TARGET = (IsFalse(STK(arg1))?_true_<Q>:_false_<Q>); continue;
			case _OP_BWNOT:
				if(obj_type(STK(arg1)) == OT_INTEGER) {
					SQInteger t = _integer(STK(arg1));
					TARGET = SQInteger(~t);
					continue;
				}
				Raise_Error(_SC("attempt to perform a bitwise op on a %s"), GetTypeName(STK(arg1)));
				SQ_THROW();
			case _OP_CLOSURE: {
				SQClosure<Q> *c = ci->_closure._unVal.pClosure;
				SQFunctionProto<Q> *fp = c->_function._unVal.pFunctionProto;
				if(!CLOSURE_OP(TARGET,fp->_functions[arg1]._unVal.pFunctionProto)) { SQ_THROW(); }
				continue;
			}
			case _OP_YIELD:{
				if(ci->_generator) {
					if(sarg1 != MAX_FUNC_STACKSIZE) temp_reg = STK(arg1);
					_GUARD(ci->_generator->Yield(this));
					traps -= ci->_etraps;
					if(sarg1 != MAX_FUNC_STACKSIZE) STK(arg1) = temp_reg;
				}
				else { Raise_Error(_SC("trying to yield a '%s',only genenerator can be yielded"), GetTypeName<Q>(ci->_generator)); SQ_THROW();}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps == 0);
					outres = temp_reg;
					return true;
				}
					
				}
				continue;
			case _OP_RESUME:
				if(obj_type(STK(arg1)) != OT_GENERATOR){ Raise_Error(_SC("trying to resume a '%s',only genenerator can be resumed"), GetTypeName(STK(arg1))); SQ_THROW();}
				_GUARD(_generator(STK(arg1))->Resume(this, arg0));
				traps += ci->_etraps;
                continue;
			case _OP_FOREACH:{ int tojump;
				_GUARD(FOREACH_OP(STK(arg0),STK(arg2),STK(arg2+1),STK(arg2+2),arg2,sarg1,tojump));
				ci->_ip += tojump; }
				continue;
			case _OP_POSTFOREACH:
				assert(obj_type(STK(arg0)) == OT_GENERATOR);
				if(_generator(STK(arg0))->_state == SQGenerator<Q>::eDead)
					ci->_ip += (sarg1 - 1);
				continue;
			case _OP_DELEGATE: _GUARD(DELEGATE_OP(TARGET,STK(arg1),STK(arg2))); continue;
			case _OP_CLONE:
				if(!Clone(STK(arg1), TARGET))
				{ Raise_Error(_SC("cloning a %s"), GetTypeName(STK(arg1))); SQ_THROW();}
				continue;
			case _OP_TYPEOF: TypeOf(STK(arg1), TARGET); continue;
			case _OP_PUSHTRAP:{
				SQInstruction *_iv = _funcproto(_closure(ci->_closure)->_function)->_instructions;
				_etraps.push_back(SQExceptionTrap(_top,_stackbase, &_iv[(ci->_ip-_iv)+arg1], arg0)); traps++;
				ci->_etraps++;
							  }
				continue;
			case _OP_POPTRAP: {
				for(SQInteger i = 0; i < arg0; i++) {
					_etraps.pop_back(); traps--;
					ci->_etraps--;
				}
							  }
				continue;
			case _OP_THROW:	Raise_Error(TARGET); SQ_THROW(); continue;
			case _OP_CLASS: _GUARD(CLASS_OP(TARGET,arg1,arg2)); continue;
			case _OP_NEWSLOTA:
				bool bstatic = (arg0&NEW_SLOT_STATIC_FLAG)?true:false;
				if(obj_type(STK(arg1)) == OT_CLASS) {
					if(obj_type(_class(STK(arg1))->_metamethods[MT_NEWMEMBER]) != OT_NULL ) {
						Push(STK(arg1)); Push(STK(arg2)); Push(STK(arg3));
						Push((arg0&NEW_SLOT_ATTRIBUTES_FLAG) ? STK(arg2-1) : _null_<Q>);
						int nparams = 4;
						if(Call(_class(STK(arg1))->_metamethods[MT_NEWMEMBER], nparams, _top - nparams, temp_reg,SQFalse)) {
							Pop(nparams);
							continue;
						}
					}
				}
				_GUARD(NewSlot(STK(arg1), STK(arg2), STK(arg3),bstatic));
				if((arg0&NEW_SLOT_ATTRIBUTES_FLAG)) {
					_class(STK(arg1))->SetAttributes(STK(arg2),STK(arg2-1));
				}
				continue;
			}
			
		}
	}
exception_trap:
	{
		SQObjectPtr currerror = _lasterror;
#ifdef _DEBUG_DUMP
		dumpstack(_stackbase);
#endif
		SQInteger n = 0;
		SQInteger last_top = _top;
		if(ci) {
			if(_ss(this)->_notifyallexceptions) CallErrorHandler(currerror);

			if(traps) {
				do {
					if(ci->_etraps > 0) {
						SQExceptionTrap &et = _etraps.top();
						ci->_ip = et._ip;
						_top = et._stacksize;
						_stackbase = et._stackbase;
						_stack._vals[_stackbase+et._extarget] = currerror;
						_etraps.pop_back(); traps--; ci->_etraps--;
						CLEARSTACK(last_top);
						goto exception_restore;
					}
					//if is a native closure
					if(obj_type(ci->_closure) != OT_CLOSURE && n)
						break;
					if(ci->_generator) ci->_generator->Kill();
					PopVarArgs(ci->_vargs);
					POP_CALLINFO(this);
					n++;
				} while(_callsstacksize);
			}
			else {
				//call the hook
				if(raiseerror && !_ss(this)->_notifyallexceptions)
					CallErrorHandler(currerror);
			}
			//remove call stack until a C function is found or the cstack is empty
			if(ci) do {
				SQBool exitafterthisone = ci->_root;
				if(ci->_generator) ci->_generator->Kill();
				_stackbase -= ci->_prevstkbase;
				_top = _stackbase + ci->_prevtop;
				PopVarArgs(ci->_vargs);
				POP_CALLINFO(this);
				if( (ci && obj_type(ci->_closure) != OT_CLOSURE) || exitafterthisone) break;
			} while(_callsstacksize);

			CLEARSTACK(last_top);
		}
		_lasterror = currerror;
		return false;
	}
	assert(0);
}

template <Squirk Q>
bool SQVM<Q>::CreateClassInstance(SQClass<Q> *theclass, SQObjectPtr<Q> &inst, SQObjectPtr<Q> &constructor)
{
	inst = theclass->CreateInstance();
	if(!theclass->Get(_ss(this)->_constructoridx,constructor)) {
		constructor = _null_<Q>;
	}
	return true;
}

template <Squirk Q>
void SQVM<Q>::CallErrorHandler(SQObjectPtr<Q> &error)
{
	if(obj_type(_errorhandler) != OT_NULL) {
		SQObjectPtr<Q> out;
		Push(_roottable); Push(error);
		Call(_errorhandler, 2, _top-2, out,SQFalse);
		Pop(2);
	}
}

template <Squirk Q>
void SQVM<Q>::CallDebugHook(SQInteger type,SQInteger forcedline)
{
	SQObjectPtr<Q> temp_reg;
	SQInteger nparams=5;
	SQFunctionProto<Q> *func=_funcproto(_closure(ci->_closure)->_function);
	Push(_roottable); Push(type); Push(func->_sourcename); Push(forcedline?forcedline:func->GetLine(ci->_ip)); Push(func->_name);
	Call(_debughook,nparams,_top-nparams,temp_reg,SQFalse);
	Pop(nparams);
}

template <Squirk Q>
bool SQVM<Q>::CallNative(SQNativeClosure<Q> *nclosure,SQInteger nargs,SQInteger stackbase,SQObjectPtr<Q> &retval,bool &suspend)
{
	if (_nnativecalls + 1 > MAX_NATIVE_CALLS) { Raise_Error(_SC("Native stack overflow")); return false; }
	SQInteger nparamscheck = nclosure->_nparamscheck;
	if(((nparamscheck > 0) && (nparamscheck != nargs))
		|| ((nparamscheck < 0) && (nargs < (-nparamscheck)))) {
		Raise_Error(_SC("wrong number of parameters"));
		return false;
		}

	SQInteger tcs;
	if((tcs = nclosure->_typecheck.size())) {
		for(SQInteger i = 0; i < nargs && i < tcs; i++)
			if((nclosure->_typecheck._vals[i] != -1) && !(obj_type(_stack._vals[stackbase+i]) & nclosure->_typecheck[i])) {
                Raise_ParamTypeError(i,nclosure->_typecheck._vals[i],obj_type(_stack._vals[stackbase+i]));
				return false;
			}
	}
	_nnativecalls++;
	if ((_top + MIN_STACK_OVERHEAD) > (SQInteger)_stack.size()) {
		_stack.resize(_stack.size() + (MIN_STACK_OVERHEAD<<1));
	}
	SQInteger oldtop = _top;
	SQInteger oldstackbase = _stackbase;
	_top = stackbase + nargs;
	CallInfo<Q> lci;
	lci._closure = nclosure;
	lci._generator = NULL;
	lci._etraps = 0;
	lci._prevstkbase = (SQInt32) (stackbase - _stackbase);
	lci._ncalls = 1;
	lci._prevtop = (SQInt32) (oldtop - oldstackbase);
	PUSH_CALLINFO(this, lci);
	_stackbase = stackbase;
	//push free variables
	SQInteger outers = nclosure->_outervalues.size();
	for (SQInteger i = 0; i < outers; i++) {
		Push(nclosure->_outervalues[i]);
	}

	if(obj_type(nclosure->_env) == OT_WEAKREF) {
		_stack[stackbase] = _weakref(nclosure->_env)->_obj;
	}

	
	SQInteger ret = (nclosure->_function)(this);
	_nnativecalls--;
	suspend = false;
	if( ret == SQ_SUSPEND_FLAG) suspend = true;
	else if (ret < 0) { 
		_stackbase = oldstackbase;
		_top = oldtop;
		POP_CALLINFO(this);
		Raise_Error(_lasterror);
		return false;
	}
	
	if (ret != 0){ retval = TOP(); TOP().Null(); }
	else { retval = _null_<Q>; }
	_stackbase = oldstackbase;
	_top = oldtop;
	POP_CALLINFO(this);
	return true;
}

template <Squirk Q>
bool SQVM<Q>::Get(const SQObjectPtr<Q> &self,const SQObjectPtr<Q> &key,SQObjectPtr<Q> &dest,bool raw, bool fetchroot)
{
	switch(obj_type(self)){
	case OT_TABLE:
		if(_table(self)->Get(key,dest))return true;
		break;
	case OT_ARRAY:
		if(sq_isnumeric(key)){
			return _array(self)->Get(tointeger(key),dest);
		}
		break;
	case OT_INSTANCE:
		if(_instance(self)->Get(key,dest)) return true;
		break;
	default:break; //shut up compiler
	}
	if(FallBackGet(self,key,dest,raw)) return true;

	if(fetchroot) {
		if(_rawval(STK(0)) == _rawval(self) &&
			obj_type(STK(0)) == obj_type(self)) {
				return _table(_roottable)->Get(key,dest);
		}
	}
	return false;
}

template <Squirk Q>
bool SQVM<Q>::FallBackGet(const SQObjectPtr<Q> &self,const SQObjectPtr<Q> &key,SQObjectPtr<Q> &dest,bool raw)
{
	switch(obj_type(self)){
	case OT_CLASS: 
		return _class(self)->Get(key,dest);
		break;
	case OT_TABLE:
	case OT_USERDATA:
        //delegation
		if(_delegable(self)->_delegate) {
			if(Get(SQObjectPtr(_delegable(self)->_delegate),key,dest,raw,false))
				return true;	
			if(raw)return false;
			Push(self);Push(key);
			if(CallMetaMethod(_delegable(self),MT_GET,2,dest))
				return true;
		}
		if(obj_type(self) == OT_TABLE) {
			if(raw) return false;
			return _table_ddel->Get(key,dest);
		}
		return false;
		break;
	case OT_ARRAY:
		if(raw)return false;
		return _array_ddel->Get(key,dest);
	case OT_STRING:
		if(sq_isnumeric(key)){
			SQInteger n=tointeger(key);
			if(abs((int)n)<_string(self)->_len){
				if(n<0)n=_string(self)->_len-n;
				dest=SQInteger(_stringval(self)[n]);
				return true;
			}
			return false;
		}
		else {
			if(raw)return false;
			return _string_ddel->Get(key,dest);
		}
		break;
	case OT_INSTANCE:
		if(raw)return false;
		Push(self);Push(key);
		if(!CallMetaMethod(_delegable(self),MT_GET,2,dest)) {
			return _instance_ddel->Get(key,dest);
		}
		return true;
	case OT_INTEGER:case OT_FLOAT:case OT_BOOL: 
		if(raw)return false;
		return _number_ddel->Get(key,dest);
	case OT_GENERATOR: 
		if(raw)return false;
		return _generator_ddel->Get(key,dest);
	case OT_CLOSURE: case OT_NATIVECLOSURE:	
		if(raw)return false;
		return _closure_ddel->Get(key,dest);
	case OT_THREAD:
		if(raw)return false;
		return  _thread_ddel->Get(key,dest);
	case OT_WEAKREF:
		if(raw)return false;
		return  _weakref_ddel->Get(key,dest);
	default:return false;
	}
	return false;
}

template <Squirk Q>
bool SQVM<Q>::Set(const SQObjectPtr<Q> &self,const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val,bool fetchroot)
{
	switch(obj_type(self)){
	case OT_TABLE:
		if(_table(self)->Set(key,val))
			return true;
		if(_table(self)->_delegate) {
			if(Set(_table(self)->_delegate,key,val,false)) {
				return true;
			}
		}
		//keeps going
	case OT_USERDATA:
		if(_delegable(self)->_delegate) {
			SQObjectPtr<Q> t;
			Push(self);Push(key);Push(val);
			if(CallMetaMethod(_delegable(self),MT_SET,3,t)) return true;
		}
		break;
	case OT_INSTANCE:{
		if(_instance(self)->Set(key,val))
			return true;
		SQObjectPtr<Q> t;
		Push(self);Push(key);Push(val);
		if(CallMetaMethod(_delegable(self),MT_SET,3,t)) return true;
		}
		break;
	case OT_ARRAY:
		if(!sq_isnumeric(key)) {Raise_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key)); return false; }
		return _array(self)->Set(tointeger(key),val);
	default:
		Raise_Error(_SC("trying to set '%s'"),GetTypeName(self));
		return false;
	}
	if(fetchroot) {
		if(_rawval(STK(0)) == _rawval(self) &&
			obj_type(STK(0)) == obj_type(self)) {
				return _table(_roottable)->Set(key,val);
			}
	}
	return false;
}

template <Squirk Q>
bool SQVM<Q>::Clone(const SQObjectPtr<Q> &self,SQObjectPtr<Q> &target)
{
	SQObjectPtr<Q> temp_reg;
	SQObjectPtr<Q> newobj;
	switch(obj_type(self)){
	case OT_TABLE:
		newobj = _table(self)->Clone();
		goto cloned_mt;
	case OT_INSTANCE:
		newobj = _instance(self)->Clone(_ss(this));
cloned_mt:
		if(_delegable(newobj)->_delegate){
			Push(newobj);
			Push(self);
			CallMetaMethod(_delegable(newobj),MT_CLONED,2,temp_reg);
		}
		target = newobj;
		return true;
	case OT_ARRAY: 
		target = _array(self)->Clone();
		return true;
	default: return false;
	}
}

template <Squirk Q>
bool SQVM<Q>::NewSlot(const SQObjectPtr<Q> &self,const SQObjectPtr<Q> &key,const SQObjectPtr<Q> &val,bool bstatic)
{
	if(obj_type(key) == OT_NULL) { Raise_Error(_SC("null cannot be used as index")); return false; }
	switch(obj_type(self)) {
	case OT_TABLE: {
		bool rawcall = true;
		if(_table(self)->_delegate) {
			SQObjectPtr<Q> res;
			if(!_table(self)->Get(key,res)) {
				Push(self);Push(key);Push(val);
				rawcall = !CallMetaMethod(_table(self),MT_NEWSLOT,3,res);
			}
		}
		if(rawcall) _table(self)->NewSlot(key,val); //cannot fail
		
		break;}
	case OT_INSTANCE: {
		SQObjectPtr<Q> res;
		Push(self);Push(key);Push(val);
		if(!CallMetaMethod(_instance(self),MT_NEWSLOT,3,res)) {
			Raise_Error(_SC("class instances do not support the new slot operator"));
			return false;
		}
		break;}
	case OT_CLASS: 
		if(!_class(self)->NewSlot(_ss(this),key,val,bstatic)) {
			if(_class(self)->_locked) {
				Raise_Error(_SC("trying to modify a class that has already been instantiated"));
				return false;
			}
			else {
				SQObjectPtr oval = PrintObjVal(key);
				Raise_Error(_SC("the property '%s' already exists"),_stringval(oval));
				return false;
			}
		}
		break;
	default:
		Raise_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key));
		return false;
		break;
	}
	return true;
}

template <Squirk Q>
bool SQVM<Q>::DeleteSlot(const SQObjectPtr<Q> &self,const SQObjectPtr<Q> &key,SQObjectPtr<Q> &res)
{
	switch(obj_type(self)) {
	case OT_TABLE:
	case OT_INSTANCE:
	case OT_USERDATA: {
		SQObjectPtr<Q> t;
		bool handled = false;
		if(_delegable(self)->_delegate) {
			Push(self);Push(key);
			handled = CallMetaMethod(_delegable(self),MT_DELSLOT,2,t);
		}

		if(!handled) {
			if(obj_type(self) == OT_TABLE) {
				if(_table(self)->Get(key,t)) {
					_table(self)->Remove(key);
				}
				else {
					Raise_IdxError((SQObject<Q> &)key);
					return false;
				}
			}
			else {
				Raise_Error(_SC("cannot delete a slot from %s"),GetTypeName(self));
				return false;
			}
		}
		res = t;
				}
		break;
	default:
		Raise_Error(_SC("attempt to delete a slot from a %s"),GetTypeName(self));
		return false;
	}
	return true;
}

template <Squirk Q>
bool SQVM<Q>::Call(SQObjectPtr<Q> &closure,SQInteger nparams,SQInteger stackbase,SQObjectPtr<Q> &outres,SQBool raiseerror)
{
#ifdef _DEBUG
SQInteger prevstackbase = _stackbase;
#endif
	switch(obj_type(closure)) {
	case OT_CLOSURE:
		return Execute(closure, _top - nparams, nparams, stackbase,outres,raiseerror);
		break;
	case OT_NATIVECLOSURE:{
		bool suspend = false;
		bool ret = CallNative(_nativeclosure(closure), nparams, stackbase, outres, suspend);
		if (suspend) {
			_suspended = SQTrue;
			_suspended_target = ci->_target;
			_suspended_root = ci->_root;
			_suspended_traps = ci->_etraps;
			_suspend_varargs = ci->_vargs;
			outres = closure;
		}
		return ret;
		}
		break;
	case OT_CLASS: {
		SQObjectPtr<Q> constr;
		SQObjectPtr<Q> temp;
		CreateClassInstance(_class(closure),outres,constr);
		if(obj_type(constr) != OT_NULL) {
			_stack[stackbase] = outres;
			return Call(constr,nparams,stackbase,temp,raiseerror);
		}
		return true;
		}
		break;
	default:
		return false;
	}
#ifdef _DEBUG
	if(!_suspended) {
		assert(_stackbase == prevstackbase);
	}
#endif
	return true;
}

template <Squirk Q>
bool SQVM<Q>::CallMetaMethod(SQDelegable<Q> *del,SQMetaMethod mm,SQInteger nparams,SQObjectPtr<Q> &outres)
{
	SQObjectPtr<Q> closure;
	if(del->GetMetaMethod(this, mm, closure)) {
		if(Call(closure, nparams, _top - nparams, outres, SQFalse)) {
			Pop(nparams);
			return true;
		}
	}
	Pop(nparams);
	return false;
}

template <Squirk Q>
void SQVM<Q>::Remove(SQInteger n) {
	n = (n >= 0)?n + _stackbase - 1:_top + n;
	for(SQInteger i = n; i < _top; i++){
		_stack[i] = _stack[i+1];
	}
	_stack[_top] = _null_<Q>;
	_top--;
}

template <Squirk Q>
void SQVM<Q>::Pop() {
	_stack[--_top] = _null_<Q>;
}

template <Squirk Q>
void SQVM<Q>::Pop(SQInteger n) {
	for(SQInteger i = 0; i < n; i++){
		_stack[--_top] = _null_<Q>;
	}
}

template <Squirk Q>
void SQVM<Q>::Push(const SQObjectPtr<Q> &o) { _stack[_top++] = o; }
template <Squirk Q>
SQObjectPtr<Q> &SQVM<Q>::Top() { return _stack[_top-1]; }
template <Squirk Q>
SQObjectPtr<Q> &SQVM<Q>::PopGet() { return _stack[--_top]; }
template <Squirk Q>
SQObjectPtr<Q> &SQVM<Q>::GetUp(SQInteger n) { return _stack[_top+n]; }
template <Squirk Q>
SQObjectPtr<Q> &SQVM<Q>::GetAt(SQInteger n) { return _stack[n]; }

#ifdef _DEBUG_DUMP
template <Squirk Q>
void SQVM<Q>::dumpstack(SQInteger stackbase,bool dumpall)
{
	SQInteger size=dumpall?_stack.size():_top;
	SQInteger n=0;
	scprintf(_SC("\n>>>>stack dump<<<<\n"));
	CallInfo<Q> &ci=_callsstack[_callsstacksize-1];
	scprintf(_SC("IP: %p\n"),ci._ip);
	scprintf(_SC("prev stack base: %d\n"),ci._prevstkbase);
	scprintf(_SC("prev top: %d\n"),ci._prevtop);
	for(SQInteger i=0;i<size;i++){
		SQObjectPtr<Q> &obj=_stack[i];	
		if(stackbase==i)scprintf(_SC(">"));else scprintf(_SC(" "));
		scprintf(_SC("[%d]:"),n);
		switch(obj_type(obj)){
		case OT_FLOAT:			scprintf(_SC("FLOAT %.3f"),_float(obj));break;
		case OT_INTEGER:		scprintf(_SC("INTEGER %d"),_integer(obj));break;
		case OT_BOOL:			scprintf(_SC("BOOL %s"),_integer(obj)?"true":"false");break;
		case OT_STRING:			scprintf(_SC("STRING %s"),_stringval(obj));break;
		case OT_NULL:			scprintf(_SC("NULL"));	break;
		case OT_TABLE:			scprintf(_SC("TABLE %p[%p]"),_table(obj),_table(obj)->_delegate);break;
		case OT_ARRAY:			scprintf(_SC("ARRAY %p"),_array(obj));break;
		case OT_CLOSURE:		scprintf(_SC("CLOSURE [%p]"),_closure(obj));break;
		case OT_NATIVECLOSURE:	scprintf(_SC("NATIVECLOSURE"));break;
		case OT_USERDATA:		scprintf(_SC("USERDATA %p[%p]"),_userdataval(obj),_userdata(obj)->_delegate);break;
		case OT_GENERATOR:		scprintf(_SC("GENERATOR %p"),_generator(obj));break;
		case OT_THREAD:			scprintf(_SC("THREAD [%p]"),_thread(obj));break;
		case OT_USERPOINTER:	scprintf(_SC("USERPOINTER %p"),_userpointer(obj));break;
		case OT_CLASS:			scprintf(_SC("CLASS %p"),_class(obj));break;
		case OT_INSTANCE:		scprintf(_SC("INSTANCE %p"),_instance(obj));break;
		case OT_WEAKREF:		scprintf(_SC("WEAKERF %p"),_weakref(obj));break;
		default:
			assert(0);
			break;
		};
		scprintf(_SC("\n"));
		++n;
	}
}
#endif
