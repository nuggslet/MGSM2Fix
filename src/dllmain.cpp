#include "stdafx.h"
#include "helper.hpp"

#include "squirrel.h"
#include "sqvm.h"
#include "sqstate.h"
#include "sqrdbg.h"
#include "sqdbgserver.h"

#include "resource.h"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE fixModule = 0;

inipp::Ini<char> ini;

// INI Variables
int iInjectionDelay;
bool bDebuggerEnabled;
int iDebuggerPort;
bool bDebuggerAutoUpdate;
bool bSmoothing = true;

// Variables
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "0.1";

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

    inipp::get_value(ini.sections["MGSM2Fix Parameters"], "InjectionDelay", iInjectionDelay);

    inipp::get_value(ini.sections["Squirrel Debugger"], "Enabled", bDebuggerEnabled);
    inipp::get_value(ini.sections["Squirrel Debugger"], "Port", iDebuggerPort);
    inipp::get_value(ini.sections["Squirrel Debugger"], "AutoUpdate", bDebuggerAutoUpdate);

    inipp::get_value(ini.sections["Screen"], "Smoothing", bSmoothing);

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);

    LOG_F(INFO, "Config Parse: bDebuggerEnabled: %d", bDebuggerEnabled);
    LOG_F(INFO, "Config Parse: iDebuggerPort: %d", iDebuggerPort);
    LOG_F(INFO, "Config Parse: bDebuggerAutoUpdate: %d", bDebuggerAutoUpdate);
    LOG_F(INFO, "Config Parse: bSmoothing: %d", bSmoothing);

    if (bDebuggerEnabled)
    {
        LOG_F(INFO, "Config Parse: Debugger enabled, other features will be disabled.");
    }
}

void DetectGame()
{
    // Get game name and exe path
    LPSTR exePath = new CHAR[_MAX_PATH];
    GetModuleFileName(baseModule, exePath, MAX_PATH);
    sExePath = string(exePath);
    sExeName = sExePath.substr(sExePath.find_last_of("/\\") + 1);

    LOG_F(INFO, "Game Name: %s", sExeName.c_str());
    LOG_F(INFO, "Game Path: %s", sExePath.c_str());

    if (sExeName == "METAL GEAR SOLID.exe")
    {
        LOG_F(INFO, "Detected game is: Metal Gear Solid");
    }
}

DWORD32 M2_mallocAddress = 0;
DWORD32 M2_reallocAddress = 0;
DWORD32 M2_freeAddress = 0;
void ScanFunctions()
{
    // MGS 1: Squirrel call
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_mallocScanResult = Memory::PatternScan(baseModule, "8B FF 55 8B EC 56 8B 75 08 83 FE E0 77 30 85 F6");
        if (MGS1_mallocScanResult)
        {
            M2_mallocAddress = (uintptr_t)MGS1_mallocScanResult;
            LOG_F(INFO, "MGS 1: malloc is 0x%" PRIxPTR ".", M2_mallocAddress);
        }
        else if (!MGS1_mallocScanResult)
        {
            LOG_F(INFO, "MGS 1: malloc scan failed.");
        }

        uint8_t* MGS1_reallocScanResult = Memory::PatternScan(baseModule, "8B FF 55 8B EC 57 8B 7D 08 85 FF 75 0B FF 75 0C");
        if (MGS1_reallocScanResult)
        {
            M2_reallocAddress = (uintptr_t)MGS1_reallocScanResult;
            LOG_F(INFO, "MGS 1: realloc is 0x%" PRIxPTR ".", M2_reallocAddress);
        }
        else if (!MGS1_reallocScanResult)
        {
            LOG_F(INFO, "MGS 1: realloc scan failed.");
        }

        uint8_t* MGS1_freeScanResult = Memory::PatternScan(baseModule, "8B FF 55 8B EC 83 7D 08 00 74 2D FF 75 08 6A 00");
        if (MGS1_freeScanResult)
        {
            M2_freeAddress = (uintptr_t)MGS1_freeScanResult;
            LOG_F(INFO, "MGS 1: free is 0x%" PRIxPTR ".", M2_freeAddress);
        }
        else if (!MGS1_freeScanResult)
        {
            LOG_F(INFO, "MGS 1: free scan failed.");
        }
    }
}

void SquirrelPatch()
{
    // MGS 1: Squirrel patch
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_SQSharedStateScanResult = Memory::PatternScan(baseModule, "66 C7 86 AC 00 00 00 00 00 8B 4D F4 64 89 0D 00");
        if (MGS1_SQSharedStateScanResult)
        {
            uint8_t* MGS1_SQSharedStateFlagsPTR = (uint8_t*)(MGS1_SQSharedStateScanResult + 7);
            uint8_t MGS1_SQSharedStateFlags[] = { true, true };
            Memory::PatchBytes((uintptr_t)MGS1_SQSharedStateFlagsPTR, (const char*)MGS1_SQSharedStateFlags, sizeof(MGS1_SQSharedStateFlags));
            LOG_F(INFO, "MGS 1: M2: Patched SQSharedState, exceptions & debug info enabled.");
        }
        else if (!MGS1_SQSharedStateScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: SQSharedState pattern scan failed.");
        }
    }
}

// MGS 1: Squirrel hook
DWORD32 MGS1_SQVMReturnJMP;
DWORD32 MGS1_SQVMInstancePTR = 0;
void __declspec(naked) MGS1_SQVM_CC()
{
    __asm
    {
        mov[MGS1_SQVMInstancePTR], esi
        mov dword ptr[esi + 0A4h], 0FFFFFFFFh
        jmp[MGS1_SQVMReturnJMP]
    }
}

