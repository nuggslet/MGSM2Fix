#pragma once

#include "squirrel.h"

class Input
{
public:
	Input() = default;
	void SetVM(HSQUIRRELVM v);

	bool Get();
	bool Get(const SQChar *name, SQObjectType type);
	bool GetClosure(const SQChar *name);

	bool SetBool(const SQChar *name, SQBool value);
	bool SetInteger(const SQChar *name, SQInteger value);

	bool GetFloat(const SQChar *name, SQFloat* value);

private:
	HSQUIRRELVM m_vm;
};
