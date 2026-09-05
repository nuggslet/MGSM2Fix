/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCSTATE_H_
#define _SQFUNCSTATE_H_
///////////////////////////////////
#include "squtils.h"

template <Squirk Q>
struct SQFuncState
{
	SQFuncState(SQSharedState<Q> *ss,SQFuncState<Q> *parent,CompilerErrorFunc efunc,void *ed);
	~SQFuncState();
#ifdef _DEBUG_DUMP
	void Dump(SQFunctionProto<Q> *func);
#endif
	void Error(const SQChar *err);
	SQFuncState<Q> *PushChildState(SQSharedState<Q> *ss);
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
	SQInteger PushLocalVariable(const SQObject<Q> &name);
	void AddParameter(const SQObject<Q> &name);
	void AddOuterValue(const SQObject<Q> &name);
	SQInteger GetLocalVariable(const SQObject<Q> &name);
	SQInteger GetOuterVariable(const SQObject<Q> &name);
	SQInteger GenerateCode();
	SQInteger GetStackSize();
	SQInteger CalcStackFrameSize();
	void AddLineInfos(SQInteger line,bool lineop,bool force=false);
	SQFunctionProto<Q> *BuildProto();
	SQInteger AllocStackPos();
	SQInteger PushTarget(SQInteger n=-1);
	SQInteger PopTarget();
	SQInteger TopTarget();
	SQInteger GetUpTarget(SQInteger n);
	bool IsLocal(SQUnsignedInteger stkpos);
	SQObject<Q> CreateString(const SQChar *s,SQInteger len = -1);
	SQObject<Q> CreateTable();
	bool IsConstant(const SQObject<Q> &name,SQObject<Q> &e);
	SQInteger _returnexp;
	SQLocalVarInfoVec<Q> _vlocals;
	SQIntVec _targetstack;
	SQInteger _stacksize;
	bool _varparams;
	bool _bgenerator;
	SQIntVec _unresolvedbreaks;
	SQIntVec _unresolvedcontinues;
	SQObjectPtrVec<Q> _functions;
	SQObjectPtrVec<Q> _parameters;
	SQOuterVarVec<Q> _outervalues;
	SQInstructionVec _instructions;
	SQLocalVarInfoVec<Q> _localvarinfos;
	SQObjectPtr<Q> _literals;
	SQObjectPtr<Q> _strings;
	SQObjectPtr<Q> _name;
	SQObjectPtr<Q> _sourcename;
	SQInteger _nliterals;
	SQLineInfoVec _lineinfos;
	SQFuncState<Q> *_parent;
	SQIntVec _breaktargets;
	SQIntVec _continuetargets;
	SQIntVec _defaultparams;
	SQInteger _lastline;
	SQInteger _traps; //contains number of nested exception traps
	bool _optimization;
	SQSharedState<Q> *_sharedstate;
	sqvector<SQFuncState<Q>*> _childstates;
	SQInteger GetConstant(const SQObject<Q> &cons);
private:
	CompilerErrorFunc _errfunc;
	void *_errtarget;
};

template SQFuncState<Squirk::Standard>;
template SQFuncState<Squirk::AlignObject>;
template SQFuncState<Squirk::StandardShared>;
template SQFuncState<Squirk::AlignObjectShared>;

#endif //_SQFUNCSTATE_H_

