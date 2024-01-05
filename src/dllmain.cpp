#include "stdafx.h"
#include "helper.hpp"

#include "sqrdbg.h"
#include "sqdbgserver.h"

#include "resource.h"

#include "m2binary.h"

#include "emutask.h"
#include "inputhub.h"
#include "input.h"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE fixModule;

string sFixVer = "1.1";
inipp::Ini<char> ini;

HSQREMOTEDBG gDBG;
EmuTask gEmuTask;
InputHub gInputHub;
Input gInput;

// INI Variables
bool bDebuggerEnabled;
int iDebuggerPort;
bool bDebuggerAutoUpdate;
bool bDebuggerExclusive;
optional<bool> bSmoothing;
optional<bool> bScanline;
optional<bool> bDotMatrix;
int iLevel;
int iNativeLevel;
bool bCustomResolution;
int iCustomResX;
int iCustomResY;
bool bWindowedMode;
bool bBorderlessMode;
bool bAnalogMode;
bool bLauncherSkipNotice;
bool bLauncherStartGame;
bool bGameDevMenu;
bool bPatchesGlobalRAM = true;
bool bPatchesGlobalCDROM = true;
bool bPatchesUnderpants = true;
bool bPatchesGhosts = true;
bool bPatchesMedicine = true;

// Variables
string sFullscreenMode;
string sCustomResX;
string sCustomResY;

std::filesystem::path sExePath;
std::string sExeName;

struct GameInfo
{
    std::string GameTitle;
    std::string ExeName;
    int SteamAppId;
};

enum class MgsGame
{
    Unknown,
    MGS,
    MGSR,
};

const std::map<MgsGame, GameInfo> kGames = {
    {MgsGame::MGS, {"Metal Gear Solid", "METAL GEAR SOLID.exe", 2131630}},
    {MgsGame::MGSR, {"Metal Gear / Snake's Revenge (NES)", "MGS MC1 Bonus Content.exe", 2306740}},
};

const GameInfo* game = nullptr;
MgsGame eGameType = MgsGame::Unknown;

typedef struct {
    bool hooked;
    SQChar src[MAX_PATH];
    SQInteger line;
} FixData;

typedef enum {
    PAD_BUTTON_A     =        0x1,
    PAD_BUTTON_B     =        0x2,
    PAD_BUTTON_C     =        0x4,
    PAD_BUTTON_X     =        0x8,
    PAD_BUTTON_Y     =       0x10,
    PAD_BUTTON_L     =       0x10,
    PAD_BUTTON_Z     =       0x20,
    PAD_BUTTON_R     =       0x20,
    PAD_DOWN         =       0x40,
    PAD_LEFT         =       0x80,
    PAD_RIGHT        =      0x100,
    PAD_COIN         =      0x200,
    PAD_SELECT       =      0x200,
    PAD_START        =      0x400,
    PAD_UP           =      0x800,
    PAD_DISCONNECTED =     0x1000,
    PAD_BUTTON_L2    =     0x4000,
    PAD_BUTTON_R2    =     0x8000,
    PAD_RPD_GC_POS   =  0x2000000,
    PAD_RPD_GC_NEG   =  0x4000000,
    PAD_SHORTCUT     = 0x20000000,
    PAD_MENU         = 0x40000000,
    PAD_RPD          = 0x80000000,
} M2EpiPadFlag;

typedef enum {
    MD_6B_DISABLE =  0x1,
    MD_MULTITAP_0 =  0x2,
    MD_MULTITAP_1 =  0x4,
    USE_ANALOG    =  0x8,
    DIRECTION_4   = 0x10,
} M2EpiArchSubInfo;

typedef enum {
    BUTTON_CIRCLE   =  0,
    BUTTON_CROSS    =  1,
    BUTTON_TRIANGLE =  2,
    BUTTON_SQUARE   =  3,
    BUTTON_R1       =  4,
    BUTTON_R2       =  5,
    BUTTON_R3       =  6,
    BUTTON_L1       =  7,
    BUTTON_L2       =  8,
    BUTTON_L3       =  9,
    BUTTON_SELECT   = 10,
    BUTTON_START    = 11,
    BUTTON_TOUCH    = 12,
} PlatformButtonId;

