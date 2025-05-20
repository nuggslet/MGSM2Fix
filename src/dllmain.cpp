#include "stdafx.h"
#include "helper.hpp"

#include "sqrdbg.h"
#include "sqdbgserver.h"

#include "sqrat.h"

#include "resource.h"

#include "m2binary.h"

#include "emutask.h"
#include "inputhub.h"
#include "input.h"

#include "emulator.h"

using namespace std;

HMODULE gBaseModule = GetModuleHandle(NULL);
HMODULE gFixModule;

string sFixVer = "2.2";
inipp::Ini<char> ini;

template <Squirk T>
HSQREMOTEDBG<T> gDBG;
template <Squirk T>
EmuTask<T> gEmuTask;
template <Squirk T>
InputHub<T> gInputHub;
template <Squirk T>
Input<T> gInput;

// INI Variables
bool bDebuggerEnabled;
int iDebuggerPort;
bool bDebuggerAutoUpdate;
bool bDebuggerExclusive;
optional<bool> bSmoothing;
optional<bool> bScanline;
optional<bool> bDotMatrix;
bool bError;
int iLevel;
int iNativeLevel;
int iEmulatorLevel;
bool bExternalEnabled;
int iExternalWidth;
int iExternalHeight;
bool bWindowedMode;
bool bBorderlessMode;
bool bInternalEnabled;
int iInternalWidth;
int iInternalHeight;
bool bInternalWidescreen;
int iLayerWidth = 7680;
int iLayerHeight = 4320;
bool bAnalogMode;
bool bLauncherSkipNotice;
bool bLauncherStartGame;
bool bGameStageSelect;
bool bPatchesDisableRAM;
bool bPatchesDisableCDROM;
bool bPatchesRemoveUnderpants = true;
bool bPatchesEnableMosaic = true;
bool bPatchesRestoreGhosts = true;
bool bPatchesRestoreMedicine = true;

// Variables
string sFullscreenMode;
string sExternalWidth;
string sExternalHeight;

std::filesystem::path sExePath;
std::string sExeName;


struct M2FixInfo
{
    std::string GameTitle;
    std::string ExeName;
    int SteamAppId;
};

const std::map<M2FixGame, M2FixInfo> kGames = {
    {M2FixGame::MGS1,           {"Metal Gear Solid", "MGS1\\METAL GEAR SOLID.exe", 2131630}},
    {M2FixGame::MGSR,           {"Metal Gear / Snake's Revenge (Bonus Content)", "MGS Master Collection Bonus Content\\MGS MC1 Bonus Content.exe", 2306740}},
    {M2FixGame::Contra,         {"Contra Anniversary", "Contra Anniversary Collection\\game.exe", 1018020}},
    {M2FixGame::Dracula,        {"Castlevania Anniversary", "Castlevania Anniversary Collection\\game.exe", 1018010}},
    {M2FixGame::DraculaAdvance, {"Castlevania Advance", "Castlevania Advance Collection\\game.exe", 1552550}},
    {M2FixGame::Ray,            {"Ray’z Arcade Chronology", "Ray’z Arcade Chronology\\game.exe", 2478020}},
    {M2FixGame::Darius,         {"Darius Cozmic Arcade", "Darius Cozmic Collection Arcade\\game.exe", 1638330}},
    {M2FixGame::DariusHD,       {"G-Darius HD", "G-Darius HD\\gdarahd.exe", 1640160}},
};

const M2FixInfo* kGame = nullptr;
M2FixGame eGameType = M2FixGame::Unknown;

std::map<std::string, std::string> kEnv;

typedef struct {
    bool hooked;
    SQChar src[MAX_PATH];
    SQInteger line;
} M2FixData;

