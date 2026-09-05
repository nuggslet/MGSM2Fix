#pragma once

#include "sqrat.h"

template <Squirk Q>
class SQSystemProf : public Sqrat::Object<Q>
{
public:
	SQSystemProf() : Sqrat::Object<Q>(Sqrat::RootTable<Q>()[_SC("s_root_systemprof")][_SC("root")])
	{}

	static std::string GetName()
	{
		return SQSystemProf()["dev_name"].Cast<std::string>();
	}
};

template SQSystemProf<Squirk::Standard>;
template SQSystemProf<Squirk::AlignObject>;
template SQSystemProf<Squirk::StandardShared>;
template SQSystemProf<Squirk::AlignObjectShared>;