void Logging()
{
    loguru::add_file("MGSM2Fix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");

    LOG_F(INFO, "MGSM2Fix v%s loaded", sFixVer.c_str());
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(".\\MGSM2Fix.ini");
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

    inipp::get_value(ini.sections["Tracing"], "Level", iLevel);
    inipp::get_value(ini.sections["Tracing"], "NativeLevel", iNativeLevel);

    inipp::get_value(ini.sections["Custom Resolution"], "Enabled", bCustomResolution);
    inipp::get_value(ini.sections["Custom Resolution"], "Width", iCustomResX);
    inipp::get_value(ini.sections["Custom Resolution"], "Height", iCustomResY);
    inipp::get_value(ini.sections["Custom Resolution"], "Windowed", bWindowedMode);
    inipp::get_value(ini.sections["Custom Resolution"], "Borderless", bBorderlessMode);

    inipp::get_value(ini.sections["Input"], "Analog", bAnalogMode);

    inipp::get_value(ini.sections["Launcher"], "SkipNotice", bLauncherSkipNotice);
    inipp::get_value(ini.sections["Launcher"], "StartGame", bLauncherStartGame);

    inipp::get_value(ini.sections["Patches"], "GlobalRAM", bPatchesGlobalRAM);
    inipp::get_value(ini.sections["Patches"], "GlobalCDROM", bPatchesGlobalCDROM);

    inipp::get_value(ini.sections["Patches"], "Underpants", bPatchesUnderpants);
    inipp::get_value(ini.sections["Patches"], "Ghosts", bPatchesGhosts);
    inipp::get_value(ini.sections["Patches"], "Medicine", bPatchesMedicine);

    inipp::get_value(ini.sections["Game"], "DevMenu", bGameDevMenu);

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
    LOG_F(INFO, "Config Parse: bCustomResolution: %d", bCustomResolution);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: bWindowedMode: %d", bWindowedMode);
    LOG_F(INFO, "Config Parse: bBorderlessMode: %d", bBorderlessMode);
    LOG_F(INFO, "Config Parse: bAnalogMode: %d", bAnalogMode);
    LOG_F(INFO, "Config Parse: bLauncherSkipNotice: %d", bLauncherSkipNotice);
    LOG_F(INFO, "Config Parse: bLauncherStartGame: %d", bLauncherStartGame);
    LOG_F(INFO, "Config Parse: bPatchesGlobalRAM: %d", bPatchesGlobalRAM);
    LOG_F(INFO, "Config Parse: bPatchesGlobalCDROM: %d", bPatchesGlobalCDROM);
    LOG_F(INFO, "Config Parse: bPatchesUnderpants: %d", bPatchesUnderpants);
    LOG_F(INFO, "Config Parse: bPatchesGhosts: %d", bPatchesGhosts);
    LOG_F(INFO, "Config Parse: bPatchesMedicine: %d", bPatchesMedicine);
    LOG_F(INFO, "Config Parse: bGameDevMenu: %d", bGameDevMenu);

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

    if (iCustomResX <= 0 || iCustomResY <= 0)
    {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        iCustomResX = (int)desktop.right;
        iCustomResY = (int)desktop.bottom;
    }

    sFullscreenMode = bWindowedMode ? "0" : "1";
    sCustomResX = to_string(iCustomResX);
    sCustomResY = to_string(iCustomResY);
}

bool DetectGame()
{
    // Get game name and exe path
    CHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileName(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();

    LOG_F(INFO, "Module Name: %s", sExeName.c_str());
    LOG_F(INFO, "Module Path: %s", sExePath.string().c_str());
    LOG_F(INFO, "Module Address: %p", baseModule);
    LOG_F(INFO, "Module Timestamp: %u", Memory::ModuleTimestamp(baseModule));

    eGameType = MgsGame::Unknown;
    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            LOG_F(INFO, "Detected game: %s (app %d)", info.GameTitle.c_str(), info.SteamAppId);
            eGameType = type;
            game = &info;
            return true;
        }
    }

    LOG_F(INFO, "Failed to detect supported game, %s isn't supported by MGSM2Fix", sExeName.c_str());
    return false;
}

LPVOID Resource(UINT id, LPCSTR type, LPDWORD size)
{
    HRSRC hRes = FindResource(fixModule, MAKEINTRESOURCE(id), type);
    if (!hRes) return NULL;

    HGLOBAL h = LoadResource(fixModule, hRes);
    if (!h) return NULL;

    LPVOID p = LockResource(h);
    if (!p) return NULL;

    DWORD dw = SizeofResource(fixModule, hRes);
    if (size) *size = dw;

    return p;
}