void Logging()
{
    // Get game name and exe path
    char exePath[_MAX_PATH] = { 0 };
    GetModuleFileName(gBaseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = (sExePath / sExePath.filename()).string();

    bool logDirExists = std::filesystem::is_directory(sExePath / "logs");
    if (!logDirExists)
    {
        std::filesystem::create_directory(sExePath / "logs"); //create a "logs" subdirectory in the game folder to keep the main directory tidy.
    }
    loguru::add_file((sExePath / "logs" / "MGSM2Fix.log").string().c_str(), loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");
    if (!logDirExists)
    {
        LOG_F(INFO, "New log subdirectory created.");
    }
    LOG_F(INFO, "MGSM2Fix v%s loaded", sFixVer.c_str());
}

///Prints CPU, GPU, and RAM info to the log to expedite common troubleshooting, since it's usually folks who are below minumum specs.
void LogSysInfo()
{
#ifndef _WIN32
    LOG_F(INFO, "System Details - Steam Deck/Linux");
    return;
#endif


    std::array<int, 4> integerBuffer = {};
    constexpr size_t sizeofIntegerBuffer = sizeof(int) * integerBuffer.size();
    std::array<char, 64> charBuffer = {};
    std::array<std::uint32_t, 3> functionIds = {
        0x8000'0002, // Manufacturer  
        0x8000'0003, // Model 
        0x8000'0004  // Clock-speed
    };

    std::string cpu;
    for (int id : functionIds)
    {
        __cpuid(integerBuffer.data(), id);
        std::memcpy(charBuffer.data(), integerBuffer.data(), sizeofIntegerBuffer);
        cpu += std::string(charBuffer.data());
    }

    LOG_F(INFO, "System Details - CPU: {}", cpu);

    std::string deviceString;
    for (int i = 0; ; i++)
    {
        DISPLAY_DEVICE dd = { sizeof(dd), 0 };
        BOOL f = EnumDisplayDevices(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME);
        if (!f)
        {
            break; //that's all, folks.
        }
        char deviceStringBuffer[128];

#ifdef UNICODE
        WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, deviceStringBuffer, sizeof(deviceStringBuffer), NULL, NULL);
#else
        // Convert ANSI to wide first
        wchar_t wDeviceString[128];
        MultiByteToWideChar(CP_ACP, 0, dd.DeviceString, -1, wDeviceString, 128);
        WideCharToMultiByte(CP_UTF8, 0, wDeviceString, -1, deviceStringBuffer, sizeof(deviceStringBuffer), NULL, NULL);
#endif

        if (deviceString == deviceStringBuffer) //each monitor reports what gpu is driving it, lets just double check in case we're looking at a laptop with mixed usage.
        {
            continue;
        }
        deviceString = deviceStringBuffer;
        LOG_F(INFO, "System Details - GPU: {}", deviceString);
    }

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    double totalMemory = status.ullTotalPhys / 1024 / 1024;    ///Total physical RAM in MB.
    LOG_F(INFO, "System Details - RAM: {} GB ({} MB)", ceil((totalMemory / 1024) * 100) / 100, totalMemory);
}

void ReadConfig()
{
    std::array<std::string, 4> paths = { "", "plugins", "scripts", "update" };
    std::filesystem::path foundPath;
    for (const auto& path : paths)
    {
        if (std::filesystem::exists(sExePath / path / "MGSM2Fix64.asi") || std::filesystem::exists(sExePath / path / "MGSM2Fix32.asi"))
        {
            foundPath = path;
            break;
        }
    }

    // Initialise config
    std::ifstream iniFile((sExePath / foundPath / "MGSM2Fix.ini").string());
    if (!iniFile)
    {
        LOG_F(ERROR, "Failed to load config file.");
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

    inipp::get_value(ini.sections["Tracing"], "Error", bError);
    inipp::get_value(ini.sections["Tracing"], "Level", iLevel);
    inipp::get_value(ini.sections["Tracing"], "NativeLevel", iNativeLevel);
    inipp::get_value(ini.sections["Tracing"], "EmulatorLevel", iEmulatorLevel);

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
    inipp::get_value(ini.sections["Internal Resolution"], "LayerWidth", iLayerWidth);
    inipp::get_value(ini.sections["Internal Resolution"], "LayerHeight", iLayerHeight);

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

    // Log config parse
    LOG_F(INFO, "Config Parse: bDebuggerEnabled: %d", bDebuggerEnabled);
    LOG_F(INFO, "Config Parse: iDebuggerPort: %d", iDebuggerPort);
    LOG_F(INFO, "Config Parse: bDebuggerAutoUpdate: %d", bDebuggerAutoUpdate);
    LOG_F(INFO, "Config Parse: bDebuggerExclusive: %d", bDebuggerExclusive);
    LOG_F(INFO, "Config Parse: bSmoothing: %d<%d>", bSmoothing.has_value(), bSmoothing ? bSmoothing.value() : -1);
    LOG_F(INFO, "Config Parse: bScanline: %d<%d>", bScanline.has_value(), bScanline ? bScanline.value() : -1);
    LOG_F(INFO, "Config Parse: bDotMatrix: %d<%d>", bDotMatrix.has_value(), bDotMatrix ? bDotMatrix.value() : -1);
    LOG_F(INFO, "Config Parse: iLevel: %d", iLevel);
    LOG_F(INFO, "Config Parse: iNativeLevel: %d", iNativeLevel);
    LOG_F(INFO, "Config Parse: iEmulatorLevel: %d", iEmulatorLevel);
    LOG_F(INFO, "Config Parse: bExternalEnabled: %d", bExternalEnabled);
    LOG_F(INFO, "Config Parse: iExternalWidth: %d", iExternalWidth);
    LOG_F(INFO, "Config Parse: iExternalHeight: %d", iExternalHeight);
    LOG_F(INFO, "Config Parse: bWindowedMode: %d", bWindowedMode);
    LOG_F(INFO, "Config Parse: bBorderlessMode: %d", bBorderlessMode);
    LOG_F(INFO, "Config Parse: bInternalEnabled: %d", bInternalEnabled);
    LOG_F(INFO, "Config Parse: iInternalHeight: %d", iInternalHeight);
    LOG_F(INFO, "Config Parse: bInternalWidescreen: %d", bInternalWidescreen);
    LOG_F(INFO, "Config Parse: iLayerWidth: %d", iLayerWidth);
    LOG_F(INFO, "Config Parse: iLayerHeight: %d", iLayerHeight);
    LOG_F(INFO, "Config Parse: bAnalogMode: %d", bAnalogMode);
    LOG_F(INFO, "Config Parse: bLauncherSkipNotice: %d", bLauncherSkipNotice);
    LOG_F(INFO, "Config Parse: bLauncherStartGame: %d", bLauncherStartGame);
    LOG_F(INFO, "Config Parse: bPatchesDisableRAM: %d", bPatchesDisableRAM);
    LOG_F(INFO, "Config Parse: bPatchesDisableCDROM: %d", bPatchesDisableCDROM);
    LOG_F(INFO, "Config Parse: bPatchesRemoveUnderpants: %d", bPatchesRemoveUnderpants);
    LOG_F(INFO, "Config Parse: bPatchesEnableMosaic: %d", bPatchesEnableMosaic);
    LOG_F(INFO, "Config Parse: bPatchesRestoreGhosts: %d", bPatchesRestoreGhosts);
    LOG_F(INFO, "Config Parse: bPatchesRestoreMedicine: %d", bPatchesRestoreMedicine);
    LOG_F(INFO, "Config Parse: bGameStageSelect: %d", bGameStageSelect);

    if (bDebuggerEnabled && bDebuggerExclusive)
    {
        LOG_F(INFO, "Config Parse: Debugger enabled in exclusive mode, other features will be disabled.");
    }

    // Force windowed mode if borderless is enabled but windowed is not. There is undoubtedly a more elegant way to handle this.
    if (bBorderlessMode)
    {
        bWindowedMode = true;
        LOG_F(INFO, "Config Parse: Borderless mode enabled.");
    }

    if (bAnalogMode)
    {
        LOG_F(INFO, "Config Parse: Analog mode enabled.");
    }

    if (iExternalWidth <= 0 || iExternalHeight <= 0)
    {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        iExternalWidth = (int)desktop.right;
        iExternalHeight = (int)desktop.bottom;
    }

    if (iInternalHeight <= 0)
    {
        iInternalHeight = iExternalHeight;
    }

    iInternalWidth = (iInternalHeight * 4) / 3;
    sFullscreenMode = bWindowedMode ? "0" : "1";
    sExternalWidth = to_string(iExternalWidth);
    sExternalHeight = to_string(iExternalHeight);
}

bool DetectGame()
{
    {
        auto envFree = [](char *p) { FreeEnvironmentStrings(p); };
        auto envBlock = std::unique_ptr<char, decltype(envFree)>{
            GetEnvironmentStrings(), envFree
        };

        for (char *i = envBlock.get(); *i != 0; ++i)
        {
            std::string key, value;
            for (; *i != '='; ++i) key += *i;
            ++i;
            for (; *i != 0; ++i) value += *i;
            kEnv[key] = value;
        }
    }
    LOG_F(INFO, "Module Name: %s", sExeName.c_str());
    LOG_F(INFO, "Module Path: %s", sExePath.string().c_str());
    LOG_F(INFO, "Module Address: %p", gBaseModule);
    LOG_F(INFO, "Module Version: %u", Memory::GetModuleVersion(gBaseModule));

    eGameType = M2FixGame::Unknown;
    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            LOG_F(INFO, "Detected game: %s (app %d)", info.GameTitle.c_str(), info.SteamAppId);
            eGameType = type;
            kGame = &info;
            return true;
        }
    }

    LOG_F(INFO, "Failed to detect supported game, %s isn't supported by MGSM2Fix", sExeName.c_str());
    return false;
}

