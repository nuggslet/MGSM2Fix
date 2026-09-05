#pragma once

#include "m2fixbase.h"

class Analog : public M2FixBase
{
public:
	static auto & GetInstance()
	{
		static Analog instance;
		return instance;
	}

	static void LoadInstance() {
		GetInstance().Load();
	}

	virtual void Load() override;
};