uintptr_t M2_mallocAddress;
uintptr_t M2_reallocAddress;
uintptr_t M2_freeAddress;
void *M2_malloc(size_t size)
{
    void * (*_M2_malloc)(size_t) = (void* (*)(size_t))M2_mallocAddress;
    return _M2_malloc(size);
}
void *M2_realloc(void *p, size_t size)
{
    void * (*_M2_realloc)(void *, size_t) = (void * (*)(void *, size_t))M2_reallocAddress;
    return _M2_realloc(p, size);
}
void M2_free(void *p)
{
    void (*_M2_free)(void*) = (void (*)(void *))M2_freeAddress;
    _M2_free(p);
}

void SetHook(HSQUIRRELVM v)
{
    if (bDebuggerEnabled && !gDBG) {
        gDBG = sq_rdbg_init(v, iDebuggerPort, bDebuggerAutoUpdate, bDebuggerExclusive);
        sq_rdbg_waitforconnections(gDBG);
    }

    SQInteger Hook(HSQUIRRELVM v);
    if (sq_isnull(v->_debughook)) {
        sq_pushregistrytable(v);
        sq_pushstring(v, _SC("_m2_debug_hook_"), -1);
        sq_pushuserpointer(v, v);
        sq_newclosure(v, Hook, 1);
        sq_newslot(v, -3, false);
        sq_pop(v, 1);

        sq_pushregistrytable(v);
        sq_pushstring(v, _SC("_m2_debug_hook_"), -1);
        sq_rawget(v, -2);
        sq_setdebughook(v);
        sq_pop(v, 1);
    }
}

void SQNew(HSQUIRRELVM v)
{
    LOG_F(INFO, "M2: SQVM is 0x%" PRIxPTR ", SQSharedState is 0x%" PRIxPTR ".", (uintptr_t)v, (uintptr_t)_ss(v));
    _ss(v)->_debuginfo = true;
}

void SQHookFunction(HSQUIRRELVM v, const SQChar *func, SQFUNCTION hook, HSQOBJECT *obj = NULL)
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

void SQHookMethod(HSQUIRRELVM v, const SQChar *name, const SQChar *func, SQFUNCTION hook, HSQOBJECT *obj = NULL)
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

    buf[scstrcspn(buf, "\r\n")] = 0;
    LOG_F(INFO, "M2: printf: %s", buf);
    free(buf);
}

const char* ConfigOverride(string *key)
{
    LOG_F(INFO, "M2: MWinResCfg::Get(\"%s\")", key->c_str());

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
        return sCustomResX.c_str();
    }

    if (*key == "WINDOW_H" || *key == "SCREEN_H" || *key == "LAST_CLIENT_SIZE_Y") {
        return sCustomResY.c_str();
    }

    return NULL;
}