LPVOID Resource(UINT id, LPCSTR type, LPDWORD size)
{
    HRSRC hRes = FindResource(gFixModule, MAKEINTRESOURCE(id), type);
    if (!hRes) return NULL;

    HGLOBAL h = LoadResource(gFixModule, hRes);
    if (!h) return NULL;

    LPVOID p = LockResource(h);
    if (!p) return NULL;

    DWORD dw = SizeofResource(gFixModule, hRes);
    if (size) *size = dw;

    return p;
}

uintptr_t M2_mallocAddress;
uintptr_t M2_reallocAddress;
uintptr_t M2_freeAddress;
void *M2_malloc(size_t size)
{
    if (M2_mallocAddress == 0 && M2_freeAddress == 0) {
        void * (_cdecl * _M2_realloc)(void *, size_t, size_t) = (void * (_cdecl *)(void *, size_t, size_t)) M2_reallocAddress;
        return _M2_realloc(nullptr, 0, size);
    }

    void * (*_M2_malloc)(size_t) = (void* (*)(size_t)) M2_mallocAddress;
    return _M2_malloc(size);
}
void *M2_realloc(void *p, size_t length, size_t size)
{
    if (M2_mallocAddress == 0 && M2_freeAddress == 0) {
        void * (_cdecl * _M2_realloc)(void *, size_t, size_t) = (void * (_cdecl *)(void *, size_t, size_t)) M2_reallocAddress;
        return _M2_realloc(p, length, size);
    }

    void * (*_M2_realloc)(void *, size_t) = (void * (*)(void *, size_t)) M2_reallocAddress;
    return _M2_realloc(p, size);
}
void M2_free(void *p)
{
    if (M2_mallocAddress == 0 && M2_freeAddress == 0) {
        void * (_cdecl * _M2_realloc)(void *, size_t, size_t) = (void * (_cdecl *)(void *, size_t, size_t)) M2_reallocAddress;
        _M2_realloc(p, 0, 0);
        return;
    }

    void (*_M2_free)(void*) = (void (*)(void *)) M2_freeAddress;
    _M2_free(p);
}

template <Squirk T> SQInteger Hook(HSQUIRRELVM<T> v);
template <Squirk T>
void SetHook(HSQUIRRELVM<T> v)
{
    if (bDebuggerEnabled && !gDBG<T>) {
        gDBG<T> = sq_rdbg_init(v, iDebuggerPort, bDebuggerAutoUpdate, bDebuggerExclusive);
        sq_rdbg_waitforconnections(gDBG<T>);
    }

    if (sq_isnull(v->_debughook)) {
        sq_pushregistrytable(v);
        sq_pushstring(v, _SC("_m2_debug_hook_"), -1);
        sq_pushuserpointer(v, v);
        sq_newclosure(v, Hook<T>, 1);
        sq_newslot(v, -3, false);
        sq_pop(v, 1);

        sq_pushregistrytable(v);
        sq_pushstring(v, _SC("_m2_debug_hook_"), -1);
        sq_rawget(v, -2);
        sq_setdebughook(v);
        sq_pop(v, 1);
    }
}

template <Squirk T>
void SQNew(HSQUIRRELVM<T> v)
{
    LOG_F(INFO, "Squirrel: SQVM is 0x%" PRIxPTR ", SQSharedState is 0x%" PRIxPTR " & scratchpad is 0x%" PRIxPTR " (%" PRIuPTR " bytes).",
        (uintptr_t) v, (uintptr_t) _ss(v), (uintptr_t) _ss(v)->_scratchpad, (uintptr_t) _ss(v)->_scratchpadsize);
    _ss(v)->_debuginfo = true;
}

template <Squirk T>
void SQHookFunction(HSQUIRRELVM<T> v, const SQChar *func, SQFUNCTION<T> hook, HSQOBJECT<T> *obj = NULL)
{
    sq_pushroottable(v);
    {
        if (obj) {
            sq_pushstring(v, func, -1);
            if (SQ_SUCCEEDED(sq_get(v, -2))) {
                sq_getstackobj(v, -1, obj);
                sq_addref(v, obj);
                sq_pop(v, 1);
            }
        }

        sq_pushstring(v, func, -1);
        sq_newclosure(v, hook, 0);
        sq_newslot(v, -3, false);

        sq_pop(v, 1);
    }
}

template <Squirk T>
void SQHookMethod(HSQUIRRELVM<T> v, const SQChar *name, const SQChar *func, SQFUNCTION<T> hook, HSQOBJECT<T> *obj = NULL)
{
    sq_pushroottable(v);
    {
        sq_pushstring(v, name, -1);
        sq_get(v, -2);
        {
            if (obj) {
                sq_pushstring(v, func, -1);
                if (SQ_SUCCEEDED(sq_get(v, -2))) {
                    sq_getstackobj(v, -1, obj);
                    sq_addref(v, obj);
                    sq_pop(v, 1);
                }
            }

            sq_pushstring(v, func, -1);
            sq_newclosure(v, hook, 0);
            sq_newslot(v, -3, false);

            sq_pop(v, 1);
        }

        sq_pop(v, 1);
    }
}

