// Native tests exercise the SAME planner Ketchup calls, without starting MGS1.
// Optional argument: a directory of real VR PPFs and JSON companions. Emit all
// four English/grenade plans for verify_patch_options.py to apply to the discs.
#include "../../src/games/mgs1_patch_options.h"
#include "../../src/m2fix/patch_range.h"
#include "../../src/m2fix/ram_patch.h"
#include <chrono>
#include <iostream>

using namespace MGS1PatchOptions;
void require(bool pass, const char *message) { if (!pass) throw std::runtime_error(message); }

void synthetic_checks() {
    PatchRange range{981, 1, 100, 200};
    require(range.matches(981, 1, 100) && range.matches(981, 1, 199), "USA range boundaries");
    require(!range.matches(99, 1, 150) && !range.matches(981, 0, 150) &&
            !range.matches(981, 1, 200), "other titles/discs and end excluded");
    std::vector<Ketchup_RamPatch> patches{{100, {1, 2, 3}}, {101, {4}}};
    NormalizeRamPatches(patches);
    std::map<unsigned, unsigned char> ram{{100, 1}, {101, 0}, {102, 3}};
    auto read = [&](unsigned at) { return ram.at(at); };
    auto write = [&](unsigned at, unsigned char value) { ram.at(at) = value; };
    require(RepairRamPatches(patches, read, write) == 1 && ram[101] == 4,
            "interior corruption repaired with last mod winning");
    require(RepairRamPatches(patches, read, write) == 0, "intact image receives no writes");
    const auto dir = std::filesystem::temp_directory_path() /
        ("m2fix_synthetic_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code ec; std::filesystem::remove_all(path, ec); } } cleanup{dir};
    std::vector<unsigned char> data(60, 0);
    const std::string magic("PPF30\2", 6);
    std::copy(magic.begin(), magic.end(), data.begin());
    std::vector<uint64_t> offsets{100, 200, 300, 400, 500};
    for (auto at : offsets) {
        for (unsigned n = 0; n < 8; ++n) data.push_back(static_cast<unsigned char>(at >> (8 * n)));
        data.push_back(1); data.push_back('4');
    }
    const auto mission = dir / MissionFile;
    std::ofstream(mission, std::ios::binary).write(reinterpret_cast<const char *>(data.data()), data.size());
    std::ofstream(dir / "INTEGRAL_vr_en_items.ppf").put('\0');
    auto has = [](const Plan &plan, const char *name) {
        return std::any_of(plan.patches.begin(), plan.patches.end(), [&](const Patch &p) { return p.path.filename() == name; });
    };
    auto missing = prepare(dir, 99, "VR-DISK", 0, Settings{});
    require(!has(missing, MissionFile) && has(missing, "INTEGRAL_vr_en_items.ppf"), "missing companion preserves unrelated English screens");
    nlohmann::json meta{{"schema", 1}, {"fingerprint", fingerprint(data)}, {"grenade_digits", offsets}};
    std::ofstream(mission.string() + ".json") << meta;
    for (bool english : {false, true}) for (bool grenade : {false, true}) {
        Settings config; config.vrEnglish = english; config.grenade = grenade;
        auto plan = prepare(dir, 99, "VR-DISK", 0, config);
        require(has(plan, MissionFile) == english, "synthetic English selection");
        for (const auto &patch : plan.patches) if (patch.path == mission) {
            require(patch.overrides.size() == 5, "synthetic five digit overrides");
            for (auto [at, value] : patch.overrides) require(value == (grenade ? '4' : '5'), "synthetic grenade selection");
        }
    }
    meta["fingerprint"] = "stale";
    std::ofstream(mission.string() + ".json") << meta;
    auto stale = prepare(dir, 99, "VR-DISK", 0, Settings{});
    require(!has(stale, MissionFile) && has(stale, "INTEGRAL_vr_en_items.ppf"), "stale companion isolates failure");
    meta["grenade_digits"][1] = meta["grenade_digits"][0];
    bool rejected = false;
    try { digits(data, meta); } catch (const std::exception &) { rejected = true; }
    require(rejected, "synthetic duplicate offsets refused");
    meta["grenade_digits"] = offsets;
    data.pop_back(); rejected = false;
    try { digits(data, meta); } catch (const std::exception &) { rejected = true; }
    require(rejected, "synthetic truncated record refused");
}

int main(int argc, char **argv)
{
    try {
        synthetic_checks();
        Settings defaults;
        require(defaults.english && defaults.vrEnglish && defaults.grenade, "fix defaults");
        require(!defaults.missions && !defaults.extras && !defaults.movies && !defaults.titleBonuses, "unlock defaults");
        require(classify(99, "INTEGRAL", 0, "INTEGRAL_disc1_en_items.ppf") == Kind::English, "disc 1 English");
        require(classify(99, "INTEGRAL", 1, "INTEGRAL_disc2_en_abst.ppf") == Kind::English, "disc 2 English");
        require(classify(99, "INTEGRAL", 0, "INTEGRAL_disc2_en_abst.ppf") == Kind::Other, "wrong disc untouched");
        require(classify(981, "USA", 0, "INTEGRAL_disc1_en_items.ppf") == Kind::Other, "wrong title untouched");
        require(classify(99, "VR-DISK", 0, "INTEGRAL_vr_en_memcard.ppf") == Kind::VREnglish, "VR English family");
        require(classify(99, "VR-DISK", 0, "INTEGRAL_VR_EN_MISSIONS.PPF") == Kind::VREnglish, "case-insensitive name");
        require(classify(101, "USA", 0, "VRUS_unlock_missions.ppf") == Kind::Missions, "USA VR unlock");
        require(classify(99, "VR-DISK", 0, "INTEGRAL_vr_unlock_missions.ppf") == Kind::Missions, "Integral VR unlock");
        require(classify(99, "VR-DISK", 0, "INTEGRAL_vr_unlock_extras.ppf") == Kind::Extras, "extras unlock");
        require(classify(99, "VR-DISK", 0, "INTEGRAL_vr_unlock_movies.ppf") == Kind::Movies, "movies unlock");
        require(classify(99, "INTEGRAL", 1, "INTEGRAL_disc2_unlock_title.ppf") == Kind::Title, "Integral title unlock");
        require(classify(981, "USA", 0, "MGS1_disc1_unlock_title.ppf") == Kind::Title, "USA title unlock");
        require(classify(99, "VR-DISK", 0, "my_custom_patch.ppf") == Kind::Other, "unknown mod untouched");
        require(classify(99, "VR-DISK", 0, "INTEGRAL_vr_en_custom.ppf") == Kind::Other, "unknown family untouched");
        require(classify(99, "VR-DISK", 0, "INTEGRAL_vr_fix_grenade_delay_raw.ppf") == Kind::RawGrenade, "raw addon excluded");
        require(fingerprint({}) == "cbf29ce484222325", "FNV empty vector");
        require(fingerprint({'h','e','l','l','o'}) == "a430d84680aabd0b", "FNV known vector");
        if (argc == 1) { std::cout << "patch option unit checks passed\n"; return 0; }
        nlohmann::json output = nlohmann::json::array();
        for (bool english : {false, true}) for (bool grenade : {false, true}) {
            Settings config;
            config.vrEnglish = english;
            config.grenade = grenade;
            const auto plan = prepare(argv[1], 99, "VR-DISK", 0, config);
            nlohmann::json patches = nlohmann::json::array();
            bool hasMission = false;
            unsigned addons = 0;
            for (const auto &patch : plan.patches) {
                auto name = patch.path.filename().string();
                auto kind = classify(99, "VR-DISK", 0, name);
                if (name == MissionFile) {
                    hasMission = true;
                    require(patch.overrides.size() == 5, "five mission overrides");
                    for (const auto &[address, value] : patch.overrides) require(value == (grenade ? '4' : '5'), "correct override value");
                }
                if (kind == Kind::GrenadeJP || kind == Kind::GrenadeEN) {
                    ++addons;
                    require((kind == Kind::GrenadeEN) == english, "correct addon language");
                }
                require(kind != Kind::Missions && kind != Kind::Extras && kind != Kind::Movies, "unlocks default disabled");
                nlohmann::json overrides = nlohmann::json::array();
                for (const auto &[at, value] : patch.overrides) overrides.push_back({at, value});
                patches.push_back({{"file", name}, {"overrides", overrides}});
            }
            require(hasMission == english, "English toggle");
            require(addons == (grenade ? 1u : 0u), "grenade toggle");
            output.push_back({{"english", english}, {"grenade", grenade}, {"patches", patches}, {"messages", plan.messages}});
        }
        // Every unlock can independently select its corresponding installed aid.
        for (unsigned mask = 0; mask < 8; ++mask) {
            Settings config;
            config.vrEnglish = config.grenade = false;
            config.missions = (mask & 1) != 0; config.extras = (mask & 2) != 0; config.movies = (mask & 4) != 0;
            unsigned seen = 0;
            for (const auto &patch : prepare(argv[1], 99, "VR-DISK", 0, config).patches) {
                auto kind = classify(99, "VR-DISK", 0, patch.path.filename().string());
                if (kind == Kind::Missions) seen |= 1;
                if (kind == Kind::Extras) seen |= 2;
                if (kind == Kind::Movies) seen |= 4;
            }
            unsigned installed = 0;
            for (const auto &entry : std::filesystem::directory_iterator(argv[1])) {
                const auto kind = classify(99, "VR-DISK", 0, entry.path().filename().string());
                if (kind == Kind::Missions) installed |= 1;
                if (kind == Kind::Extras) installed |= 2;
                if (kind == Kind::Movies) installed |= 4;
            }
            require(seen == (mask & installed), "independent installed VR unlock controls");
        }
        const auto fixtures = std::filesystem::temp_directory_path() / "mgsm2fix_patch_options_tests";
        std::filesystem::create_directories(fixtures);
        for (const auto &name : {"INTEGRAL_disc1_en_items.ppf", "INTEGRAL_disc2_en_abst.ppf",
                                "INTEGRAL_disc1_unlock_title.ppf", "INTEGRAL_disc2_unlock_title.ppf",
                                "MGS1_disc1_unlock_title.ppf", "MGS1_disc2_unlock_title.ppf",
                                "VRUS_unlock_missions.ppf", "unrelated.ppf"}) std::ofstream(fixtures / name).put('\0');
        for (unsigned title : {99u, 981u, 101u}) for (unsigned disk : {0u, 1u}) {
            if (title == 101 && disk) continue;
            const std::string version = title == 99 ? "INTEGRAL" : "USA";
            for (bool enabled : {false, true}) {
                Settings config;
                config.english = config.titleBonuses = config.missions = enabled;
                unsigned controlled = 0;
                bool unrelated = false;
                for (const auto &patch : prepare(fixtures, title, version, disk, config).patches) {
                    const auto name = patch.path.filename().string();
                    const auto kind = classify(title, version, disk, name);
                    if (kind == Kind::English || kind == Kind::Title || kind == Kind::Missions) ++controlled;
                    if (name == "unrelated.ppf") unrelated = true;
                }
                require(unrelated, "unrelated mods always retained");
                require(controlled == (enabled ? (title == 99 ? 2u : 1u) : 0u), "main-disc and USA switches");
            }
        }
        const auto negative = fixtures / "invalid_companion";
        std::filesystem::create_directories(negative);
        const auto source = std::filesystem::path(argv[1]) / MissionFile;
        std::filesystem::copy_file(source, negative / MissionFile, std::filesystem::copy_options::overwrite_existing);
        auto data = bytes(source);
        auto meta = metadata(source, data);
        // A stale companion must disable the mission patch before
        // any PPF is applied, never guess a digit address in a different layout.
        meta["fingerprint"] = "stale";
        std::ofstream((negative / MissionFile).string() + ".json") << meta;
        auto invalid = prepare(negative, 99, "VR-DISK", 0, Settings{});
        for (const auto &patch : invalid.patches) require(patch.path.filename() != MissionFile, "stale base rejected");
        require(!invalid.messages.empty(), "stale base explained");
        // Duplicate/missing digit offsets and truncated records are rejected.
        meta = metadata(source, data);
        meta["grenade_digits"][1] = meta["grenade_digits"][0];
        bool rejected = false;
        try { digits(data, meta); } catch (const std::exception &) { rejected = true; }
        require(rejected, "duplicate digit offsets rejected");
        meta = metadata(source, data);
        data.pop_back();
        rejected = false;
        try { digits(data, meta); } catch (const std::exception &) { rejected = true; }
        require(rejected, "truncated PPF rejected");
        std::cout << output.dump(2) << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
