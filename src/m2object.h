#pragma once

#include "squirrel.h"

class M2Object
{
public:
	M2Object(const SQChar *name);
	void SetVM(HSQUIRRELVM v);

	bool Get();
	bool Get(const SQChar *name, SQObjectType type);
	bool GetClosure(const SQChar *name);

	bool SetBool(const SQChar *name, SQBool value);
	bool SetInteger(const SQChar *name, SQInteger value);
	bool SetFloat(const SQChar *name, SQFloat value);

	bool GetBool(const SQChar *name, SQBool *value);
	bool GetInteger(const SQChar *name, SQInteger *value);
	bool GetFloat(const SQChar *name, SQFloat *value);

private:
	const SQChar *m_name;
	HSQUIRRELVM m_vm;
};