void M2Print(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char *buf = (char*) malloc(_vscprintf(fmt, va) + 1);
    vsprintf(buf, fmt, va);
    va_end(va);

    buf[strcspn(buf, "\r\n")] = 0;
    LOG_F(INFO, "Emulator: printf: %s", buf);
    free(buf);
}

const char* M2_GetCfgValue(string *key)
{
    LOG_F(INFO, "M2: MWinResCfg::GetValue(\"%s\")", key->c_str());

    if (*key == "FULLSCREEN_MODE" || *key == "BOOT_FULLSCREEN") {
        return sFullscreenMode.c_str();
    }

    if (*key == "FULLSCREEN_CURSOR" || *key == "MOUSE_CURSOR") {
        return "0";
    }

    if (*key == "WINDOW_VSYNC") {
        return "0";
    }

    if (*key == "WINDOW_W" || *key == "SCREEN_W" || *key == "LAST_CLIENT_SIZE_X") {
        return sExternalWidth.c_str();
    }

    if (*key == "WINDOW_H" || *key == "SCREEN_H" || *key == "LAST_CLIENT_SIZE_Y") {
        return sExternalHeight.c_str();
    }

    return NULL;
}

vector<string> M2_FileFilter;
vector<vector<unsigned char>> M2_DataFilter;
void FilterPatches()
{
    if (eGameType == M2FixGame::MGS1 && bPatchesRemoveUnderpants) {
        vector<string> MGS1_FileFilter_Underpants = {
            "0046a5", "0046a6",
            "0057c3", "0057c4", "0057c5",
            "0099fc", "0099fd",
        };
        M2_FileFilter.insert(M2_FileFilter.end(),
            MGS1_FileFilter_Underpants.begin(),
            MGS1_FileFilter_Underpants.end()
        );
    }

    if (eGameType == M2FixGame::MGS1 && bPatchesRestoreGhosts) {
        M2_FileFilter.push_back("shinrei");
    }

    if (eGameType == M2FixGame::MGS1 && bPatchesRestoreMedicine) {
        vector<unsigned char> MGS1_DataFilter_Medicine = { 0, 152, 0, 72, 152, 72, 152, 152, 152 };
        M2_DataFilter.push_back(MGS1_DataFilter_Medicine);
    }
}

unsigned char* M2_PadPTR;
void M2_ReadPad(unsigned int addr, unsigned int id, unsigned char* state)
{
    if (M2_PadPTR != state) {
        LOG_F(INFO, "Emulator: Pad state is 0x%" PRIxPTR ".", state);
    }

    M2_PadPTR = state;
}

template <Squirk T>
void AnalogLoop(HSQUIRRELVM<T> v)
{
    if (!M2_PadPTR) return;

    gInputHub<T>.SetDirectionMerge(0);
    gEmuTask<T>.SetInputDirectionMerge(0);

    gInputHub<T>.SetDeadzone(0.0);
    gEmuTask<T>.SetInputDeadzone(0.0);

    SQFloat xL = 0.0, yL = 0.0;
    gInput<T>.GetAnalogStickX(&xL);
    gInput<T>.GetAnalogStickY(&yL);

    SQFloat xR = 0.0, yR = 0.0;
    gInput<T>.GetRightAnalogStickX(&xR);
    gInput<T>.GetRightAnalogStickY(&yR);

    // Normalize an axis from (-1, 1) to (0, 255) with 128 = center
    // https://github.com/grumpycoders/pcsx-redux/blob/a072e38d78c12a4ce1dadf951d9cdfd7ea59220b/src/core/pad.cc#L664-L673
    const auto axisToUint8 = [](float axis) {
        constexpr float scale = 1.3f;
        const float scaledValue = std::clamp<float>(axis * scale, -1.0f, 1.0f);
        return (uint8_t)(std::clamp<float>(std::round(((scaledValue + 1.0f) / 2.0f) * 255.0f), 0.0f, 255.0f));
    };

    extern SQInteger MGS1_PlaySide;
    if (MGS1_PlaySide == 0) {
        M2_PadPTR[0x44] = axisToUint8(xL);
        M2_PadPTR[0x45] = axisToUint8(yL);
        M2_PadPTR[0x46] = axisToUint8(xR);
        M2_PadPTR[0x47] = axisToUint8(yR);

        for (unsigned int i = 0x48; i < 0x54; i++) {
            M2_PadPTR[i] = 128;
        }
    } else {
        for (unsigned int i = 0x44; i < 0x4C; i++) {
            M2_PadPTR[i] = 128;
        }

        M2_PadPTR[0x4C] = axisToUint8(xL);
        M2_PadPTR[0x4D] = axisToUint8(yL);
        M2_PadPTR[0x4E] = axisToUint8(xR);
        M2_PadPTR[0x4F] = axisToUint8(yR);

        for (unsigned int i = 0x50; i < 0x54; i++) {
            M2_PadPTR[i] = 128;
        }
    }
}

template <Squirk T>
SQInteger SQ_util_is_notice_skipable(HSQUIRRELVM<T> v)
{
    sq_pushbool(v, true);
    return 1;
}

bool M2_LaunchIntent = true;
template <Squirk T>
SQInteger SQ_util_get_launch_intent_id(HSQUIRRELVM<T> v)
{
    if (M2_LaunchIntent)
        sq_pushstring(v, _SC("MAIN_STORY"), -1);
    else
        sq_pushstring(v, _SC(""), -1);

    return 1;
}

template <Squirk T>
SQInteger SQ_util_clear_launch_intent_id(HSQUIRRELVM<T> v)
{
    M2_LaunchIntent = false;
    return 0;
}

SQInteger M2_StartPadId = 4;
template <Squirk T>
SQInteger SQ_SystemEtc_setStartPadId(HSQUIRRELVM<T> v)
{
    sq_getinteger(v, 1, &M2_StartPadId);
    return 0;
}

template <Squirk T>
SQInteger SQ_SystemEtc_getStartPadId(HSQUIRRELVM<T> v)
{
    sq_pushinteger(v, M2_StartPadId);
    return 1;
}

