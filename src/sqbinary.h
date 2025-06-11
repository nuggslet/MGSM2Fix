#pragma once

#include "sqinvoker.h"

template <Squirk Q>
class SQBinary : public SQInvoker<Q>
{
public:
	SQBinary(HSQOBJECT<Q> binary) : SQInvoker<Q>(binary) {}

	SQInteger At(SQInteger offset) {
		return this->Invoke<SQInteger>(__func__, offset);
	}

	SQInteger Size() {
		return this->Invoke<SQInteger>(__func__);
	}

};

template SQBinary<Squirk::Standard>;
template SQBinary<Squirk::AlignObject>;
template SQBinary<Squirk::StandardShared>;
template SQBinary<Squirk::AlignObjectShared>;
