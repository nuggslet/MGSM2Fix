/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <stdarg.h>
#include <setjmp.h>
#include "sqopcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqcompiler.h"
#include "sqfuncstate.h"
#include "sqlexer.h"
#include "sqvm.h"
#include "sqtable.h"

#define DEREF_NO_DEREF	-1
#define DEREF_FIELD		-2

struct ExpState
{
	ExpState()
	{
		_deref = DEREF_NO_DEREF;
		_freevar = false;
		_class_or_delete = false;
		_funcarg = false;
	}
	bool _class_or_delete;
	bool _funcarg;
	bool _freevar;
	SQInteger _deref;
};

typedef sqvector<ExpState> ExpStateVec;

#define _exst (_expstates.top())

#define BEGIN_BREAKBLE_BLOCK()	SQInteger __nbreaks__=_fs->_unresolvedbreaks.size(); \
							SQInteger __ncontinues__=_fs->_unresolvedcontinues.size(); \
							_fs->_breaktargets.push_back(0);_fs->_continuetargets.push_back(0);

#define END_BREAKBLE_BLOCK(continue_target) {__nbreaks__=_fs->_unresolvedbreaks.size()-__nbreaks__; \
					__ncontinues__=_fs->_unresolvedcontinues.size()-__ncontinues__; \
					if(__ncontinues__>0)ResolveContinues(_fs,__ncontinues__,continue_target); \
					if(__nbreaks__>0)ResolveBreaks(_fs,__nbreaks__); \
					_fs->_breaktargets.pop_back();_fs->_continuetargets.pop_back();}

