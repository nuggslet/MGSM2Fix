/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCSTATE_H_
#define _SQFUNCSTATE_H_
///////////////////////////////////
#include "squtils.h"

template <Squirk T>
struct SQFuncState
{
	SQFuncState(SQSharedState<T> *ss,SQFuncState<T> *parent,CompilerErrorFunc efunc,void *ed);
	~SQFuncState();
#ifdef _DEBUG_DUMP
	void Dump(SQFunctionProto<T> *func);
#endif
	void Error(const SQChar *err);
	SQFuncState<T> *PushChildState(SQSharedState<T> *ss);
	void PopChildState();
	void AddInstruction(SQOpcode _op,SQInteger arg0=0,SQInteger arg1=0,SQInteger arg2=0,SQInteger arg3=0){SQInstruction i(_op,arg0,arg1,arg2,arg3);AddInstruction(i);}
	void AddInstruction(SQInstruction &i);
	void SetIntructionParams(SQInteger pos,SQInteger arg0,SQInteger arg1,SQInteger arg2=0,SQInteger arg3=0);
	void SetIntructionParam(SQInteger pos,SQInteger arg,SQInteger val);
	SQInstruction &GetInstruction(SQInteger pos){return _instructions[pos];}
	void PopInstructions(SQInteger size){for(SQInteger i=0;i<size;i++)_instructions.pop_back();}
	void SetStackSize(SQInteger n);
	void SnoozeOpt(){_optimization=false;}
	void AddDefaultParam(SQInteger trg) { _defaultparams.push_back(trg); }
	SQInteger GetDefaultParamCount() { return _defaultparams.size(); }
	SQInteger GetCurrentPos(){return _instructions.size()-1;}
	SQInteger GetNumericConstant(const SQInteger cons);
	SQInteger GetNumericConstant(const SQFloat cons);
	SQInteger PushLocalVariable(const SQObject<T> &name);
	void AddParameter(const SQObject<T> &name);
	void AddOuterValue(const SQObject<T> &name);
	SQInteger GetLocalVariable(const SQObject<T> &name);
	SQInteger GetOuterVariable(const SQObject<T> &name);
	SQInteger GenerateCode();
	SQInteger GetStackSize();
	SQInteger CalcStackFrameSize();
	void AddLineInfos(SQInteger line,bool lineop,bool force=false);
	SQFunctionProto<T> *BuildProto();
	SQInteger AllocStackPos();
	SQInteger PushTarget(SQInteger n=-1);
	SQInteger PopTarget();
	SQInteger TopTarget();
	SQInteger GetUpTarget(SQInteger n);
	bool IsLocal(SQUnsignedInteger stkpos);
	SQObject<T> CreateString(const SQChar *s,SQInteger len = -1);
	SQObject<T> CreateTable();
	bool IsConstant(const SQObject<T> &name,SQObject<T> &e);
	SQInteger _returnexp;
	SQLocalVarInfoVec<T> _vlocals;
	SQIntVec _targetstack;
	SQInteger _stacksize;
	bool _varparams;
	bool _bgenerator;
	SQIntVec _unresolvedbreaks;
	SQIntVec _unresolvedcontinues;
	SQObjectPtrVec<T> _functions;
	SQObjectPtrVec<T> _parameters;
	SQOuterVarVec<T> _outervalues;
	SQInstructionVec _instructions;
	SQLocalVarInfoVec<T> _localvarinfos;
	SQObjectPtr<T> _literals;
	SQObjectPtr<T> _strings;
	SQObjectPtr<T> _name;
	SQObjectPtr<T> _sourcename;
	SQInteger _nliterals;
	SQLineInfoVec _lineinfos;
	SQFuncState<T> *_parent;
	SQIntVec _breaktargets;
	SQIntVec _continuetargets;
	SQIntVec _defaultparams;
	SQInteger _lastline;
	SQInteger _traps; //contains number of nested exception traps
	bool _optimization;
	SQSharedState<T> *_sharedstate;
	sqvector<SQFuncState<T>*> _childstates;
	SQInteger GetConstant(const SQObject<T> &cons);
private:
	CompilerErrorFunc _errfunc;
	void *_errtarget;
};

template SQFuncState<Squirk::Standard>;
template SQFuncState<Squirk::AlignObject>;

#endif //_SQFUNCSTATE_H_