void SquirrelHook()
{
    // MGS 1: Squirrel hook
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_SQVMScanResult = Memory::PatternScan(baseModule, "C7 86 A4 00 00 00 FF FF FF FF 89 86 A0 00 00 00");
        if (MGS1_SQVMScanResult)
        {
            DWORD32 MGS1_SQVMAddress = (uintptr_t)MGS1_SQVMScanResult;
            int MGS1_SQVMHookLength = Memory::GetHookLength((char*)MGS1_SQVMAddress, 4);
            MGS1_SQVMReturnJMP = MGS1_SQVMAddress + MGS1_SQVMHookLength;
            Memory::DetourFunction32((void*)MGS1_SQVMAddress, MGS1_SQVM_CC, MGS1_SQVMHookLength);

            LOG_F(INFO, "MGS 1: M2: SQVM hook length is %d bytes.", MGS1_SQVMHookLength);
            LOG_F(INFO, "MGS 1: M2: SQVM hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SQVMAddress);
        }
        else if (!MGS1_SQVMScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: SQVM pattern scan failed.");
        }
    }
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

// M2: Forbid smoothing
HSQOBJECT M2_ForbidSmoothingFunc = { OT_NULL };
void InvokeForbidSmoothing(HSQUIRRELVM v, const SQChar *src, SQInteger line)
{
    if (bSmoothing) return;
    if (M2_ForbidSmoothingFunc._type == OT_WEAKREF) return;
    if (strcmp(src, "system/script/systemdata_setting_screen.nut") != 0 || line != 15) return;

    if (sq_isnull(M2_ForbidSmoothingFunc))
    {
        const SQChar* M2_ForbidSmoothingNUT = (SQChar*)Resource(IDR_NUT2, "NUT", NULL);
        if (!M2_ForbidSmoothingNUT)
            LOG_F(INFO, "M2: Error loading the smoothing function.");
        else
        {
            if (SQ_FAILED(sq_compilebuffer(v, M2_ForbidSmoothingNUT, (SQInteger)scstrlen(M2_ForbidSmoothingNUT), _SC("M2_FORBID_SMOOTHING"), SQFalse)))
                LOG_F(INFO, "M2: Error compiling the smoothing function.");

            sq_getstackobj(v, -1, &M2_ForbidSmoothingFunc);
            sq_addref(v, &M2_ForbidSmoothingFunc);
            sq_pop(v, 1);
        }
    }

    if (!sq_isnull(M2_ForbidSmoothingFunc))
    {
        sq_pushobject(v, M2_ForbidSmoothingFunc);
        sq_pushroottable(v);
        if (SQ_FAILED(sq_call(v, 1, SQFalse, SQFalse)))
            LOG_F(INFO, "M2: Error calling the smoothing function.");
        else
        {
            LOG_F(INFO, "M2: Successfully called the smoothing function.");
            M2_ForbidSmoothingFunc._type = OT_WEAKREF;
        }
        sq_pop(v, 1);
    }
}

SQInteger Hook(HSQUIRRELVM v)
{
    v->_debughook._type = OT_NULL;

    SQUserPointer up;
    SQInteger event_type, line;
    const SQChar *src, *func;
    sq_getinteger(v, 2, &event_type);
    sq_getstring(v, 3, &src);
    sq_getinteger(v, 4, &line);
    sq_getstring(v, 5, &func);
    sq_getuserpointer(v, -1, &up);

    InvokeForbidSmoothing(v, src, line);

    v->_debughook._type = OT_NATIVECLOSURE;
    return 0;
}

void SquirrelMain(HSQUIRRELVM v)
{
    LOG_F(INFO, "M2: SQVM is 0x%" PRIxPTR ".", (uintptr_t)v);
    LOG_F(INFO, "M2: SQSharedState is 0x%" PRIxPTR ".", (uintptr_t)_ss(v));

    // sq_enabledebuginfo(v, true);
    LOG_F(INFO, "M2: Debug info is %s.", _ss(v)->_debuginfo ? "enabled" : "disabled");
    // sq_notifyallexceptions(v, true);
    LOG_F(INFO, "M2: Exceptions are %s.", _ss(v)->_notifyallexceptions ? "enabled" : "disabled");

    if (bDebuggerEnabled) {
        HSQREMOTEDBG dbg = sq_rdbg_init(v, iDebuggerPort, bDebuggerAutoUpdate);
        sq_rdbg_waitforconnections(dbg);
        return;
    }

    sq_pushregistrytable(v);
    sq_pushstring(v, _SC("_m2_debug_hook_"), -1);
    sq_pushuserpointer(v, v);
    sq_newclosure(v, Hook, 1);
    sq_newslot(v, -3, SQFalse);
    sq_pop(v, 1);

    sq_pushregistrytable(v);
    sq_pushstring(v, _SC("_m2_debug_hook_"), -1);
    sq_rawget(v, -2);
    sq_setdebughook(v);
    sq_pop(v, 1);
}

void SquirrelSetup()
{
    // MGS 1: Squirrel setup
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        while (MGS1_SQVMInstancePTR == 0) {
            Sleep(1);
        }

        SquirrelMain((HSQUIRRELVM)MGS1_SQVMInstancePTR);
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    DetectGame();

    SquirrelPatch();
    ScanFunctions();
    SquirrelHook();

    Sleep(iInjectionDelay);
    SquirrelSetup();

    return true; // end thread
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
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);

        if (mainHandle)
        {
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
