#pragma once

#include "m2object.h"

template <Squirk T>
class Input : public M2Object<T>
{
public:
	Input() : M2Object<T>(_SC("g_input")) {}

	bool GetAnalogStickX(SQFloat *value);
	bool GetAnalogStickY(SQFloat *value);
	bool GetRightAnalogStickX(SQFloat *value);
	bool GetRightAnalogStickY(SQFloat *value);
};

template Input<Squirk::Standard>;
template Input<Squirk::AlignObject>;
