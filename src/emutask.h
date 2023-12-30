#pragma once

#include "m2object.h"

class EmuTask : public M2Object
{
public:
	EmuTask() : M2Object(_SC("g_emu_task")) {}

	bool SetInfoInteger(const SQChar *key, SQInteger value);
	bool GetInfoInteger(const SQChar *key, SQInteger *value);

	bool GetRamValue(SQInteger width, SQInteger offset, SQInteger *value);
	bool SetRamValue(SQInteger width, SQInteger offset, SQInteger value);

	bool EntryCdRomPatch(SQInteger offset, SQArray *data);
	bool ReleaseCdRomPatch();

	bool SetSmoothing(SQBool enable);
	bool SetScanline(SQBool enable);
	bool SetInputDirectionMerge(SQInteger mode);
	bool SetInputDeadzone(SQFloat value);
};
