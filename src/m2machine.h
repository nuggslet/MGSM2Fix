#pragma once

#include "m2fixbase.h"
#include "stdafx.h"

class M2Machine: public M2FixBase
{
public:
	M2Machine() {}

	static auto & GetInstance()
	{
		static M2Machine instance;
		return instance;
	}

	virtual void Load() {}
	virtual void BindModules() {}
	virtual void UpdateScreenGeometry(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int) {}
};
