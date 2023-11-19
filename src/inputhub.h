#pragma once

#include "m2object.h"

class InputHub : public M2Object
{
public:
	InputHub() : M2Object("g_inputHub") {}

	bool SetDirectionMerge(SQInteger mode);
	bool SetDeadzone(SQFloat value);
};
