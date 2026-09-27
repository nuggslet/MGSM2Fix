#include "settings.hpp"
#include <iostream>
int main() {
    try {
        Temp work(fs::temp_directory_path());
        auto bytes = unpackRecipes();
        Zip recipes(bytes);
        auto manifest = Json::parse(recipes.read("manifest.json"));
        need(manifest["format"] == 1, "Recipe version");
        for (auto variant : {"collection", "raw"})
            for (auto &[name, info] : manifest["packages"][variant].items())
                if (!ends(name, ".ppf"))
                    need(digest(recipes.read(std::string(variant) + "/" + name)) == info["sha256"],
                         "Recipe metadata hash");
        Options japanese;
        japanese.images = true;
        japanese.story = japanese.vr = japanese.english = false;
        need(outputDirectoryName(japanese) == L"Integral-images" &&
                 imageFilename(japanese, "vr") == "Integral vr.bin",
             "Japanese image names must not claim English content");
        japanese.images = false;
        japanese.grenade = true;
        need(outputDirectoryName(japanese) == L"Integral-patches",
             "Japanese grenade package name");
        Options english;
        need(outputDirectoryName(english) == L"Integral-English-patches" &&
                 imageFilename(english, "1") == "Integral English 1.bin",
             "Default English names");
        auto multiBin = work.path / L"multi-bin-source";
        fs::create_directory(multiBin);
        {
            Zip archive(multiBin / L"unrelated.zip", true);
            archive.add("track1.bin", Bytes{1});
            archive.add("track2.bin", Bytes{2});
            archive.finish();
        }
        Options scan;
        scan.images = true;
        scan.steam = false;
        scan.source = multiBin;
        scan.destination = work.path;
        std::string scanError;
        try {
            build(scan, [](const std::string &) {});
        } catch (const std::exception &e) {
            scanError = e.what();
        }
        need(scanError.find("Missing Integral discs") != std::string::npos,
             "Folder scan should skip unrelated multi-BIN ZIPs");
        Bytes sector(2352);
        sector[15] = 2;
        for (size_t i = 24; i < 2072; ++i)
            sector[i] = (unsigned char)(i * 7);
        parity(sector);
        need(digest(sector) == "93f68b0d1a596ad97d27a18faf76aacfa47af4b48c4f298b690d7f3dfd9dafd2",
             "Sector parity differs from reference");
        auto path = work.path / L"MGSM2Fix.ini";
        std::string original = "; caf\xe9\r\n[Patches]\r\nIntegralEnglishPatch = false ; "
                               "keep\r\nIntegralVREnglishPatch = false\r\nGrenadeDelayFix = "
                               "false\r\n[Unknown]\r\ncustom=untouched";
        writeText(path, original);
        Ini ini;
        ini.load(path);
        need(ini.encoding == 1252, "Legacy encoding");
        need(missing(ini).empty(), "Disabled PPFs should not warn");
        ini.values[{"Patches", "IntegralEnglishPatch"}] = "true";
        need(missing(ini).size() == 1, "Missing story PPF warning");
        ini.save(path);
        need(textFile(fs::path(path.wstring() + L".bak")) == original, "INI backup");
        auto expected = original;
        expected.replace(expected.find("false"), 5, "true");
        need(textFile(path) == expected,
             "Preserve comments, whitespace, encoding, unknown entries");
        writeText(path, expected + "\r\n; external");
        bool rejected = false;
        try {
            ini.save(path);
        } catch (...) {
            rejected = true;
        }
        need(rejected, "External INI edit not detected");
        ini.load();
        ini.path = work.path / L"checks" / L"MGSM2Fix.ini";
        auto required = missing(ini);
        need(required.size() == 3, "Default enabled feature groups");
        for(auto& group:required) for(auto& name:group.paths)
            writeText(ini.path.parent_path()/wide(name), "present");
        need(missing(ini).empty(), "Installed files should clear the notice");
        fs::remove(ini.path.parent_path()/L"mods/INTEGRAL/VR-DISK/INTEGRAL_vr_en_missions.ppf.json");
        need(missing(ini).size()==1, "Missing JSON companion must warn");

        for (auto &d : schema())
            if (d.key.second == "StageSelect")
                need(validate(d, "s11a") == "s11a", "Stage selection");
        auto game = work.path / L"MGS1", package = work.path / L"package";
        writeText(game / L"METAL GEAR SOLID.exe", "");
        writeText(game / L"windata/alldata.bin", "");
        std::string name = "mods/INTEGRAL/test.ppf";
        writeText(package / wide(name), "new");
        writeText(package / wide(name + ".json"), "companion");
        writeText(package / L"SHA256SUMS.txt",
                  digest(readFile(package / wide(name))) + "  " + name + "\n" +
                      digest(readFile(package / wide(name + ".json"))) + "  " + name + ".json\n");
        writeText(game / wide(name), "old");
        writeText(game / L"mods/other.ppf", "keep");
        auto backup = install(package, game);
        need(textFile(backup / wide(name)) == "old" && textFile(game / wide(name)) == "new" &&
                 textFile(game / L"mods/other.ppf") == "keep",
             "Install/backups/preserve other mods");
        writeText(package / wide(name), "bad");
        rejected = false;
        try {
            install(package, game);
        } catch (...) {
            rejected = true;
        }
        need(rejected && textFile(game / wide(name)) == "new",
             "Corrupt package touched installation");
        rejected = false;
        try {
            safePath(game, "mods/../../outside.ppf");
        } catch (...) {
            rejected = true;
        }
        need(rejected, "Path traversal");

        writeText(package / wide(name), "new");
        writeText(game / wide(name), "before rollback");
        HANDLE locked = CreateFileW((game / wide(name + ".json")).c_str(), GENERIC_READ,
                                    FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        need(locked != INVALID_HANDLE_VALUE, "Test lock");
        rejected = false;
        try {
            install(package, game);
        } catch (...) {
            rejected = true;
        }
        CloseHandle(locked);
        need(rejected && textFile(game / wide(name)) == "before rollback",
             "Install rollback after partial write");

        // A failed open must not partially replace the live document.
        Ini active;active.load(path);auto previous=active;
        auto bad=work.path/L"duplicate.ini";writeText(bad,"[Game]\nEnglishText=true\nEnglishText=false\n");
        rejected=false;try{active.load(bad);}catch(...){rejected=true;}
        need(rejected&&active.path==previous.path&&active.original==previous.original&&active.values==previous.values,
             "Failed INI open changed the active document");
        for(auto newline:{std::string("\n"),std::string("\r\n")}) {
            auto unicode=work.path/L"\u65e5\u672c\u8a9e settings.ini";
            std::string content="\xef\xbb\xbf; preserved"+newline+"[Game]"+newline+"GiveItems = "+newline+"EnglishText = false ; comment"+newline;
            writeText(unicode,content);Ini document;document.load(unicode);
            need(document.render()==document.original,"No-op INI round trip");
            document.values[{"Game","GiveItems"}]="0, 12";
            document.values[{"Game","EnglishText"}]="true";
            document.values[{"New Section","Added"}]="value";
            document.save(unicode);need(document.bom&&document.values.at({"Game","GiveItems"})=="0, 12","BOM/empty entry edit");
            need(textFile(unicode).find("true ; comment")!=std::string::npos,"Inline comment preservation");
        }
        // Read-only destination must leave the original file unchanged.
        auto lockedIni=work.path/L"readonly.ini";writeText(lockedIni,"[Game]\nEnglishText=false\n");
        Ini readonly;readonly.load(lockedIni);readonly.values[{"Game","EnglishText"}]="true";
        SetFileAttributesW(lockedIni.c_str(),FILE_ATTRIBUTE_READONLY);
        rejected=false;try{readonly.save(lockedIni);}catch(...){rejected=true;}
        SetFileAttributesW(lockedIni.c_str(),FILE_ATTRIBUTE_NORMAL);
        need(rejected&&textFile(lockedIni)=="[Game]\nEnglishText=false\n","Read-only INI protection");
        for(auto& d:schema()) {
            if(d.kind=="bool") {rejected=false;try{validate(d,"maybe");}catch(...){rejected=true;}need(rejected,"Boolean validation");}
            if(d.kind=="int") {rejected=false;try{validate(d,"-1");}catch(...){rejected=true;}need(rejected,"Integer validation");}
            if(d.key.second=="GiveWeapons") {rejected=false;try{validate(d,"10");}catch(...){rejected=true;}need(rejected,"Weapon range validation");}
        }
        std::cout << "Native tests passed: recipes, sector parity, INI preservation/conflicts, "
                     "missing PPFs, installer/backups, corrupt input and traversal.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
