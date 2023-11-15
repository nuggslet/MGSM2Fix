#include "stdafx.h"

#include "helper.hpp"

#include "sqrdbg.h"
#include "sqdbgserver.h"

#include "resource.h"

#include "emutask.h"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE fixModule = 0;

inipp::Ini<char> ini;

HSQREMOTEDBG gDBG;
EmuTask gEmuTask;

// INI Variables
bool bDebuggerEnabled;
int iDebuggerPort;
bool bDebuggerAutoUpdate;
bool bSmoothing = true;
bool bScanline;
bool bDotMatrix;
int iLevel;
int iNativeLevel;
bool bCustomResolution;
int iCustomResX;
int iCustomResY;
bool bWindowedMode;
bool bBorderlessMode;
string sFullscreenMode;
string sCustomResX;
string sCustomResY;

// Variables
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "0.3";

typedef struct {
    bool hooked;
    SQChar src[MAX_PATH];
    SQInteger line;
} FixData;

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

    inipp::get_value(ini.sections["Screen"], "Smoothing", bSmoothing);
    inipp::get_value(ini.sections["Screen"], "Scanline", bScanline);
    inipp::get_value(ini.sections["Screen"], "DotMatrix", bDotMatrix);

    inipp::get_value(ini.sections["Tracing"], "Level", iLevel);
    inipp::get_value(ini.sections["Tracing"], "NativeLevel", iNativeLevel);

    inipp::get_value(ini.sections["Custom Resolution"], "Enabled", bCustomResolution);
    inipp::get_value(ini.sections["Custom Resolution"], "Width", iCustomResX);
    inipp::get_value(ini.sections["Custom Resolution"], "Height", iCustomResY);
    inipp::get_value(ini.sections["Custom Resolution"], "Windowed", bWindowedMode);
    inipp::get_value(ini.sections["Custom Resolution"], "Borderless", bBorderlessMode);

    // Log config parse
    LOG_F(INFO, "Config Parse: bDebuggerEnabled: %d", bDebuggerEnabled);
    LOG_F(INFO, "Config Parse: iDebuggerPort: %d", iDebuggerPort);
    LOG_F(INFO, "Config Parse: bDebuggerAutoUpdate: %d", bDebuggerAutoUpdate);
    LOG_F(INFO, "Config Parse: bSmoothing: %d", bSmoothing);
    LOG_F(INFO, "Config Parse: bScanline: %d", bScanline);
    LOG_F(INFO, "Config Parse: bDotMatrix: %d", bDotMatrix);
    LOG_F(INFO, "Config Parse: iLevel: %d", iLevel);
    LOG_F(INFO, "Config Parse: iNativeLevel: %d", iNativeLevel);
    LOG_F(INFO, "Config Parse: bCustomResolution: %d", bCustomResolution);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: bWindowedMode: %d", bWindowedMode);
    LOG_F(INFO, "Config Parse: bBorderlessMode: %d", bBorderlessMode);

    if (bDebuggerEnabled)
    {
        LOG_F(INFO, "Config Parse: Debugger enabled, other features will be disabled.");
    }

    // Force windowed mode if borderless is enabled but windowed is not. There is undoubtedly a more elegant way to handle this.
    if (bBorderlessMode)
    {
        bWindowedMode = true;
        LOG_F(INFO, "Config Parse: Borderless mode enabled.");
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

void BorderlessPatch()
{
    // MGS 1: Borderless patch
    if (sExeName == "METAL GEAR SOLID.exe" && bBorderlessMode)
    {
        uint8_t* MGS1_MWinResCfgSetWindowScanResult = Memory::PatternScan(baseModule, "B8 00 00 CE 02 BE 00 00 CA 02");
        if (MGS1_MWinResCfgSetWindowScanResult)
        {
            uint8_t* MGS1_MWinResCfgSetWindowPTR = (uint8_t*)MGS1_MWinResCfgSetWindowScanResult;
            uint8_t MGS1_MWinResCfgSetWindowFlags[] = { "\xB8\x00\x00\x00\x90\xBE\x00\x00\x00\x90" };
            Memory::PatchBytes((uintptr_t)MGS1_MWinResCfgSetWindowPTR, (const char*)MGS1_MWinResCfgSetWindowFlags, sizeof(MGS1_MWinResCfgSetWindowFlags) - 1);
            LOG_F(INFO, "MGS 1: M2: Borderless: MWinResCfg::SetWindow patched.");
        }
        else if (!MGS1_MWinResCfgSetWindowScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: MWinResCfg::SetWindow pattern scan failed.");
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
            uint8_t MGS1_SQSharedStateFlags[] = { true, false };
            Memory::PatchBytes((uintptr_t)MGS1_SQSharedStateFlagsPTR, (const char*)MGS1_SQSharedStateFlags, sizeof(MGS1_SQSharedStateFlags));
            LOG_F(INFO, "MGS 1: M2: SQSharedState::SQSharedState patched, debug info enabled.");
        }
        else if (!MGS1_SQSharedStateScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: SQSharedState::SQSharedState pattern scan failed.");
        }
    }
}

void SetHook(HSQUIRRELVM v)
{
    SQInteger Hook(HSQUIRRELVM v);
    if (sq_isnull(v->_debughook)) {
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
}

void SquirrelNew(HSQUIRRELVM v)
{
    LOG_F(INFO, "M2: SQVM is 0x%" PRIxPTR ", SQSharedState is 0x%" PRIxPTR ".", (uintptr_t)v, (uintptr_t)_ss(v));
}

// MGS 1: Squirrel hook
DWORD32 MGS1_SQVMReturnJMP;
void __declspec(naked) MGS1_SQVM_CC()
{
    __asm
    {
        push esi
        push eax
        push ecx

        push ebp
        mov ebp, esp

        push esi
        call SquirrelNew

        mov esp, ebp
        pop ebp

        pop ecx
        pop eax
        pop esi

        mov dword ptr[esi + 0A4h], 0FFFFFFFFh
        jmp[MGS1_SQVMReturnJMP]
    }
}

SQInteger HookNative(SQFUNCTION func, HSQUIRRELVM v);
DWORD32 MGS1_SQVMCallNativeReturnJMP;
void __declspec(naked) MGS1_SQVMCallNative_CC()
{
    __asm
    {
        push ebp
        mov ebp, esp

        push esi
        push eax
        call HookNative

        mov esp, ebp
        pop ebp

        mov ecx, [ebp + 18h]
        jmp[MGS1_SQVMCallNativeReturnJMP]
    }
}

DWORD32 MGS1_SqratBindFuncReturnJMP;
void __declspec(naked) MGS1_SqratBindFunc_CC()
{
    __asm
    {
        push[esp + 10h]
        push -1
        push [edi + 4]
        call sq_setnativeclosurename
        add esp, 12

        movzx eax, [esp + 20h]
        jmp[MGS1_SqratBindFuncReturnJMP]
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

            LOG_F(INFO, "MGS 1: M2: SQVM::SQVM hook length is %d bytes.", MGS1_SQVMHookLength);
            LOG_F(INFO, "MGS 1: M2: SQVM::SQVM hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SQVMAddress);
        }
        else if (!MGS1_SQVMScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: SQVM::SQVM pattern scan failed.");
        }

        uint8_t* MGS1_SQVMCallNativeScanResult = Memory::PatternScan(baseModule, "FF D0 8B 4D 18 83 C4 04 FF 8E 98 00 00 00 C6 01");
        if (MGS1_SQVMCallNativeScanResult)
        {
            DWORD32 MGS1_SQVMCallNativeAddress = (uintptr_t)MGS1_SQVMCallNativeScanResult;
            int MGS1_SQVMCallNativeHookLength = Memory::GetHookLength((char*)MGS1_SQVMCallNativeAddress, 4);
            MGS1_SQVMCallNativeReturnJMP = MGS1_SQVMCallNativeAddress + MGS1_SQVMCallNativeHookLength;
            Memory::DetourFunction32((void*)MGS1_SQVMCallNativeAddress, MGS1_SQVMCallNative_CC, MGS1_SQVMCallNativeHookLength);

            LOG_F(INFO, "MGS 1: M2: SQVM::CallNative hook length is %d bytes.", MGS1_SQVMCallNativeHookLength);
            LOG_F(INFO, "MGS 1: M2: SQVM::CallNative hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SQVMCallNativeAddress);
        }
        else if (!MGS1_SQVMCallNativeScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: SQVM::CallNative pattern scan failed.");
        }

        uint8_t* MGS1_SqratBindFuncScanResult = Memory::PatternScan(baseModule, "0F B6 44 24 20 83 C4 04 8B 4F 04 BA FD FF FF FF");
        if (MGS1_SqratBindFuncScanResult)
        {
            DWORD32 MGS1_SqratBindFuncAddress = (uintptr_t)MGS1_SqratBindFuncScanResult;
            int MGS1_SqratBindFuncHookLength = Memory::GetHookLength((char*)MGS1_SqratBindFuncAddress, 4);
            MGS1_SqratBindFuncReturnJMP = MGS1_SqratBindFuncAddress + MGS1_SqratBindFuncHookLength;
            Memory::DetourFunction32((void*)MGS1_SqratBindFuncAddress, MGS1_SqratBindFunc_CC, MGS1_SqratBindFuncHookLength);

            LOG_F(INFO, "MGS 1: M2: Sqrat::BindFunc hook length is %d bytes.", MGS1_SqratBindFuncHookLength);
            LOG_F(INFO, "MGS 1: M2: Sqrat::BindFunc hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SqratBindFuncAddress);
        }
        else if (!MGS1_SqratBindFuncScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: Sqrat::BindFunc pattern scan failed.");
        }
    }
}

const char* ConfigOverride(string *key)
{
    LOG_F(INFO, "M2: MWinResCfg::Get(\"%s\")", key->c_str());

    if (*key == "FULLSCREEN_MODE" || *key == "BOOT_FULLSCREEN") {
        return sFullscreenMode.c_str();
    }

    if (*key == "WINDOW_W" || *key == "SCREEN_W" || *key == "LAST_CLIENT_SIZE_X") {
        return sCustomResX.c_str();
    }

    if (*key == "WINDOW_H" || *key == "SCREEN_H" || *key == "LAST_CLIENT_SIZE_Y") {
        return sCustomResY.c_str();
    }

    return NULL;
}

DWORD32 MGS1_MWinResCfgGetReturnJMP;
void __declspec(naked) MGS1_MWinResCfgGet_CC()
{
    __asm
    {
        push ebp
        mov ebp, esp

        push eax
        push ecx

        mov eax, esp
        add eax, 10h

        push eax
        call ConfigOverride
        add esp, 4
        cmp eax, 0

        pop ecx

        je MGS1_MWinResCfgGet_RET
        mov esp, ebp
        pop ebp
        retn 1Ch

    MGS1_MWinResCfgGet_RET:
        pop eax
        mov esp, ebp
        pop ebp

        push ebp
        mov ebp, esp
        push 0FFFFFFFFh
        jmp[MGS1_MWinResCfgGetReturnJMP]
    }
}

void ConfigHook()
{
    // MGS 1: Configuration hook
    if (sExeName == "METAL GEAR SOLID.exe" && bCustomResolution)
    {
        uint8_t* MGS1_MWinResCfgGetScanResult = Memory::PatternScan(baseModule, "50 C6 01 00 E8 ?? ?? ?? FF 8B CF E8 78 0B 00 00");
        if (MGS1_MWinResCfgGetScanResult)
        {
            DWORD32 MGS1_MWinResCfgGetAddress = (uintptr_t)(MGS1_MWinResCfgGetScanResult - 0x48);
            int MGS1_MWinResCfgGetHookLength = Memory::GetHookLength((char*)MGS1_MWinResCfgGetAddress, 4);
            MGS1_MWinResCfgGetReturnJMP = MGS1_MWinResCfgGetAddress + MGS1_MWinResCfgGetHookLength;
            Memory::DetourFunction32((void*)MGS1_MWinResCfgGetAddress, MGS1_MWinResCfgGet_CC, MGS1_MWinResCfgGetHookLength);

            LOG_F(INFO, "MGS 1: M2: MWinResCfg::Get hook length is %d bytes.", MGS1_MWinResCfgGetHookLength);
            LOG_F(INFO, "MGS 1: M2: MWinResCfg::Get hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_MWinResCfgGetAddress);
        }
        else if (!MGS1_MWinResCfgGetScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: MWinResCfg::Get pattern scan failed.");
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

void FixNativeCall(HSQUIRRELVM v, SQFUNCTION func, SQNativeClosure *closure, const SQChar *name)
{
    if (!name) return;

    // Do this here as the native call is surprisingly expensive (?!)
    if (strcmp(name, "setDotmatrix") == 0) {
        SQObjectPtr* obj = &v->_stack._vals[v->_stackbase + 1];
        _integer(*obj) = bDotMatrix;
    }
}

void FixLoop(HSQUIRRELVM v, SQInteger event_type, const SQChar *src, SQInteger line)
{
    gEmuTask.SetVM(v);

    gEmuTask.SetSmoothing(bSmoothing);
    gEmuTask.SetScanline(bScanline);
}

void TraceParameter(stringstream &trace, SQObjectPtr obj, int level)
{
    switch (obj._type) {
    case OT_BOOL:
        trace << _integer(obj) ? "true" : "false";
        break;
    case OT_INTEGER:
        trace << _integer(obj);
        break;
    case OT_FLOAT:
        trace << _float(obj);
        break;
    case OT_STRING:
    {
        string str(_string(obj)->_val, _string(obj)->_len);
        str.erase(remove(str.begin(), str.end(), '\n'), str.cend());
        trace << "\"" << str.c_str() << "\"";
        break;
    }
    case OT_ARRAY:
    {
        int i = 0;
        SQObjectPtr ent;
        trace << "[";
        if (level >= 1) {
            while (_array(obj)->Get(i++, ent)) {
                TraceParameter(trace, ent, level - 1);
                if (i != _array(obj)->_values.size()) trace << ", ";
            }
        }
        trace << "]";
        break;
    }
    case OT_TABLE:
    {
        SQObjectPtr i = SQObjectPtr(0);
        SQObjectPtr key, value;
        trace << "{";
        if (level >= 1) {
            while (_table(obj)->Next(false, i, key, value) > 0) {
                TraceParameter(trace, key, level - 1);
                trace << ": ";
                TraceParameter(trace, value, level - 1);
                i = SQObjectPtr(_integer(i) + 1);
                if (_table(obj)->Next(false, i, key, value) > 0) trace << ", ";
            }
        }
        trace << "}";
        break;
    }
    case OT_CLASS:
        trace << hex << "C" << (uintptr_t) _class(obj)->_typetag << dec;
        if (_class(obj)->_base) {
            trace << "::";
            TraceParameter(trace, _class(obj)->_base, level);
        }
        break;
    case OT_INSTANCE:
        trace << hex << "I" << (uintptr_t)_instance(obj)->_userpointer << dec;
        if (_instance(obj)->_class) {
            trace << "?";
            TraceParameter(trace, _instance(obj)->_class, level);
        }
        break;
    case OT_CLOSURE:
    {
        SQFunctionProto *proto = _funcproto(_closure(obj)->_function);
        if (proto && sq_isstring(proto->_name)) {
            trace << _string(proto->_name)->_val;
        }
        trace << "()";
        break;
    }
    case OT_NATIVECLOSURE:
        if (sq_isstring(_nativeclosure(obj)->_name))
            trace << _string(_nativeclosure(obj)->_name)->_val;
        trace << hex << "{" << "0x" << _nativeclosure(obj)->_function << "}()" << dec;
        break;
    case OT_USERPOINTER:
        trace << "(void *) " << _userpointer(obj);
        break;
    case OT_NULL:
        trace << "null";
        break;
    default:
        trace << hex << "OT_" << obj._type << dec;
        break;
    }
}

void TraceNative(HSQUIRRELVM v, SQFUNCTION func, SQNativeClosure *closure, const SQChar *name)
{
    stringstream trace;

    FixData* data = (FixData*)v->_foreignptr;
    if (data && data->src[0] != 0) {
        trace << data->src << ":" << data->line << " -> ";
        data->src[0] = 0;
    }

    if (name) trace << name;
    trace << hex << "{" << "0x" << func << "}(" << dec;
    trace << hex << "0x" << v << dec;

    if (closure) {
        if (iNativeLevel >= 2) {
            for (SQInteger i = 0; i < closure->_typecheck.size(); i++) {
                if (i == 0) trace << ", ";
                SQObjectPtr obj = v->_stack._vals[v->_stackbase + i];
                TraceParameter(trace, obj, iNativeLevel - 2);
                if ((i + 1) < closure->_typecheck.size()) trace << ", ";
            }
        }
    }

    trace << ")";

    string tracestring = trace.str();
    LOG_F(INFO, "M2: CallNative: %s", tracestring.c_str());
}

void Trace(HSQUIRRELVM v)
{
    SQUserPointer up;
    SQInteger event_type, line;
    const SQChar *src, *func;
    sq_getinteger(v, 2, &event_type);
    sq_getstring(v, 3, &src);
    sq_getinteger(v, 4, &line);
    sq_getstring(v, 5, &func);
    sq_getuserpointer(v, -1, &up);

    SQVM::CallInfo &my = v->_callsstack[v->_callsstacksize - 1];
    SQVM::CallInfo &ci = v->_callsstack[v->_callsstacksize - 2];

    if (event_type == _SC('c') || event_type == _SC('r')) {
        SQClosure *closure = _closure(ci._closure);
        if (closure) {
            SQFunctionProto *proto = _funcproto(closure->_function);
            if (proto && sq_isstring(proto->_name)) {
                stringstream trace;

                trace << (_SC('c') ? "Call" : "Return") << ": " << src << ":" << line;
                trace << " " << (event_type == _SC('c') ? "->" : "<-") << " " << func;

                trace << "(";
                if (iLevel >= 2) {
                    for (SQInteger i = 0; i < proto->_nparameters; i++) {
                        SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + i];
                        TraceParameter(trace, obj, iLevel - 2);
                        if ((i + 1) < proto->_nparameters) trace << ", ";
                    }
                }
                trace << ")";

                string tracestring = trace.str();
                LOG_F(INFO, "M2: %s", tracestring.c_str());
            }
        }
    }
    else if (event_type == _SC('l')) {
        LOG_F(INFO, "M2: Line: %s:%d", src, line);
    }
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
            name = _string(closure->_name)->_val;
        }
    }

    if (iNativeLevel >= 1) {
        TraceNative(v, func, closure, name);
    }

    if (name && strcmp(name, "printf") == 0) {
        SQChar *print = NULL;
        SQInteger length = 0;
        if (SQ_SUCCEEDED(sqstd_format(v, 2, &length, &print))) {
            print[strcspn(print, "\r\n")] = 0;
            LOG_F(INFO, "M2: Print: %s", print);
        }
    }

    FixNativeCall(v, func, closure, name);

    return func(v);
}

bool SquirrelMain(HSQUIRRELVM v)
{
    LOG_F(INFO, "M2: SQVM 0x%" PRIxPTR " hooked: debug info is %s, exceptions are %s.", (uintptr_t)v, (_ss(v)->_debuginfo ? "enabled" : "disabled"), (_ss(v)->_notifyallexceptions ? "enabled" : "disabled"));

    if (bDebuggerEnabled && !gDBG) {
        gDBG = sq_rdbg_init(v, iDebuggerPort, bDebuggerAutoUpdate);
        sq_rdbg_waitforconnections(gDBG);
        return true;
    }

    if (bDebuggerEnabled && gDBG) {
        gDBG->Init(v);
        return true;
    }

    return false;
}

SQInteger Hook(HSQUIRRELVM v)
{
    SQObjectPtr debughook = v->_debughook;
    v->_debughook = _null_;

    FixData* data = (FixData*)v->_foreignptr;

    if (!data) {
        v->_foreignptr = calloc(1, sizeof(FixData));
        data = (FixData*)v->_foreignptr;
    }

    if (!data->hooked) {
        data->hooked = true;
        if (SquirrelMain(v)) return 0;
    }

    const SQChar *src;
    SQInteger event_type, line;
    sq_getinteger(v, 2, &event_type);
    sq_getstring(v, 3, &src);
    sq_getinteger(v, 4, &line);

    if (iNativeLevel >= 1) {
        strcpy(data->src, src);
        data->line = line;
    }

    if (iLevel >= 1) {
        Trace(v);
    }

    FixLoop(v, event_type, src, line);

    v->_debughook = debughook;
    return 0;
}

mutex mainThreadFinishedMutex;
condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;
DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    DetectGame();

    ConfigHook();
    BorderlessPatch();

    SquirrelPatch();
    SquirrelHook();
    ScanFunctions();

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

DWORD32 memsetReturnJMP;
void __declspec(naked) memset_CC()
{
    __asm
    {
        call memsetWait
        mov ecx, [esp + 12]
        movzx eax, byte ptr[esp + 8]
        jmp[memsetReturnJMP]
    }
}

void memsetHook()
{
    uint8_t* memsetResult = Memory::PatternScan(baseModule, "8B 4C 24 0C 0F B6 44 24 08 8B D7 8B 7C 24 04 85");
    if (memsetResult)
    {
        DWORD32 memsetAddress = (uintptr_t)memsetResult;
        int memsetHookLength = Memory::GetHookLength((char*)memsetAddress, 4);
        memsetReturnJMP = memsetAddress + memsetHookLength;
        Memory::DetourFunction32((void*)memsetAddress, memset_CC, memsetHookLength);
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
        memsetHook();
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
