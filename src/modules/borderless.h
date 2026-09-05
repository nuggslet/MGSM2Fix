#pragma once

#include "m2fixbase.h"

class Borderless : public M2FixBase
{
public:
	static auto & GetInstance()
	{
		static Borderless instance;
		return instance;
	}

	static void LoadInstance() {
		GetInstance().Load();
	}

	virtual void Load() override;
};
