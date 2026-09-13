#pragma once

// Pure file selection and validation, shared by Ketchup and the native tests.
// Unknown mods are left alone. Known optional patches remain external assets.
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace MGS1PatchOptions {
struct Settings {
    bool english = true, vrEnglish = true, grenade = true;
    bool missions = false, extras = false, movies = false, titleBonuses = false;
};
enum class Kind { Other, English, VREnglish, Missions, Extras, Movies, Title, GrenadeJP, GrenadeEN, RawGrenade };
inline constexpr const char *MissionFile = "INTEGRAL_vr_en_missions.ppf";
inline constexpr const char *GrenadeJP = "INTEGRAL_vr_fix_grenade_delay.ppf";
inline constexpr const char *GrenadeEN = "INTEGRAL_vr_fix_grenade_delay_english.ppf";

inline std::string lower(std::string name)
{
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

inline Kind classify(unsigned title, const std::string &version, unsigned disk, std::string name)
{
    name = lower(name);
    const auto matches = [&](const std::string &prefix, const std::vector<std::string> &families) {
        for (const auto &family : families) if (name == prefix + family + ".ppf") return true;
        return false;
    };
    if (title == 99 && version == "INTEGRAL" && disk < 2) {
        const auto prefix = "integral_disc" + std::to_string(disk + 1);
        if (matches(prefix + "_en_", {"items", "menu", "menu2", "menu3", "menu3_raw", "option", "preope", "brf", "savemsg", "camsave", "abst", "pad2"})) return Kind::English;
        if (name == prefix + "_unlock_title.ppf") return Kind::Title;
    }
    if (title == 981 && version == "USA" && disk < 2 &&
        name == "mgs1_disc" + std::to_string(disk + 1) + "_unlock_title.ppf") return Kind::Title;
    if (title == 101 && version == "USA" && disk == 0 && name == "vrus_unlock_missions.ppf") return Kind::Missions;
    if (title != 99 || version != "VR-DISK" || disk != 0) return Kind::Other;
    if (matches("integral_vr_en_", {"missions", "items", "savemsg", "option", "title", "camsave", "movie", "movie_e3", "memcard"})) return Kind::VREnglish;
    if (name == "integral_vr_unlock_missions.ppf") return Kind::Missions;
    if (name == "integral_vr_unlock_extras.ppf") return Kind::Extras;
    if (name == "integral_vr_unlock_movies.ppf") return Kind::Movies;
    if (name == "integral_vr_fix_grenade_delay.ppf") return Kind::GrenadeJP;
    if (name == "integral_vr_fix_grenade_delay_english.ppf") return Kind::GrenadeEN;
    if (name == "integral_vr_fix_grenade_delay_raw.ppf" || name == "integral_vr_fix_grenade_delay_english_raw.ppf") return Kind::RawGrenade;
    return Kind::Other;
}

struct Patch {
    std::filesystem::path path;
    std::map<uint64_t, unsigned char> overrides;
};
struct Plan {
    std::vector<Patch> patches;
    std::vector<std::string> messages;
};
inline std::vector<unsigned char> bytes(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input || input.tellg() < 0 || input.tellg() > (64 << 20)) throw std::runtime_error("missing or oversized PPF: " + path.string());
    std::vector<unsigned char> data(static_cast<size_t>(input.tellg()));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(data.data()), data.size())) throw std::runtime_error("cannot read PPF: " + path.string());
    return data;
}
inline std::string fingerprint(const std::vector<unsigned char> &data)
{
    // FNV-1a detects stale companions; this is a layout guard, not authentication.
    uint64_t value = 14695981039346656037ull;
    for (auto c : data) value = (value ^ c) * 1099511628211ull;
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}
inline nlohmann::json metadata(const std::filesystem::path &path, const std::vector<unsigned char> &data)
{
    std::ifstream input(path.string() + ".json");
    nlohmann::json meta;
    if (!input || !(input >> meta)) throw std::runtime_error("missing patch companion: " + path.string() + ".json");
    if (meta.at("schema") != 1 || meta.at("fingerprint") != fingerprint(data)) throw std::runtime_error("stale patch companion: " + path.string());
    return meta;
}
inline std::map<uint64_t, unsigned char> digits(const std::vector<unsigned char> &data, const nlohmann::json &meta)
{
    const auto offsets = meta.at("grenade_digits").get<std::vector<uint64_t>>();
    if (offsets.size() != 5) throw std::runtime_error("expected five grenade digits");
    std::map<uint64_t, unsigned char> result;
    if (data.size() < 60 || std::string(data.begin(), data.begin() + 6) != std::string("PPF30\2", 6) || data[56] || data[58]) throw std::runtime_error("unsupported companion PPF");
    size_t pos = data[57] ? 1084 : 60;
    if (pos > data.size()) throw std::runtime_error("truncated PPF block check");
    while (pos < data.size()) {
        if (data.size() - pos < 9) throw std::runtime_error("truncated PPF record");
        uint64_t off = 0;
        for (unsigned n = 0; n < 8; ++n) off |= uint64_t(data[pos + n]) << (8 * n);
        const size_t count = data[pos + 8];
        pos += 9;
        if (!count || count > data.size() - pos || off > UINT64_MAX - count) throw std::runtime_error("invalid PPF record");
        for (auto address : offsets) {
            if (address >= off && address - off < count) {
                auto value = data[pos + static_cast<size_t>(address - off)];
                if (value != '4' && value != '5') throw std::runtime_error("grenade digit is not 4 or 5");
                if (!result.emplace(address, value).second) throw std::runtime_error("duplicate grenade digit write");
            }
        }
        pos += count;
    }
    if (result.size() != 5) throw std::runtime_error("missing grenade digit writes");
    return result;
}

