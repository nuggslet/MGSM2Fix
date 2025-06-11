#pragma once

#include "m2fixbase.h"

class EPI : public M2FixBase
{
public:
	static auto & GetInstance()
	{
		static EPI instance;
		return instance;
	}

	static void LoadInstance() {
		GetInstance().Load();
	}

	virtual void Load() override;

private:
	static void Print(const char *fmt, ...);
};