template <Squirk Q>
class SQCompiler
{
public:
	SQCompiler(SQVM<Q> *v, SQLEXREADFUNC rg, SQUserPointer up, const SQChar* sourcename, bool raiseerror, bool lineinfo)
	{
		_vm=v;
		_lex.Init(_ss(v), rg, up,ThrowError,this);
		_sourcename = SQString<Q>::Create(_ss(v), sourcename);
		_lineinfo = lineinfo;_raiseerror = raiseerror;
		compilererror = NULL;
	}
	static void ThrowError(void *ud, const SQChar *s) {
		SQCompiler<Q> *c = (SQCompiler<Q> *)ud;
		c->Error(s);
	}
	void Error(const SQChar *s, ...)
	{
		static SQChar temp[256];
		va_list vl;
		va_start(vl, s);
		scvsprintf(temp, s, vl);
		va_end(vl);
		compilererror = temp;
		longjmp(_errorjmp,1);
	}
	void Lex(){	_token = _lex.Lex();}
	void PushExpState(){ _expstates.push_back(ExpState()); }
	bool IsDerefToken(SQInteger tok)
	{
		switch(tok){
		case _SC('='): case _SC('('): case TK_NEWSLOT:
		case TK_MODEQ: case TK_MULEQ: case TK_DIVEQ: case TK_MINUSEQ: case TK_PLUSEQ: case TK_PLUSPLUS: case TK_MINUSMINUS: return true;
		}
		return false;
	}
	ExpState PopExpState()
	{
		ExpState ret = _expstates.top();
		_expstates.pop_back();
		return ret;
	}
	SQObject<Q> Expect(SQInteger tok)
	{
		
		if(_token != tok) {
			if(_token == TK_CONSTRUCTOR && tok == TK_IDENTIFIER) {
				//ret = SQString::Create(_ss(_vm),_SC("constructor"));
				//do nothing
			}
			else {
				const SQChar *etypename;
				if(tok > 255) {
					switch(tok)
					{
					case TK_IDENTIFIER:
						etypename = _SC("IDENTIFIER");
						break;
					case TK_STRING_LITERAL:
						etypename = _SC("STRING_LITERAL");
						break;
					case TK_INTEGER:
						etypename = _SC("INTEGER");
						break;
					case TK_FLOAT:
						etypename = _SC("FLOAT");
						break;
					default:
						etypename = _lex.Tok2Str(tok);
					}
					Error(_SC("expected '%s'"), etypename);
				}
				Error(_SC("expected '%c'"), tok);
			}
		}
		SQObjectPtr<Q> ret;
		switch(tok)
		{
		case TK_IDENTIFIER:
			ret = _fs->CreateString(_lex._svalue);
			break;
		case TK_STRING_LITERAL:
			ret = _fs->CreateString(_lex._svalue,_lex._longstr.size()-1);
			break;
		case TK_INTEGER:
			ret = SQObjectPtr<Q>(_lex._nvalue);
			break;
		case TK_FLOAT:
			ret = SQObjectPtr<Q>(_lex._fvalue);
			break;
		}
		Lex();
		return ret;
	}
	bool IsEndOfStatement() { return ((_lex._prevtoken == _SC('\n')) || (_token == SQUIRREL_EOB) || (_token == _SC('}')) || (_token == _SC(';'))); }
	void OptionalSemicolon()
	{
		if(_token == _SC(';')) { Lex(); return; }
		if(!IsEndOfStatement()) {
			Error(_SC("end of statement expected (; or lf)"));
		}
	}
	void MoveIfCurrentTargetIsLocal() {
		SQInteger trg = _fs->TopTarget();
		if(_fs->IsLocal(trg)) {
			trg = _fs->PopTarget(); //no pops the target and move it
			_fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), trg);
		}
	}
	bool Compile(SQObjectPtr<Q> &o)
	{
		_debugline = 1;
		_debugop = 0;

		SQFuncState<Q> funcstate(_ss(_vm), NULL,ThrowError,this);
		funcstate._name = SQString<Q>::Create(_ss(_vm), _SC("main"));
		_fs = &funcstate;
		_fs->AddParameter(_fs->CreateString(_SC("this")));
		_fs->_sourcename = _sourcename;
		SQInteger stacksize = _fs->GetStackSize();
		if(setjmp(_errorjmp) == 0) {
			Lex();
			while(_token > 0){
				Statement();
				if(_lex._prevtoken != _SC('}')) OptionalSemicolon();
			}
			CleanStack(stacksize);
			_fs->AddLineInfos(_lex._currentline, _lineinfo, true);
			_fs->AddInstruction(_OP_RETURN, 0xFF);
			_fs->SetStackSize(0);
			o =_fs->BuildProto();
#ifdef _DEBUG_DUMP
			_fs->Dump(_funcproto(o));
#endif
		}
		else {
			if(_raiseerror && _ss(_vm)->_compilererrorhandler) {
				_ss(_vm)->_compilererrorhandler(_vm, compilererror, obj_type(_sourcename) == OT_STRING?_stringval(_sourcename):_SC("unknown"),
					_lex._currentline, _lex._currentcolumn);
			}
			_vm->_lasterror = SQString<Q>::Create(_ss(_vm), compilererror, -1);
			return false;
		}
		return true;
	}
	void Statements()
	{
		while(_token != _SC('}') && _token != TK_DEFAULT && _token != TK_CASE) {
			Statement();
			if(_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
		}
	}
	void Statement()
	{
		_fs->AddLineInfos(_lex._currentline, _lineinfo);
		switch(_token){
		case _SC(';'):	Lex();					break;
		case TK_IF:		IfStatement();			break;
		case TK_WHILE:		WhileStatement();		break;
		case TK_DO:		DoWhileStatement();		break;
		case TK_FOR:		ForStatement();			break;
		case TK_FOREACH:	ForEachStatement();		break;
		case TK_SWITCH:	SwitchStatement();		break;
		case TK_LOCAL:		LocalDeclStatement();	break;
		case TK_RETURN:
		case TK_YIELD: {
			SQOpcode op;
			if(_token == TK_RETURN) {
				op = _OP_RETURN;
				
			}
			else {
				op = _OP_YIELD;
				_fs->_bgenerator = true;
			}
			Lex();
			if(!IsEndOfStatement()) {
				SQInteger retexp = _fs->GetCurrentPos()+1;
				CommaExpr();
				if(op == _OP_RETURN && _fs->_traps > 0)
					_fs->AddInstruction(_OP_POPTRAP, _fs->_traps, 0);
				_fs->_returnexp = retexp;
				_fs->AddInstruction(op, 1, _fs->PopTarget());
			}
			else{ 
				if(op == _OP_RETURN && _fs->_traps > 0)
					_fs->AddInstruction(_OP_POPTRAP, _fs->_traps ,0);
				_fs->_returnexp = -1;
				_fs->AddInstruction(op, 0xFF); 
			}
			break;}
		case TK_BREAK:
			if(_fs->_breaktargets.size() <= 0)Error(_SC("'break' has to be in a loop block"));
			if(_fs->_breaktargets.top() > 0){
				_fs->AddInstruction(_OP_POPTRAP, _fs->_breaktargets.top(), 0);
			}
			_fs->AddInstruction(_OP_JMP, 0, -1234);
			_fs->_unresolvedbreaks.push_back(_fs->GetCurrentPos());
			Lex();
			break;
		case TK_CONTINUE:
			if(_fs->_continuetargets.size() <= 0)Error(_SC("'continue' has to be in a loop block"));
			if(_fs->_continuetargets.top() > 0) {
				_fs->AddInstruction(_OP_POPTRAP, _fs->_continuetargets.top(), 0);
			}
			_fs->AddInstruction(_OP_JMP, 0, -1234);
			_fs->_unresolvedcontinues.push_back(_fs->GetCurrentPos());
			Lex();
			break;
		case TK_FUNCTION:
			FunctionStatement();
			break;
		case TK_CLASS:
			ClassStatement();
			break;
		case TK_ENUM:
			EnumStatement();
			break;
		case _SC('{'):{
				SQInteger stacksize = _fs->GetStackSize();
				Lex();
				Statements();
				Expect(_SC('}'));
				_fs->SetStackSize(stacksize);
			}
			break;
		case TK_TRY:
			TryCatchStatement();
			break;
		case TK_THROW:
			Lex();
			CommaExpr();
			_fs->AddInstruction(_OP_THROW, _fs->PopTarget());
			break;
		case TK_CONST:
			{
			Lex();
			SQObject<Q> id = Expect(TK_IDENTIFIER);
			Expect('=');
			SQObject<Q> val = ExpectScalar();
			OptionalSemicolon();
			SQTable<Q> *enums = _table(_ss(_vm)->_consts);
			SQObjectPtr<Q> strongid = id;
			enums->NewSlot(strongid,SQObjectPtr<Q>(val));
			strongid.Null();
			}
			break;
		default:
			CommaExpr();
			_fs->PopTarget();
			break;
		}
		_fs->SnoozeOpt();
	}
	void EmitDerefOp(SQOpcode op)
	{
		SQInteger val = _fs->PopTarget();
		SQInteger key = _fs->PopTarget();
		SQInteger src = _fs->PopTarget();
        _fs->AddInstruction(op,_fs->PushTarget(),src,key,val);
	}
	void Emit2ArgsOP(SQOpcode op, SQInteger p3 = 0)
	{
		SQInteger p2 = _fs->PopTarget(); //src in OP_GET
		SQInteger p1 = _fs->PopTarget(); //key in OP_GET
		_fs->AddInstruction(op,_fs->PushTarget(), p1, p2, p3);
	}
	void EmitCompoundArith(SQInteger tok,bool deref)
	{
		SQInteger oper;
		switch(tok){
		case TK_MINUSEQ: oper = '-'; break;
		case TK_PLUSEQ: oper = '+'; break;
		case TK_MULEQ: oper = '*'; break;
		case TK_DIVEQ: oper = '/'; break;
		case TK_MODEQ: oper = '%'; break;
		default: oper = 0; //shut up compiler
			assert(0); break;
		};
		if(deref) {
			SQInteger val = _fs->PopTarget();
			SQInteger key = _fs->PopTarget();
			SQInteger src = _fs->PopTarget();
			//mixes dest obj and source val in the arg1(hack?)
			_fs->AddInstruction(_OP_COMPARITH,_fs->PushTarget(),(src<<16)|val,key,oper);
		}
		else {
			Emit2ArgsOP(_OP_COMPARITHL, oper);
		}
	}
	void CommaExpr()
	{
		for(Expression();_token == ',';_fs->PopTarget(), Lex(), CommaExpr());
	}
	ExpState Expression(bool funcarg = false)
	{
		PushExpState();
		_exst._class_or_delete = false;
		_exst._funcarg = funcarg;
		LogicalOrExp();
		switch(_token)  {
		case _SC('='):
		case TK_NEWSLOT:
		case TK_MINUSEQ:
		case TK_PLUSEQ:
		case TK_MULEQ:
		case TK_DIVEQ:
		case TK_MODEQ:
		{
				SQInteger op = _token;
				SQInteger ds = _exst._deref;
				bool freevar = _exst._freevar;
				if(ds == DEREF_NO_DEREF) Error(_SC("can't assign expression"));
				Lex(); Expression();

				switch(op){
				case TK_NEWSLOT:
					if(freevar) Error(_SC("free variables cannot be modified"));
					if(ds == DEREF_FIELD)
						EmitDerefOp(_OP_NEWSLOT);
					else //if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
						Error(_SC("can't 'create' a local slot"));
					break;
				case _SC('='): //ASSIGN
					if(freevar) Error(_SC("free variables cannot be modified"));
					if(ds == DEREF_FIELD)
						EmitDerefOp(_OP_SET);
					else {//if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
						SQInteger p2 = _fs->PopTarget(); //src in OP_GET
						SQInteger p1 = _fs->TopTarget(); //key in OP_GET
						_fs->AddInstruction(_OP_MOVE, p1, p2);
					}
					break;
				case TK_MINUSEQ:
				case TK_PLUSEQ:
				case TK_MULEQ:
				case TK_DIVEQ:
				case TK_MODEQ:
					EmitCompoundArith(op,ds == DEREF_FIELD);
					break;
				}
			}
			break;
		case _SC('?'): {
			Lex();
			_fs->AddInstruction(_OP_JZ, _fs->PopTarget());
			SQInteger jzpos = _fs->GetCurrentPos();
			SQInteger trg = _fs->PushTarget();
			Expression();
			SQInteger first_exp = _fs->PopTarget();
			if(trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
			SQInteger endfirstexp = _fs->GetCurrentPos();
			_fs->AddInstruction(_OP_JMP, 0, 0);
			Expect(_SC(':'));
			SQInteger jmppos = _fs->GetCurrentPos();
			Expression();
			SQInteger second_exp = _fs->PopTarget();
			if(trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
			_fs->SetIntructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
			_fs->SetIntructionParam(jzpos, 1, endfirstexp - jzpos + 1);
			_fs->SnoozeOpt();
			}
			break;
		}
		return PopExpState();
	}
	void BIN_EXP(SQOpcode op, void (SQCompiler<Q>::*f)(void),SQInteger op3 = 0)
	{
		Lex(); (this->*f)();
		SQInteger op1 = _fs->PopTarget();SQInteger op2 = _fs->PopTarget();
		_fs->AddInstruction(op, _fs->PushTarget(), op1, op2, op3);
	}
	void LogicalOrExp()
	{
		LogicalAndExp();
		for(;;) if(_token == TK_OR) {
			SQInteger first_exp = _fs->PopTarget();
			SQInteger trg = _fs->PushTarget();
			_fs->AddInstruction(_OP_OR, trg, 0, first_exp, 0);
			SQInteger jpos = _fs->GetCurrentPos();
			if(trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
			Lex(); LogicalOrExp();
			_fs->SnoozeOpt();
			SQInteger second_exp = _fs->PopTarget();
			if(trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
			_fs->SnoozeOpt();
			_fs->SetIntructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
			break;
		}else return;
	}
	void LogicalAndExp()
	{
		BitwiseOrExp();
		for(;;) switch(_token) {
		case TK_AND: {
			SQInteger first_exp = _fs->PopTarget();
			SQInteger trg = _fs->PushTarget();
			_fs->AddInstruction(_OP_AND, trg, 0, first_exp, 0);
			SQInteger jpos = _fs->GetCurrentPos();
			if(trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
			Lex(); LogicalAndExp();
			_fs->SnoozeOpt();
			SQInteger second_exp = _fs->PopTarget();
			if(trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
			_fs->SnoozeOpt();
			_fs->SetIntructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
			break;
			}
		case TK_IN: BIN_EXP(_OP_EXISTS, &SQCompiler::BitwiseOrExp); break;
		case TK_INSTANCEOF: BIN_EXP(_OP_INSTANCEOF, &SQCompiler::BitwiseOrExp); break;
		default:
			return;
		}
	}
	void BitwiseOrExp()
	{
		BitwiseXorExp();
		for(;;) if(_token == _SC('|'))
		{BIN_EXP(_OP_BITW, &SQCompiler::BitwiseXorExp,BW_OR);
		}else return;
	}
	void BitwiseXorExp()
	{
		BitwiseAndExp();
		for(;;) if(_token == _SC('^'))
		{BIN_EXP(_OP_BITW, &SQCompiler::BitwiseAndExp,BW_XOR);
		}else return;
	}
	void BitwiseAndExp()
	{
		CompExp();
		for(;;) if(_token == _SC('&'))
		{BIN_EXP(_OP_BITW, &SQCompiler::CompExp,BW_AND);
		}else return;
	}
	void CompExp()
	{
		ShiftExp();
		for(;;) switch(_token) {
		case TK_EQ: BIN_EXP(_OP_EQ, &SQCompiler::ShiftExp); break;
		case _SC('>'): BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp,CMP_G); break;
		case _SC('<'): BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp,CMP_L); break;
		case TK_GE: BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp,CMP_GE); break;
		case TK_LE: BIN_EXP(_OP_CMP, &SQCompiler::ShiftExp,CMP_LE); break;
		case TK_NE: BIN_EXP(_OP_NE, &SQCompiler::ShiftExp); break;
		default: return;	
		}
	}
	void ShiftExp()
	{
		PlusExp();
		for(;;) switch(_token) {
		case TK_USHIFTR: BIN_EXP(_OP_BITW, &SQCompiler::PlusExp,BW_USHIFTR); break;
		case TK_SHIFTL: BIN_EXP(_OP_BITW, &SQCompiler::PlusExp,BW_SHIFTL); break;
		case TK_SHIFTR: BIN_EXP(_OP_BITW, &SQCompiler::PlusExp,BW_SHIFTR); break;
		default: return;	
		}
	}
	void PlusExp()
	{
		MultExp();
		for(;;) switch(_token) {
		case _SC('+'): case _SC('-'):
			BIN_EXP(_OP_ARITH, &SQCompiler::MultExp,_token); break;
		default: return;
		}
	}
	
	void MultExp()
	{
		PrefixedExpr();
		for(;;) switch(_token) {
		case _SC('*'): case _SC('/'): case _SC('%'):
			BIN_EXP(_OP_ARITH, &SQCompiler::PrefixedExpr,_token); break;
		default: return;
		}
	}
	//if 'pos' != -1 the previous variable is a local variable
	void PrefixedExpr()
	{
		SQInteger pos = Factor();
		for(;;) {
			switch(_token) {
			case _SC('.'): {
				pos = -1;
				Lex(); 
				if(_token == TK_PARENT) {
					Lex();
					if(!NeedGet())
						Error(_SC("parent cannot be set"));
					SQInteger src = _fs->PopTarget();
					_fs->AddInstruction(_OP_GETPARENT, _fs->PushTarget(), src);
				}
				else {
					_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(Expect(TK_IDENTIFIER)));
					if(NeedGet()) Emit2ArgsOP(_OP_GET);
				}
				_exst._deref = DEREF_FIELD;
				_exst._freevar = false;
				}
				break;
			case _SC('['):
				if(_lex._prevtoken == _SC('\n')) Error(_SC("cannot brake deref/or comma needed after [exp]=exp slot declaration"));
				Lex(); Expression(); Expect(_SC(']')); 
				pos = -1;
				if(NeedGet()) Emit2ArgsOP(_OP_GET);
				_exst._deref = DEREF_FIELD;
				_exst._freevar = false;
				break;
			case TK_MINUSMINUS:
			case TK_PLUSPLUS:
			if(_exst._deref != DEREF_NO_DEREF && !IsEndOfStatement()) { 
				SQInteger tok = _token; Lex();
				if(pos < 0)
					Emit2ArgsOP(_OP_PINC,tok == TK_MINUSMINUS?-1:1);
				else {//if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
					SQInteger src = _fs->PopTarget();
					_fs->AddInstruction(_OP_PINCL, _fs->PushTarget(), src, 0, tok == TK_MINUSMINUS?-1:1);
				}
				
			}
			return;
			break;	
			case _SC('('): 
				{
				if(_exst._deref != DEREF_NO_DEREF) {
					if(pos<0) {
						SQInteger key = _fs->PopTarget(); //key
						SQInteger table = _fs->PopTarget(); //table etc...
						SQInteger closure = _fs->PushTarget();
						SQInteger ttarget = _fs->PushTarget();
						_fs->AddInstruction(_OP_PREPCALL, closure, key, table, ttarget);
					}
					else{
						_fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
					}
				}
				else
					_fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
				_exst._deref = DEREF_NO_DEREF;
				Lex();
				FunctionCallArgs();
				 }
				break;
			default: return;
			}
		}
	}
	SQInteger Factor()
	{
		switch(_token)
		{
		case TK_STRING_LITERAL: {
				//SQObjectPtr id(SQString::Create(_ss(_vm), _lex._svalue,_lex._longstr.size()-1));
				_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(_fs->CreateString(_lex._svalue,_lex._longstr.size()-1)));
				Lex(); 
			}
			break;
		case TK_VARGC: Lex(); _fs->AddInstruction(_OP_VARGC, _fs->PushTarget()); break;
		case TK_VARGV: { Lex();
			Expect(_SC('['));
			Expression();
			Expect(_SC(']'));
			SQInteger src = _fs->PopTarget();
			_fs->AddInstruction(_OP_GETVARGV, _fs->PushTarget(), src);
					   }
			break;
		case TK_IDENTIFIER:
		case TK_CONSTRUCTOR:
		case TK_THIS:{
			_exst._freevar = false;
			SQObject<Q> id;
			SQObject<Q> constant;
				switch(_token) {
					case TK_IDENTIFIER: id = _fs->CreateString(_lex._svalue); break;
					case TK_THIS: id = _fs->CreateString(_SC("this")); break;
					case TK_CONSTRUCTOR: id = _fs->CreateString(_SC("constructor")); break;
				}
				SQInteger pos = -1;
				Lex();
				if((pos = _fs->GetLocalVariable(id)) == -1) {
					//checks if is a free variable
					if((pos = _fs->GetOuterVariable(id)) != -1) {
						_exst._deref = _fs->PushTarget();
						_fs->AddInstruction(_OP_LOADFREEVAR, _exst._deref ,pos);	
						_exst._freevar = true;
					}
					else if(_fs->IsConstant(id,constant)) { //line 634
						SQObjectPtr<Q> constval;
						SQObject<Q> constid;
						if(obj_type(constant) == OT_TABLE) {
							Expect('.'); constid = Expect(TK_IDENTIFIER);
							if(!_table(constant)->Get(constid,constval)) {
								constval.Null();
								Error(_SC("invalid constant [%s.%s]"), _stringval(id),_stringval(constid));
							}
						}
						else {
							constval = constant;
						}
						_exst._deref = _fs->PushTarget();
						SQObjectType ctype = obj_type(constval);
						if(ctype == OT_INTEGER && (_integer(constval) & (~0x7FFFFFFF)) == 0) {
							_fs->AddInstruction(_OP_LOADINT, _exst._deref,_integer(constval));
						}
						else if(ctype == OT_FLOAT && sizeof(SQFloat) == sizeof(SQInt32)) {
							SQFloat f = _float(constval);
							_fs->AddInstruction(_OP_LOADFLOAT, _exst._deref,*((SQInt32 *)&f));
						}
						else {
							_fs->AddInstruction(_OP_LOAD, _exst._deref, _fs->GetConstant(constval));
						}

						_exst._freevar = true;
					}
					else {
						_fs->PushTarget(0);
						_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
						if(NeedGet()) Emit2ArgsOP(_OP_GET);
						_exst._deref = DEREF_FIELD;
					}
				}
				
				else{
					_fs->PushTarget(pos);
					_exst._deref = pos;
				}
				return _exst._deref;
			}
			break;
		case TK_PARENT: Lex();_fs->AddInstruction(_OP_GETPARENT, _fs->PushTarget(), 0); break;
		case TK_DOUBLE_COLON:  // "::"
			_fs->AddInstruction(_OP_LOADROOTTABLE, _fs->PushTarget());
			_exst._deref = DEREF_FIELD;
			_token = _SC('.'); //hack
			return -1;
			break;
		case TK_NULL: 
			_fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(),1);
			Lex();
			break;
		case TK_INTEGER: {
			if((_lex._nvalue & (~0x7FFFFFFF)) == 0) { //does it fit in 32 bits?
				_fs->AddInstruction(_OP_LOADINT, _fs->PushTarget(),_lex._nvalue);
			}
			else {
				_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetNumericConstant(_lex._nvalue));
			}
			Lex();
						 }
			break;
		case TK_FLOAT:
			if(sizeof(SQFloat) == sizeof(SQInt32)) {
				_fs->AddInstruction(_OP_LOADFLOAT, _fs->PushTarget(),*((SQInt32 *)&_lex._fvalue));
			}
			else {
				_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetNumericConstant(_lex._fvalue));
			}
			Lex();
			break;
		case TK_TRUE: case TK_FALSE:
			_fs->AddInstruction(_OP_LOADBOOL, _fs->PushTarget(),_token == TK_TRUE?1:0);
			Lex();
			break;
		case _SC('['): {
				_fs->AddInstruction(_OP_NEWARRAY, _fs->PushTarget());
				SQInteger apos = _fs->GetCurrentPos(),key = 0;
				Lex();
				while(_token != _SC(']')) {
                    Expression(); 
					if(_token == _SC(',')) Lex();
					SQInteger val = _fs->PopTarget();
					SQInteger array = _fs->TopTarget();
					_fs->AddInstruction(_OP_APPENDARRAY, array, val);
					key++;
				}
				_fs->SetIntructionParam(apos, 1, key);
				Lex();
			}
			break;
		case _SC('{'):{
			_fs->AddInstruction(_OP_NEWTABLE, _fs->PushTarget());
			Lex();ParseTableOrClass(_SC(','));
				 }
			break;
		case TK_FUNCTION: FunctionExp(_token);break;
		case TK_CLASS: Lex(); ClassExp();break;
		case _SC('-'): UnaryOP(_OP_NEG); break;
		case _SC('!'): UnaryOP(_OP_NOT); break;
		case _SC('~'): UnaryOP(_OP_BWNOT); break;
		case TK_TYPEOF : UnaryOP(_OP_TYPEOF); break;
		case TK_RESUME : UnaryOP(_OP_RESUME); break;
		case TK_CLONE : UnaryOP(_OP_CLONE); break;
		case TK_MINUSMINUS : 
		case TK_PLUSPLUS :PrefixIncDec(_token); break;
		case TK_DELETE : DeleteExpr(); break;
		case TK_DELEGATE : DelegateExpr(); break;
		case _SC('('): Lex(); CommaExpr(); Expect(_SC(')'));
			break;
		default: Error(_SC("expression expected"));
		}
		return -1;
	}
	void UnaryOP(SQOpcode op)
	{
		Lex(); PrefixedExpr();
		SQInteger src = _fs->PopTarget();
		_fs->AddInstruction(op, _fs->PushTarget(), src);
	}
	bool NeedGet()
	{
		switch(_token) {
		case _SC('='): case _SC('('): case TK_NEWSLOT: case TK_PLUSPLUS: case TK_MINUSMINUS:
		case TK_PLUSEQ: case TK_MINUSEQ: case TK_MULEQ: case TK_DIVEQ: case TK_MODEQ:
			return false;
		}
		return (!_exst._class_or_delete) || (_exst._class_or_delete && (_token == _SC('.') || _token == _SC('[')));
	}
	
	void FunctionCallArgs()
	{
		SQInteger nargs = 1;//this
		 while(_token != _SC(')')) {
			 Expression(true);
			 MoveIfCurrentTargetIsLocal();
			 nargs++; 
			 if(_token == _SC(',')){ 
				 Lex(); 
				 if(_token == ')') Error(_SC("expression expected, found ')'"));
			 }
		 }
		 Lex();
		 for(SQInteger i = 0; i < (nargs - 1); i++) _fs->PopTarget();
		 SQInteger stackbase = _fs->PopTarget();
		 SQInteger closure = _fs->PopTarget();
         _fs->AddInstruction(_OP_CALL, _fs->PushTarget(), closure, stackbase, nargs);
	}
	void ParseTableOrClass(SQInteger separator,SQInteger terminator = '}')
	{
		SQInteger tpos = _fs->GetCurrentPos(),nkeys = 0;
		
		while(_token != terminator) {
			bool hasattrs = false;
			bool isstatic = false;
			//check if is an attribute
			if(separator == ';') {
				if(_token == TK_ATTR_OPEN) {
					_fs->AddInstruction(_OP_NEWTABLE, _fs->PushTarget()); Lex();
					ParseTableOrClass(',',TK_ATTR_CLOSE);
					hasattrs = true;
				}
				if(_token == TK_STATIC) {
					isstatic = true;
					Lex();
				}
			}
			switch(_token) {
				case TK_FUNCTION:
				case TK_CONSTRUCTOR:{
					SQInteger tk = _token;
					Lex();
					SQObject<Q> id = tk == TK_FUNCTION ? Expect(TK_IDENTIFIER) : _fs->CreateString(_SC("constructor"));
					Expect(_SC('('));
					_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
					CreateFunction(id);
					_fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, 0);
								  }
								  break;
				case _SC('['):
					Lex(); CommaExpr(); Expect(_SC(']'));
					Expect(_SC('=')); Expression();
					break;
				default :
					_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(Expect(TK_IDENTIFIER)));
					Expect(_SC('=')); Expression();
			}

			if(_token == separator) Lex();//optional comma/semicolon
			nkeys++;
			SQInteger val = _fs->PopTarget();
			SQInteger key = _fs->PopTarget();
			SQInteger attrs = hasattrs ? _fs->PopTarget():-1;
			assert(hasattrs && attrs == key-1 || !hasattrs);
			unsigned char flags = (hasattrs?NEW_SLOT_ATTRIBUTES_FLAG:0)|(isstatic?NEW_SLOT_STATIC_FLAG:0);
			SQInteger table = _fs->TopTarget(); //<<BECAUSE OF THIS NO COMMON EMIT FUNC IS POSSIBLE
			_fs->AddInstruction(_OP_NEWSLOTA, flags, table, key, val);
			//_fs->PopTarget();
		}
		if(separator == _SC(',')) //hack recognizes a table from the separator
			_fs->SetIntructionParam(tpos, 1, nkeys);
		Lex();
	}
	void LocalDeclStatement()
	{
		SQObject<Q> varname;
		do {
			Lex(); varname = Expect(TK_IDENTIFIER);
			if(_token == _SC('=')) {
				Lex(); Expression();
				SQInteger src = _fs->PopTarget();
				SQInteger dest = _fs->PushTarget();
				if(dest != src) _fs->AddInstruction(_OP_MOVE, dest, src);
			}
			else{
				_fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(),1);
			}
			_fs->PopTarget();
			_fs->PushLocalVariable(varname);
		
		} while(_token == _SC(','));
	}
	void IfStatement()
	{
		SQInteger jmppos;
		bool haselse = false;
		Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
		_fs->AddInstruction(_OP_JZ, _fs->PopTarget());
		SQInteger jnepos = _fs->GetCurrentPos();
		SQInteger stacksize = _fs->GetStackSize();
		
		Statement();
		//
		if(_token != _SC('}') && _token != TK_ELSE) OptionalSemicolon();
		
		CleanStack(stacksize);
		SQInteger endifblock = _fs->GetCurrentPos();
		if(_token == TK_ELSE){
			haselse = true;
			stacksize = _fs->GetStackSize();
			_fs->AddInstruction(_OP_JMP);
			jmppos = _fs->GetCurrentPos();
			Lex();
			Statement(); OptionalSemicolon();
			CleanStack(stacksize);
			_fs->SetIntructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
		}
		_fs->SetIntructionParam(jnepos, 1, endifblock - jnepos + (haselse?1:0));
	}
	void WhileStatement()
	{
		SQInteger jzpos, jmppos;
		SQInteger stacksize = _fs->GetStackSize();
		jmppos = _fs->GetCurrentPos();
		Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
		
		BEGIN_BREAKBLE_BLOCK();
		_fs->AddInstruction(_OP_JZ, _fs->PopTarget());
		jzpos = _fs->GetCurrentPos();
		stacksize = _fs->GetStackSize();
		
		Statement();
		
		CleanStack(stacksize);
		_fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
		_fs->SetIntructionParam(jzpos, 1, _fs->GetCurrentPos() - jzpos);
		
		END_BREAKBLE_BLOCK(jmppos);
	}
	void DoWhileStatement()
	{
		Lex();
		SQInteger jzpos = _fs->GetCurrentPos();
		SQInteger stacksize = _fs->GetStackSize();
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		CleanStack(stacksize);
		Expect(TK_WHILE);
		SQInteger continuetrg = _fs->GetCurrentPos();
		Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
		_fs->AddInstruction(_OP_JNZ, _fs->PopTarget(), jzpos - _fs->GetCurrentPos() - 1);
		END_BREAKBLE_BLOCK(continuetrg);
	}
	void ForStatement()
	{
		Lex();
		SQInteger stacksize = _fs->GetStackSize();
		Expect(_SC('('));
		if(_token == TK_LOCAL) LocalDeclStatement();
		else if(_token != _SC(';')){
			CommaExpr();
			_fs->PopTarget();
		}
		Expect(_SC(';'));
		_fs->SnoozeOpt();
		SQInteger jmppos = _fs->GetCurrentPos();
		SQInteger jzpos = -1;
		if(_token != _SC(';')) { CommaExpr(); _fs->AddInstruction(_OP_JZ, _fs->PopTarget()); jzpos = _fs->GetCurrentPos(); }
		Expect(_SC(';'));
		_fs->SnoozeOpt();
		SQInteger expstart = _fs->GetCurrentPos() + 1;
		if(_token != _SC(')')) {
			CommaExpr();
			_fs->PopTarget();
		}
		Expect(_SC(')'));
		_fs->SnoozeOpt();
		SQInteger expend = _fs->GetCurrentPos();
		SQInteger expsize = (expend - expstart) + 1;
		SQInstructionVec exp;
		if(expsize > 0) {
			for(SQInteger i = 0; i < expsize; i++)
				exp.push_back(_fs->GetInstruction(expstart + i));
			_fs->PopInstructions(expsize);
		}
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		SQInteger continuetrg = _fs->GetCurrentPos();
		if(expsize > 0) {
			for(SQInteger i = 0; i < expsize; i++)
				_fs->AddInstruction(exp[i]);
		}
		_fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1, 0);
		if(jzpos>  0) _fs->SetIntructionParam(jzpos, 1, _fs->GetCurrentPos() - jzpos);
		CleanStack(stacksize);
		
		END_BREAKBLE_BLOCK(continuetrg);
	}
	void ForEachStatement()
	{
		SQObject<Q> idxname, valname;
		Lex(); Expect(_SC('(')); valname = Expect(TK_IDENTIFIER);
		if(_token == _SC(',')) {
			idxname = valname;
			Lex(); valname = Expect(TK_IDENTIFIER);
		}
		else{
			idxname = _fs->CreateString(_SC("@INDEX@"));
		}
		Expect(TK_IN);
		
		//save the stack size
		SQInteger stacksize = _fs->GetStackSize();
		//put the table in the stack(evaluate the table expression)
		Expression(); Expect(_SC(')'));
		SQInteger container = _fs->TopTarget();
		//push the index local var
		SQInteger indexpos = _fs->PushLocalVariable(idxname);
		_fs->AddInstruction(_OP_LOADNULLS, indexpos,1);
		//push the value local var
		SQInteger valuepos = _fs->PushLocalVariable(valname);
		_fs->AddInstruction(_OP_LOADNULLS, valuepos,1);
		//push reference index
		SQInteger itrpos = _fs->PushLocalVariable(_fs->CreateString(_SC("@ITERATOR@"))); //use invalid id to make it inaccessible
		_fs->AddInstruction(_OP_LOADNULLS, itrpos,1);
		SQInteger jmppos = _fs->GetCurrentPos();
		_fs->AddInstruction(_OP_FOREACH, container, 0, indexpos);
		SQInteger foreachpos = _fs->GetCurrentPos();
		_fs->AddInstruction(_OP_POSTFOREACH, container, 0, indexpos);
		//generate the statement code
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		_fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
		_fs->SetIntructionParam(foreachpos, 1, _fs->GetCurrentPos() - foreachpos);
		_fs->SetIntructionParam(foreachpos + 1, 1, _fs->GetCurrentPos() - foreachpos);
		//restore the local variable stack(remove index,val and ref idx)
		CleanStack(stacksize);
		END_BREAKBLE_BLOCK(foreachpos - 1);
	}
	void SwitchStatement()
	{
		Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
		Expect(_SC('{'));
		SQInteger expr = _fs->TopTarget();
		bool bfirst = true;
		SQInteger tonextcondjmp = -1;
		SQInteger skipcondjmp = -1;
		SQInteger __nbreaks__ = _fs->_unresolvedbreaks.size();
		_fs->_breaktargets.push_back(0);
		while(_token == TK_CASE) {
			//_fs->AddLineInfos(_lex._currentline, _lineinfo); think about this one
			if(!bfirst) {
				_fs->AddInstruction(_OP_JMP, 0, 0);
				skipcondjmp = _fs->GetCurrentPos();
				_fs->SetIntructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);
			}
			//condition
			Lex(); Expression(); Expect(_SC(':'));
			SQInteger trg = _fs->PopTarget();
			_fs->AddInstruction(_OP_EQ, trg, trg, expr);
			_fs->AddInstruction(_OP_JZ, trg, 0);
			//end condition
			if(skipcondjmp != -1) {
				_fs->SetIntructionParam(skipcondjmp, 1, (_fs->GetCurrentPos() - skipcondjmp));
			}
			tonextcondjmp = _fs->GetCurrentPos();
			SQInteger stacksize = _fs->GetStackSize();
			Statements();
			_fs->SetStackSize(stacksize);
			bfirst = false;
		}
		if(tonextcondjmp != -1)
			_fs->SetIntructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);
		if(_token == TK_DEFAULT) {
		//	_fs->AddLineInfos(_lex._currentline, _lineinfo);
			Lex(); Expect(_SC(':'));
			SQInteger stacksize = _fs->GetStackSize();
			Statements();
			_fs->SetStackSize(stacksize);
		}
		Expect(_SC('}'));
		_fs->PopTarget();
		__nbreaks__ = _fs->_unresolvedbreaks.size() - __nbreaks__;
		if(__nbreaks__ > 0)ResolveBreaks(_fs, __nbreaks__);
		_fs->_breaktargets.pop_back();
		
	}
	void FunctionStatement()
	{
		SQObject<Q> id;
		Lex(); id = Expect(TK_IDENTIFIER);
		_fs->PushTarget(0);
		_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
		if(_token == TK_DOUBLE_COLON) Emit2ArgsOP(_OP_GET);
		
		while(_token == TK_DOUBLE_COLON) {
			Lex();
			id = Expect(TK_IDENTIFIER);
			_fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
			if(_token == TK_DOUBLE_COLON) Emit2ArgsOP(_OP_GET);
		}
		Expect(_SC('('));
		CreateFunction(id);
		_fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, 0);
		EmitDerefOp(_OP_NEWSLOT);
		_fs->PopTarget();
	}
	void ClassStatement()
	{
		ExpState es;
		Lex(); PushExpState();
		_exst._class_or_delete = true;
		_exst._funcarg = false;
		PrefixedExpr();
		es = PopExpState();
		if(es._deref == DEREF_NO_DEREF) Error(_SC("invalid class name"));
		if(es._deref == DEREF_FIELD) {
			ClassExp();
			EmitDerefOp(_OP_NEWSLOT);
			_fs->PopTarget();
		}
		else Error(_SC("cannot create a class in a local with the syntax(class <local>)"));
	}
	SQObject<Q> ExpectScalar()
	{
		SQObject<Q> val;
		switch(_token) {
			case TK_INTEGER:
				val._type = OT_INTEGER;
				val._unVal.nInteger = _lex._nvalue;
				break;
			case TK_FLOAT:
				val._type = OT_FLOAT;
				val._unVal.fFloat = _lex._fvalue;
				break;
			case TK_STRING_LITERAL:
				val = _fs->CreateString(_lex._svalue,_lex._longstr.size()-1);
				break;
			case '-':
				Lex();
				switch(_token)
				{
				case TK_INTEGER:
					val._type = OT_INTEGER;
					val._unVal.nInteger = -_lex._nvalue;
				break;
				case TK_FLOAT:
					val._type = OT_FLOAT;
					val._unVal.fFloat = -_lex._fvalue;
				break;
				default:
					Error(_SC("scalar expected : integer,float"));
				}
				break;
			default:
			Error(_SC("scalar expected : integer,float or string"));
		}
		Lex();
		return val;
	}
	void EnumStatement()
	{
		
		Lex(); 
		SQObject<Q> id = Expect(TK_IDENTIFIER);
		Expect(_SC('{'));
		
		SQObject<Q> table = _fs->CreateTable();
		SQInteger nval = 0;
		while(_token != _SC('}')) {
			SQObject<Q> key = Expect(TK_IDENTIFIER);
			SQObject<Q> val;
			if(_token == _SC('=')) {
				Lex();
				val = ExpectScalar();
			}
			else {
				val._type = OT_INTEGER;
				val._unVal.nInteger = nval++;
			}
			_table(table)->NewSlot(SQObjectPtr(key),SQObjectPtr(val));
			if(_token == ',') Lex();
		}
		SQTable<Q> *enums = _table(_ss(_vm)->_consts);
		SQObjectPtr<Q> strongid = id;
		/*SQObjectPtr dummy;
		if(enums->Get(strongid,dummy)) {
			dummy.Null(); strongid.Null();
			Error(_SC("enumeration already exists"));
		}*/
		enums->NewSlot(SQObjectPtr<Q>(strongid),SQObjectPtr<Q>(table));
		strongid.Null();
		Lex();
		
	}
	void TryCatchStatement()
	{
		SQObject<Q> exid;
		Lex();
		_fs->AddInstruction(_OP_PUSHTRAP,0,0);
		_fs->_traps++;
		if(_fs->_breaktargets.size()) _fs->_breaktargets.top()++;
		if(_fs->_continuetargets.size()) _fs->_continuetargets.top()++;
		SQInteger trappos = _fs->GetCurrentPos();
		Statement();
		_fs->_traps--;
		_fs->AddInstruction(_OP_POPTRAP, 1, 0);
		if(_fs->_breaktargets.size()) _fs->_breaktargets.top()--;
		if(_fs->_continuetargets.size()) _fs->_continuetargets.top()--;
		_fs->AddInstruction(_OP_JMP, 0, 0);
		SQInteger jmppos = _fs->GetCurrentPos();
		_fs->SetIntructionParam(trappos, 1, (_fs->GetCurrentPos() - trappos));
		Expect(TK_CATCH); Expect(_SC('(')); exid = Expect(TK_IDENTIFIER); Expect(_SC(')'));
		SQInteger stacksize = _fs->GetStackSize();
		SQInteger ex_target = _fs->PushLocalVariable(exid);
		_fs->SetIntructionParam(trappos, 0, ex_target);
		Statement();
		_fs->SetIntructionParams(jmppos, 0, (_fs->GetCurrentPos() - jmppos), 0);
		CleanStack(stacksize);
	}
	void FunctionExp(SQInteger ftype)
	{
		Lex(); Expect(_SC('('));
		CreateFunction(_null_<Q>);
		_fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, ftype == TK_FUNCTION?0:1);
	}
	void ClassExp()
	{
		SQInteger base = -1;
		SQInteger attrs = -1;
		if(_token == TK_EXTENDS) {
			Lex(); Expression();
			base = _fs->TopTarget();
		}
		if(_token == TK_ATTR_OPEN) {
			Lex();
			_fs->AddInstruction(_OP_NEWTABLE, _fs->PushTarget());
			ParseTableOrClass(_SC(','),TK_ATTR_CLOSE);
			attrs = _fs->TopTarget();
		}
		Expect(_SC('{'));
		if(attrs != -1) _fs->PopTarget();
		if(base != -1) _fs->PopTarget();
		_fs->AddInstruction(_OP_CLASS, _fs->PushTarget(), base, attrs);
		ParseTableOrClass(_SC(';'));
	}
	void DelegateExpr()
	{
		Lex(); CommaExpr();
		Expect(_SC(':'));
		CommaExpr();
		SQInteger table = _fs->PopTarget(), delegate = _fs->PopTarget();
		_fs->AddInstruction(_OP_DELEGATE, _fs->PushTarget(), table, delegate);
	}
	void DeleteExpr()
	{
		ExpState es;
		Lex(); PushExpState();
		_exst._class_or_delete = true;
		_exst._funcarg = false;
		PrefixedExpr();
		es = PopExpState();
		if(es._deref == DEREF_NO_DEREF) Error(_SC("can't delete an expression"));
		if(es._deref == DEREF_FIELD) Emit2ArgsOP(_OP_DELETE);
		else Error(_SC("cannot delete a local"));
	}
	void PrefixIncDec(SQInteger token)
	{
		ExpState es;
		Lex(); PushExpState();
		_exst._class_or_delete = true;
		_exst._funcarg = false;
		PrefixedExpr();
		es = PopExpState();
		if(es._deref == DEREF_FIELD) Emit2ArgsOP(_OP_INC,token == TK_MINUSMINUS?-1:1);
		else {
			SQInteger src = _fs->PopTarget();
			_fs->AddInstruction(_OP_INCL, _fs->PushTarget(), src, 0, token == TK_MINUSMINUS?-1:1);
		}
	}
	void CreateFunction(SQObject<Q> &name)
	{
		
		SQFuncState<Q> *funcstate = _fs->PushChildState(_ss(_vm));
		funcstate->_name = name;
		SQObject<Q> paramname;
		funcstate->AddParameter(_fs->CreateString(_SC("this")));
		funcstate->_sourcename = _sourcename;
		SQInteger defparams = 0;
		while(_token!=_SC(')')) {
			if(_token == TK_VARPARAMS) {
				if(defparams > 0) Error(_SC("function with default parameters cannot have variable number of parameters"));
				funcstate->_varparams = true;
				Lex();
				if(_token != _SC(')')) Error(_SC("expected ')'"));
				break;
			}
			else {
				paramname = Expect(TK_IDENTIFIER);
				funcstate->AddParameter(paramname);
				if(_token == _SC('=')) { 
					Lex();
					Expression();
					funcstate->AddDefaultParam(_fs->TopTarget());
					defparams++;
				}
				else {
					if(defparams > 0) Error(_SC("expected '='"));
				}
				if(_token == _SC(',')) Lex();
				else if(_token != _SC(')')) Error(_SC("expected ')' or ','"));
			}
		}
		Expect(_SC(')'));
		for(SQInteger n = 0; n < defparams; n++) {
			_fs->PopTarget();
		}
		//outer values
		if(_token == _SC(':')) {
			Lex(); Expect(_SC('('));
			while(_token != _SC(')')) {
				paramname = Expect(TK_IDENTIFIER);
				//outers are treated as implicit local variables
				funcstate->AddOuterValue(paramname);
				if(_token == _SC(',')) Lex();
				else if(_token != _SC(')')) Error(_SC("expected ')' or ','"));
			}
			Lex();
		}
		
		SQFuncState<Q> *currchunk = _fs;
		_fs = funcstate;
		Statement();
		funcstate->AddLineInfos(_lex._prevtoken == _SC('\n')?_lex._lasttokenline:_lex._currentline, _lineinfo, true);
        funcstate->AddInstruction(_OP_RETURN, -1);
		funcstate->SetStackSize(0);
		//_fs->->_stacksize = _fs->_stacksize;
		SQFunctionProto<Q> *func = funcstate->BuildProto();
#ifdef _DEBUG_DUMP
		funcstate->Dump(func);
#endif
		_fs = currchunk;
		_fs->_functions.push_back(func);
		_fs->PopChildState();
	}
	void CleanStack(SQInteger stacksize)
	{
		if(_fs->GetStackSize() != stacksize)
			_fs->SetStackSize(stacksize);
	}
	void ResolveBreaks(SQFuncState<Q> *funcstate, SQInteger ntoresolve)
	{
		while(ntoresolve > 0) {
			SQInteger pos = funcstate->_unresolvedbreaks.back();
			funcstate->_unresolvedbreaks.pop_back();
			//set the jmp instruction
			funcstate->SetIntructionParams(pos, 0, funcstate->GetCurrentPos() - pos, 0);
			ntoresolve--;
		}
	}
	void ResolveContinues(SQFuncState<Q> *funcstate, SQInteger ntoresolve, SQInteger targetpos)
	{
		while(ntoresolve > 0) {
			SQInteger pos = funcstate->_unresolvedcontinues.back();
			funcstate->_unresolvedcontinues.pop_back();
			//set the jmp instruction
			funcstate->SetIntructionParams(pos, 0, targetpos - pos, 0);
			ntoresolve--;
		}
	}
