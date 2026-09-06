#pragma once

#include "stdafx.h"

typedef struct {
	unsigned int id;
	unsigned int ram_base;
	unsigned int ram_range;
} Ketchup_DiskInfo;

typedef struct {
	std::string name;
	std::vector<Ketchup_DiskInfo> disks;
} Ketchup_VersionInfo;

typedef struct {
	unsigned int id;
	std::string name;
	std::vector<Ketchup_VersionInfo> versions;
} Ketchup_TitleInfo;

template <Squirk Q = Squirk::Standard>
class Ketchup
{
public:
	Ketchup() {}

	static bool Process(HSQUIRRELVM<Q> v);

	constexpr static unsigned int PSX_ImageBase = 0x10000;
	constexpr static unsigned int PSX_SectorSize = 0x800;
	constexpr static unsigned int PSX_SectorStride = 0x130;
	constexpr static unsigned int PSX_SectorRange = PSX_SectorSize + PSX_SectorStride;

	static unsigned int PSX_DiskRange(unsigned int size) {
		return (size / PSX_SectorSize) * PSX_SectorRange;
	}

private:
	static std::filesystem::path RootPath(Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::string base = "mods");

	static bool ApplyBlock(HSQUIRRELVM<Q> v,
		Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk,
		uint64_t offset, unsigned char *data, size_t size);
	static bool Apply(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::ifstream &data);

	static bool ApplyPPF3(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::ifstream &data);
	static int MetaPPF_FileId(std::ifstream &data, int version);

	static bool ProcessDisk(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk);
	static bool ProcessVersion(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version);
	static bool ProcessTitle(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title);
};
