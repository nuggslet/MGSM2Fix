#pragma once

#include "sqrat.h"

#include "sqglobals.h"
#include "sqsystemdata.h"

template <Squirk Q>
class SQTitleProf : public Sqrat::Object<Q>
{
public:
	SQTitleProf() : Sqrat::Object<Q>(Sqrat::RootTable<Q>()[_SC("s_current_title_prof")])
	{}

	static std::string GetName()
	{
		return SQTitleProf()["title"]["dev_name"].Cast<std::string>();
	}

	static unsigned GetVersions()
	{
		return SQTitleProf()["m2epi"]["version"].GetSize();
	}

	static std::string GetExecutable(std::string version = SQSystemData<Q>::SettingETC::GetVersion())
	{
		return SQTitleProf()["m2epi"]["version"][version.c_str()]["rom"].Cast<std::string>();
	}

	static unsigned GetDisks(std::string version = SQSystemData<Q>::SettingETC::GetVersion())
	{
		auto obj = SQTitleProf()["m2epi"]["version"][version.c_str()]["disk"];
		if (obj.IsNull()) return 0;
		switch (obj.GetType()) {
			case OT_STRING: return 1;
			default: return obj.GetSize();
		}
	}
	
	static std::string GetDisk(
		unsigned id = SQGlobals<Q>::GetDisk(),
		std::string version = SQSystemData<Q>::SettingETC::GetVersion()
	) {
		auto obj = SQTitleProf()["m2epi"]["version"][version.c_str()]["disk"];
		auto disks = GetDisks(version);
		if (id >= disks) return {};
		if (disks > 1) obj = obj[id];
		return obj.Cast<std::string>();
	}

	static unsigned GetMemoryDefine(std::string name,
		unsigned disk = SQGlobals<Q>::GetDisk(),
		std::string version = SQSystemData<Q>::SettingETC::GetVersion()
	) {
		auto obj = SQTitleProf()["memory_define"];
		if (GetVersions() > 1) obj = obj[version.c_str()];
		obj = obj[name.c_str()];
		if (obj.GetType() != OT_INTEGER) obj = obj[disk];
		return obj.Cast<unsigned>();
	}
};

template SQTitleProf<Squirk::Standard>;
template SQTitleProf<Squirk::AlignObject>;
template SQTitleProf<Squirk::StandardShared>;
template SQTitleProf<Squirk::AlignObjectShared>;
