#include "m2fix.h"
#include "sqhook.h"

#include "sqbinary.h"
#include "sqemutask.h"
#include "sqglobals.h"
#include "sqhelper.h"
#include "sqsystemprof.h"
#include "sqtitleprof.h"
#include "sqsystemdata.h"

#include "m2/epi.h"
#include "ketchup.h"

template <Squirk Q>
unsigned int SQHook<Q>::InitializeFinishCount = 0;
template <Squirk Q>
std::vector<std::string> SQHook<Q>::FileFilter = {};
template <Squirk Q>
std::vector<std::vector<unsigned char>> SQHook<Q>::DataFilter = {};
template <Squirk Q>
bool SQHook<Q>::CdRomShellOpen = false;
template <Squirk Q>
unsigned int SQHook<Q>::ScreenWidth = 0;
template <Squirk Q>
unsigned int SQHook<Q>::ScreenHeight = 0;
template <Squirk Q>
unsigned int SQHook<Q>::ScreenScaleX = 0;
template <Squirk Q>
unsigned int SQHook<Q>::ScreenScaleY = 0;
template <Squirk Q>
unsigned int SQHook<Q>::ScreenMode = 0;
template <Squirk Q>
bool SQHook<Q>::LaunchIntent = true;
template <Squirk Q>
SQInteger SQHook<Q>::StartPadId = 4;

template <Squirk Q>
std::vector<std::pair<std::string, SQFUNCTION<Q>>> SQHook<Q>::ReturnTable = {
    {"init_system_1st",                         SQReturn_init_system_1st},
    {"init_system_last",                        SQReturn_init_system_last},
    {"_update_gadgets",                         _SQReturn_update_gadgets},
    {"_set_disk_patch",                         _SQReturn_set_disk_patch},
    {"_get_disk_path",                          _SQReturn_get_disk_path},
    {"set_current_title_dev_id",                SQReturn_set_current_title_dev_id},
    {"set_game_regionTag",                      SQReturn_set_game_regionTag},
    {"onInitializeFinish",                      SQReturn_onInitializeFinish},
    {"setSmoothing",                            SQReturn_setSmoothing},
    {"_setting_screen_set_parameter_auto_size", _SQReturn_setting_screen_set_parameter_auto_size},
    {"util_get_multimonitor_screen_bounds",     SQReturn_util_get_multimonitor_screen_bounds},
    {"util_get_memory_define_table",            SQReturn_util_get_memory_define_table},
};

template <Squirk Q>
std::vector<std::pair<const SQChar *, SQFUNCTION<Q>>> SQHook<Q>::NativeTable = {
    {"setRamValue",       SQNative_setRamValue},
    {"entryCdRomPatch",   SQNative_entryCdRomPatch},
    {"setCdRomShellOpen", SQNative_setCdRomShellOpen},
    {"setupCdRom",        SQNative_setupCdRom},
    {"setLogFilename",    SQNative_setLogFilename},
    {"setDotmatrix",      SQNative_setDotmatrix},
};

template <Squirk Q>
HSQREMOTEDBG<Q> SQHook<Q>::DBG;

