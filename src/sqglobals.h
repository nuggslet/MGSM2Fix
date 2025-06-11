#pragma once

#include "sqrat.h"

template <Squirk Q>
class SQGlobals : public Sqrat::Object<Q>
{
public:
	SQGlobals() : Sqrat::Object<Q>(Sqrat::RootTable<Q>()) {}

	static int GetTitle()
	{
		return SQGlobals().GetSlot(_SC("s_current_title_dev_id")).Cast<int>();
	}

	static std::string GetExecutable()
	{
		auto rom = SQGlobals().GetSlot(_SC("s_rom_fileparse"));
		return
			rom.GetSlot("path").Cast<std::string>() +
			rom.GetSlot("name").Cast<std::string>() +
			rom.GetSlot("ext").Cast<std::string>();
	}

	static int GetDisk()
	{
		return SQGlobals().GetSlot(_SC("s_now_disk_no")).Cast<int>();
	}
};

template SQGlobals<Squirk::Standard>;
template SQGlobals<Squirk::AlignObject>;
template SQGlobals<Squirk::StandardShared>;
template SQGlobals<Squirk::AlignObjectShared>;