vector<string> M2_FileFilter;
vector<vector<unsigned char>> M2_DataFilter;
void FilterPatches()
{
    if (eGameType == MgsGame::MGS && !bPatchesUnderpants) {
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

    if (eGameType == MgsGame::MGS && !bPatchesGhosts) {
        M2_FileFilter.push_back("shinrei");
    }

    if (eGameType == MgsGame::MGS && !bPatchesMedicine) {
        vector<unsigned char> MGS1_DataFilter_Medicine = { 0, 152, 0, 72, 152, 72, 152, 152, 152 };
        M2_DataFilter.push_back(MGS1_DataFilter_Medicine);
    }
}

unsigned char* M2_EpiPadStatePTR;
void M2_EpiPadState(unsigned int addr, unsigned int id, unsigned char* state)
{
    if (M2_EpiPadStatePTR != state) {
        LOG_F(INFO, "M2: Pad state is 0x%" PRIxPTR ".", state);
    }

    M2_EpiPadStatePTR = state;
}

void AnalogLoop(HSQUIRRELVM v)
{
    if (!M2_EpiPadStatePTR) return;

    gInputHub.SetDirectionMerge(0);
    gEmuTask.SetInputDirectionMerge(0);

    gInputHub.SetDeadzone(0.0);
    gEmuTask.SetInputDeadzone(0.0);

    SQFloat xL = 0.0, yL = 0.0;
    gInput.GetAnalogStickX(&xL);
    gInput.GetAnalogStickY(&yL);

    SQFloat xR = 0.0, yR = 0.0;
    gInput.GetRightAnalogStickX(&xR);
    gInput.GetRightAnalogStickY(&yR);

    // Normalize an axis from (-1, 1) to (0, 255) with 128 = center
    // https://github.com/grumpycoders/pcsx-redux/blob/a072e38d78c12a4ce1dadf951d9cdfd7ea59220b/src/core/pad.cc#L664-L673
    const auto axisToUint8 = [](float axis) {
        constexpr float scale = 1.3f;
        const float scaledValue = std::clamp<float>(axis * scale, -1.0f, 1.0f);
        return (uint8_t)(std::clamp<float>(std::round(((scaledValue + 1.0f) / 2.0f) * 255.0f), 0.0f, 255.0f));
    };

    extern SQInteger MGS1_PlaySide;
    if (MGS1_PlaySide == 0) {
        M2_EpiPadStatePTR[0x44] = axisToUint8(xL);
        M2_EpiPadStatePTR[0x45] = axisToUint8(yL);
        M2_EpiPadStatePTR[0x46] = axisToUint8(xR);
        M2_EpiPadStatePTR[0x47] = axisToUint8(yR);

        for (unsigned int i = 0x48; i < 0x54; i++) {
            M2_EpiPadStatePTR[i] = 128;
        }
    } else {
        for (unsigned int i = 0x44; i < 0x4C; i++) {
            M2_EpiPadStatePTR[i] = 128;
        }

        M2_EpiPadStatePTR[0x4C] = axisToUint8(xL);
        M2_EpiPadStatePTR[0x4D] = axisToUint8(yL);
        M2_EpiPadStatePTR[0x4E] = axisToUint8(xR);
        M2_EpiPadStatePTR[0x4F] = axisToUint8(yR);

        for (unsigned int i = 0x50; i < 0x54; i++) {
            M2_EpiPadStatePTR[i] = 128;
        }
    }
}

SQInteger SQ_util_is_notice_skipable(HSQUIRRELVM v)
{
    sq_pushbool(v, true);
    return 1;
}

bool M2_LaunchIntent = true;
SQInteger SQ_util_get_launch_intent_id(HSQUIRRELVM v)
{
    if (M2_LaunchIntent)
        sq_pushstring(v, _SC("MAIN_STORY"), -1);
    else
        sq_pushstring(v, _SC(""), -1);

    return 1;
}

SQInteger SQ_util_clear_launch_intent_id(HSQUIRRELVM v)
{
    M2_LaunchIntent = false;
    return 0;
}

SQInteger M2_StartPadId = 4;
SQInteger SQ_SystemEtc_setStartPadId(HSQUIRRELVM v)
{
    sq_getinteger(v, 1, &M2_StartPadId);
    return 0;
}

SQInteger SQ_SystemEtc_getStartPadId(HSQUIRRELVM v)
{
    sq_pushinteger(v, M2_StartPadId);
    return 1;
}

SQInteger MGS1_GlobalsPTR;
SQInteger MGS1_LoaderPTR;

SQObject SQObj_util_get_memory_define_table;
SQInteger SQ_util_get_memory_define_table(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);

    SQObjectPtr ret;
    SQObjectPtr closure = SQObj_util_get_memory_define_table;
    bool res = v->Call(closure, nargs, v->_stackbase, ret, false);
    if (res) {
        if (eGameType == MgsGame::MGS) {
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

SQInteger SQReturn_init_system_1st(HSQUIRRELVM v)
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
        SQ_util_get_memory_define_table, &SQObj_util_get_memory_define_table);

    return 0;
}

SQInteger _SQ_init_emulator_get_arch_sub_info(HSQUIRRELVM v)
{
    sq_pushinteger(v, bAnalogMode ? USE_ANALOG : 0);
    return 1;
}

SQInteger SQReturn_init_system_last(HSQUIRRELVM v)
{
    SQHookFunction(v, _SC("_init_emulator_get_arch_sub_info"), _SQ_init_emulator_get_arch_sub_info);
    return 0;
}

SQInteger _SQReturn_update_gadgets(HSQUIRRELVM v)
{
    if (MGS1_GlobalsPTR != 0 && MGS1_LoaderPTR != 0) {
        SQInteger MGS1_StageNamePTR = MGS1_GlobalsPTR;
        SQInteger MGS1_GameStatePTR = MGS1_GlobalsPTR + 16;

        char MGS1_StageName[8] = { 0 };
        char MGS1_LoaderName[8] = { 0 };
        for (int i = 0; i < sizeof(MGS1_StageName) / sizeof(uint32_t); i++) {
            gEmuTask.GetRamValue(32, MGS1_StageNamePTR + (i * sizeof(uint32_t)),
                (SQInteger *) &MGS1_StageName[i * sizeof(uint32_t)]);
            gEmuTask.GetRamValue(32, MGS1_LoaderPTR + (i * sizeof(uint32_t)),
                (SQInteger *) &MGS1_LoaderName[i * sizeof(uint32_t)]);
        }

        SQInteger MGS1_CurrentStagePTR = MGS1_GameStatePTR + (6 * sizeof(short));
        SQInteger MGS1_CurrentStage = 0;
        gEmuTask.GetRamValue(16, MGS1_CurrentStagePTR, &MGS1_CurrentStage);

        if (bGameDevMenu) {
            if (strcmp(MGS1_LoaderName, "title") == 0 && strcmp(MGS1_StageName, "select") != 0) {
                strcpy(MGS1_LoaderName, "select");
                for (int i = 0; i < sizeof(MGS1_StageName) / sizeof(uint32_t); i++) {
                    gEmuTask.SetRamValue(32, MGS1_LoaderPTR + (i * sizeof(uint32_t)),
                        *(SQInteger *)&MGS1_LoaderName[i * sizeof(uint32_t)]);
                }
            }
        }
    }

    return 0;
}

void *LoadImage(void *dst, void *src, size_t num)
{
    LOG_F(INFO, "M2: Loading image at 0x%" PRIxPTR " with size %d bytes.", src, num);

    uint8_t* MGS1_LoaderScanResult = Memory::PatternScanBuffer(src, num, "00 00 00 00 69 6E 69 74 00 00 00 00");
    if (MGS1_LoaderScanResult)
        MGS1_LoaderPTR = (MGS1_LoaderScanResult - 0x30) - (uint8_t *) src;
    else
        MGS1_LoaderPTR = 0;

    return memmove(dst, src, num);
}

SQInteger MGS1_PlaySide;
SQInteger SQReturn_set_playside_mgs(HSQUIRRELVM v)
{
    SQVM::CallInfo &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 1];
    MGS1_PlaySide = _integer(obj);
    return 0;
}

