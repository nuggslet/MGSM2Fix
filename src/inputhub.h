#pragma once

#include "m2object.h"

template <Squirk T>
class InputHub : public M2Object<T>
{
public:
	InputHub() : M2Object<T>(_SC("g_inputHub")) {}

	bool SetDirectionMerge(SQInteger mode);
	bool SetDeadzone(SQFloat value);
};

template InputHub<Squirk::Standard>;
template InputHub<Squirk::AlignObject>;
