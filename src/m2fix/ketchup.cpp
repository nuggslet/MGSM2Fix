#include "m2fix.h"
#include "sqhook.h"
#include "ketchup.h"
#include "../games/mgs1_patch_options.h"

#include <algorithm>

#include "sqemutask.h"
#include "sqglobals.h"
#include "sqsystemdata.h"

template <Squirk Q>
bool Ketchup<Q>::ApplyBlock(HSQUIRRELVM<Q> v,
	Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk,
	uint64_t offset, unsigned char *data, size_t size)
{
	Sqrat::Array<Q> block(v, size);
	for (size_t i = 0; i < size; i++) {
		block.SetValue(i, data[i]);
	}
#ifndef _WIN64
	SQEmuTask<Q>::EntryCdRomPatch(static_cast<SQInteger>(offset), block);
#else
	SQEmuTask<Q>::EntryCdRomPatch(static_cast<SQInteger>(offset), false, block);
#endif
	spdlog::info("[SQ] [Ketchup] CD-ROM write 0x{:08x} with {} bytes.", offset, size);

	// Keep the write so ReportOverlaps() can find two patches that disagree
	// about the same disc byte. Only until the folder is done, and only up to a
	// limit - a patch set can be megabytes, and this is a diagnostic.
	if (!WritesTruncated) {
		if (WriteBytes + size > WriteByteLimit) {
			WritesTruncated = true;
			spdlog::info("[SQ] [Ketchup] overlap tracking stopped after {} bytes.", WriteBytes);
		} else {
			Writes.push_back({ offset, WriteSource,
				std::vector<unsigned char>(data, data + size) });
			WriteBytes += size;
		}
	}

	// The CD-ROM write above is enough for anything the game reads off the disc
	// at runtime (stage overlays, RADIO.DAT and friends), but not for the boot
	// executable: the Master Collection never reads that from the disc, it
	// preloads it from a ROM snapshot. Those bytes have to be written straight
	// to machine memory instead - and writing them here does not stick, because
	// the Master Collection finishes setting up machine memory *after* the disk
	// patch is entered and discards most of it. So collect the mapped writes
	// and let Update() apply them once the machine is actually up.
	size_t mapped = 0, mirrored = 0, tail = 0;
	for (size_t i = 0; i < size; i++) {
		uint64_t position = offset + i;
		if (position < disk.ram_base || position >= (disk.ram_base + disk.ram_range))
			continue;

		mapped++;
		unsigned int address = static_cast<unsigned int>(position - disk.ram_base);
		unsigned int sector = address / PSX_SectorRange;
		unsigned int pos = address % PSX_SectorRange;

		// A raw sector is 2352 bytes of which 2048 are payload. The rest - the
		// header and the EDC/ECC tail - is not part of the file, so a write
		// that runs into it reaches the disc image and never the RAM mirror.
		// That used to be silent: the log said the patch loaded while the bytes
		// were quietly gone. An executable patch built without splitting its
		// runs at the payload boundary loses exactly this way.
		if (pos >= PSX_SectorSize) { tail++; continue; }

		unsigned int ram = PSX_ImageBase + (sector * PSX_SectorSize) + pos;

		// Coalesce runs so verification stays cheap.
		if (!RamPatches.empty()) {
			auto &last = RamPatches.back();
			if (last.address + last.data.size() == ram) {
				last.data.push_back(data[i]);
				mirrored++;
				continue;
			}
		}
		RamPatches.push_back({ ram, { data[i] } });
		mirrored++;
	}

	if (tail) {
		spdlog::warn("[SQ] [Ketchup] {} of {} byte(s) written at 0x{:08x} land in a raw"
			" sector's 304-byte tail and are NOT mirrored into RAM ({} of {} mapped"
			" byte(s) were). Split the patch's runs at the 2048-byte payload boundary.",
			tail, size, offset, mirrored, mapped);
	}

	return true;
}