inline Plan prepare(const std::filesystem::path &root, unsigned title, const std::string &version, unsigned disk, const Settings &settings)
{
    Plan plan;
    std::vector<std::filesystem::path> files;
    for (const auto &entry : std::filesystem::directory_iterator(root)) if (entry.is_regular_file()) files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    std::filesystem::path mission;
    for (const auto &path : files) if (classify(title, version, disk, path.filename().string()) == Kind::VREnglish && lower(path.filename().string()) == lower(MissionFile)) mission = path;
    bool english = settings.vrEnglish && !mission.empty();
    bool validEnglish = true;
    std::string missionFingerprint;
    std::map<uint64_t, unsigned char> replacement;
    if (english) {
        try {
            const auto data = bytes(mission);
            replacement = digits(data, metadata(mission, data));
            missionFingerprint = fingerprint(data);
            for (auto &digit : replacement) digit.second = settings.grenade ? '4' : '5';
        } catch (const std::exception &error) {
            validEnglish = false;
            english = false;
            plan.messages.push_back(std::string("VR English missions skipped; reinstall the matching mission PPF and JSON companion: ") + error.what());
        }
    }
    for (const auto &path : files) {
        const auto kind = classify(title, version, disk, path.filename().string());
        bool enabled = true;
        switch (kind) {
        case Kind::English: enabled = settings.english; break;
        case Kind::VREnglish: enabled = settings.vrEnglish && (path != mission || validEnglish); break;
        case Kind::Missions: enabled = settings.missions; break;
        case Kind::Extras: enabled = settings.extras; break;
        case Kind::Movies: enabled = settings.movies; break;
        case Kind::Title: enabled = settings.titleBonuses; break;
        case Kind::GrenadeJP: enabled = settings.grenade && !english; break;
        case Kind::GrenadeEN: enabled = settings.grenade && english; break;
        case Kind::RawGrenade: enabled = false; break;
        default: break;
        }
        if (!enabled) {
            plan.messages.push_back("disabled or alternate layout: " + path.filename().string());
            continue;
        }
        if (kind == Kind::GrenadeJP || kind == Kind::GrenadeEN) {
            try {
                const auto data = bytes(path);
                const auto meta = metadata(path, data);
                const auto layout = kind == Kind::GrenadeEN ? "english" : "japanese";
                if (meta.at("layout") != layout) throw std::runtime_error("wrong grenade layout");
                const auto addonDigits = digits(data, meta);
                for (const auto &[address, value] : addonDigits) {
                    if (value != '4' || (kind == Kind::GrenadeEN && !replacement.count(address))) throw std::runtime_error("grenade addon digit mismatch");
                }
                if (kind == Kind::GrenadeEN && meta.at("base_fingerprint") != missionFingerprint) throw std::runtime_error("grenade addon does not match English base");
            } catch (const std::exception &error) {
                plan.messages.push_back(std::string("grenade addon skipped; reinstall the matching grenade PPF and JSON companion: ") + error.what());
                continue;
            }
        }
        plan.patches.push_back({path, path == mission ? replacement : std::map<uint64_t, unsigned char>{}});
    }
    return plan;
}
} // namespace MGS1PatchOptions