SQInteger MGS1_GlobalsPTR;
SQInteger MGS1_LoaderPTR;

template <Squirk T>
SQObject<T> SQObj_util_get_memory_define_table;
template <Squirk T>
SQInteger SQ_util_get_memory_define_table(HSQUIRRELVM<T> v)
{
    SQInteger nargs = sq_gettop(v);

    SQObjectPtr<T> ret;
    SQObjectPtr<T> closure = SQObj_util_get_memory_define_table<T>;
    bool res = v->Call(closure, nargs, v->_stackbase, ret, false);
    if (res) {
        if (eGameType == M2FixGame::MGS1) {
            sq_pushobject(v, ret);
            sq_pushstring(v, _SC("scene_name"), -1);
            if (SQ_SUCCEEDED(sq_get(v, -2))) {
                sq_getinteger(v, -1, &MGS1_GlobalsPTR);
                sq_pop(v, 1);
            }
            sq_pop(v, 1);
        }

        sq_pushobject(v, ret);
        return 1;
    }

    return -1;
}

template <Squirk T>
SQInteger SQReturn_init_system_1st(HSQUIRRELVM<T> v)
{
    if (bLauncherSkipNotice) {
        SQHookFunction(v, _SC("util_is_notice_skipable"), SQ_util_is_notice_skipable);
    }

    if (bLauncherStartGame) {
        SQHookFunction(v, _SC("util_get_launch_intent_id"), SQ_util_get_launch_intent_id);
        SQHookFunction(v, _SC("util_clear_launch_intent_id"), SQ_util_clear_launch_intent_id);
    }

    SQHookMethod(v, _SC("SystemEtc"), _SC("setStartPadId"), SQ_SystemEtc_setStartPadId);
    SQHookMethod(v, _SC("SystemEtc"), _SC("getStartPadId"), SQ_SystemEtc_getStartPadId);

    SQHookFunction(v, _SC("util_get_memory_define_table"),
        SQ_util_get_memory_define_table, &SQObj_util_get_memory_define_table<T>);

    return 0;
}

template <Squirk T>
SQInteger _SQ_init_emulator_get_arch_sub_info(HSQUIRRELVM<T> v)
{
    sq_pushinteger(v, USE_ANALOG);
    return 1;
}

template <Squirk T>
SQInteger SQReturn_init_system_last(HSQUIRRELVM<T> v)
{
    SQHookFunction(v, _SC("_init_emulator_get_arch_sub_info"), _SQ_init_emulator_get_arch_sub_info);
    return 0;
}

template <Squirk T>
SQInteger _SQReturn_update_gadgets(HSQUIRRELVM<T> v)
{
    if (MGS1_GlobalsPTR != 0 && MGS1_LoaderPTR != 0) {
        SQInteger MGS1_StageNamePTR = MGS1_GlobalsPTR;
        SQInteger MGS1_GameStatePTR = MGS1_GlobalsPTR + 16;

        char MGS1_StageName[8] = { 0 };
        char MGS1_LoaderName[8] = { 0 };
        for (int i = 0; i < sizeof(MGS1_StageName) / sizeof(uint32_t); i++) {
            gEmuTask<T>.GetRamValue(32, MGS1_StageNamePTR + (i * sizeof(uint32_t)),
                (SQInteger *) &MGS1_StageName[i * sizeof(uint32_t)]);
            gEmuTask<T>.GetRamValue(32, MGS1_LoaderPTR + (i * sizeof(uint32_t)),
                (SQInteger *) &MGS1_LoaderName[i * sizeof(uint32_t)]);
        }

        SQInteger MGS1_CurrentStagePTR = MGS1_GameStatePTR + (6 * sizeof(short));
        SQInteger MGS1_CurrentStage = 0;
        gEmuTask<T>.GetRamValue(16, MGS1_CurrentStagePTR, &MGS1_CurrentStage);

        if (bGameStageSelect) {
            if (strcmp(MGS1_LoaderName, "title") == 0 && strcmp(MGS1_StageName, "select") != 0) {
                strcpy(MGS1_LoaderName, "select");
                for (int i = 0; i < sizeof(MGS1_StageName) / sizeof(uint32_t); i++) {
                    gEmuTask<T>.SetRamValue(32, MGS1_LoaderPTR + (i * sizeof(uint32_t)),
                        *(SQInteger *)&MGS1_LoaderName[i * sizeof(uint32_t)]);
                }
            }
        }
    }

    return 0;
}

void *M2LoadImage(void *dst, void *src, size_t num)
{
    LOG_F(INFO, "Emulator: Loading image at 0x%" PRIxPTR " with size %d bytes.", src, num);

    uint8_t* MGS1_LoaderScanResult = Memory::PatternScanBuffer(src, num, "00 00 00 00 69 6E 69 74 00 00 00 00");
    if (MGS1_LoaderScanResult)
        MGS1_LoaderPTR = (MGS1_LoaderScanResult - 0x30) - (uint8_t *) src;
    else
        MGS1_LoaderPTR = 0;

    return memmove(dst, src, num);
}

SQInteger MGS1_PlaySide;
template <Squirk T>
SQInteger SQReturn_set_playside_mgs(HSQUIRRELVM<T> v)
{
    auto &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 1];
    MGS1_PlaySide = _integer(obj);
    return 0;
}

template <Squirk T>
SQInteger SQNative_setDotmatrix(HSQUIRRELVM<T> v)
{
    if (bDotMatrix) {
        // Do this here as the native call is surprisingly expensive (?!)
        SQObjectPtr<T>* obj = &v->_stack._vals[v->_stackbase + 1];
        _integer(*obj) = bDotMatrix.value();
    }
    return 0;
}

template <Squirk T>
SQInteger SQNative_setRamValue(HSQUIRRELVM<T> v)
{
    SQObjectPtr<T>* width = &v->_stack._vals[v->_stackbase + 1];
    SQObjectPtr<T>* offset = &v->_stack._vals[v->_stackbase + 2];
    SQObjectPtr<T>* value = &v->_stack._vals[v->_stackbase + 3];

    SQObjectPtr<T>* patch = &v->_stack._vals[v->_stackbase - 5];
    if (!sq_istable(*patch)) return 0;

    SQInteger address = 0;

    sq_pushobject(v, *patch);
    sq_pushstring(v, "offset", -1);
    if (SQ_SUCCEEDED(sq_get(v, -2))) {
        sq_getinteger(v, -1, &address);
        sq_pop(v, 1);
    }
    sq_pop(v, 1);

    if (bPatchesDisableRAM && address != 0x200000) {
        if (_integer(*offset) == address) {
            LOG_F(INFO, "Patch: filtering RAM patch offset 0x%" PRIxPTR ".", address);
        }
        return 1;
    }

    return 0;
}

