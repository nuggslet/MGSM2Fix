#include "stdafx.h"

using namespace std;

#include "emutask.h"
template <Squirk T>
extern EmuTask<T> gEmuTask;

extern SQInteger M2_DevId;
extern string M2_DevType;
extern SQInteger M2_DiskId;
extern SQInteger M2_Tray;

typedef struct {
	unsigned int id;
	unsigned int ram_base;
	unsigned int ram_range;
} Ketchup_DiskInfo;

typedef struct {
	string name;
	vector<Ketchup_DiskInfo> disks;
} Ketchup_VersionInfo;

typedef struct {
	unsigned int id;
	string name;
	vector<Ketchup_VersionInfo> versions;
} Ketchup_TitleInfo;

constexpr unsigned int gImageBase = 0x10000;
constexpr unsigned int gSectorSize = 0x800;
constexpr unsigned int gSectorStride = 0x130;
constexpr unsigned int gSectorRange = gSectorSize + gSectorStride;

auto cRange = [](unsigned int size) constexpr -> unsigned int
{
	return (size / gSectorSize) * gSectorRange;
};

vector<Ketchup_TitleInfo> gTitles = {
	{99, "INTEGRAL", {
		{"INTEGRAL", {
			{0, 0x131D2238, cRange(0x9C000)},
			{1, 0x0EB38078, cRange(0x9C000)},
		}},
		{"VR-DISK", {
			{0, 0x000865F8, cRange(0x99800)},
		}},
	}},
	{101, "VR-DISK_US", {
		{"USA", {
			{0, 0x0000E5C8, cRange(0x9C800)},
		}},
	}},
	{102, "VR-DISK_EU", {
		{"EUROPE", {
			{0, 0x0018CCA8, cRange(0x9D000)},
		}},
	}},
	{980, "MGS1_JP", {
		{"JAPAN", {
			{0, 0x127F1478, cRange(0x9C800)},
			{1, 0x0E0B4AA8, cRange(0x9C800)},
		}},
	}},
	{981, "MGS1_US", {
		{"USA", {
			{0, 0x0000E5C8, cRange(0x9E800)},
			{1, 0x0000E5C8, cRange(0x9E800)},
		}},
	}},
	{982, "MGS1_UK", {
		{"UK", {
			{0, 0x119BF0C8, cRange(0x9D000)},
			{1, 0x0D442538, cRange(0x9D000)},
		}},
	}},
	{983, "MGS1_DE", {
		{"GERMANY", {
			{0, 0x119BE798, cRange(0x9D000)},
			{1, 0x0D442538, cRange(0x9D000)},
		}},
	}},
	{984, "MGS1_FR", {
		{"FRANCE", {
			{0, 0x119BF9F8, cRange(0x9D000)},
			{1, 0x0D442538, cRange(0x9D000)},
		}},
	}},
	{985, "MGS1_IT", {
		{"ITALY", {
			{0, 0x119BF0C8, cRange(0x9D000)},
			{1, 0x0D442538, cRange(0x9D000)},
		}},
	}},
	{986, "MGS1_ES", {
		{"SPAIN", {
			{0, 0x119BF9F8, cRange(0x9D000)},
			{1, 0x0D441C08, cRange(0x9D000)},
		}},
	}},
};

template <Squirk T>
bool Ketchup_ApplyBlock(HSQUIRRELVM<T> v,
	Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk,
	int64_t offset, unsigned char *data, size_t size)
{
	SQArray<T> *block = SQArray<T>::Create(_ss(v), size);
	for (size_t i = 0; i < size; i++) {
		block->Set(i, data[i]);
	}
	gEmuTask<T>.EntryCdRomPatch(offset, block);
	LOG_F(INFO, "Ketchup: CD-ROM write 0x%08" PRIx64 " with %d bytes.", offset, size);

	// If tray is open this isn't a cold boot, so we can skip this.
	while (!M2_Tray && size != 0) {
		if (offset >= disk.ram_base && offset < (disk.ram_base + disk.ram_range)) {
			unsigned int address = offset - disk.ram_base;
			unsigned int sector = address / gSectorRange;
			unsigned int pos = address % gSectorRange;

			if (pos < gSectorSize) {
				address = (sector * gSectorSize) + pos;
				gEmuTask<T>.SetRamValue(CHAR_BIT, gImageBase + address, *data);
				LOG_F(INFO, "Ketchup: Mapped RAM write 0x%08X [0x%08" PRIx64 "] with value 0x%02X.",
					gImageBase + address, offset, *data);
			}
		}

		size--; data++; offset++;
	}

	return true;
}

