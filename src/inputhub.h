#pragma once

#include "squirrel.h"

class InputHub
{
public:
	InputHub() = default;
	void SetVM(HSQUIRRELVM v);

	bool Get();
	bool Get(const SQChar *name, SQObjectType type);
	bool GetClosure(const SQChar *name);

	bool SetBool(const SQChar *name, SQBool value);
	bool SetInteger(const SQChar *name, SQInteger value);

private:
	HSQUIRRELVM m_vm;
};
