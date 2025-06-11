#pragma once

#include "m2fixbase.h"

#include "sqhelper.h"

#include "sqrdbg.h"
#include "sqdbgserver.h"

template <Squirk Q>
class M2FixData
{
public:
    // Set once we have discovered this SQVM instance.
    bool hooked;
    // The actual native closure function pointer.
    // Temporarily replaced by the native call hook.
    // Restored and called by the hook.
    SQFUNCTION<Q> native;
    // Last-seen source-level debug information from line evaluation.
    // Deferred for native call logging where this is otherwise lost.
    // This can only be populated as a consequence of enabling debuginfo.
    char event_type;
    std::string src;
    unsigned line;
    std::string func;
};

template <Squirk Q = Squirk::Standard>
class SQHook: public M2FixBase
{
public:
    SQHook() {};

    static auto & GetInstance()
    {
        static SQHook instance;
        return instance;
    }

    static void LoadInstance() {
        GetInstance().Load();
    }

    virtual void Load() override;

    static void HookFunction(HSQUIRRELVM<Q> v, const SQChar *func, SQFUNCTION<Q> hook, HSQOBJECT<Q> *obj = nullptr);
    static void HookMethod(HSQUIRRELVM<Q> v, const SQChar *name, const SQChar *func, SQFUNCTION<Q> hook, HSQOBJECT<Q> *obj = nullptr);

    static void SetReturnHook(const char *name, SQFUNCTION<Q> func);
    static void SetNativeCallHook(const char *name, SQFUNCTION<Q> func);
    static void SetPatchFileFilter(std::string file);
    static void SetPatchDataFilter(std::vector<unsigned char> data);

    static bool IsCdRomShellOpen()
    {
        return CdRomShellOpen;
    }

#ifdef _WIN64
    static HSQUIRRELVM<Q> Create(HSQUIRRELVM<Q> v, SQSharedState<Q> *ss);
    static bool CallNative(HSQUIRRELVM<Q> v, SQNativeClosure<Q> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &retval, bool &suspend);
    static void BindFunc(Sqrat::Table<Q> *ctx, const SQChar *name, void *method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar);
#else
    static HSQUIRRELVM<Q> __fastcall Create(HSQUIRRELVM<Q> v, uintptr_t _EDX, SQSharedState<Q> *ss);
    static bool __fastcall CallNative(HSQUIRRELVM<Q> v, uintptr_t _EDX, SQNativeClosure<Q> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &retval, bool &suspend);
    static void __fastcall BindFunc(Sqrat::Table<Q> *ctx, uintptr_t _EDX, const SQChar *name, void *method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar);
#endif

private:
    static M2FixData<Q> *EnsureFixData(HSQUIRRELVM<Q> v);

    static bool Main(HSQUIRRELVM<Q> v);

    static SQInteger Hook(HSQUIRRELVM<Q> v);
    static void SetHook(HSQUIRRELVM<Q> v);

    static SQInteger HookNative(HSQUIRRELVM<Q> v);

    static void FixScript(HSQUIRRELVM<Q> v);
    static bool FixNative(HSQUIRRELVM<Q> v, SQFUNCTION<Q> func, SQNativeClosure<Q> *closure, const SQChar *name);

    static void TraceParameter(std::stringstream & trace, SQObjectPtr<Q> obj, int level);
    static void TraceNext(std::stringstream & trace, HSQUIRRELVM<Q> v);
    static void TraceNative(HSQUIRRELVM<Q> v, SQFUNCTION<Q> func, SQNativeClosure<Q> *closure, const SQChar *name);
    static void Trace(HSQUIRRELVM<Q> v);

    static SQInteger SQReturn_init_system_1st(HSQUIRRELVM<Q> v);
    static SQInteger SQReturn_init_system_last(HSQUIRRELVM<Q> v);
    static SQInteger SQReturn_onInitializeFinish(HSQUIRRELVM<Q> v);
    static SQInteger _SQ_init_emulator_get_arch_sub_info(HSQUIRRELVM<Q> v);
    static SQInteger _SQReturn_update_gadgets(HSQUIRRELVM<Q> v);

    static SQInteger SQReturn_setSmoothing(HSQUIRRELVM<Q> v);
    static SQInteger SQReturn_set_current_title_dev_id(HSQUIRRELVM<Q> v);
    static SQInteger SQReturn_set_game_regionTag(HSQUIRRELVM<Q> v);
    static SQInteger SQReturn_util_get_memory_define_table(HSQUIRRELVM<Q> v);
    static SQInteger _SQReturn_get_disk_path(HSQUIRRELVM<Q> v);
    static SQInteger _SQReturn_set_disk_patch(HSQUIRRELVM<Q> v);

    static SQInteger SQReturn_util_get_multimonitor_screen_bounds(HSQUIRRELVM<Q> v);
    static SQInteger _SQReturn_setting_screen_set_parameter_auto_size(HSQUIRRELVM<Q> v);

    static SQInteger SQ_util_is_notice_skipable(HSQUIRRELVM<Q> v);
    static SQInteger SQ_util_get_launch_intent_id(HSQUIRRELVM<Q> v);
    static SQInteger SQ_util_clear_launch_intent_id(HSQUIRRELVM<Q> v);
    static SQInteger SQ_SystemEtc_setStartPadId(HSQUIRRELVM<Q> v);
    static SQInteger SQ_SystemEtc_getStartPadId(HSQUIRRELVM<Q> v);

    static SQInteger SQNative_setRamValue(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setupCdRom(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_entryCdRomPatch(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setCdRomShellOpen(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setLogFilename(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setDotmatrix(HSQUIRRELVM<Q> v);

private:
    static HSQREMOTEDBG<Q> DBG;
    static std::vector<std::string> FileFilter;
    static std::vector<std::vector<unsigned char>> DataFilter;
    static std::vector<std::pair<std::string, SQFUNCTION<Q>>> ReturnTable;
    static std::vector<std::pair<const SQChar *, SQFUNCTION<Q>>> NativeTable;
    static unsigned int ThreadCount;
    static unsigned int InitializeFinishCount;
    static unsigned int ScreenWidth;
    static unsigned int ScreenHeight;
    static unsigned int ScreenScaleX;
    static unsigned int ScreenScaleY;
    static unsigned int ScreenMode;
    static bool LaunchIntent;
    static SQInteger StartPadId;
    static bool CdRomShellOpen;
};