template <Squirk T>
SQInteger SQNative_entryCdRomPatch(HSQUIRRELVM<T> v)
{
    SQObjectPtr<T>* offset = &v->_stack._vals[v->_stackbase + 1];
    SQObjectPtr<T>* data = &v->_stack._vals[v->_stackbase + 2];

    SQObjectPtr<T>* patch = &v->_stack._vals[v->_stackbase - 3];
    if (sq_isinstance(*data)) patch = &v->_stack._vals[v->_stackbase - 4];

    const SQChar *file = NULL;
    vector<unsigned char> buffer;

    sq_pushobject(v, *patch);
    sq_pushstring(v, "file", -1);
    if (SQ_SUCCEEDED(sq_get(v, -2))) {
        sq_getstring(v, -1, &file);
        sq_pop(v, 1);
    }
    else if (sq_isarray(*data)) {
        int i = 0;
        SQObjectPtr<T> ent;
        while (_array(*data)->Get(i++, ent)) {
            buffer.push_back(_integer(ent)); // lmao
        }
    }
    sq_pop(v, 1);

    if (file) {
        if (bPatchesDisableCDROM) {
            LOG_F(INFO, "Patch: filtering CD-ROM patch file %s.", file);
            return 1;
        }

        for (auto &filter : M2_FileFilter) {
            if (strncmp(filter.c_str(), file, filter.length()) == 0)
            {
                LOG_F(INFO, "Patch: filtering CD-ROM patch file %s.", file);
                return 1;
            }
        }
    }
    else if (!buffer.empty()) {
        if (bPatchesDisableCDROM) {
            LOG_F(INFO, "Patch: filtering CD-ROM patch offset 0x%" PRIxPTR ".", _integer(*offset));
            return 1;
        }

        for (auto &filter : M2_DataFilter) {
            if (filter == buffer)
            {
                LOG_F(INFO, "Patch: filtering CD-ROM patch offset 0x%" PRIxPTR ".", _integer(*offset));
                return 1;
            }
        }
    }

    return 0;
}

string M2_Disk;
template <Squirk T>
SQInteger SQNative_setupCdRom(HSQUIRRELVM<T> v)
{
    HSQOBJECT<T> obj; sq_getstackobj(v, 2, &obj);
    M2_Disk.assign(_stringval(obj));

    LOG_F(INFO, "M2: Mounted CD-ROM image: %s.", M2_Disk.c_str());
    return 0;
}

SQInteger M2_Tray;
template <Squirk T>
SQInteger SQNative_setCdRomShellOpen(HSQUIRRELVM<T> v)
{
    HSQOBJECT<T> obj; sq_getstackobj(v, 2, &obj);
    M2_Tray = _integer(obj);

    LOG_F(INFO, "M2: CD-ROM tray is %s.", M2_Tray ? "open" : "closed");
    return 0;
}

string M2_ROM;
template <Squirk T>
SQInteger SQNative_init(HSQUIRRELVM<T> v)
{
    // Do some strange things to confirm that this is EmuTask::init() and not another init().
    gEmuTask<T>.SetVM(v);
    if (!gEmuTask<T>.Get()) return 0;

    HSQOBJECT<T> obj; sq_getstackobj(v, -1, &obj);
    HSQOBJECT<T> instance; sq_getstackobj(v, 1, &instance);
    sq_pop(v, 2);

    // Check that `this` is g_emu_task, therefore g_emu_task.init()...
    if (!sq_isinstance(obj) || !sq_isinstance(instance) ||
        _instance(obj) != _instance(instance)) {
        return 0;
    }

    sq_getstackobj(v, 5, &obj);
    M2_ROM.assign(_stringval(obj));

    LOG_F(INFO, "M2: Loaded ROM image: %s.", M2_ROM.c_str());
    return 0;
}

template <Squirk T>
vector<pair<const SQChar *, SQFUNCTION<T>>> M2_NativeCallTable = {
    make_pair("setDotmatrix", SQNative_setDotmatrix<T>),
    make_pair("setRamValue", SQNative_setRamValue<T>),
    make_pair("entryCdRomPatch", SQNative_entryCdRomPatch<T>),
    make_pair("setCdRomShellOpen", SQNative_setCdRomShellOpen<T>),
    make_pair("setupCdRom", SQNative_setupCdRom<T>),
    make_pair("init", SQNative_init<T>),
};

template <Squirk T>
bool FixNativeCall(HSQUIRRELVM<T> v, SQFUNCTION<T> func, SQNativeClosure<T> *closure, const SQChar *name)
{
    if (!name) return true;

    for (auto &func : M2_NativeCallTable<T>) {
        if (strcmp(name, func.first) == 0) {
            if (func.second(v)) return false;
            break;
        }
    }

    return true;
}

template <Squirk T>
SQInteger SQReturn_setSmoothing(HSQUIRRELVM<T> v)
{
    if (bSmoothing) gEmuTask<T>.SetSmoothing(bSmoothing.value());
    if (bScanline) gEmuTask<T>.SetScanline(bScanline.value());
    return 0;
}

template <Squirk T> bool Ketchup_Process(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger _SQReturn_set_disk_patch(HSQUIRRELVM<T> v)
{
    gEmuTask<T>.SetVM(v);
    Ketchup_Process(v);
    return 0;
}

SQInteger M2_DevId;
template <Squirk T>
SQInteger SQReturn_set_current_title_dev_id(HSQUIRRELVM<T> v)
{
    auto &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 1];
    M2_DevId = _integer(obj);

    LOG_F(INFO, "M2: Set title ID: %d.", M2_DevId);
    return 0;
}

