#include "m2fix.h"
#include "m2config.h"

void M2Config::Load()
{
    std::array<std::string, 4> directories = { "", "plugins", "scripts", "update" };
    std::filesystem::path directory;
    auto name = M2Fix::FixName();
    auto base = M2Hook::GetInstance().ModuleLocation().parent_path();
    for (const auto & path : directories)
    {
        if (std::filesystem::exists(base / path / (name + "64.asi")) ||
            std::filesystem::exists(base / path / (name + "32.asi")) ||
            std::filesystem::exists(base / path / (name + ".asi")))
        {
            directory = path;
            break;
        }
    }

    // Initialise config
    std::ifstream iniFile(base / directory / M2Fix::ConfigFile());
    auto & ini = GetInstance().m_ini;
    if (!iniFile)
    {
        spdlog::critical("Failed to load config file.");
        spdlog::critical("Make sure {} is present in the game folder.", M2Fix::ConfigFile());
    }
    else
    {
        ini.parse(iniFile);
        if (!ini.errors.empty())
        {
            spdlog::error("Error parsing ini file, encountered {} errors at these lines:", ini.errors.size());
            std::cout << "Error parsing ini file, encountered " << ini.errors.size() << " errors at these lines:" << std::endl;
            for (auto err : ini.errors)
            {
                spdlog::error(err);
                std::cout << err << std::endl;
            }
        }
    }

    inipp::get_value(ini.sections["Squirrel Debugger"], "Enabled", bDebuggerEnabled);
    inipp::get_value(ini.sections["Squirrel Debugger"], "Port", iDebuggerPort);
    inipp::get_value(ini.sections["Squirrel Debugger"], "AutoUpdate", bDebuggerAutoUpdate);
    inipp::get_value(ini.sections["Squirrel Debugger"], "Exclusive", bDebuggerExclusive);

    {
        bool _bSmoothing;
        if (inipp::get_value(ini.sections["Screen"], "Smoothing", _bSmoothing))
            bSmoothing = _bSmoothing;
    }
    {
        bool _bScanline;
        if (inipp::get_value(ini.sections["Screen"], "Scanline", _bScanline))
            bScanline = _bScanline;
    }
    {
        bool _bDotMatrix;
        if (inipp::get_value(ini.sections["Screen"], "DotMatrix", _bDotMatrix))
            bDotMatrix = _bDotMatrix;
    }

    inipp::get_value(ini.sections["Tracing"], "Break", bBreak);
    inipp::get_value(ini.sections["Tracing"], "Console", bConsole);
    inipp::get_value(ini.sections["Tracing"], "Error", bError);
    inipp::get_value(ini.sections["Tracing"], "Level", iLevel);
    inipp::get_value(ini.sections["Tracing"], "NativeLevel", iNativeLevel);
    inipp::get_value(ini.sections["Tracing"], "EmulatorLevel", iEmulatorLevel);
    inipp::get_value(ini.sections["Tracing"], "RendererLevel", iRendererLevel);

    for (auto & section : { "Custom Resolution", "External Resolution" }) {
        inipp::get_value(ini.sections[section], "Enabled", bExternalEnabled);
        inipp::get_value(ini.sections[section], "Width", iExternalWidth);
        inipp::get_value(ini.sections[section], "Height", iExternalHeight);
        inipp::get_value(ini.sections[section], "Windowed", bExternalWindowed);
        inipp::get_value(ini.sections[section], "Borderless", bExternalBorderless);
    }

    inipp::get_value(ini.sections["Internal Resolution"], "Enabled", bInternalEnabled);
    inipp::get_value(ini.sections["Internal Resolution"], "Height", iInternalHeight);
    inipp::get_value(ini.sections["Internal Resolution"], "Widescreen", bInternalWidescreen);
    inipp::get_value(ini.sections["Internal Resolution"], "Borderless", bInternalBorderless);

    {
        bool _bAnalog;
        if (inipp::get_value(ini.sections["Input"], "Analog", _bAnalog))
            bAnalog = _bAnalog;
    }
    {
        bool _bSwapSticks;
        if (inipp::get_value(ini.sections["Input"], "SwapSticks", _bSwapSticks))
            bSwapSticks = _bSwapSticks;
    }

    inipp::get_value(ini.sections["Input"], "RemoveDeadzone", bRemoveDeadzone);

    inipp::get_value(ini.sections["Launcher"], "SkipNotice", bLauncherSkipNotice);
    inipp::get_value(ini.sections["Launcher"], "StartGame", bLauncherStartGame);

    inipp::get_value(ini.sections["Patches"], "DisableRAM", bPatchesDisableRAM);
    inipp::get_value(ini.sections["Patches"], "DisableCDROM", bPatchesDisableCDROM);

    inipp::get_value(ini.sections["Patches"], "DisableFont", bPatchesDisableFont);

    inipp::get_value(ini.sections["Patches"], "RemoveUnderpants", bPatchesRemoveUnderpants);
    inipp::get_value(ini.sections["Patches"], "EnableMosaic", bPatchesEnableMosaic);
    inipp::get_value(ini.sections["Patches"], "RestoreGhosts", bPatchesRestoreGhosts);
    inipp::get_value(ini.sections["Patches"], "RestoreMedicine", bPatchesRestoreMedicine);
    inipp::get_value(ini.sections["Patches"], "ThinTexturedQuads", bPatchesThinTexturedQuads);
    {
        // Tri-state, and it accepts a bool for anyone who assumes one: `true`
        // is the game's own text, which is what the old RestoreBrightnessText
        // meant, and `false` leaves the collection's version alone.
        std::string mode;
        if (inipp::get_value(ini.sections["Patches"], "BrightnessText", mode)) {
            for (auto &c : mode) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if      (mode == "fixed")                        eBrightnessText = M2BrightnessText::Fixed;
            else if (mode == "original" || mode == "true")    eBrightnessText = M2BrightnessText::Original;
            else if (mode == "collection" || mode == "false") eBrightnessText = M2BrightnessText::Collection;
            else spdlog::warn("[Config] BrightnessText: '{}' is not one of fixed / original /"
                              " collection, keeping the default.", mode);
        }
    }
    inipp::get_value(ini.sections["Patches"], "PreserveConfiguration", bPatchesPreserveConfiguration);
    inipp::get_value(ini.sections["Patches"], "IntegralEnglishPatch", bPatchesIntegralEnglish);
    inipp::get_value(ini.sections["Patches"], "IntegralVREnglishPatch", bPatchesIntegralVREnglish);
    inipp::get_value(ini.sections["Patches"], "GrenadeDelayFix", bPatchesGrenadeDelay);
    inipp::get_value(ini.sections["Game"], "UnlockVRMissions", bGameUnlockVRMissions);
    inipp::get_value(ini.sections["Game"], "UnlockVRExtras", bGameUnlockVRExtras);
    inipp::get_value(ini.sections["Game"], "UnlockVRMovies", bGameUnlockVRMovies);
    inipp::get_value(ini.sections["Game"], "UnlockTitleBonuses", bGameUnlockTitleBonuses);
    spdlog::info("[Config] IntegralEnglishPatch={}, IntegralVREnglishPatch={}, GrenadeDelayFix={}",
        bPatchesIntegralEnglish, bPatchesIntegralVREnglish, bPatchesGrenadeDelay);
    spdlog::info("[Config] UnlockVRMissions={}, UnlockVRExtras={}, UnlockVRMovies={}, UnlockTitleBonuses={}",
        bGameUnlockVRMissions, bGameUnlockVRExtras, bGameUnlockVRMovies, bGameUnlockTitleBonuses);

    {
        // StageSelect is a bool that also accepts a menu name: `true` opens the
        // developer top menu ("select" - TITLE / DEMO ALL / SOUND TEST), and a
        // name such as `select3` opens that stage-list menu directly, since the
        // retail top menu does not link to the four stage lists.
        std::string v;
        if (inipp::get_value(ini.sections["Game"], "StageSelect", v)) {
            std::string lower = v;
            for (auto &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") { bGameStageSelect = true; sGameStageSelect = "select"; }
            else if (lower == "false" || lower == "0" || lower == "no" || lower == "off" || lower.empty()) { bGameStageSelect = false; sGameStageSelect.clear(); }
            else if (lower.size() <= 7) { bGameStageSelect = true; sGameStageSelect = lower; }
            else spdlog::warn("[Config] StageSelect: '{}' is longer than a stage name (7 chars), ignored.", v);
        }
    }
    inipp::get_value(ini.sections["Game"], "EnglishText", bGameEnglishText);
    inipp::get_value(ini.sections["Game"], "UnlockBriefing", bGameUnlockBriefing);
    {
        // Comma-separated ids: items are MGS1's IT_* numbering (Camera is 12),
        // weapons its WP_* numbering, which is its own list of ten and not the
        // order the menu shows.
        auto ids = [](const std::string &list, const char *what, int limit,
                      std::vector<int> &out) {
            std::stringstream ss(list);
            std::string tok;
            while (std::getline(ss, tok, ',')) {
                try {
                    int id = std::stoi(tok);
                    if (id >= 0 && id < limit) out.push_back(id);
                    else spdlog::warn("[Config] {}: {} is not an id (0..{}), ignored.", what, id, limit - 1);
                } catch (...) {
                    if (!tok.empty() && tok.find_first_not_of(" \t") != std::string::npos)
                        spdlog::warn("[Config] {}: '{}' is not a number, ignored.", what, tok);
                }
            }
        };
        std::string list;
        if (inipp::get_value(ini.sections["Game"], "GiveItems", list))
            ids(list, "GiveItems", 24, vGameGiveItems);
        list.clear();
        if (inipp::get_value(ini.sections["Game"], "GiveWeapons", list))
            ids(list, "GiveWeapons", 10, vGameGiveWeapons);
    }

    inipp::get_value(ini.sections["Update Notifications"], "CheckForUpdates", bShouldCheckForUpdates);
    inipp::get_value(ini.sections["Update Notifications"], "ConsoleNotifications", bConsoleUpdateNotifications);

    inipp::get_value(ini.sections["Fixes"], "DisableWindowsFullscreenOptimization", bDisableWindowsFullscreenOptimization);
    inipp::get_value(ini.sections["Fixes"], "DisableWindowsSlideshowWarning", bDisableWindowsSlideshowWarning);

    if (bExternalBorderless) {
        bExternalWindowed = true;
    }

    if (iExternalWidth <= 0 || iExternalHeight <= 0) {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        iExternalWidth  = static_cast<int>(desktop.right);
        iExternalHeight = static_cast<int>(desktop.bottom);
    }

    if (iInternalHeight <= 0) {
        iInternalHeight = iExternalHeight;
    }

    sFullscreenMode = bExternalWindowed ? "0" : "1";
    sExternalWidth  = std::to_string(iExternalWidth);
    sExternalHeight = std::to_string(iExternalHeight);

    // Log config parse
    spdlog::info("[Config] bDebuggerEnabled: {}", bDebuggerEnabled);
    spdlog::info("[Config] iDebuggerPort: {}", iDebuggerPort);
    spdlog::info("[Config] bDebuggerAutoUpdate: {}", bDebuggerAutoUpdate);
    spdlog::info("[Config] bDebuggerExclusive: {}", bDebuggerExclusive);
    if (bSmoothing) spdlog::info("[Config] bSmoothing: {}", *bSmoothing);
    if (bScanline)  spdlog::info("[Config] bScanline: {}", *bScanline);
    if (bDotMatrix) spdlog::info("[Config] bDotMatrix: {}", *bDotMatrix);
    spdlog::info("[Config] bBreak: {}", bBreak);
    spdlog::info("[Config] bConsole: {}", bConsole);
    spdlog::info("[Config] bError: {}", bError);
    spdlog::info("[Config] iLevel: {}", iLevel);
    spdlog::info("[Config] iNativeLevel: {}", iNativeLevel);
    spdlog::info("[Config] iEmulatorLevel: {}", iEmulatorLevel);
    spdlog::info("[Config] iRendererLevel: {}", iRendererLevel);
    spdlog::info("[Config] bExternalEnabled: {}", bExternalEnabled);
    spdlog::info("[Config] iExternalWidth: {}", iExternalWidth);
    spdlog::info("[Config] iExternalHeight: {}", iExternalHeight);
    spdlog::info("[Config] bExternalWindowed: {}", bExternalWindowed);
    spdlog::info("[Config] bExternalBorderless: {}", bExternalBorderless);
    spdlog::info("[Config] bInternalEnabled: {}", bInternalEnabled);
    spdlog::info("[Config] iInternalHeight: {}", iInternalHeight);
    spdlog::info("[Config] bInternalWidescreen: {}", bInternalWidescreen);
    spdlog::info("[Config] bInternalBorderless: {}", bInternalBorderless);
    if (bAnalog)     spdlog::info("[Config] bAnalog: {}", *bAnalog);
    if (bSwapSticks) spdlog::info("[Config] bSwapSticks: {}", *bSwapSticks);
    spdlog::info("[Config] bRemoveDeadzone: {}", bRemoveDeadzone);
    spdlog::info("[Config] bLauncherSkipNotice: {}", bLauncherSkipNotice);
    spdlog::info("[Config] bLauncherStartGame: {}", bLauncherStartGame);
    spdlog::info("[Config] bPatchesDisableRAM: {}", bPatchesDisableRAM);
    spdlog::info("[Config] bPatchesDisableCDROM: {}", bPatchesDisableCDROM);
    spdlog::info("[Config] bPatchesDisableFont: {}", bPatchesDisableFont);
    spdlog::info("[Config] bPatchesRemoveUnderpants: {}", bPatchesRemoveUnderpants);
    spdlog::info("[Config] bPatchesEnableMosaic: {}", bPatchesEnableMosaic);
    spdlog::info("[Config] bPatchesRestoreGhosts: {}", bPatchesRestoreGhosts);
    spdlog::info("[Config] bPatchesRestoreMedicine: {}", bPatchesRestoreMedicine);
    spdlog::info("[Config] bPatchesThinTexturedQuads: {}", bPatchesThinTexturedQuads);
    spdlog::info("[Config] eBrightnessText: {}",
        eBrightnessText == M2BrightnessText::Fixed    ? "fixed"    :
        eBrightnessText == M2BrightnessText::Original ? "original" : "collection");
    spdlog::info("[Config] bPatchesPreserveConfiguration: {}", bPatchesPreserveConfiguration);
    spdlog::info("[Config] bGameStageSelect: {} ({})", bGameStageSelect, sGameStageSelect);
    spdlog::info("[Config] bGameEnglishText: {}", bGameEnglishText);
    spdlog::info("[Config] bGameUnlockBriefing: {}", bGameUnlockBriefing);
    if (!vGameGiveWeapons.empty()) {
        std::string ids;
        for (int id : vGameGiveWeapons) ids += (ids.empty() ? "" : ",") + std::to_string(id);
        spdlog::info("[Config] vGameGiveWeapons: {}", ids);
    }
    if (!vGameGiveItems.empty()) {
        std::string ids;
        for (int id : vGameGiveItems) ids += (ids.empty() ? "" : ",") + std::to_string(id);
        spdlog::info("[Config] vGameGiveItems: {}", ids);
    }
    spdlog::info("[Config] bShouldCheckForUpdates: {}", bShouldCheckForUpdates);
    spdlog::info("[Config] bConsoleUpdateNotifications: {}", bConsoleUpdateNotifications);
    spdlog::info("[Config] bDisableWindowsFullscreenOptimization: {}", bDisableWindowsFullscreenOptimization);
    spdlog::info("[Config] bDisableWindowsSlideshowWarning: {}", bDisableWindowsSlideshowWarning);

    if (bDebuggerEnabled && bDebuggerExclusive)
    {
        spdlog::info("[Config] Debugger enabled in exclusive mode, other features will be disabled.");
    }

    spdlog::info("----------");
}