template <Squirk Q>
void Ketchup<Q>::ReportOverlaps()
{
	if (Writes.size() > 1) {
		std::vector<const Ketchup_Write *> order;
		order.reserve(Writes.size());
		for (auto &write : Writes) order.push_back(&write);
		std::sort(order.begin(), order.end(),
			[](const Ketchup_Write *a, const Ketchup_Write *b) {
				return a->offset < b->offset;
			});

		unsigned int reports = 0;
		for (size_t i = 0; i + 1 < order.size() && reports < 8; i++) {
			const Ketchup_Write *a = order[i];
			uint64_t end = a->offset + a->data.size();
			for (size_t j = i + 1; j < order.size() && order[j]->offset < end; j++) {
				const Ketchup_Write *b = order[j];
				if (a->source == b->source) continue;

				// Agreeing on a byte is harmless whoever writes it last; only a
				// disagreement makes the outcome depend on load order.
				uint64_t to = (std::min)(end, b->offset + b->data.size());
				uint64_t differs = 0, first = 0;
				for (uint64_t p = b->offset; p < to; p++) {
					if (a->data[p - a->offset] == b->data[p - b->offset]) continue;
					if (!differs) first = p;
					differs++;
				}
				if (!differs) continue;

				reports++;
				spdlog::warn("[SQ] [Ketchup] {} and {} disagree over {} of {} shared"
					" byte(s) from 0x{:08x} (first at 0x{:08x}). Whichever loads later"
					" wins, and that is file name order.",
					WriteSources[a->source], WriteSources[b->source],
					differs, to - b->offset, b->offset, first);
				break;
			}
		}
	}

	Writes.clear();
	Writes.shrink_to_fit();
	WriteSources.clear();
	WriteBytes = 0;
	WritesTruncated = false;
}


template <Squirk Q>
void Ketchup<Q>::Update()
{
	if (RamPatches.empty()) return;

	// Mid disc swap - the image is being torn down, leave it alone.
	if (SQHook<Q>::IsCdRomShellOpen()) return;

	// Bound retries for recurring foreign writers. Inspect every byte, including
	// interior bytes, but write only differences rather than the entire image.
	unsigned int interval = RamCheckInterval * (RamApplies < 8 ? 1 : 16);
	if (RamTick++ % interval != 0) return;
	const size_t bytes = RepairRamPatches(RamPatches,
		[](unsigned address) { return SQEmuTask<Q>::GetRamValue(CHAR_BIT, address); },
		[](unsigned address, unsigned char value) { SQEmuTask<Q>::SetRamValue(CHAR_BIT, address, value); });
	if (!bytes) return;

	// Only worth logging the first few; after that it is a reapply loop and the
	// log would drown in it.
	if (++RamApplies <= 4) {
		spdlog::info("[SQ] [Ketchup] Applied {} bytes of RAM patches in {} blocks (pass {}).",
			bytes, RamPatches.size(), RamApplies);
	}
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
	data.read(reinterpret_cast<char *>(&magic), sizeof(magic));

	if (magic != 'ZID.') {
		return 0;
	}

	data.seekg(-index, std::ios_base::end);
	data.read(reinterpret_cast<char *>(&length), index);
	return length;
}

