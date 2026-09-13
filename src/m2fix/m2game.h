#pragma once

#include "stdafx.h"

#include "m2fixbase.h"
#include "m2machine.h"
#include "ketchup.h"

class M2Game : public M2FixBase
{
public:
	M2Game() {}

	static auto & GetInstance()
	{
		static M2Game instance;
		return instance;
	}

	virtual std::vector<std::reference_wrapper<M2Machine>> MachineInstances()
	{
		return { M2Machine::GetInstance() };
	}

	virtual std::vector<Ketchup_TitleInfo> *SQKetchupHook() { return nullptr; }

	virtual void SQOnMemoryDefine() {}
	virtual void SQOnUpdateGadgets() {}
	virtual void SQOnInitSystemFirst() {}
	virtual void SQOnInitSystemLast() {}
	// A Squirrel-side setRamValue is about to run. Return true to have the
	// caller's Squirrel call stack logged (the game decides which writes matter).
	// The game may change `value`; the write then carries the new value.
	virtual bool SQOnRamWrite(unsigned width, unsigned offset, unsigned &value) { return false; }
	// Likewise for a Squirrel-side getRamValue.
	virtual bool SQOnRamRead(unsigned width, unsigned offset) { return false; }

	virtual std::any EPIModuleHook() { return {}; }

	virtual void EPIOnLoadImage(void *image, unsigned int size) {}
	virtual bool EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args) { return true; }
	virtual bool EPIOnCommandCPU(std::any cpu, int cmd, unsigned int **args) { return true; }

	virtual void GWRenderGeometry(int & gw_width, int & gw_height, int & fb_width, int & fb_height, int & img_width, int & img_height) {}
	virtual bool GWBlank() { return false; }
};

#include "mgs1.h"
#include "mgs1in4.h"
