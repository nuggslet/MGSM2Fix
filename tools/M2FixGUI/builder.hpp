#pragma once
#include "core_base.hpp"
#include "archive.hpp"
#include "image.hpp"

inline Bytes unpackRecipes() {
    auto b = resource(101);
    size_t size = (size_t)at(b, 0, 8);
    need(size < 128 * 1024 * 1024, "Recipe size limit");
    Bytes out(size);
    DECOMPRESSOR_HANDLE h = nullptr;
    need(CreateDecompressor(COMPRESS_ALGORITHM_LZMS, nullptr, &h), "LZMS unavailable");
    SIZE_T written = 0;
    BOOL ok = Decompress(h, b.data() + 8, b.size() - 8, out.data(), out.size(), &written);
    CloseDecompressor(h);
    need(ok && written == size, "Recipe decompression failed");
    return out;
}
struct Options {
    bool steam = true, images = false, story = true, vr = true, grenade = true, english = true;
    std::string disc = "all";
    fs::path source, destination;
};
inline std::string outputLabel(const Options &o) {
    return o.story || o.vr || (o.images && o.english) ? "Integral English" : "Integral";
}
inline std::wstring outputDirectoryName(const Options &o) {
    auto label = outputLabel(o);
    std::replace(label.begin(), label.end(), ' ', '-');
    return wide(label + (o.images ? "-images" : "-patches"));
}
inline std::string imageFilename(const Options &o, const std::string &disc) {
    return outputLabel(o) + " " + disc + ".bin";
}
inline bool keep(const std::string &name, const Options &o) {
    if (name.find("_zz_ecc") != name.npos)
        return false;
    if (name.find("_fix_grenade_delay") != name.npos)
        return o.grenade && (o.vr || name.find("_english") == name.npos);
    if (name.find("_disc1_") != name.npos || name.find("_disc2_") != name.npos)
        return o.story;
    if (name.find("_vr_en_") != name.npos)
        return o.vr;
    throw std::runtime_error("Unclassified recipe: " + name);
}
inline fs::path build(const Options &o, const std::function<void(std::string)> &log) {
    need(fs::is_directory(o.destination), "Choose an existing output folder");
    auto final = o.destination / outputDirectoryName(o);
    need(!fs::exists(final), "Output already exists; choose another destination: " + str(final));
    Bytes packed = unpackRecipes();
    Zip recipes(packed);
    Json manifest = Json::parse(recipes.read("manifest.json"));
    Temp temp(o.destination);
    auto result = temp.path / L"result";
    fs::create_directory(result);
    std::vector<DiscSpec> selected;
    for (auto &d : specs)
        if (o.images ? (o.disc == "all" || o.disc == d.id)
                     : (d.id == "vr" ? (o.vr || o.grenade) : o.story))
            selected.push_back(d);
    need(!selected.empty(), "Select at least one patch");
    std::map<std::string, fs::path> inputs;
    auto verify = [&](const fs::path &p, const DiscSpec &d) {
        Mapping m(p);
        need(m.size == manifest["discs"][d.id]["size"].get<uint64_t>() &&
                 m.sha() == manifest["discs"][d.id]["sha256"],
             "Disc " + d.id + " does not match supported unpatched Integral image");
    };
    if (o.steam) {
        log("Restoring verified discs from Steam...");
        auto container = o.source / L"windata/dlc/dlc_japan.bin";
        auto index = archiveIndex(container),
             main = archiveIndex(o.source / L"windata/alldata.bin");
        Mapping archive(container), alldata(o.source / L"windata/alldata.bin");
        for (auto &d : selected) {
            auto member = "system/roms/integral_disc" + (d.id == "vr" ? std::string("3") : d.id) +
                          "-washed.bin";
            auto extent = index.at(member);
            size_t size = extent[1];
            need(extent[0].get<uint64_t>() == d.base && size == manifest["discs"][d.id]["size"] &&
                     d.base + size <= archive.size,
                 "Steam disc extent mismatch");
            auto boot = manifest["boots"][d.id];
            auto loc = main.at(lower(boot["member"].get<std::string>()));
            auto blob = alldata.slice(loc[0], loc[1]);
            std::string sig = "lz4a";
            auto a = std::search(blob.begin(), blob.end(), sig.begin(), sig.end());
            need(a != blob.end(), "Missing snapshot");
            a = std::search(a + 4, blob.end(), sig.begin(), sig.end());
            need(a != blob.end(), "Missing RAM snapshot");
            size_t p = (a - blob.begin()) + 4;
            size_t compressed = (size_t)at(blob, p, 4), sizeRam = (size_t)at(blob, p + 4, 4),
                   name = (size_t)at(blob, p + 8, 1);
            need(sizeRam == 2097152, "RAM size mismatch");
            auto ram = lz4(blob, p + 9 + name, compressed, sizeRam);
            need(digest(ram) == boot["snapshot_sha256"],
                 "Unsupported Steam startup snapshot; verify game files");
            auto exe = recipes.read("boots/" + d.id + ".xor");
            size_t address = boot["address"];
            need(exe.size() == boot["size"] && exe.size() >= 2048 &&
                     address + exe.size() - 2048 <= ram.size(),
                 "Boot recipe bounds");
            for (size_t i = 2048; i < exe.size(); ++i)
                exe[i] ^= ram[address + i - 2048];
            need(digest(exe) == boot["sha256"], "Boot restoration hash mismatch");
            auto output = temp.path / wide(d.id + ".bin");
            {
                std::ofstream f(output, std::ios::binary);
                need(bool(f.write((char *)archive.data + d.base, size)),
                     "Cannot write restored disc");
            }
            auto [lba, length] = exeExtent(archive, d.base, d.exe);
            need(length == exe.size(), "Boot extent mismatch");
            {
                Mapping disc(output, true);
                for (size_t i = 0; i < exe.size(); i += 2048) {
                    auto at = (lba + i / 2048) * 2352 + 24;
                    need(at + 2048 <= disc.size, "Boot write bounds");
                    memcpy(disc.data + at, exe.data() + i, std::min<size_t>(2048, exe.size() - i));
                }
                disc.flush();
            }
            verify(output, d);
            inputs[d.id] = output;
            log("Verified Steam disc " + d.id);
        }
    } else {
        std::vector<fs::path> candidates;
        if (fs::is_regular_file(o.source))
            candidates.push_back(o.source);
        else if (fs::is_directory(o.source))
            for (auto &e : fs::directory_iterator(o.source))
                if (e.is_regular_file() && (lower(str(e.path().extension())) == ".bin" ||
                                            lower(str(e.path().extension())) == ".zip"))
                    candidates.push_back(e.path());
        std::stable_sort(candidates.begin(), candidates.end(), [](auto &a, auto &b) {
            return (lower(str(a)).find("integral") != std::string::npos) >
                   (lower(str(b)).find("integral") != std::string::npos);
        });
        const bool scanFolder = fs::is_directory(o.source);
        for (auto &candidate : candidates) {
            if (inputs.size() == selected.size())
                break;
            try {
                fs::path raw = candidate;
                bool extracted = false;
                if (lower(str(candidate.extension())) == ".zip") {
                    Zip zip(candidate);
                    int member = -1;
                    uint64_t size = 0;
                    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip.z); ++i) {
                        mz_zip_archive_file_stat s{};
                        need(mz_zip_reader_file_stat(&zip.z, i, &s), "ZIP index error");
                        if (ends(lower(s.m_filename), ".bin")) {
                            if (member >= 0)
                                throw std::runtime_error("ZIP contains multiple BIN files");
                            member = (int)i;
                            size = s.m_uncomp_size;
                        }
                    }
                    if (member < 0)
                        continue;
                    bool wanted = false;
                    for (auto &d : selected)
                        wanted |= !inputs.count(d.id) && size == manifest["discs"][d.id]["size"];
                    if (!wanted)
                        continue;
                    log("Extracting " + str(candidate.filename()));
                    raw = temp.path / L"redump.bin";
                    std::ofstream out(raw, std::ios::binary);
                    auto callback = [](void *opaque, mz_uint64, const void *buf, size_t n) -> size_t {
                        auto &f = *(std::ofstream *)opaque;
                        f.write((const char *)buf, n);
                        return f ? n : 0;
                    };
                    need(mz_zip_reader_extract_to_callback(&zip.z, member, callback, &out, 0),
                         "ZIP extraction failed");
                    out.close();
                    extracted = true;
                }
                std::string id;
                {
                    Mapping m(raw);
                    for (auto &d : selected)
                        if (!inputs.count(d.id) && m.size == manifest["discs"][d.id]["size"]) {
                            if (m.sha() == manifest["discs"][d.id]["sha256"]) {
                                id = d.id;
                                break;
                            }
                        }
                }
                if (!id.empty()) {
                    if (extracted) {
                        auto dest = temp.path / wide(id + ".bin");
                        fs::rename(raw, dest);
                        raw = dest;
                    }
                    inputs[id] = raw;
                    log("Verified Redump disc " + id);
                } else if (extracted)
                    fs::remove(raw);
            } catch (const std::exception &e) {
                if (!scanFolder)
                    throw;
                log("Skipping " + str(candidate.filename()) + ": " + e.what());
            }
        }
        need(inputs.size() == selected.size(),
             "Missing Integral discs: select unpatched supported Integral BINs or "
             "ZIPs");
    }
    std::string variant = o.images ? "raw" : "collection";
    Json selectedReport = {{"story", o.story}, {"vr", o.vr}, {"grenade", o.grenade}};
    for (auto &d : selected) {
        log("Building disc " + d.id + " patches...");
        Mapping image(inputs.at(d.id));
        std::map<uint64_t, Bytes> writes;
        std::map<std::string, Bytes> files;
        auto materialize = [&](const std::string &name, const std::string &which) {
            auto data = recipes.read(which + "/" + name);
            if (ends(name, ".ppf"))
                records(data, [&](uint64_t off, size_t p, size_t n) {
                    need(off + n <= image.size, "PPF outside image");
                    for (size_t i = 0; i < n; ++i)
                        data[p + i] ^= image.data[off + i];
                });
            need(digest(data) == manifest["packages"][which][name]["sha256"],
                 "Generated patch hash mismatch: " + name);
            return data;
        };
        for (auto &[name, info] : manifest["packages"][variant].items()) {
            if ((!info.contains("disc") || info["disc"].is_null() || info["disc"] != d.id) ||
                !(ends(name, ".ppf") || ends(name, ".ppf.json")) || !keep(name, o))
                continue;
            files[name] = materialize(name, variant);
        }
        if (o.images && !o.vr && o.grenade && d.id == "vr") {
            auto name = d.folder + "/INTEGRAL_vr_fix_grenade_delay.ppf";
            files[name] = materialize(name, "collection");
        }
        if (!o.images) {
            for (auto &[name, data] : files)
                writeFile(safePath(result, name), data);
        } else {
            for (auto &[name, data] : files)
                if (ends(name, ".ppf"))
                    records(data, [&](uint64_t off, size_t p, size_t n) {
                        for (size_t i = 0; i < n; ++i) {
                            uint64_t sector = (off + i) / 2352;
                            auto found = writes.find(sector);
                            if (found == writes.end()) {
                                auto original = image.slice(sector * 2352, 2352), fixed = original;
                                parity(fixed);
                                need(original == fixed, "Original sector parity mismatch");
                                found = writes.emplace(sector, std::move(original)).first;
                            }
                            found->second[(off + i) % 2352] = data[p + i];
                        }
                    });
            if (o.english)
                for (auto &[off, bytes] : language(image, d)) {
                    auto sector = off / 2352;
                    if (!writes.count(sector))
                        writes[sector] = image.slice(sector * 2352, 2352);
                    std::copy(bytes.begin(), bytes.end(), writes[sector].begin() + off % 2352);
                }
            auto output = result / wide(imageFilename(o, d.id));
            fs::copy_file(inputs.at(d.id), output);
            {
                Mapping target(output, true);
                for (auto &[sector, data] : writes) {
                    parity(data);
                    memcpy(target.data + sector * 2352, data.data(), 2352);
                }
                target.flush();
            }
            auto cue = output;
            cue.replace_extension(L".cue");
            writeText(cue, "FILE \"" + str(output.filename()) +
                               "\" BINARY\r\n  TRACK 01 MODE2/2352\r\n    INDEX 01 "
                               "00:00:00\r\n");
            log("Written " + str(output.filename()));
        }
    }
    if (!o.images) {
        auto report = Json::parse(recipes.read("collection/build-report.json"));
        Json outputs = Json::object();
        for (auto &[name, info] : report["outputs"].items())
            for (auto &e : fs::recursive_directory_iterator(result))
                if (e.is_regular_file() && str(e.path().filename()) == name)
                    outputs[name] = info;
        report["outputs"] = outputs;
        report["selected_patches"] = selectedReport;
        writeText(result / L"build-report.json", report.dump(2) + "\n");
        std::string readme =
            std::string(o.story || o.vr ? "Integral English" : "Integral") +
            " - M2Fix collection patches\r\n\r\nInstall the mods folder in your MGS1 "
            "Master Collection folder. Keep JSON companions.\r\nEnable corresponding M2Fix INI "
            "options and restart the game.\r\nUnchecked patches already installed are not "
            "removed.\r\n";
        if (o.story || o.vr)
            readme += "English translation derived from FoxdieTeam/mgs_reversing; see "
                      "CREDITS.md.\r\n";
        writeText(result / L"README.txt", readme);
        writeFile(result / L"CREDITS.md", resource(103));
        std::map<std::string, Bytes> files;
        for (auto &e : fs::recursive_directory_iterator(result))
            if (e.is_regular_file())
                files[str(e.path().lexically_relative(result))] = readFile(e.path());
        std::string sums;
        for (auto &[name, b] : files) {
            auto normalized = name;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            sums += digest(b) + "  " + normalized + "\n";
        }
        writeText(result / L"SHA256SUMS.txt", sums);
        files["SHA256SUMS.txt"] = Bytes(sums.begin(), sums.end());
        Zip output(result / wide(o.story || o.vr ? "Integral-English-collection.zip"
                                                 : "Integral-collection.zip"), true);
        for (auto &[name, b] : files) {
            auto normalized = name;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            output.add(normalized, b);
        }
        output.finish();
    } else {
        selectedReport["english_default"] = o.english;
        writeText(result / L"selected-patches.json", selectedReport.dump(2) + "\n");
    }
    need(!fs::exists(final), "Output appeared while building");
    fs::rename(result, final);
    log("Completed: " + str(final));
    return final;
}