template <Squirk Q>
void SQHook<Q>::SetHook(HSQUIRRELVM<Q> v)
{
    if (M2Config::bDebuggerEnabled && !DBG) {
        DBG = sq_rdbg_init(v, M2Config::iDebuggerPort, M2Config::bDebuggerAutoUpdate, M2Config::bDebuggerExclusive);
        sq_rdbg_waitforconnections(DBG);
    }

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

template <Squirk Q>
#ifdef _WIN64
HSQUIRRELVM<Q> SQHook<Q>::Create(HSQUIRRELVM<Q> v, SQSharedState<Q> *ss)
#else
HSQUIRRELVM<Q> __fastcall SQHook<Q>::Create(HSQUIRRELVM<Q> v, uintptr_t _EDX, SQSharedState<Q> *ss)
#endif
{
#ifdef _WIN64
    v = M2Hook::GetInstance().Invoke<decltype(v)>(Create, v, ss);
#else
    v = M2Hook::GetInstance().Invoke<decltype(v)>(Create, v, _EDX, ss);
#endif

    SQInteger scratchpadsize = _ss(v)->_scratchpadsize;
    SQChar *scratchpad = _ss(v)->_scratchpad;

    Sqrat::DefaultVM<Q>::Set(v);
    spdlog::info("[SQ] SQVM is {}, SQSharedState is {} & scratchpad is {} ({} bytes).",
        fmt::ptr(v), fmt::ptr(_ss(v)), fmt::ptr(scratchpad), scratchpadsize);
    _ss(v)->_debuginfo = true;

    return v;
}

template <Squirk Q>
#ifdef _WIN64
bool SQHook<Q>::CallNative(HSQUIRRELVM<Q> v, SQNativeClosure<Q> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &retval, bool &suspend)
#else
bool __fastcall SQHook<Q>::CallNative(HSQUIRRELVM<Q> v, uintptr_t _EDX, SQNativeClosure<Q> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &retval, bool &suspend)
#endif
{
    M2FixData<Q> *data = EnsureFixData(v);
    SQFUNCTION<Q> function = nclosure->_function;

    data->native = function;
    nclosure->_function = HookNative;

    bool ret = v->CallNative(nclosure, nargs, stackbase, retval, suspend);
    nclosure->_function = function;

    return ret;
}

template <Squirk Q>
#ifdef _WIN64
void SQHook<Q>::BindFunc(Sqrat::Table<Q> *ctx, const SQChar *name, void *method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar)
#else
void __fastcall SQHook<Q>::BindFunc(Sqrat::Table<Q> *ctx, uintptr_t _EDX, const SQChar *name, void *method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar)
#endif
{
#ifdef _WIN64
    M2Hook::GetInstance().Invoke<void>(BindFunc, ctx, name, method, methodSize, func, staticVar);
#else
    M2Hook::GetInstance().Invoke<void>(BindFunc, ctx, _EDX, name, method, methodSize, func, staticVar);
#endif

    auto vm = ctx->GetVM();
    auto obj = ctx->GetFunction(name).GetFunc();
    if (!sq_isnull(obj)) {
        sq_pushobject(vm, obj);
        sq_setnativeclosurename(vm, -1, name);
        sq_pop(vm, 1);
    }

    if (M2Config::iNativeLevel >= 1) {
        spdlog::info("Sqrat: BindFunc(0x{:x}, 0x{:x}, \"{}\").", fmt::underlying(obj_type(obj)), _rawval(obj), name);
    }
}

template <Squirk Q>
void SQHook<Q>::HookFunction(HSQUIRRELVM<Q> v, const SQChar *func, SQFUNCTION<Q> hook, HSQOBJECT<Q> *obj)
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

template <Squirk Q>
void SQHook<Q>::HookMethod(HSQUIRRELVM<Q> v, const SQChar *name, const SQChar *func, SQFUNCTION<Q> hook, HSQOBJECT<Q> *obj)
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

template <Squirk Q>
void SQHook<Q>::SetReturnHook(const char *name, SQFUNCTION<Q> func)
{
    ReturnTable.push_back({ name, func });
}

template <Squirk Q>
void SQHook<Q>::SetNativeCallHook(const char *name, SQFUNCTION<Q> func)
{
    NativeTable.push_back({ name, func });
}

template <Squirk Q>
void SQHook<Q>::SetPatchFileFilter(std::string file)
{
    FileFilter.push_back(file);
}

template <Squirk Q>
void SQHook<Q>::SetPatchDataFilter(std::vector<unsigned char> data)
{
    DataFilter.push_back(data);
}

template <Squirk Q>
SQInteger SQHook<Q>::SQ_util_is_notice_skipable(HSQUIRRELVM<Q> v)
{
    sq_pushbool(v, true);
    return 1;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQ_util_get_launch_intent_id(HSQUIRRELVM<Q> v)
{
    if (LaunchIntent)
        sq_pushstring(v, _SC("MAIN_STORY"), -1);
    else
        sq_pushstring(v, _SC(""), -1);

    return 1;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQ_util_clear_launch_intent_id(HSQUIRRELVM<Q> v)
{
    LaunchIntent = false;
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQ_SystemEtc_setStartPadId(HSQUIRRELVM<Q> v)
{
    sq_getinteger(v, 1, &StartPadId);
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQ_SystemEtc_getStartPadId(HSQUIRRELVM<Q> v)
{
    sq_pushinteger(v, StartPadId);
    return 1;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_util_get_memory_define_table(HSQUIRRELVM<Q> v)
{
    M2Fix::GameInstance().SQOnMemoryDefine();
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_init_system_1st(HSQUIRRELVM<Q> v)
{
    if (M2Config::bLauncherSkipNotice) {
        HookFunction(v, _SC("util_is_notice_skipable"), SQ_util_is_notice_skipable);
    }

    if (M2Config::bLauncherStartGame) {
        HookFunction(v, _SC("util_get_launch_intent_id"), SQ_util_get_launch_intent_id);
        HookFunction(v, _SC("util_clear_launch_intent_id"), SQ_util_clear_launch_intent_id);
    }

    HookMethod(v, _SC("SystemEtc"), _SC("setStartPadId"), SQ_SystemEtc_setStartPadId);
    HookMethod(v, _SC("SystemEtc"), _SC("getStartPadId"), SQ_SystemEtc_getStartPadId);

    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::_SQ_init_emulator_get_arch_sub_info(HSQUIRRELVM<Q> v)
{
    sq_pushinteger(v, USE_ANALOG);
    return 1;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_init_system_last(HSQUIRRELVM<Q> v)
{
    HookFunction(v, _SC("_init_emulator_get_arch_sub_info"), _SQ_init_emulator_get_arch_sub_info);
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::_SQReturn_update_gadgets(HSQUIRRELVM<Q> v)
{
    M2Fix::GameInstance().SQOnUpdateGadgets();
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQNative_setDotmatrix(HSQUIRRELVM<Q> v)
{
    if (M2Config::bDotMatrix) {
        // Do this here as the native call is surprisingly expensive (?!)
        SQObjectPtr<Q>* obj = &v->_stack._vals[v->_stackbase + 1];
        _integer(*obj) = M2Config::bDotMatrix.value();
    }
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQNative_setRamValue(HSQUIRRELVM<Q> v)
{
    //&v->_stack._vals[v->_stackbase + 1];
    unsigned width = SQHelper<Q>::GetObject(2).Cast<unsigned>();
    //&v->_stack._vals[v->_stackbase + 2];
    unsigned offset = SQHelper<Q>::GetObject(3).Cast<unsigned>();
    //&v->_stack._vals[v->_stackbase + 3];
    unsigned value = SQHelper<Q>::GetObject(4).Cast<unsigned>();

    //&v->_stack._vals[v->_stackbase - 5];
    auto patch = SQHelper<Q>::GetObject(-10);
    if (patch.GetType() != OT_TABLE) return 0;

    unsigned address = patch["offset"].Cast<unsigned>();

    if (M2Config::bPatchesDisableRAM && address != 0x200000) {
        if (offset != address) return 1;
        spdlog::info("[SQ] [Patch] filtering RAM patch offset 0x{:x}.", address);
        return 1;
    }

    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQNative_entryCdRomPatch(HSQUIRRELVM<Q> v)
{
    //&v->_stack._vals[v->_stackbase + 1];
    unsigned offset = SQHelper<Q>::GetObject(2).Cast<unsigned>();

    //&v->_stack._vals[v->_stackbase + 2];
    auto data = SQHelper<Q>::GetObject(3);

    //&v->_stack._vals[v->_stackbase - 3];
    //&v->_stack._vals[v->_stackbase - 4];
    auto patch = SQHelper<Q>::GetObject(data.GetType() != OT_INSTANCE ? -7 : -8);

    auto file = patch["file"].Cast<std::string>();
    auto buffer = SQHelper<Q>::template MakeVector<unsigned char>(data);

    if (M2Config::bPatchesDisableCDROM && !file.empty()) {
        spdlog::info("[SQ] [Patch] filtering CD-ROM patch file {}.", file);
        return 1;
    }
    if (M2Config::bPatchesDisableCDROM && !buffer.empty()) {
        spdlog::info("[SQ] [Patch] filtering CD-ROM patch offset 0x{:x}.", offset);
        return 1;
    }

    for (auto &filter : FileFilter)
    {
        if (file.empty()) break;
        if (file.starts_with(filter)) {
            spdlog::info("[SQ] [Patch] filtering CD-ROM patch file {}.", file);
            return 1;
        }
    }
    for (auto &filter : DataFilter)
    {
        if (buffer.empty()) break;
        if (filter == buffer) {
            spdlog::info("[SQ] [Patch] filtering CD-ROM patch offset 0x{:x}.", offset);
            return 1;
        }
    }

    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQNative_setupCdRom(HSQUIRRELVM<Q> v)
{
    spdlog::info("[SQ] CD-ROM image is {}.", SQTitleProf<Q>::GetDisk());
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQNative_setCdRomShellOpen(HSQUIRRELVM<Q> v)
{
    CdRomShellOpen = SQHelper<Q>::GetObject(2).Cast<bool>();
    spdlog::info("[SQ] CD-ROM tray is {}.", CdRomShellOpen ? "open" : "closed");
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQNative_setLogFilename(HSQUIRRELVM<Q> v)
{
    spdlog::info("[SQ] ROM image is {}.", SQTitleProf<Q>::GetExecutable());
    return 0;
}

template <Squirk Q>
bool SQHook<Q>::FixNative(HSQUIRRELVM<Q> v, SQFUNCTION<Q> func, SQNativeClosure<Q> *closure, const SQChar *name)
{
    if (!name) return true;

    for (auto &func : NativeTable) {
        if (strcmp(name, func.first) == 0) {
            if (func.second(v)) return false;
            break;
        }
    }

    return true;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_setSmoothing(HSQUIRRELVM<Q> v)
{
    if (M2Config::bSmoothing) SQEmuTask<Q>::SetSmoothing(M2Config::bSmoothing.value());
    if (M2Config::bScanline)  SQEmuTask<Q>::SetScanline(M2Config::bScanline.value());
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::_SQReturn_set_disk_patch(HSQUIRRELVM<Q> v)
{
    Ketchup<Q>::Process(v);
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_set_current_title_dev_id(HSQUIRRELVM<Q> v)
{
    spdlog::info("[SQ] Title ID is {}.", SQGlobals<Q>::GetTitle());
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_set_game_regionTag(HSQUIRRELVM<Q> v)
{
    spdlog::info("[SQ] Title version is {}.", SQSystemData<Q>::SettingETC::GetVersion());
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::_SQReturn_get_disk_path(HSQUIRRELVM<Q> v)
{
    spdlog::info("[SQ] Disk ID is {}.", SQGlobals<Q>::GetDisk());
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_onInitializeFinish(HSQUIRRELVM<Q> v)
{
    if (InitializeFinishCount == 0) {
        for (auto & Machine : M2Fix::GameInstance().MachineInstances()) {
            Machine.get().BindModules();
        }
    }

    InitializeFinishCount++;
    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::SQReturn_util_get_multimonitor_screen_bounds(HSQUIRRELVM<Q> v)
{
    auto bounds = SQHelper<Q>::GetObject(-10);

    if (bounds["width"].Cast<unsigned>()  != ScreenWidth ||
        bounds["height"].Cast<unsigned>() != ScreenHeight) {
        ScreenWidth = bounds["width"].Cast<unsigned>();
        ScreenHeight = bounds["height"].Cast<unsigned>();

        spdlog::info("[SQ] Screen bounds are {}x{}.", ScreenWidth, ScreenHeight);
    }

    unsigned x = 4 * ScreenHeight;
    unsigned y = 3 * ScreenWidth;
    unsigned gcd = std::gcd(x, y);
    ScreenScaleX = y / gcd;
    ScreenScaleY = x / gcd;

    for (auto & Machine : M2Fix::GameInstance().MachineInstances()) {
        Machine.get().UpdateScreenGeometry(
            ScreenWidth,
            ScreenHeight,
            ScreenScaleX,
            ScreenScaleY,
            ScreenMode
        );
    }

    return 0;
}

template <Squirk Q>
SQInteger SQHook<Q>::_SQReturn_setting_screen_set_parameter_auto_size(HSQUIRRELVM<Q> v)
{
    if (SQHelper<Q>::GetObject(-24).Cast<unsigned>() != ScreenMode) {
        ScreenMode = SQHelper<Q>::GetObject(-24).Cast<unsigned>();

        spdlog::info("[SQ] Screen mode is {}.", ScreenMode);
    }

    for (auto & Machine : M2Fix::GameInstance().MachineInstances()) {
        Machine.get().UpdateScreenGeometry(
            ScreenWidth,
            ScreenHeight,
            ScreenScaleX,
            ScreenScaleY,
            ScreenMode
        );
    }

    return 0;
}

template <Squirk Q>
void SQHook<Q>::FixScript(HSQUIRRELVM<Q> v)
{
    M2FixData<Q> *data = EnsureFixData(v);
    if (data->func.empty()) return;
    
    switch (data->event_type) {
        case 'r':
            for (auto &func : ReturnTable) {
                if (func.first != data->func) continue;
                func.second(v);
                break;
            }
            break;
        default: break;
    }
}

template <Squirk Q>
SQInteger SQHook<Q>::HookNative(HSQUIRRELVM<Q> v)
{
    M2FixData<Q> *data = EnsureFixData(v);
    SQFUNCTION<Q> func = data->native;

    Sqrat::DefaultVM<Q>::Set(v);
    SetHook(v);

    if (M2Config::bError && sq_isstring(v->_lasterror)) {
        spdlog::error("[SQ] [Error] {}", _stringval(v->_lasterror));
        sq_reseterror(v);
    }

    const SQChar *name = nullptr;
    SQNativeClosure<Q> *closure = nullptr;
    if (v && v->ci && sq_isnativeclosure(v->ci->_closure)) {
        closure = _nativeclosure(v->ci->_closure);
        if (sq_isstring(closure->_name)) {
            name = _stringval(closure->_name);
        }
        closure->_function = func;
    }

    // Ignore the calls to the debug hook
    if (func == Hook) return func(v);

    if (M2Config::iNativeLevel >= 1) {
        TraceNative(v, func, closure, name);
    }

    if (name && (strcmp(name, "printf") == 0 || strcmp(name, "print") == 0)) {
        SQChar *str = nullptr;
        SQInteger length = 0;

        const SQChar *format = SQHelper<Q>::GetObject(2).Cast<const SQChar *>();
        if (format && *format != 0 && SQ_SUCCEEDED(sqstd_format(v, 2, &length, &str))) {
            str[scstrcspn(str, "\r\n")] = 0;
            if (*str != 0) {
                spdlog::info("[SQ] [printf] {}", str);
            }
        }
    }

    if (FixNative(v, func, closure, name)) {
        return func(v);
    }

    return 0;
}

template <Squirk Q>
M2FixData<Q> *SQHook<Q>::EnsureFixData(HSQUIRRELVM<Q> v)
{
    // Use Squirrel foreignptr to attach our own data to each SQVM instance.
    return SQHelper<Q>::template AcquireForeignObject<M2FixData<Q>>(v);
}

template <Squirk Q>
bool SQHook<Q>::Main(HSQUIRRELVM<Q> v)
{
    spdlog::info("[SQ] SQVM {} hooked: debug info is {}, exceptions are {}.", fmt::ptr(v),
        (_ss(v)->_debuginfo ? "enabled" : "disabled"),
        (_ss(v)->_notifyallexceptions ? "enabled" : "disabled")
    );

    if (M2Config::bDebuggerEnabled && M2Config::bDebuggerExclusive && DBG) {
        DBG->Init(v);
        return true;
    }

    return false;
}

template <Squirk Q> SQInteger debug_hook(HSQUIRRELVM<Q> v, HSQUIRRELVM<Q> _v, HSQREMOTEDBG<Q> rdbg);
template <Squirk Q>
SQInteger SQHook<Q>::Hook(HSQUIRRELVM<Q> v)
{
    Sqrat::DefaultVM<Q>::Set(v);
    SQObjectPtr debughook = v->_debughook;
    v->_debughook = _null_<Q>;

    Sqrat::RootTable<Q> root = Sqrat::RootTable<Q>();

    if (M2Config::bError && sq_isstring(v->_lasterror)) {
        spdlog::error("[SQ] [Error] {}", _stringval(v->_lasterror));
        sq_reseterror(v);
    }

    if (M2Config::bDebuggerEnabled && !M2Config::bDebuggerExclusive && DBG) {
        debug_hook(static_cast<HSQUIRRELVM<Q>>(nullptr), v, DBG);
    }

    M2FixData<Q> *data = SQHook<Q>::EnsureFixData(v);

    if (!data->hooked) {
        data->hooked = true;
        if (Main(v)) return 0;
    }

    data->event_type = SQHelper<Q>::GetObject(2).Cast<int>();
    data->src        = SQHelper<Q>::GetObject(3).Cast<std::string>();
    data->line       = SQHelper<Q>::GetObject(4).Cast<unsigned>();
    data->func       = SQHelper<Q>::GetObject(5).Cast<std::string>();

    if (M2Config::iLevel >= 1) {
        Trace(v);
    }

    FixScript(v);

    v->_debughook = debughook;
    return 0;
}

void SQUtils::Print(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    std::vector<char> buf(_vscprintf(fmt, va) + 1, 0);
    char *data = buf.data();
    vsprintf(data, fmt, va);
    va_end(va);

    data[strcspn(data, "\r\n")] = 0;
    if (strlen(data) == 0) return;

    std::string str(data);
    const std::set<char> chars{ ' ', '\t', '\r', '\n' };
    int count = std::ranges::count_if(str, [chars](char c) {
        return chars.contains(c);
    });
    if (count == str.length()) return;

    spdlog::info("[SQ] [scprintf] {}", data);
}

template <Squirk Q>
void SQHook<Q>::Load()
{
    extern void * (__fastcall * _sq_vm_realloc)(void *, SQUnsignedInteger, SQUnsignedInteger);

    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        case M2FixGame::Contra:
        case M2FixGame::Dracula:
        case M2FixGame::DraculaAdvance:
        case M2FixGame::Darius:
        case M2FixGame::Darius101:
        {
            _sq_vm_realloc = reinterpret_cast<decltype(_sq_vm_realloc)>(
                M2Hook::GetInstance().Scan(
                    "53 55 56 8B F1 8B EA 57 85 F6 75 25 8B 74 24 14",
                    0, "[SQ-32] sq_vm_realloc"
                ));

            break;
        }

        case M2FixGame::MGSR:
        case M2FixGame::DraculaDominus:
        case M2FixGame::Ray:
        case M2FixGame::Gradius:
        {
            _sq_vm_realloc = reinterpret_cast<decltype(_sq_vm_realloc)>(
                M2Hook::GetInstance().Scan(
                    "48 89 5C 24 18 48 89 7C 24 20 41 56 48 83 EC 20 "
                    "41 8B F8 44 8B F2 48 8B D9 48 85 C9 75 ?? 39 0D",
                    0, "[SQ-64] sq_vm_realloc"
                ));

            break;
        }

        default: break;
    }

    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        {
            M2Hook::GetInstance().Hook(
                "C7 86 A4 00 00 00 FF FF FF FF 89 86 A0 00 00 00",
                -0x111, SQHook<Squirk::Standard>::Create, "[SQ-32<Standard>] SQVM::SQVM"
            );

            M2Hook::GetInstance().Hook(
                "FF D0 8B 4D 18 83 C4 04 FF 8E 98 00 00 00 C6 01",
                -0x286, SQHook<Squirk::Standard>::CallNative, "[SQ-32<Standard>] SQVM::CallNative"
            );

            M2Hook::GetInstance().Hook(
                "0F B6 44 24 20 83 C4 04 8B 4F 04 BA FD FF FF FF",
                -0x52, SQHook<Squirk::Standard>::BindFunc, "[SQ-32<Standard>] Sqrat::BindFunc"
            );

            break;
        }

        case M2FixGame::Contra:
        case M2FixGame::Dracula:
        case M2FixGame::DraculaAdvance:
        case M2FixGame::Darius:
        case M2FixGame::Darius101:
        {
            bool ret = false;

            ret = M2Hook::GetInstance().Hook(
                "C7 86 D4 00 00 00 FF FF FF FF 89 86 D0 00 00 00",
                -0x185, SQHook<Squirk::AlignObject>::Create, "[SQ-32<AlignObject>] SQVM::SQVM"
            );
            if (!ret) {
                M2Hook::GetInstance().Hook(
                    "C7 86 C4 00 00 00 FF FF FF FF 89 86 C0 00 00 00",
                    -0x138, SQHook<Squirk::AlignObjectShared>::Create, "[SQ-32<AlignObjectShared>] SQVM::SQVM"
                );
            }

            ret = M2Hook::GetInstance().Hook(
                "FF D0 8B 4D 18 83 C4 04 FF 8E C8 00 00 00 C6 01",
                -0x2C3, SQHook<Squirk::AlignObject>::CallNative, "[SQ-32<AlignObject>] SQVM::CallNative"
            );
            if (!ret) {
                M2Hook::GetInstance().Hook(
                    "FF D0 8B 4D 18 83 C4 04 FF 8E B8 00 00 00 C6 01",
                    -0x2DB, SQHook<Squirk::AlignObjectShared>::CallNative, "[SQ-32<AlignObjectShared>] SQVM::CallNative"
                );
            }

            ret = M2Hook::GetInstance().Hook(
                "0F B6 44 24 20 83 C4 04 8B 4F 08 BA FD FF FF FF",
                -0x58, SQHook<Squirk::AlignObject>::BindFunc, "[SQ-32<AlignObject>] Sqrat::BindFunc"
            );
            if (!ret) {
                M2Hook::GetInstance().Hook(
                    "0F B6 43 18 83 C4 04 8B 4E 08 BA FD FF FF FF 50",
                    -0x17E, SQHook<Squirk::AlignObjectShared>::BindFunc, "[SQ-32<AlignObjectShared>] Sqrat::BindFunc"
                );
            }

            break;
        }

        case M2FixGame::MGSR:
        case M2FixGame::DraculaDominus:
        case M2FixGame::Ray:
        case M2FixGame::Gradius:
        {
            M2Hook::GetInstance().Hook(
                "48 C7 81 FC 00 00 00 FF FF FF FF 48 89 B1 E0 00",
                -0xB3, SQHook<Squirk::StandardShared>::Create, "[SQ-64<StandardShared>] SQVM::SQVM"
            );

            M2Hook::GetInstance().Hook(
                "FF 53 68 FF 8F F0 00 00 00 48 8B 8C 24 F8 00 00",
                -0x2D0, SQHook<Squirk::StandardShared>::CallNative, "[SQ-64<StandardShared>] SQVM::CallNative"
            );

            M2Hook::GetInstance().Hook(
                "44 0F B6 44 24 78 BA FD FF FF FF 49 8B 4E 08 E8",
                -0x25A, SQHook<Squirk::StandardShared>::BindFunc, "[SQ-64<StandardShared>] Sqrat::BindFunc"
            );

            break;
        }

        default: break;
    }
}

template void SQHook<Squirk::Standard>::Load();

#ifndef _WIN64
static_assert(sizeof(SQVM<Squirk::Standard>) == 0xB0);                      // MGS 1
static_assert(sizeof(SQSharedState<Squirk::Standard>) == 0xB8);             // MGS 1
static_assert(sizeof(SQVM<Squirk::AlignObject>) == 0xE0);                   // CO | CA
static_assert(sizeof(SQSharedState<Squirk::AlignObject>) == 0x138);         // CO | CA
static_assert(sizeof(SQVM<Squirk::AlignObjectShared>) == 0xD0);             // CA (EPIC)
static_assert(sizeof(SQSharedState<Squirk::AlignObjectShared>) == 0x148);   // CA (EPIC)
static_assert(sizeof(M2VMExt<Squirk::Standard>) == 0x0C);                   // MGS 1
static_assert(sizeof(M2VMExt<Squirk::AlignObject>) == 0x14);                // CO | CA
static_assert(sizeof(M2SharedStateExt<Squirk::AlignObjectShared>) == 0x14); // CA (EPIC)
#else
static_assert(sizeof(SQVM<Squirk::StandardShared>) == 0x108);               // MG | SR
static_assert(sizeof(SQSharedState<Squirk::StandardShared>) == 0x178);      // MG | SR
static_assert(sizeof(M2SharedStateExt<Squirk::StandardShared>) == 0x18);    // MG | SR
#endif
