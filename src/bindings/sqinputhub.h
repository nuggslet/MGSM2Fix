#pragma once

#include "sqinvoker.h"

template <Squirk Q>
class SQInputHub : public SQInvoker<Q>
{
public:
	SQInputHub() : SQInvoker<Q>(_SC("g_inputHub")) {}

	static void SetDirectionMerge(SQInteger mode) {
		return SQInputHub().Invoke<void>(__func__, mode);
	}

	static void SetDeadzone(SQFloat value) {
		return SQInputHub().Invoke<void>(__func__, value);
	}

};

template SQInputHub<Squirk::Standard>;
template SQInputHub<Squirk::AlignObject>;
template SQInputHub<Squirk::StandardShared>;
template SQInputHub<Squirk::AlignObjectShared>;
