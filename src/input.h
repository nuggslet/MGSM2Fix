#pragma once

#include "m2object.h"

class Input : public M2Object
{
public:
	Input() : M2Object(_SC("g_input")) {}

	bool GetAnalogStickX(SQFloat *value);
	bool GetAnalogStickY(SQFloat *value);
	bool GetRightAnalogStickX(SQFloat *value);
	bool GetRightAnalogStickY(SQFloat *value);
};
