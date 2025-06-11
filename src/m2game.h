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

	virtual std::pair<std::any, const char *> EPIModuleHook() { return {}; }

	virtual void EPIOnLoadImage(void *image, size_t size) {}
	virtual bool EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args) { return true; }
	virtual bool EPIOnCommandCPU(std::any cpu, int cmd, unsigned int **args) { return true; }

	virtual void GWRenderGeometry(int & gw_width, int & gw_height, int & fb_width, int & fb_height, int & img_width, int & img_height) {}
	virtual bool GWBlank() { return false; }
};

#include "mgs1.h"
