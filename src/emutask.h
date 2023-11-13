#pragma once

#include "squirrel.h"

class EmuTask
{
public:
	EmuTask() = default;
	void SetVM(HSQUIRRELVM v);

	bool Get();
	bool Get(const SQChar *name, SQObjectType type);
	bool GetClosure(const SQChar *name);

	bool SetBool(const SQChar *name, SQBool value);
	bool SetInteger(const SQChar *name, SQInteger value);

	bool SetSmoothing(SQBool enable);
	bool SetScanline(SQBool enable);

private:
	HSQUIRRELVM m_vm;
};