SQInteger M2_DiskId;
template <Squirk T>
SQInteger _SQReturn_get_disk_path(HSQUIRRELVM<T> v)
{
    auto &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr<T> obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 2];
    M2_DiskId = _integer(obj);

    LOG_F(INFO, "M2: Set disk ID: %d.", M2_DiskId);
    return 0;
}

string M2_DevType;
template <Squirk T>
SQInteger SQReturn_set_game_regionTag(HSQUIRRELVM<T> v)
{
    auto &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr<T> obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 1];
    M2_DevType.assign(_stringval(obj));

    LOG_F(INFO, "M2: Set title version: %s.", M2_DevType.c_str());
    return 0;
}

SQInteger M2_InitializeFinish;
template <Squirk T>
SQInteger SQReturn_onInitializeFinish(HSQUIRRELVM<T> v)
{
    extern void FixModules();

    if (M2_InitializeFinish == 0) {
        FixModules();
    }

    M2_InitializeFinish++;
    return 0;
}

SQInteger M2_ScreenWidth;
SQInteger M2_ScreenHeight;
SQInteger M2_ScreenScaleX;
SQInteger M2_ScreenScaleY;
template <Squirk T>
SQInteger SQReturn_util_get_multimonitor_screen_bounds(HSQUIRRELVM<T> v)
{
    auto &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr<T> obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 1];

    sq_pushobject(v, obj);
    {
        sq_pushstring(v, _SC("width"), -1);
        sq_get(v, -2);
        sq_getinteger(v, -1, &M2_ScreenWidth);
        sq_pop(v, 1);

        sq_pushstring(v, _SC("height"), -1);
        sq_get(v, -2);
        sq_getinteger(v, -1, &M2_ScreenHeight);
        sq_pop(v, 1);
    }
    sq_pop(v, 1);

    LOG_F(INFO, "M2: Screen bounds are %ux%u.", M2_ScreenWidth, M2_ScreenHeight);

    SQInteger x = 4 * M2_ScreenHeight;
    SQInteger y = 3 * M2_ScreenWidth;
    SQInteger gcd = std::gcd(x, y);
    M2_ScreenScaleX = y / gcd;
    M2_ScreenScaleY = x / gcd;

    return 0;
}

SQInteger M2_ScreenMode;
template <Squirk T>
SQInteger _SQReturn_setting_screen_set_parameter_auto_size(HSQUIRRELVM<T> v)
{
    auto &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr<T> obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 3];
    M2_ScreenMode = _integer(obj);

    LOG_F(INFO, "M2: Screen mode is %u.", M2_ScreenMode);
    return 0;
}

template <Squirk T>
vector<pair<const SQChar *, SQFUNCTION<T>>> M2_ReturnTable = {
    make_pair("init_system_1st", SQReturn_init_system_1st<T>),
    make_pair("init_system_last", SQReturn_init_system_last<T>),
    make_pair("set_playside_mgs", SQReturn_set_playside_mgs<T>),
    make_pair("_update_gadgets", _SQReturn_update_gadgets<T>),
    make_pair("setSmoothing", SQReturn_setSmoothing<T>),
    make_pair("_set_disk_patch", _SQReturn_set_disk_patch<T>),
    make_pair("_get_disk_path", _SQReturn_get_disk_path<T>),
    make_pair("set_current_title_dev_id", SQReturn_set_current_title_dev_id<T>),
    make_pair("set_game_regionTag", SQReturn_set_game_regionTag<T>),
    make_pair("onInitializeFinish", SQReturn_onInitializeFinish<T>),
    make_pair("util_get_multimonitor_screen_bounds", SQReturn_util_get_multimonitor_screen_bounds<T>),
    make_pair("_setting_screen_set_parameter_auto_size", _SQReturn_setting_screen_set_parameter_auto_size<T>),
};

template <Squirk T>
void FixLoop(HSQUIRRELVM<T> v, SQInteger event_type, const SQChar *src, const SQChar *name, SQInteger line)
{
    gEmuTask<T>.SetVM(v);
    gInputHub<T>.SetVM(v);
    gInput<T>.SetVM(v);

    if (name && event_type == _SC('r')) {
        for (auto &func : M2_ReturnTable<T>) {
            if (strcmp(name, func.first) == 0) {
                func.second(v);
                break;
            }
        }
    }

    if (bAnalogMode) AnalogLoop(v);
}

template <Squirk T>
SQUIRREL_API SQRESULT _m2_sqstd_format(HSQUIRRELVM<T> v, SQInteger nformatstringidx, SQInteger *outlen, SQChar **output);

template <Squirk T> void TraceNative(HSQUIRRELVM<T> v, SQFUNCTION<T> func, SQNativeClosure<T> *closure, const SQChar *name);
template <Squirk T> SQInteger Hook(HSQUIRRELVM<T> v);
template <Squirk T>
SQInteger HookNative(SQFUNCTION<T> func, HSQUIRRELVM<T> v)
{
    SetHook(v);

    if (bError && sq_isstring(v->_lasterror)) {
        LOG_F(INFO, "Squirrel: Error: %s", _stringval(v->_lasterror));
        sq_reseterror(v);
    }

    // Ignore the calls to the debug hook
    if (func == Hook<T>) return func(v);

    const SQChar *name = NULL;
    SQNativeClosure<T> *closure = NULL;
    if (v && v->ci && sq_isnativeclosure(v->ci->_closure)) {
        closure = _nativeclosure(v->ci->_closure);
        if (sq_isstring(closure->_name)) {
            name = _stringval(closure->_name);
        }
    }

    if (iNativeLevel >= 1) {
        TraceNative(v, func, closure, name);
    }

    if (name && (strcmp(name, "printf") == 0 || strcmp(name, "print") == 0)) {
        SQChar *print = NULL;
        SQInteger length = 0;

        const SQChar *format = NULL;
        sq_getstring(v, 2, &format);

        if (format && *format != 0 && SQ_SUCCEEDED(_m2_sqstd_format(v, 2, &length, &print))) {
            print[scstrcspn(print, "\r\n")] = 0;
            if (*print != 0) {
                LOG_F(INFO, "Squirrel: printf: %s", print);
            }
            free(print);
        }
    }

    if (FixNativeCall(v, func, closure, name)) {
        return func(v);
    }

    return 0;
}

