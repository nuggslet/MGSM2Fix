#pragma once
#include "core.hpp"
using Key = std::pair<std::string, std::string>;
struct Setting {
    Key key;
    std::string def, help, kind;
};
struct Parsed {
    std::map<Key, std::string> values;
    std::map<Key, size_t> locations;
};
inline std::vector<std::string> linesOf(const std::string &text) {
    std::vector<std::string> lines;
    size_t p = 0;
    while (p < text.size()) {
        auto end = text.find('\n', p);
        if (end == text.npos) {
            lines.push_back(text.substr(p));
            break;
        }
        lines.push_back(text.substr(p, end + 1 - p));
        p = end + 1;
    }
    return lines;
}
inline Parsed parseIni(const std::vector<std::string> &lines) {
    Parsed result;
    std::string section;
    for (size_t i = 0; i < lines.size(); ++i) {
        auto line = trim(lines[i]);
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        auto equal = line.find('=');
        if (equal == line.npos)
            continue;
        Key key = {section, trim(line.substr(0, equal))};
        need(!result.values.count(key), "Duplicate INI entry: " + key.second);
        auto value = line.substr(equal + 1);
        value = value.substr(0, value.find_first_of(";#"));
        result.values[key] = trim(value);
        result.locations[key] = i;
    }
    return result;
}
inline std::vector<Setting> schema() {
    auto bytes = resource(102);
    std::string text(bytes.begin(), bytes.end()), section, comments;
    std::vector<Setting> result;
    for (auto line : linesOf(text)) {
        line = trim(line);
        if (line.empty()) {
            comments.clear();
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            comments.clear();
            continue;
        }
        if (line[0] == ';') {
            comments += trim(line.substr(1)) + "\r\n";
            continue;
        }
        auto equal = line.find('=');
        if (equal == line.npos)
            continue;
        auto name = trim(line.substr(0, equal)), value = trim(line.substr(equal + 1));
        std::string kind =
            (value == "true" || value == "false") ? "bool"
            : (!value.empty() && std::all_of(value.begin(), value.end(),
                                             [](auto c) { return isdigit((unsigned char)c); }))
                ? "int"
                : "text";
        if (name == "StageSelect" || name == "BrightnessText")
            kind = "choice";
        result.push_back({{section, name}, value, comments, kind});
        comments.clear();
    }
    result.push_back({{"Fixes", "DisableWindowsSlideshowWarning"},
                      "false",
                      "Disable the Windows slideshow / Spotlight warning.",
                      "bool"});
    return result;
}
inline std::string validate(const Setting &spec, std::string value) {
    value = trim(value);
    need(value.find_first_of("\r\n;#") == value.npos, "Invalid characters in " + spec.key.second);
    if (spec.kind == "bool")
        need(value == "true" || value == "false", "Choose true or false for " + spec.key.second);
    if (spec.kind == "int") {
        need(!value.empty() &&
                 std::all_of(value.begin(), value.end(),
                             [](auto c) { return isdigit((unsigned char)c); }) &&
                 value.size() < 11,
             "Enter a non-negative integer for " + spec.key.second);
        auto n = std::stoull(value);
        need(n <= 2147483647, "Integer too large");
        if (spec.key.second == "Port")
            need(n > 0 && n <= 65535, "Port must be 1 to 65535");
    }
    if (spec.key.second == "BrightnessText")
        need(value == "fixed" || value == "original" || value == "collection" || value == "true" ||
                 value == "false",
             "Invalid brightness setting");
    if (spec.key.second == "StageSelect")
        need(value.empty() || std::regex_match(value, std::regex("[A-Za-z0-9_]{1,7}")),
             "Stage must be up to 7 letters/digits/underscores");
    if (spec.key.second == "GiveItems" || spec.key.second == "GiveWeapons") {
        size_t p = 0;
        while (p < value.size()) {
            auto end = value.find(',', p);
            auto token = trim(value.substr(p, end == value.npos ? value.size() - p : end - p));
            need(!token.empty() && token.size() < 3 &&
                     std::all_of(token.begin(), token.end(),
                                 [](auto c) { return isdigit((unsigned char)c); }) &&
                     std::stoi(token) <= (spec.key.second == "GiveItems" ? 23 : 9),
                 "Invalid comma-separated item/weapon IDs");
            if (end == value.npos)
                break;
            p = end + 1;
            need(p < value.size(), "Trailing comma in item/weapon IDs");
        }
    }
    return value;
}
struct Ini {
    fs::path path;
    Bytes original;
    UINT encoding = CP_UTF8;
    bool bom = false;
    std::vector<std::string> lines;
    std::map<Key, std::string> values, loaded;
    void load(fs::path p = {}) {
        Ini next;
        next.path = p.empty() ? p : fs::absolute(p);
        next.original = p.empty() ? resource(102) : readFile(p);
        next.bom =
            next.original.size() >= 3 && next.original[0] == 239 && next.original[1] == 187 && next.original[2] == 191;
        std::string raw(next.original.begin() + (next.bom ? 3 : 0), next.original.end());
        next.encoding = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, raw.data(), (int)raw.size(),
                                       nullptr, 0)
                       ? CP_UTF8
                       : 1252;
        auto text = next.encoding == CP_UTF8 ? raw : utf8(wide(raw, next.encoding));
        next.lines = linesOf(text);
        next.values = parseIni(next.lines).values;
        next.loaded = next.values;
        *this = std::move(next);
    }
    bool dirty() const { return values != loaded; }
    Bytes render() {
        auto output = lines;
        std::string nl = "\n";
        for (auto &line : lines)
            if (ends(line, "\r\n")) {
                nl = "\r\n";
                break;
            }
        for (auto &[key, value] : values) {
            if (loaded.count(key) && loaded.at(key) == value)
                continue;
            auto parsed = parseIni(output);
            if (parsed.locations.count(key)) {
                auto &line = output[parsed.locations[key]];
                auto eq = line.find('=');
                size_t begin = eq + 1;
                while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t'))
                    ++begin;
                auto end = line.find_first_of(";#\r\n", begin);
                if (end == line.npos)
                    end = line.size();
                while (end > begin && (line[end - 1] == ' ' || line[end - 1] == '\t'))
                    --end;
                line = line.substr(0, begin) + value + line.substr(end);
            } else {
                size_t start = output.size(), end = output.size();
                for (size_t i = 0; i < output.size(); ++i)
                    if (trim(output[i]) == "[" + key.first + "]") {
                        start = i;
                        break;
                    }
                if (start < output.size())
                    for (size_t i = start + 1; i < output.size(); ++i)
                        if (trim(output[i]).find('[') == 0) {
                            end = i;
                            break;
                        }
                if (end && !ends(output[end - 1], "\n"))
                    output[end - 1] += nl;
                if (start == output.size()) {
                    output.push_back(nl + "[" + key.first + "]" + nl);
                    end = output.size();
                }
                output.insert(output.begin() + end, key.second + " = " + value + nl);
            }
        }
        std::string text;
        for (auto &line : output)
            text += line;
        if (encoding != CP_UTF8)
            text = utf8(wide(text), encoding);
        Bytes b(text.begin(), text.end());
        if (bom)
            b.insert(b.begin(), {239, 187, 191});
        return b;
    }
    void save(fs::path target) {
        target = fs::absolute(target);
        bool same = !path.empty() && lower(str(target)) == lower(str(path));
        if (same)
            need(fs::exists(target) && readFile(target) == original,
                 "INI changed outside the app. Reload before saving.");
        else
            need(!fs::exists(target),
                 "Save As needs a new filename; open an existing INI to edit it.");
        auto bytes = render();
        if (same && bytes == original)
            return;
        if (same) {
            auto backup = target;
            backup += L".bak";
            for (int i = 1; fs::exists(backup); ++i) {
                backup = target;
                backup += L".bak." + std::to_wstring(i);
            }
            need(CopyFileW(target.c_str(), backup.c_str(), TRUE), "Cannot back up INI");
        }
        Temp temp(target.parent_path());
        auto pending = temp.path / L"settings.ini";
        writeFile(pending, bytes);
        if (same)
            need(readFile(target) == original, "INI changed while saving");
        need(MoveFileExW(pending.c_str(), target.c_str(),
                         MOVEFILE_WRITE_THROUGH | (same ? MOVEFILE_REPLACE_EXISTING : 0)),
             "Cannot save INI");
        load(target);
    }
};
inline std::wstring registry(HKEY hive, const wchar_t *key, const wchar_t *name) {
    wchar_t value[32768];
    DWORD n = sizeof(value);
    if (RegGetValueW(hive, key, name, RRF_RT_REG_SZ, nullptr, value, &n) == ERROR_SUCCESS)
        return value;
    return {};
}
inline fs::path findGameUncached() {
    wchar_t explicitPath[32768];
    if (GetEnvironmentVariableW(L"INTEGRAL_ENGLISH_GAME", explicitPath, 32768)) {
        fs::path p(explicitPath);
        if (fs::exists(p / L"windata/alldata.bin"))
            return p;
        return {};
    }
    std::vector<fs::path> roots;
    for (auto hive : {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE})
        for (auto name : {L"SteamPath", L"InstallPath"}) {
            auto p = registry(hive,
                              hive == HKEY_CURRENT_USER ? L"Software\\Valve\\Steam"
                                                        : L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
                              name);
            if (!p.empty())
                roots.push_back(p);
        }
    roots.push_back(L"C:/Program Files (x86)/Steam");
    roots.push_back(L"C:/Program Files/Steam");
    for (wchar_t c = L'A'; c <= L'Z'; ++c)
        roots.push_back(std::wstring(1, c) + L":/Steam");
    std::set<std::wstring> seen;
    for (size_t i = 0; i < roots.size(); ++i) {
        auto root = roots[i];
        if (!seen.insert(root.wstring()).second)
            continue;
        auto game = root / L"steamapps/common/MGS1";
        if (fs::exists(game / L"windata/alldata.bin"))
            return game;
        for (auto vdf : {L"steamapps/libraryfolders.vdf", L"config/libraryfolders.vdf"})
            if (fs::exists(root / vdf)) {
                auto text = textFile(root / vdf);
                std::regex pattern("\"path\"\\s*\"([^\"]+)\"");
                for (std::sregex_iterator it(text.begin(), text.end(), pattern), end; it != end;
                     ++it) {
                    auto p = (*it)[1].str();
                    for (size_t n = 0; (n = p.find("\\\\", n)) != p.npos;)
                        p.replace(n, 2, "/");
                    if (roots.size() < 256)
                        roots.push_back(wide(p));
                }
            }
    }
    return {};
}
inline fs::path findGame() {
    static const fs::path detected = findGameUncached();
    return detected;
}
inline bool enabled(const Ini &ini, Key key, bool fallback = false) {
    auto it = ini.values.find(key);
    if (it == ini.values.end())
        return fallback;
    auto v = lower(it->second);
    return v == "true" || v == "1" || v == "yes" || v == "on";
}
struct Missing {
    std::string label;
    std::vector<std::string> paths;
    bool buildable = true;
};
inline std::vector<Missing> missing(const Ini &ini) {
    static Json manifest = []() {
        auto bytes = unpackRecipes();
        Zip z(bytes);
        return Json::parse(z.read("manifest.json"));
    }();
    std::vector<Missing> groups;
    if (enabled(ini, {"Patches", "DisableCDROM"}))
        return groups;
    for (auto &pair : std::vector<std::pair<std::string, std::string>>{
             {"IntegralEnglishPatch", "Story English"}, {"IntegralVREnglishPatch", "VR English"}})
        if (enabled(ini, {"Patches", pair.first}, true)) {
            Missing g{pair.second};
            for (auto &[name, info] : manifest["packages"]["collection"].items())
                if ((ends(name, ".ppf") || ends(name, ".ppf.json")) &&
                    (pair.first == "IntegralEnglishPatch"
                         ? name.find("/INTEGRAL/INTEGRAL/") != name.npos
                         : name.find("/INTEGRAL_vr_en_") != name.npos))
                    g.paths.push_back(name);
            groups.push_back(g);
        }
    if (enabled(ini, {"Patches", "GrenadeDelayFix"}, true)) {
        auto n = "mods/INTEGRAL/VR-DISK/INTEGRAL_vr_fix_grenade_delay" +
                 std::string(enabled(ini, {"Patches", "IntegralVREnglishPatch"}, true) ? "_english"
                                                                                       : "") +
                 ".ppf";
        groups.push_back({"Grenade display correction", {n, n + ".json"}});
    }
    for (auto suffix : {"Missions", "Extras", "Movies"})
        if (enabled(ini, {"Game", std::string("UnlockVR") + suffix})) {
            Missing g{std::string("VR ") + suffix + " unlock",
                      {"mods/INTEGRAL/VR-DISK/INTEGRAL_vr_unlock_" + lower(suffix) + ".ppf"},
                      false};
            if (std::string(suffix) == "Missions")
                g.paths.push_back("mods/VR-DISK_US/VRUS_unlock_missions.ppf");
            groups.push_back(g);
        }
    if (enabled(ini, {"Game", "UnlockTitleBonuses"})) {
        Missing g{"Title bonuses unlock", {}, false};
        for (int i = 0; i < 2; ++i) {
            g.paths.push_back("mods/INTEGRAL/INTEGRAL/" + std::to_string(i) + "/INTEGRAL_disc" +
                              std::to_string(i + 1) + "_unlock_title.ppf");
            g.paths.push_back("mods/MGS1_US/" + std::to_string(i) + "/MGS1_disc" +
                              std::to_string(i + 1) + "_unlock_title.ppf");
        }
        groups.push_back(g);
    }
    std::vector<Missing> result;
    for (auto &group : groups) {
        std::vector<std::string> absent;
        for (auto &n : group.paths) {
            std::error_code e;
            auto p = ini.path.parent_path() / wide(n);
            if (ini.path.empty() || !fs::is_regular_file(p, e) || fs::file_size(p, e) == 0 || e)
                absent.push_back(n);
        }
        if (!absent.empty()) {
            group.paths = absent;
            result.push_back(group);
        }
    }
    return result;
}
inline fs::path install(const fs::path &package, const fs::path &game) {
    need(fs::is_regular_file(game / L"METAL GEAR SOLID.exe") &&
             fs::is_regular_file(game / L"windata/alldata.bin"),
         "Select MGS1 Master Collection containing METAL GEAR SOLID.exe and "
         "windata/alldata.bin");
    std::map<std::string, Bytes> files;
    std::set<std::string> seen;
    for (auto line : linesOf(textFile(package / L"SHA256SUMS.txt"))) {
        line = trim(line);
        auto split = line.find("  ");
        need(split != line.npos, "Invalid hash manifest");
        auto name = line.substr(split + 2);
        safePath(game, name);
        if (name.find("mods/") != 0 || !(ends(name, ".ppf") || ends(name, ".ppf.json")))
            continue;
        need(seen.insert(lower(name)).second, "Duplicate package file");
        auto b = readFile(safePath(package, name));
        need(digest(b) == line.substr(0, split), "Patch failed verification: " + name);
        files[name] = std::move(b);
    }
    need(!files.empty(), "No PPF files in package");
    auto backup = temporary(game, L"M2FixGUI-patch-backup-");
    Temp temp(game);
    std::vector<std::string> changed;
    try {
        std::string list;
        for (auto &[name, b] : files) {
            auto target = safePath(game, name);
            if (fs::exists(target)) {
                need(fs::is_regular_file(target), "Install target is not a file");
                auto saved = safePath(backup, name);
                fs::create_directories(saved.parent_path());
                fs::copy_file(target, saved);
            }
            writeFile(safePath(temp.path, name), b);
            list += name + "\n";
        }
        writeText(backup / L"installed-files.txt", list);
        for (auto &[name, b] : files) {
            auto target = safePath(game, name);
            fs::create_directories(target.parent_path());
            need(MoveFileExW(safePath(temp.path, name).c_str(), target.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH),
                 "Cannot install " + name);
            changed.push_back(name);
        }
    } catch (...) {
        bool restored = true;
        for (auto it = changed.rbegin(); it != changed.rend(); ++it) {
            auto target = safePath(game, *it), saved = safePath(backup, *it);
            std::error_code e;
            if (fs::exists(saved))
                fs::copy_file(saved, target, fs::copy_options::overwrite_existing, e);
            else
                fs::remove(target, e);
            restored &= !e;
        }
        if (!restored)
            throw std::runtime_error("Install failed and some backups could not be "
                                     "restored. Recover from " +
                                     str(backup));
        throw;
    }
    return backup;
}
