/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCTION_H_
#define _SQFUNCTION_H_

#include "sqopcodes.h"

enum SQOuterType {
	otLOCAL = 0,
	otSYMBOL = 1,
	otOUTER = 2
};

template <Squirk Q>
struct SQOuterVar
{
	
	SQOuterVar(){}
	SQOuterVar(const SQObjectPtr<Q> &name,const SQObjectPtr<Q> &src,SQOuterType t)
	{
		_name = name;
		_src=src;
		_type=t;
	}
	SQOuterVar(const SQOuterVar<Q> &ov)
	{
		_type=ov._type;
		_src=ov._src;
		_name=ov._name;
	}
	SQOuterType _type;
	SQObjectPtr<Q> _name;
	SQObjectPtr<Q> _src;
};

template <Squirk Q>
struct SQLocalVarInfo
{
	SQLocalVarInfo():_start_op(0),_end_op(0){}
	SQLocalVarInfo(const SQLocalVarInfo<Q> &lvi)
	{
		_name=lvi._name;
		_start_op=lvi._start_op;
		_end_op=lvi._end_op;
		_pos=lvi._pos;
	}
	SQObjectPtr<Q> _name;
	SQUnsignedInteger _start_op;
	SQUnsignedInteger _end_op;
	SQUnsignedInteger _pos;
};

struct SQLineInfo { SQInteger _line;SQInteger _op; };

template <Squirk Q>
using SQOuterVarVec = sqvector<SQOuterVar<Q>>;
template <Squirk Q>
using SQLocalVarInfoVec = sqvector<SQLocalVarInfo<Q>>;
typedef sqvector<SQLineInfo> SQLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(SQFunctionProto<Q>) \
		+((ni-1)*sizeof(SQInstruction))+(nl*sizeof(SQObjectPtr<Q>)) \
		+(nparams*sizeof(SQObjectPtr<Q>))+(nfuncs*sizeof(SQObjectPtr<Q>)) \
		+(nouters*sizeof(SQOuterVar<Q>))+(nlineinf*sizeof(SQLineInfo)) \
		+(localinf*sizeof(SQLocalVarInfo<Q>))+(defparams*sizeof(SQInteger)))

#define _CONSTRUCT_VECTOR(type,size,ptr) { \
	for(SQInteger n = 0; n < size; n++) { \
			new (&ptr[n]) type(); \
		} \
}

#define _DESTRUCT_VECTOR(type,size,ptr) { \
	for(SQInteger nl = 0; nl < size; nl++) { \
			ptr[nl].~type(); \
	} \
}
template <Squirk Q>
struct SQFunctionProto : public SQRefCounted<Q>
{
private:
	SQFunctionProto(){
	_stacksize=0;
	_bgenerator=false;}
public:
	static SQFunctionProto<Q> *Create(SQInteger ninstructions,
		SQInteger nliterals,SQInteger nparameters,
		SQInteger nfunctions,SQInteger noutervalues,
		SQInteger nlineinfos,SQInteger nlocalvarinfos,SQInteger ndefaultparams)
	{
		SQFunctionProto<Q> *f;
		//I compact the whole class and members in a single memory allocation
		f = (SQFunctionProto<Q> *)sq_vm_malloc(_FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));
		new (f) SQFunctionProto<Q>;
		f->_ninstructions = ninstructions;
		f->_literals = (SQObjectPtr<Q>*)&f->_instructions[ninstructions];
		f->_nliterals = nliterals;
		f->_parameters = (SQObjectPtr<Q>*)&f->_literals[nliterals];
		f->_nparameters = nparameters;
		f->_functions = (SQObjectPtr<Q>*)&f->_parameters[nparameters];
		f->_nfunctions = nfunctions;
		f->_outervalues = (SQOuterVar<Q>*)&f->_functions[nfunctions];
		f->_noutervalues = noutervalues;
		f->_lineinfos = (SQLineInfo *)&f->_outervalues[noutervalues];
		f->_nlineinfos = nlineinfos;
		f->_localvarinfos = (SQLocalVarInfo<Q> *)&f->_lineinfos[nlineinfos];
		f->_nlocalvarinfos = nlocalvarinfos;
		f->_defaultparams = (SQInteger *)&f->_localvarinfos[nlocalvarinfos];
		f->_ndefaultparams = ndefaultparams;

		_CONSTRUCT_VECTOR(SQObjectPtr<Q>,f->_nliterals,f->_literals);
		_CONSTRUCT_VECTOR(SQObjectPtr<Q>,f->_nparameters,f->_parameters);
		_CONSTRUCT_VECTOR(SQObjectPtr<Q>,f->_nfunctions,f->_functions);
		_CONSTRUCT_VECTOR(SQOuterVar<Q>,f->_noutervalues,f->_outervalues);
		//_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
		_CONSTRUCT_VECTOR(SQLocalVarInfo<Q>,f->_nlocalvarinfos,f->_localvarinfos);
		return f;
	}
	void Release(){ 
		_DESTRUCT_VECTOR(SQObjectPtr<Q>,_nliterals,_literals);
		_DESTRUCT_VECTOR(SQObjectPtr<Q>,_nparameters,_parameters);
		_DESTRUCT_VECTOR(SQObjectPtr<Q>,_nfunctions,_functions);
		_DESTRUCT_VECTOR(SQOuterVar<Q>,_noutervalues,_outervalues);
		//_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
		_DESTRUCT_VECTOR(SQLocalVarInfo<Q>,_nlocalvarinfos,_localvarinfos);
		SQInteger size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
		this->~SQFunctionProto();
		sq_vm_free(this,size);
	}
	const SQChar* GetLocal(SQVM<Q> *v,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop);
	SQInteger GetLine(SQInstruction *curr);
	bool Save(SQVM<Q> *v,SQUserPointer up,SQWRITEFUNC write);
	static bool Load(SQVM<Q> *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr<Q> &ret);

	SQObjectPtr<Q> _sourcename;
	SQObjectPtr<Q> _name;
    SQInteger _stacksize;
	bool _bgenerator;
	bool _varparams;

	SQInteger _nlocalvarinfos;
	SQLocalVarInfo<Q> *_localvarinfos;

	SQInteger _nlineinfos;
	SQLineInfo *_lineinfos;

	SQInteger _nliterals;
	SQObjectPtr<Q> *_literals;

	SQInteger _nparameters;
	SQObjectPtr<Q> *_parameters;
	
	SQInteger _nfunctions;
	SQObjectPtr<Q> *_functions;

	SQInteger _noutervalues;
	SQOuterVar<Q> *_outervalues;

	SQInteger _ndefaultparams;
	SQInteger *_defaultparams;
	
	SQInteger _ninstructions;
	SQInstruction _instructions[1];
};

template SQFunctionProto<Squirk::Standard>;
template SQFunctionProto<Squirk::AlignObject>;
template SQFunctionProto<Squirk::StandardShared>;
template SQFunctionProto<Squirk::AlignObjectShared>;

#endif //_SQFUNCTION_H_