SQInteger SQNative_setDotmatrix(HSQUIRRELVM v)
{
    if (bDotMatrix) {
        // Do this here as the native call is surprisingly expensive (?!)
        SQObjectPtr* obj = &v->_stack._vals[v->_stackbase + 1];
        _integer(*obj) = bDotMatrix.value();
    }
    return 0;
}

SQInteger SQNative_setRamValue(HSQUIRRELVM v)
{
    SQObjectPtr* width = &v->_stack._vals[v->_stackbase + 1];
    SQObjectPtr* offset = &v->_stack._vals[v->_stackbase + 2];
    SQObjectPtr* value = &v->_stack._vals[v->_stackbase + 3];

    SQObjectPtr* patch = &v->_stack._vals[v->_stackbase - 5];
    if (!sq_istable(*patch)) return 0;

    SQInteger address = 0;

    sq_pushobject(v, *patch);
    sq_pushstring(v, "offset", -1);
    if (SQ_SUCCEEDED(sq_get(v, -2))) {
        sq_getinteger(v, -1, &address);
        sq_pop(v, 1);
    }
    sq_pop(v, 1);

    if (!bPatchesGlobalRAM && address != 0x200000) {
        if (_integer(*offset) == address) {
            LOG_F(INFO, "M2: filtering RAM patch offset 0x%" PRIxPTR ".", address);
        }
        return 1;
    }

    return 0;
}

