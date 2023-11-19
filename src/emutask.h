#pragma once

#include "m2object.h"

class EmuTask : public M2Object
{
public:
	EmuTask() : M2Object("g_emu_task") {}

	bool SetSmoothing(SQBool enable);
	bool SetScanline(SQBool enable);
	bool SetInputDirectionMerge(SQInteger mode);
	bool SetInputDeadzone(SQFloat value);
};