template <Squirk T>
bool SquirrelMain(HSQUIRRELVM<T> v)
{
    LOG_F(INFO, "Squirrel: SQVM 0x%" PRIxPTR " hooked: debug info is %s, exceptions are %s.", (uintptr_t) v,
        (_ss(v)->_debuginfo ? "enabled" : "disabled"),
        (_ss(v)->_notifyallexceptions ? "enabled" : "disabled")
    );

    if (bDebuggerEnabled && bDebuggerExclusive && gDBG<T>) {
        gDBG<T>->Init(v);
        return true;
        wmemcmp(NULL, NULL, 0);
    }

    return false;
}

template <Squirk T> void Trace(HSQUIRRELVM<T> v);
template <Squirk T> SQInteger debug_hook(HSQUIRRELVM<T> v, HSQUIRRELVM<T> _v, HSQREMOTEDBG<T> rdbg);
template <Squirk T>
SQInteger Hook(HSQUIRRELVM<T> v)
{
    SQObjectPtr debughook = v->_debughook;
    v->_debughook = _null_<T>;

    if (bError && sq_isstring(v->_lasterror)) {
        LOG_F(INFO, "Squirrel: Error: %s", _stringval(v->_lasterror));
        sq_reseterror(v);
    }

    if (bDebuggerEnabled && !bDebuggerExclusive && gDBG<T>) {
        debug_hook(static_cast<HSQUIRRELVM<T>>(nullptr), v, gDBG<T>);
    }

    M2FixData* data = (M2FixData *) sq_getforeignptr(v);

    if (!data) {
        sq_setforeignptr(v, calloc(1, sizeof(M2FixData)));
        data = (M2FixData *) sq_getforeignptr(v);
    }

    if (!data->hooked) {
        data->hooked = true;
        if (SquirrelMain(v)) return 0;
    }

    const SQChar *src = NULL, *func = NULL;
    SQInteger event_type = 0, line = 0;
    sq_getinteger(v, 2, &event_type);
    if (sq_gettype(v, 3) == OT_STRING) sq_getstring(v, 3, &src);
    sq_getinteger(v, 4, &line);
    if (sq_gettype(v, 5) == OT_STRING) sq_getstring(v, 5, &func);

    if (iNativeLevel >= 1) {
        strcpy(data->src, src);
        data->line = line;
    }

    if (iLevel >= 1) {
        Trace(v);
    }

    FixLoop(v, event_type, src, func, line);

    v->_debughook = debughook;
    return 0;
}

template SQInteger Hook(HSQUIRRELVM<Squirk::Standard> v);
template SQInteger Hook(HSQUIRRELVM<Squirk::AlignObject> v);

void _SQNew_Standard(HSQUIRRELVM<Squirk::Standard> v)
{
    return SQNew<Squirk::Standard>(v);
}

void _SQNew_AlignObject(HSQUIRRELVM<Squirk::AlignObject> v)
{
    return SQNew<Squirk::AlignObject>(v);
}

SQInteger _HookNative_Standard(SQFUNCTION<Squirk::Standard> func, HSQUIRRELVM<Squirk::Standard> v)
{
    return HookNative<Squirk::Standard>(func, v);
}

SQInteger _HookNative_AlignObject(SQFUNCTION<Squirk::AlignObject> func, HSQUIRRELVM<Squirk::AlignObject> v)
{
    return HookNative<Squirk::AlignObject>(func, v);
}

SQRESULT _sq_setnativeclosurename_Standard(HSQUIRRELVM<Squirk::Standard> v, SQInteger idx, const SQChar *name)
{
    return sq_setnativeclosurename<Squirk::Standard>(v, idx, name);
}

SQRESULT _sq_setnativeclosurename_AlignObject(HSQUIRRELVM<Squirk::AlignObject> v, SQInteger idx, const SQChar *name)
{
    return sq_setnativeclosurename<Squirk::AlignObject>(v, idx, name);
}
#define CALL(func) { extern void func; func; }

mutex mainThreadFinishedMutex;
condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;

DWORD __stdcall Main(void*)
{
    Logging();
    if (DetectGame())
    {
        LogSysInfo();
        ReadConfig();
        FilterPatches();

        CALL(ScanFunctions());

        CALL(M2Hook());
        CALL(SquirrelHook());

        if (bExternalEnabled) CALL(ConfigHook());
        if (bBorderlessMode) CALL(BorderlessPatch());
        if (bAnalogMode)
        {
            CALL(AnalogPatch());
            CALL(AnalogHook());
        }

        CALL(EmuHook());
        CALL(R3000Hook());
    }


    // Signal any threads which might be waiting for us before continuing
    {
        lock_guard lock(mainThreadFinishedMutex);
        mainThreadFinished = true;
        mainThreadFinishedVar.notify_all();
    }

    return true; // end thread
}

// Thanks emoose!
mutex memsetHookMutex;
bool memsetHookCalled = false;
void memsetWait()
{
    lock_guard lock(memsetHookMutex);
    if (!memsetHookCalled)
    {
        memsetHookCalled = true;

        // Wait for our main thread to finish before we return to the game
        if (!mainThreadFinished)
        {
            unique_lock finishedLock(mainThreadFinishedMutex);
            mainThreadFinishedVar.wait(finishedLock, [] { return mainThreadFinished; });
        }
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    gFixModule = hModule;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CALL(memsetHook());
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);

        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST); // set our Main thread priority higher than the games thread
            CloseHandle(mainHandle);
        }
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED); //fixes the monitor going to sleep during cutscenes.
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#ifndef _WIN64
static_assert(sizeof(SQVM<Squirk::Standard>) == 0xB0);              // MGS 1
static_assert(sizeof(SQSharedState<Squirk::Standard>) == 0xB8);     // MGS 1
static_assert(sizeof(SQVM<Squirk::AlignObject>) == 0xE0);           // CO | CA
static_assert(sizeof(SQSharedState<Squirk::AlignObject>) == 0x138); // CO | CA
#else
static_assert(sizeof(SQVM<Squirk::Standard>) == 0x108);             // MG | SR
static_assert(sizeof(SQSharedState<Squirk::Standard>) == 0x178);    // MG | SR
#endif
