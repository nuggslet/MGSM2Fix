#pragma once

#include "sqinvoker.h"

template <Squirk Q>
class SQInput : public SQInvoker<Q>
{
public:
	SQInput() : SQInvoker<Q>(_SC("g_input")) {}

	static SQFloat GetAnalogStickX() {
		return SQInput().Invoke<SQFloat>(__func__);
	}

	static SQFloat GetAnalogStickY() {
		return SQInput().Invoke<SQFloat>(__func__);
	}

	static SQFloat GetRightAnalogStickX() {
		return SQInput().Invoke<SQFloat>(__func__);
	}

	static SQFloat GetRightAnalogStickY() {
		return SQInput().Invoke<SQFloat>(__func__);
	}

};

template SQInput<Squirk::Standard>;
template SQInput<Squirk::AlignObject>;
template SQInput<Squirk::StandardShared>;
template SQInput<Squirk::AlignObjectShared>;
