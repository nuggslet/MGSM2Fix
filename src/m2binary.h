#pragma once

#include "sqpcheader.h"
#include "sqvm.h"
#include "sqarray.h"

class M2Binary
{
public:
	M2Binary(HSQUIRRELVM v, HSQOBJECT obj);

	bool GetClosure(const SQChar *name);

	bool At(SQInteger offset, SQInteger *value);
	bool Size(SQInteger *size);

protected:
	HSQOBJECT m_obj;
	HSQUIRRELVM m_vm;
};
