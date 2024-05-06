#pragma once

#include "sqpcheader.h"
#include "sqvm.h"
#include "sqarray.h"

template <Squirk T>
class M2Binary
{
public:
	M2Binary(HSQUIRRELVM<T> v, HSQOBJECT<T> obj);

	bool GetClosure(const SQChar *name);

	bool At(SQInteger offset, SQInteger *value);
	bool Size(SQInteger *size);

protected:
	HSQOBJECT<T> m_obj;
	HSQUIRRELVM<T> m_vm;
};