template <Squirk Q>
bool Ketchup<Q>::ApplyPPF3(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::ifstream &data)
{
	unsigned char ppfmem[512];
	int length = MetaPPF_FileId(data, 3);

	unsigned char image_type, block_check, undo;
	data.seekg(56, std::ios_base::beg);
	data.read(reinterpret_cast<char *>(&image_type), sizeof(image_type));
	data.read(reinterpret_cast<char *>(&block_check), sizeof(block_check));
	data.read(reinterpret_cast<char *>(&undo), sizeof(undo));
	if (!data) {
		spdlog::warn("[SQ] [Ketchup] PPF3 header is truncated; not applied.");
		return false;
	}

	data.seekg(0, std::ios_base::end);
	std::streamoff total = data.tellg();

	// The block check is 1024 bytes of the ORIGINAL image at 0x9320, there so a
	// tool can refuse a patch aimed at a different release. This fix has no way
	// to read the image back - the emulator interface only writes - so the
	// bytes are skipped, as they always were. Saying so beats implying it was
	// checked.
	std::streamoff pos = block_check ? 1084 : 60;
	std::streamoff end = total;
	if (length) end -= (length + 18 + 16 + 2);

	if (end < pos || end > total) {
		spdlog::warn("[SQ] [Ketchup] PPF3 is {} byte(s), too short for its own header"
			" (records would start at {} and end at {}); not applied.", total, pos, end);
		return false;
	}

	// Validate the whole record chain BEFORE writing anything. A malformed file
	// used to be walked to a count that never reached zero, applying garbage in
	// a loop and filling the log - which is what a 50-byte description field
	// overrun did once, for 306 MB. A patch is now either wholly good or wholly
	// refused.
	std::streamoff scan = pos;
	uint64_t records = 0;
	uint64_t offset;
	unsigned char anz;
	while (scan < end) {
		if (end - scan < 9) {
			spdlog::warn("[SQ] [Ketchup] PPF3 has {} trailing byte(s) after its last"
				" whole record; not applied.", end - scan);
			return false;
		}

		data.seekg(scan + 8, std::ios_base::beg);
		data.read(reinterpret_cast<char *>(&anz), sizeof(anz));
		if (!data) {
			spdlog::warn("[SQ] [Ketchup] PPF3 record at {} could not be read; not applied.", scan);
			return false;
		}
		if (anz == 0) {
			spdlog::warn("[SQ] [Ketchup] PPF3 record at {} has a zero length; not applied.", scan);
			return false;
		}

		std::streamoff span = 9 + static_cast<std::streamoff>(anz) * (undo ? 2 : 1);
		if (end - scan < span) {
			spdlog::warn("[SQ] [Ketchup] PPF3 record at {} runs {} byte(s) past the end"
				" of the file; not applied.", scan, span - (end - scan));
			return false;
		}

		scan += span;
		records++;
	}

	spdlog::info("[SQ] [Ketchup] PPF3 image type {}, {} record(s), {}{}.",
		image_type, records, undo ? "with undo data" : "no undo data",
		block_check ? ", block check present (skipped, unverifiable here)" : "");

	// The scan above may have left the stream at end-of-file; a seek on a
	// stream in that state is not obliged to do anything useful.
	data.clear();

	while (pos < end) {
		data.seekg(pos, std::ios_base::beg);
		data.read(reinterpret_cast<char *>(&offset), sizeof(offset));
		data.read(reinterpret_cast<char *>(&anz), sizeof(anz));
		data.read(reinterpret_cast<char *>(ppfmem), anz);
		if (!data) {
			spdlog::warn("[SQ] [Ketchup] PPF3 read failed at {} after validating;"
				" the file changed underneath us.", pos);
			return false;
		}

		// English mission text must follow GrenadeDelayFix even when no texture
		// addon is active. The companion pins the PPF and its five digit offsets.
		for (const auto &[address, value] : RecordOverrides) {
			if (address >= offset && address - offset < anz)
				ppfmem[static_cast<size_t>(address - offset)] = value;
		}
		if (!ApplyBlock(v, title, version, disk, offset, ppfmem, anz))
			return false;

		pos += 9 + static_cast<std::streamoff>(anz) * (undo ? 2 : 1);
	}

	return true;
}