SQInteger SQNative_entryCdRomPatch(HSQUIRRELVM v)
{
    SQObjectPtr* offset = &v->_stack._vals[v->_stackbase + 1];
    SQObjectPtr* data = &v->_stack._vals[v->_stackbase + 2];

    SQObjectPtr* patch = &v->_stack._vals[v->_stackbase - 3];
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
        SQObjectPtr ent;
        while (_array(*data)->Get(i++, ent)) {
            buffer.push_back(_integer(ent)); // lmao
        }
    }
    sq_pop(v, 1);

    if (file) {
        if (!bPatchesGlobalCDROM) {
            LOG_F(INFO, "M2: filtering CD-ROM patch file %s.", file);
            return 1;
        }

        for (auto &filter : M2_FileFilter) {
            if (strncmp(filter.c_str(), file, filter.length()) == 0)
            {
                LOG_F(INFO, "M2: filtering CD-ROM patch file %s.", file);
                return 1;
            }
        }
    }
    else if (!buffer.empty()) {
        if (!bPatchesGlobalCDROM) {
            LOG_F(INFO, "M2: filtering CD-ROM patch offset 0x%" PRIxPTR ".", _integer(*offset));
            return 1;
        }

        for (auto &filter : M2_DataFilter) {
            if (filter == buffer)
            {
                LOG_F(INFO, "M2: filtering CD-ROM patch offset 0x%" PRIxPTR ".", _integer(*offset));
                return 1;
            }
        }
    }

    return 0;
}

string M2_Disk;
SQInteger SQNative_setupCdRom(HSQUIRRELVM v)
{
    HSQOBJECT obj; sq_getstackobj(v, 2, &obj);
    M2_Disk.assign(_stringval(obj));

    LOG_F(INFO, "M2: Mounted CD-ROM image: %s.", M2_Disk.c_str());
    return 0;
}

SQInteger M2_Tray;
SQInteger SQNative_setCdRomShellOpen(HSQUIRRELVM v)
{
    HSQOBJECT obj; sq_getstackobj(v, 2, &obj);
    M2_Tray = _integer(obj);

    LOG_F(INFO, "M2: CD-ROM tray is %s.", M2_Tray ? "open" : "closed");
    return 0;
}

string M2_ROM;
SQInteger SQNative_init(HSQUIRRELVM v)
{
    // Do some strange things to confirm that this is EmuTask::init() and not another init().
    gEmuTask.SetVM(v);
    if (!gEmuTask.Get()) return 0;

    HSQOBJECT obj; sq_getstackobj(v, -1, &obj);
    HSQOBJECT instance; sq_getstackobj(v, 1, &instance);
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

array<pair<const SQChar *, SQFUNCTION>, 6> M2_NativeCallTable = {
    make_pair("setDotmatrix", SQNative_setDotmatrix),
    make_pair("setRamValue", SQNative_setRamValue),
    make_pair("entryCdRomPatch", SQNative_entryCdRomPatch),
    make_pair("setCdRomShellOpen", SQNative_setCdRomShellOpen),
    make_pair("setupCdRom", SQNative_setupCdRom),
    make_pair("init", SQNative_init),
};

bool FixNativeCall(HSQUIRRELVM v, SQFUNCTION func, SQNativeClosure *closure, const SQChar *name)
{
    if (!name) return true;

    for (auto &func : M2_NativeCallTable) {
        if (strcmp(name, func.first) == 0) {
            if (func.second(v)) return false;
            break;
        }
    }

    return true;
}

SQInteger SQReturn_setSmoothing(HSQUIRRELVM v)
{
    if (bSmoothing) gEmuTask.SetSmoothing(bSmoothing.value());
    if (bScanline) gEmuTask.SetScanline(bScanline.value());
    return 0;
}

SQInteger _SQReturn_set_disk_patch(HSQUIRRELVM v)
{
    extern bool Ketchup_Process(HSQUIRRELVM v);
    gEmuTask.SetVM(v);
    Ketchup_Process(v);
    return 0;
}

SQInteger M2_DevId;
SQInteger SQReturn_set_current_title_dev_id(HSQUIRRELVM v)
{
    SQVM::CallInfo &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 1];
    M2_DevId = _integer(obj);

    LOG_F(INFO, "M2: Set title ID: %d.", M2_DevId);
    return 0;
}

SQInteger M2_DiskId;
SQInteger _SQReturn_get_disk_path(HSQUIRRELVM v)
{
    SQVM::CallInfo &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 2];
    M2_DiskId = _integer(obj);

    LOG_F(INFO, "M2: Set disk ID: %d.", M2_DiskId);
    return 0;
}

string M2_DevType;
SQInteger SQReturn_set_game_regionTag(HSQUIRRELVM v)
{
    SQVM::CallInfo &my = v->_callsstack[v->_callsstacksize - 1];
    SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + 1];
    M2_DevType.assign(_stringval(obj));

    LOG_F(INFO, "M2: Set title version: %s.", M2_DevType.c_str());
    return 0;
}

