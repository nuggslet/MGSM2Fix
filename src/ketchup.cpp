#include "m2fix.h"
#include "sqhook.h"
#include "ketchup.h"

#include "sqemutask.h"
#include "sqglobals.h"
#include "sqsystemdata.h"

template <Squirk Q>
bool Ketchup<Q>::ApplyBlock(HSQUIRRELVM<Q> v,
	Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk,
	int64_t offset, unsigned char *data, size_t size)
{
	Sqrat::Array<Q> block(v, size);
	for (size_t i = 0; i < size; i++) {
		block.SetValue(i, data[i]);
	}
	SQEmuTask<Q>::EntryCdRomPatch((SQInteger) offset, block);
	spdlog::info("[Ketchup] CD-ROM write 0x{:08x} with {} bytes.", offset, size);

	// If tray is open this isn't a cold boot, so we can skip this.
	while (!SQHook<Q>::IsCdRomShellOpen() && size != 0) {
		if (offset >= disk.ram_base && offset < (disk.ram_base + disk.ram_range)) {
			unsigned int address = (unsigned int) offset - disk.ram_base;
			unsigned int sector = address / PSX_SectorRange;
			unsigned int pos = address % PSX_SectorRange;

			if (pos < PSX_SectorSize) {
				address = (sector * PSX_SectorSize) + pos;
				SQEmuTask<Q>::SetRamValue(CHAR_BIT, PSX_ImageBase + address, *data);
				spdlog::info("[Ketchup] Mapped RAM write 0x{:08x} [0x{:08x}] with value 0x{:02x}.",
					PSX_ImageBase + address, offset, *data);
			}
		}

		size--; data++; offset++;
	}

	return true;
}

template <Squirk Q>
int Ketchup<Q>::MetaPPF_FileId(std::ifstream &data, int version)
{
	unsigned int magic;
	int length;

	int index = 0;
	if (version == 2) {
		index = 4;
	} else {
		index = 2;
	}

	data.seekg(-(index + 4), std::ios_base::end);
	data.read((char *) &magic, sizeof(magic));

	if (magic != 'ZID.') {
		return 0;
	}

	data.seekg(-index, std::ios_base::end);
	data.read((char *) &length, index);
	return length;
}

template <Squirk Q>
bool Ketchup<Q>::ApplyPPF3(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::ifstream &data)
{
	unsigned char ppfmem[512];
	int length = MetaPPF_FileId(data, 3);

	unsigned char image_type, block_check, undo;
	data.seekg(56, std::ios_base::beg);
	data.read((char *) &image_type, sizeof(image_type));
	data.read((char *) &block_check, sizeof(block_check));
	data.read((char *) &undo, sizeof(undo));

	data.seekg(0, std::ios_base::end);
	std::streampos count = data.tellg();
	data.seekg(0, std::ios_base::beg);

	std::streampos pos;
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
	data.seekg(pos, std::ios_base::beg);
	do {
		data.read((char *) &offset, sizeof(offset));
		data.read((char *) &anz, sizeof(anz));
		data.read((char *) ppfmem, anz);
		if (undo) data.seekg(anz, std::ios_base::cur);

		if (!ApplyBlock(v, title, version, disk, offset, ppfmem, anz))
			return false;

		count -= (anz + 9);
		if (undo) count -= anz;
	} while (count != 0);

	return true;
}

template <Squirk Q>
bool Ketchup<Q>::Apply(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::ifstream &data)
{
	unsigned int magic;
	data.seekg(0, std::ios_base::beg);
	data.read((char *) &magic, sizeof(magic));

	switch (magic) {
		case '3FPP': return ApplyPPF3(v, title, version, disk, data);
		default: return false;
	}
}

template <Squirk Q>
std::filesystem::path Ketchup<Q>::RootPath(Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::string base)
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

template <Squirk Q>
bool Ketchup<Q>::ProcessDisk(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk)
{
	std::filesystem::directory_entry root { RootPath(title, version, disk) };
	spdlog::info("[Ketchup] base path is {}.", root.path().string());

	if (!root.exists() || !root.is_directory()) return true;
	for (const auto &entry : std::filesystem::directory_iterator(root)) {
		std::ifstream data(entry.path(), std::ios::in | std::ios::binary);
		if (Apply(v, title, version, disk, data)) {
			spdlog::info("[Ketchup] loaded {}.", entry.path().string());
		}
	}

	return true;
}

template <Squirk Q>
bool Ketchup<Q>::ProcessVersion(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version)
{
	for (auto &disk : version.disks) {
		if (disk.id != SQGlobals<Q>::GetDisk()) continue;
		return ProcessDisk(v, title, version, disk);
	}

	return false;
}

template <Squirk Q>
bool Ketchup<Q>::ProcessTitle(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title)
{
	for (auto &version : title.versions) {
		if (version.name != SQSystemData<Q>::SettingETC::GetVersion()) continue;
		return ProcessVersion(v, title, version);
	}

	return false;
}

template <Squirk Q>
bool Ketchup<Q>::Process(HSQUIRRELVM<Q> v)
{
	auto *titles = M2Fix::GameInstance().SQKetchupHook();
	if (!titles) return false;

	for (auto &title : *titles) {
		if (title.id != SQGlobals<Q>::GetTitle()) continue;
		return ProcessTitle(v, title);
	}

	return false;
}

template bool Ketchup<Squirk::Standard>::Process(HSQUIRRELVM<Squirk::Standard> v);
template bool Ketchup<Squirk::AlignObject>::Process(HSQUIRRELVM<Squirk::AlignObject> v);
template bool Ketchup<Squirk::StandardShared>::Process(HSQUIRRELVM<Squirk::StandardShared> v);
template bool Ketchup<Squirk::AlignObjectShared>::Process(HSQUIRRELVM<Squirk::AlignObjectShared> v);