template <Squirk Q>
bool Ketchup<Q>::Apply(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk, std::ifstream &data)
{
	unsigned int magic;
	data.seekg(0, std::ios_base::beg);
	data.read(reinterpret_cast<char *>(&magic), sizeof(magic));

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
bool Ketchup<Q>::ProcessBuiltins(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk)
{
	auto *patches = M2Fix::GameInstance().SQKetchupPatches();
	if (!patches) return true;

	for (auto &patch : *patches) {
		if (patch.title != title.id) continue;
		if (patch.version != version.name) continue;
		if (patch.disk != disk.id) continue;
		if (patch.data.empty()) continue;

		spdlog::info("[SQ] [Ketchup] built-in patch {} -> {} {} disk {}.",
			patch.name, title.name, version.name, disk.id);
		WriteSource = static_cast<unsigned int>(WriteSources.size());
		WriteSources.push_back("built-in " + patch.name);
		ApplyBlock(v, title, version, disk, patch.offset,
			patch.data.data(), patch.data.size());
	}

	return true;
}

template <Squirk Q>
bool Ketchup<Q>::ProcessDisk(HSQUIRRELVM<Q> v, Ketchup_TitleInfo &title, Ketchup_VersionInfo &version, Ketchup_DiskInfo &disk)
{
	// Built-ins first, and unconditionally: they do not live in the mods folder,
	// so an absent or empty one must not skip them.
	ProcessBuiltins(v, title, version, disk);

	std::filesystem::directory_entry root { RootPath(title, version, disk) };
	spdlog::info("[SQ] [Ketchup] base path is {}.", root.path().string());

	if (!root.exists() || !root.is_directory()) {
		ReportOverlaps();
		return true;
	}

	const MGS1PatchOptions::Settings settings {
		M2Config::bPatchesIntegralEnglish, M2Config::bPatchesIntegralVREnglish,
		M2Config::bPatchesGrenadeDelay, M2Config::bGameUnlockVRMissions,
		M2Config::bGameUnlockVRExtras, M2Config::bGameUnlockVRMovies,
		M2Config::bGameUnlockTitleBonuses
	};
	const auto plan = MGS1PatchOptions::prepare(root.path(), title.id, version.name, disk.id, settings);
	for (const auto &message : plan.messages) spdlog::info("[SQ] [Ketchup] {}", message);
	for (const auto &patch : plan.patches) {
		std::ifstream data(patch.path, std::ios::in | std::ios::binary);
		RecordOverrides = patch.overrides;
		WriteSource = static_cast<unsigned int>(WriteSources.size());
		WriteSources.push_back(patch.path.filename().string());
		if (Apply(v, title, version, disk, data)) {
			spdlog::info("[SQ] [Ketchup] loaded {} ({} configured byte overrides).", patch.path.string(), patch.overrides.size());
		}
	}
	RecordOverrides.clear();

	// After the whole folder, because a collision is between two of its files.
	ReportOverlaps();

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
	// Rebuilt from scratch on every disk patch setup, so a title or disk change
	// cannot leave stale writes aimed at the previous image.
	RamPatches.clear();
	RamTick = 0;
	RamApplies = 0;
	Writes.clear();
	WriteSources.clear();
	WriteSource = 0;
	WriteBytes = 0;
	WritesTruncated = false;

	auto *titles = M2Fix::GameInstance().SQKetchupHook();
	if (!titles) return false;

	for (auto &title : *titles) {
		if (title.id != SQGlobals<Q>::GetTitle()) continue;
		const bool result = ProcessTitle(v, title);
		NormalizeRamPatches(RamPatches);
		return result;
	}

	return false;
}

template bool Ketchup<Squirk::Standard>::Process(HSQUIRRELVM<Squirk::Standard> v);
template bool Ketchup<Squirk::AlignObject>::Process(HSQUIRRELVM<Squirk::AlignObject> v);
template bool Ketchup<Squirk::StandardShared>::Process(HSQUIRRELVM<Squirk::StandardShared> v);
template bool Ketchup<Squirk::AlignObjectShared>::Process(HSQUIRRELVM<Squirk::AlignObjectShared> v);


template void Ketchup<Squirk::Standard>::Update();
template void Ketchup<Squirk::AlignObject>::Update();
template void Ketchup<Squirk::StandardShared>::Update();
template void Ketchup<Squirk::AlignObjectShared>::Update();