array<pair<const SQChar *, SQFUNCTION>, 9> M2_ReturnTable = {
    make_pair("init_system_1st", SQReturn_init_system_1st),
    make_pair("init_system_last", SQReturn_init_system_last),
    make_pair("set_playside_mgs", SQReturn_set_playside_mgs),
    make_pair("_update_gadgets", _SQReturn_update_gadgets),
    make_pair("setSmoothing", SQReturn_setSmoothing),
    make_pair("_set_disk_patch", _SQReturn_set_disk_patch),
    make_pair("_get_disk_path", _SQReturn_get_disk_path),
    make_pair("set_current_title_dev_id", SQReturn_set_current_title_dev_id),
    make_pair("set_game_regionTag", SQReturn_set_game_regionTag),
};

void FixLoop(HSQUIRRELVM v, SQInteger event_type, const SQChar *src, const SQChar *name, SQInteger line)
{
    gEmuTask.SetVM(v);
    gInputHub.SetVM(v);
    gInput.SetVM(v);

    if (name && event_type == _SC('r')) {
        for (auto &func : M2_ReturnTable) {
            if (strcmp(name, func.first) == 0) {
                func.second(v);
                break;
            }
        }
    }

    if (bAnalogMode) AnalogLoop(v);
}

SQInteger HookNative(SQFUNCTION func, HSQUIRRELVM v)
{
    SetHook(v);

    // Ignore the calls to the debug hook
    SQInteger Hook(HSQUIRRELVM v);
    if (func == Hook) return func(v);

    const SQChar *name = NULL;
    SQNativeClosure *closure = NULL;
    if (v && v->ci && sq_isnativeclosure(v->ci->_closure)) {
        closure = _nativeclosure(v->ci->_closure);
        if (sq_isstring(closure->_name)) {
            name = _stringval(closure->_name);
        }
    }

    if (iNativeLevel >= 1) {
        void TraceNative(HSQUIRRELVM v, SQFUNCTION func, SQNativeClosure *closure, const SQChar *name);
        TraceNative(v, func, closure, name);
    }

    if (name && (strcmp(name, "printf") == 0 || strcmp(name, "print") == 0)) {
        SQChar *print = NULL;
        SQInteger length = 0;
        if (SQ_SUCCEEDED(sqstd_format(v, 2, &length, &print))) {
            print[scstrcspn(print, "\r\n")] = 0;
            LOG_F(INFO, "M2: sq_printf: %s", print);
        }
    }

    if (FixNativeCall(v, func, closure, name)) {
        return func(v);
    }

    return 0;
}

bool SquirrelMain(HSQUIRRELVM v)
{
    LOG_F(INFO, "M2: SQVM 0x%" PRIxPTR " hooked: debug info is %s, exceptions are %s.", (uintptr_t) v,
        (_ss(v)->_debuginfo ? "enabled" : "disabled"),
        (_ss(v)->_notifyallexceptions ? "enabled" : "disabled")
    );

    if (bDebuggerEnabled && bDebuggerExclusive && gDBG) {
        gDBG->Init(v);
        return true;
    }

    return false;
}

SQInteger Hook(HSQUIRRELVM v)
{
    SQObjectPtr debughook = v->_debughook;
    v->_debughook = _null_;

    if (bDebuggerEnabled && !bDebuggerExclusive && gDBG) {
        extern SQInteger debug_hook(HSQUIRRELVM v, HSQUIRRELVM _v, HSQREMOTEDBG rdbg);
        debug_hook(NULL, v, gDBG);
    }

    FixData* data = (FixData *) sq_getforeignptr(v);

    if (!data) {
        sq_setforeignptr(v, calloc(1, sizeof(FixData)));
        data = (FixData *) sq_getforeignptr(v);
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
        void Trace(HSQUIRRELVM v);
        Trace(v);
    }

    FixLoop(v, event_type, src, func, line);

    v->_debughook = debughook;
    return 0;
}

#define CALL(func) { extern void func; func; }

mutex mainThreadFinishedMutex;
condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;
DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    if (DetectGame())
    {
        FilterPatches();

        CALL(ScanFunctions());

        CALL(M2Hook());
        CALL(LoadHook());
        CALL(SquirrelHook());

        if (bCustomResolution) CALL(ConfigHook());
        if (bBorderlessMode) CALL(BorderlessPatch());
        if (bAnalogMode) {
            CALL(AnalogPatch());
            CALL(AnalogHook());
        }
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
    fixModule = hModule;
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
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