int Ketchup_MetaPPF_FileId(ifstream &data, int version)
{
	unsigned int magic;
	int length;

	int index = 0;
	if (version == 2) {
		index = 4;
	} else {
		index = 2;
	}

	data.seekg(-(index + 4), ios_base::end);
	data.read((char *) &magic, sizeof(magic));

	if (magic != 'ZID.') {
		return 0;
	}

	data.seekg(-index, ios_base::end);
	data.read((char *) &length, index);
	return length;
}

template <Squirk T>
bool Ketchup_ApplyPPF3(HSQUIRRELVM<T> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, ifstream &data)
{
	unsigned char ppfmem[512];
	int length = Ketchup_MetaPPF_FileId(data, 3);

	unsigned char image_type, block_check, undo;
	data.seekg(56, ios_base::beg);
	data.read((char *) &image_type, sizeof(image_type));
	data.read((char *) &block_check, sizeof(block_check));
	data.read((char *) &undo, sizeof(undo));

	data.seekg(0, ios_base::end);
	streampos count = data.tellg();
	data.seekg(0, ios_base::beg);

	streampos pos;
	if (block_check) {
		pos = 1084;
		count -= 1084;
	} else {
		pos = 60;
		count -= 60;
	}

	if (length)
		count -= (length + 18 + 16 + 2);

	int64_t offset;
	unsigned char anz;
	data.seekg(pos, ios_base::beg);
	do {
		data.read((char *) &offset, sizeof(offset));
		data.read((char *) &anz, sizeof(anz));
		data.read((char *) ppfmem, anz);
		if (undo) data.seekg(anz, ios_base::cur);

		if (!Ketchup_ApplyBlock(v, title, version, disk, offset, ppfmem, anz))
			return false;

		count -= (anz + 9);
		if (undo) count -= anz;
	} while (count != 0);

	return true;
}

template <Squirk T>
bool Ketchup_Apply(HSQUIRRELVM<T> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, ifstream &data)
{
	unsigned int magic;
	data.seekg(0, ios_base::beg);
	data.read((char *) &magic, sizeof(magic));

	switch (magic) {
		case '3FPP': return Ketchup_ApplyPPF3(v, title, version, disk, data);
		default: return false;
	}
}

std::filesystem::path Ketchup_RootPath(Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, string base = "mods")
{
	std::filesystem::path root(base);
	root /= title.name;
	if (title.versions.size() > 1)
		root /= version.name;
	if (version.disks.size() > 1) {
		char no[] = "0";
		*no += disk.id;
		root /= no;
	}

	return root;
}

template <Squirk T>
bool Ketchup_ProcessDisk(HSQUIRRELVM<T> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk)
{
	std::filesystem::directory_entry root { Ketchup_RootPath(title, version, disk) };
	LOG_F(INFO, "Ketchup: base path is %S.", root.path().c_str());

	if (!root.exists() || !root.is_directory()) return true;
	for (const auto &entry : std::filesystem::directory_iterator(root)) {
		ifstream data(entry.path(), ios::in | ios::binary);
		if (Ketchup_Apply(v, title, version, disk, data)) {
			LOG_F(INFO, "Ketchup: loaded %S.", entry.path().c_str());
		}
	}

	return true;
}

template <Squirk T>
bool Ketchup_ProcessVersion(HSQUIRRELVM<T> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version)
{
	for (auto &disk : version.disks) {
		if (disk.id != M2_DiskId) continue;
		return Ketchup_ProcessDisk(v, title, version, disk);
	}

	return false;
}

template <Squirk T>
bool Ketchup_ProcessTitle(HSQUIRRELVM<T> v, Ketchup_TitleInfo &title)
{
	for (auto &version : title.versions) {
		if (version.name != M2_DevType) continue;
		return Ketchup_ProcessVersion(v, title, version);
	}

	return false;
}

template <Squirk T>
bool Ketchup_Process(HSQUIRRELVM<T> v)
{
	for (auto &title : gTitles) {
		if (title.id != M2_DevId) continue;
		return Ketchup_ProcessTitle(v, title);
	}

	return false;
}

template bool Ketchup_Process(HSQUIRRELVM<Squirk::Standard> v);
template bool Ketchup_Process(HSQUIRRELVM<Squirk::AlignObject> v);
