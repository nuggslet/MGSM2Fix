#pragma once

#include "m2object.h"

class EmuTask : public M2Object
{
public:
	EmuTask() : M2Object(_SC("g_emu_task")) {}

	bool SetInfoInteger(const SQChar *key, SQInteger value);
	bool GetInfoInteger(const SQChar *key, SQInteger *value);

	bool SetSmoothing(SQBool enable);
	bool SetScanline(SQBool enable);
	bool SetInputDirectionMerge(SQInteger mode);
	bool SetInputDeadzone(SQFloat value);
};
