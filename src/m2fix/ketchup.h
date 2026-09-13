#pragma once

#include "stdafx.h"
#include <map>
#include "ram_patch.h"

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



// One write as it was applied, kept only until the folder has been processed
// so collisions between two patches can be reported. See ReportOverlaps().
typedef struct {
	uint64_t offset;
	unsigned int source;
	std::vector<unsigned char> data;
} Ketchup_Write;

template <Squirk Q = Squirk::Standard>
class Ketchup
{
public:
	Ketchup() {}

	static bool Process(HSQUIRRELVM<Q> v);

	// Applies, and keeps applying, the RAM half of the patch set. Writing it at
	// the time the CD-ROM patch is entered does not survive; see ApplyBlock().
	// Safe to call every frame - it only verifies periodically.
	static void Update();

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

	// Deferred RAM writes, coalesced into contiguous runs, rebuilt on each
	// disk patch setup. Verified periodically rather than every frame.
	static inline std::vector<Ketchup_RamPatch> RamPatches = {};
	static inline unsigned int RamTick = 0;
	static inline unsigned int RamApplies = 0;
	constexpr static unsigned int RamCheckInterval = 30;

	// Every write this pass made, with the patch it came from, so that two
	// patches writing the same disc byte with different values can be reported
	// once the folder is done. Ketchup applies the selected files in path order, so
	// such a pair silently resolves by file name - the reason a mod can work
	// and then stop working because another was added beside it. Dropped as
	// soon as the report is out; a very large set stops being tracked.
	static void ReportOverlaps();
	static inline std::vector<Ketchup_Write> Writes = {};
	static inline std::vector<std::string> WriteSources = {};
	static inline unsigned int WriteSource = 0;
	static inline size_t WriteBytes = 0;
	static inline bool WritesTruncated = false;
	constexpr static size_t WriteByteLimit = 32u << 20;
};
