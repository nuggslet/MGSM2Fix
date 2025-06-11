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
        inipp::get_value(ini.sections[section], "Windowed", bWindowedMode);
        inipp::get_value(ini.sections[section], "Borderless", bBorderlessMode);
    }

    inipp::get_value(ini.sections["Internal Resolution"], "Enabled", bInternalEnabled);
    inipp::get_value(ini.sections["Internal Resolution"], "Height", iInternalHeight);
    inipp::get_value(ini.sections["Internal Resolution"], "Widescreen", bInternalWidescreen);

    inipp::get_value(ini.sections["Input"], "Analog", bAnalogMode);

    inipp::get_value(ini.sections["Launcher"], "SkipNotice", bLauncherSkipNotice);
    inipp::get_value(ini.sections["Launcher"], "StartGame", bLauncherStartGame);

    inipp::get_value(ini.sections["Patches"], "DisableRAM", bPatchesDisableRAM);
    inipp::get_value(ini.sections["Patches"], "DisableCDROM", bPatchesDisableCDROM);

    inipp::get_value(ini.sections["Patches"], "RemoveUnderpants", bPatchesRemoveUnderpants);
    inipp::get_value(ini.sections["Patches"], "EnableMosaic", bPatchesEnableMosaic);
    inipp::get_value(ini.sections["Patches"], "RestoreGhosts", bPatchesRestoreGhosts);
    inipp::get_value(ini.sections["Patches"], "RestoreMedicine", bPatchesRestoreMedicine);

    inipp::get_value(ini.sections["Game"], "StageSelect", bGameStageSelect);

    // Force windowed mode if borderless is enabled but windowed is not.
    if (bBorderlessMode) {
        bWindowedMode = true;
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

    sFullscreenMode = bWindowedMode ? "0" : "1";
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
    spdlog::info("[Config] bWindowedMode: {}", bWindowedMode);
    spdlog::info("[Config] bBorderlessMode: {}", bBorderlessMode);
    spdlog::info("[Config] bInternalEnabled: {}", bInternalEnabled);
    spdlog::info("[Config] iInternalHeight: {}", iInternalHeight);
    spdlog::info("[Config] bInternalWidescreen: {}", bInternalWidescreen);
    spdlog::info("[Config] bAnalogMode: {}", bAnalogMode);
    spdlog::info("[Config] bLauncherSkipNotice: {}", bLauncherSkipNotice);
    spdlog::info("[Config] bLauncherStartGame: {}", bLauncherStartGame);
    spdlog::info("[Config] bPatchesDisableRAM: {}", bPatchesDisableRAM);
    spdlog::info("[Config] bPatchesDisableCDROM: {}", bPatchesDisableCDROM);
    spdlog::info("[Config] bPatchesRemoveUnderpants: {}", bPatchesRemoveUnderpants);
    spdlog::info("[Config] bPatchesEnableMosaic: {}", bPatchesEnableMosaic);
    spdlog::info("[Config] bPatchesRestoreGhosts: {}", bPatchesRestoreGhosts);
    spdlog::info("[Config] bPatchesRestoreMedicine: {}", bPatchesRestoreMedicine);
    spdlog::info("[Config] bGameStageSelect: {}", bGameStageSelect);

    if (bDebuggerEnabled && bDebuggerExclusive)
    {
        spdlog::info("[Config] Debugger enabled in exclusive mode, other features will be disabled.");
    }

    spdlog::info("----------");
}