private:
	SQInteger _token;
	SQFuncState<Q> *_fs;
	SQObjectPtr<Q> _sourcename;
	SQLexer<Q> _lex;
	bool _lineinfo;
	bool _raiseerror;
	SQInteger _debugline;
	SQInteger _debugop;
	ExpStateVec _expstates;
	SQChar *compilererror;
	jmp_buf _errorjmp;
	SQVM<Q> *_vm;
};

template <Squirk Q>
bool Compile(SQVM<Q> *vm,SQLEXREADFUNC rg, SQUserPointer up, const SQChar *sourcename, SQObjectPtr<Q> &out, bool raiseerror, bool lineinfo)
{
	SQCompiler<Q> p(vm, rg, up, sourcename, raiseerror, lineinfo);
	return p.Compile(out);
}

template bool Compile(SQVM<Squirk::Standard> *vm, SQLEXREADFUNC rg, SQUserPointer up, const SQChar *sourcename, SQObjectPtr<Squirk::Standard> &out, bool raiseerror, bool lineinfo);
template bool Compile(SQVM<Squirk::AlignObject> *vm, SQLEXREADFUNC rg, SQUserPointer up, const SQChar *sourcename, SQObjectPtr<Squirk::AlignObject> &out, bool raiseerror, bool lineinfo);
template bool Compile(SQVM<Squirk::StandardShared> *vm, SQLEXREADFUNC rg, SQUserPointer up, const SQChar *sourcename, SQObjectPtr<Squirk::StandardShared> &out, bool raiseerror, bool lineinfo);
template bool Compile(SQVM<Squirk::AlignObjectShared> *vm, SQLEXREADFUNC rg, SQUserPointer up, const SQChar *sourcename, SQObjectPtr<Squirk::AlignObjectShared> &out, bool raiseerror, bool lineinfo);
