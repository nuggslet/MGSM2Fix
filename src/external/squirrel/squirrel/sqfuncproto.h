/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCTION_H_
#define _SQFUNCTION_H_

#include "sqopcodes.h"

enum SQOuterType {
	otLOCAL = 0,
	otSYMBOL = 1,
	otOUTER = 2
};

template <Squirk T>
struct SQOuterVar
{
	
	SQOuterVar(){}
	SQOuterVar(const SQObjectPtr<T> &name,const SQObjectPtr<T> &src,SQOuterType t)
	{
		_name = name;
		_src=src;
		_type=t;
	}
	SQOuterVar(const SQOuterVar<T> &ov)
	{
		_type=ov._type;
		_src=ov._src;
		_name=ov._name;
	}
	SQOuterType _type;
	SQObjectPtr<T> _name;
	SQObjectPtr<T> _src;
};

template <Squirk T>
struct SQLocalVarInfo
{
	SQLocalVarInfo():_start_op(0),_end_op(0){}
	SQLocalVarInfo(const SQLocalVarInfo<T> &lvi)
	{
		_name=lvi._name;
		_start_op=lvi._start_op;
		_end_op=lvi._end_op;
		_pos=lvi._pos;
	}
	SQObjectPtr<T> _name;
	SQUnsignedInteger _start_op;
	SQUnsignedInteger _end_op;
	SQUnsignedInteger _pos;
};

struct SQLineInfo { SQInteger _line;SQInteger _op; };

template <Squirk T>
using SQOuterVarVec = sqvector<SQOuterVar<T>>;
template <Squirk T>
using SQLocalVarInfoVec = sqvector<SQLocalVarInfo<T>>;
typedef sqvector<SQLineInfo> SQLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(SQFunctionProto<T>) \
		+((ni-1)*sizeof(SQInstruction))+(nl*sizeof(SQObjectPtr<T>)) \
		+(nparams*sizeof(SQObjectPtr<T>))+(nfuncs*sizeof(SQObjectPtr<T>)) \
		+(nouters*sizeof(SQOuterVar<T>))+(nlineinf*sizeof(SQLineInfo)) \
		+(localinf*sizeof(SQLocalVarInfo<T>))+(defparams*sizeof(SQInteger)))

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
template <Squirk T>
struct SQFunctionProto : public SQRefCounted<T>
{
private:
	SQFunctionProto(){
	_stacksize=0;
	_bgenerator=false;}
public:
	static SQFunctionProto<T> *Create(SQInteger ninstructions,
		SQInteger nliterals,SQInteger nparameters,
		SQInteger nfunctions,SQInteger noutervalues,
		SQInteger nlineinfos,SQInteger nlocalvarinfos,SQInteger ndefaultparams)
	{
		SQFunctionProto<T> *f;
		//I compact the whole class and members in a single memory allocation
		f = (SQFunctionProto<T> *)sq_vm_malloc(_FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));
		new (f) SQFunctionProto<T>;
		f->_ninstructions = ninstructions;
		f->_literals = (SQObjectPtr<T>*)&f->_instructions[ninstructions];
		f->_nliterals = nliterals;
		f->_parameters = (SQObjectPtr<T>*)&f->_literals[nliterals];
		f->_nparameters = nparameters;
		f->_functions = (SQObjectPtr<T>*)&f->_parameters[nparameters];
		f->_nfunctions = nfunctions;
		f->_outervalues = (SQOuterVar<T>*)&f->_functions[nfunctions];
		f->_noutervalues = noutervalues;
		f->_lineinfos = (SQLineInfo *)&f->_outervalues[noutervalues];
		f->_nlineinfos = nlineinfos;
		f->_localvarinfos = (SQLocalVarInfo<T> *)&f->_lineinfos[nlineinfos];
		f->_nlocalvarinfos = nlocalvarinfos;
		f->_defaultparams = (SQInteger *)&f->_localvarinfos[nlocalvarinfos];
		f->_ndefaultparams = ndefaultparams;

		_CONSTRUCT_VECTOR(SQObjectPtr<T>,f->_nliterals,f->_literals);
		_CONSTRUCT_VECTOR(SQObjectPtr<T>,f->_nparameters,f->_parameters);
		_CONSTRUCT_VECTOR(SQObjectPtr<T>,f->_nfunctions,f->_functions);
		_CONSTRUCT_VECTOR(SQOuterVar<T>,f->_noutervalues,f->_outervalues);
		//_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
		_CONSTRUCT_VECTOR(SQLocalVarInfo<T>,f->_nlocalvarinfos,f->_localvarinfos);
		return f;
	}
	void Release(){ 
		_DESTRUCT_VECTOR(SQObjectPtr<T>,_nliterals,_literals);
		_DESTRUCT_VECTOR(SQObjectPtr<T>,_nparameters,_parameters);
		_DESTRUCT_VECTOR(SQObjectPtr<T>,_nfunctions,_functions);
		_DESTRUCT_VECTOR(SQOuterVar<T>,_noutervalues,_outervalues);
		//_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
		_DESTRUCT_VECTOR(SQLocalVarInfo<T>,_nlocalvarinfos,_localvarinfos);
		SQInteger size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
		this->~SQFunctionProto();
		sq_vm_free(this,size);
	}
	const SQChar* GetLocal(SQVM<T> *v,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop);
	SQInteger GetLine(SQInstruction *curr);
	bool Save(SQVM<T> *v,SQUserPointer up,SQWRITEFUNC write);
	static bool Load(SQVM<T> *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr<T> &ret);

	SQObjectPtr<T> _sourcename;
	SQObjectPtr<T> _name;
    SQInteger _stacksize;
	bool _bgenerator;
	bool _varparams;

	SQInteger _nlocalvarinfos;
	SQLocalVarInfo<T> *_localvarinfos;

	SQInteger _nlineinfos;
	SQLineInfo *_lineinfos;

	SQInteger _nliterals;
	SQObjectPtr<T> *_literals;

	SQInteger _nparameters;
	SQObjectPtr<T> *_parameters;
	
	SQInteger _nfunctions;
	SQObjectPtr<T> *_functions;

	SQInteger _noutervalues;
	SQOuterVar<T> *_outervalues;

	SQInteger _ndefaultparams;
	SQInteger *_defaultparams;
	
	SQInteger _ninstructions;
	SQInstruction _instructions[1];
};

template SQFunctionProto<Squirk::Standard>;
template SQFunctionProto<Squirk::AlignObject>;

#endif //_SQFUNCTION_H_
