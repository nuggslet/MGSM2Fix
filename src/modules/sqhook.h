#pragma once

#include "m2fixbase.h"
#include "../m2fix/patch_range.h"

#include "sqhelper.h"

#include "sqrdbg.h"
#include "sqdbgserver.h"

#include "sqsystemprof.h"

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

    static void Command();

    static void HookFunction(HSQUIRRELVM<Q> v, const SQChar *func, SQFUNCTION<Q> hook, HSQOBJECT<Q> *obj = nullptr);
    static void HookMethod(HSQUIRRELVM<Q> v, const SQChar *name, const SQChar *func, SQFUNCTION<Q> hook, HSQOBJECT<Q> *obj = nullptr);
    static void HookMethod(HSQUIRRELVM<Q> v, HSQOBJECT<Q> name, const SQChar *func, SQFUNCTION<Q> hook, HSQOBJECT<Q> *obj = nullptr);

    static void SetCallHook(const char *name, SQFUNCTION<Q> func);
    static void SetReturnHook(const char *name, SQFUNCTION<Q> func);
    static void SetLoadScriptHook(const char *name, SQFUNCTION<Q> func);
    static void SetNativeCallHook(const char *name, SQFUNCTION<Q> func);
    static void SetPatchFileBlacklist(std::string file);
    static void SetPatchRangeBlacklist(unsigned title, unsigned disk, uint64_t start, uint64_t end);
    static void SetPatchWatch(uint64_t start, uint64_t end, std::string label);
    static void SetPatchDataBlacklist(std::vector<unsigned char> data);
    static void SetTextureWhitelist(unsigned int data);

    static bool IsCdRomShellOpen()
    {
        return CdRomShellOpen;
    }

#ifdef _WIN64
    static HSQUIRRELVM<Q> CreateVM(HSQUIRRELVM<Q> v, SQSharedState<Q> *ss);
    static bool CallNative(HSQUIRRELVM<Q> v, SQNativeClosure<Q> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &retval, bool &suspend);
    static void BindFunc(Sqrat::Table<Q> *ctx, const SQChar *name, void *method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar);
    static void NewClosure(HSQUIRRELVM<Q> v, SQFUNCTION<Q> func, SQUnsignedInteger nfreevars);
#else
    static HSQUIRRELVM<Q> __fastcall CreateVM(HSQUIRRELVM<Q> v, uintptr_t _EDX, SQSharedState<Q> *ss);
    static bool __fastcall CallNative(HSQUIRRELVM<Q> v, uintptr_t _EDX, SQNativeClosure<Q> *nclosure, SQInteger nargs, SQInteger stackbase, SQObjectPtr<Q> &retval, bool &suspend);
    static void __fastcall BindFunc(Sqrat::Table<Q> *ctx, uintptr_t _EDX, const SQChar *name, void *method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar);
    static void __fastcall NewClosure(HSQUIRRELVM<Q> v, uintptr_t _EDX, SQFUNCTION<Q> func, SQUnsignedInteger nfreevars);
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
    static SQInteger SQCall_util_load_script(HSQUIRRELVM<Q> v);
    static SQInteger SQReturn_util_load_script(HSQUIRRELVM<Q> v);
    static SQInteger SQReturn_constructor(HSQUIRRELVM<Q> v);
    static SQInteger SQLoadScript_pause_main_mgs(HSQUIRRELVM<Q> v);
    static SQInteger SQLoadScript_mode_title_select_mgs(HSQUIRRELVM<Q> v);
    static SQInteger _SQ_MenuModeTitleMgsOptionScreen_is_highp(HSQUIRRELVM<Q> v);
    static SQInteger _SQ_MenuModePauseMgsOptionScreen_is_highp(HSQUIRRELVM<Q> v);
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
    static SQInteger SQNative_getRamValue(HSQUIRRELVM<Q> v);
    static std::string CallStack(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setupCdRom(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_entryCdRomPatch(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_releaseCdRomPatch(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setCdRomShellOpen(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setLogFilename(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_setDotmatrix(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_entryTexturePatch(HSQUIRRELVM<Q> v);
    static SQInteger SQNative_releaseTexturePatch(HSQUIRRELVM<Q> v);

private:
    static inline HSQREMOTEDBG<Q> DBG = nullptr;
    static inline std::vector<std::string> FileBlacklist = {};
    // Half-open disc-image ranges. Filters by where a patch writes rather than
    // what its file is called, which is what you want for an asset whose patch
    // names differ per disk - the collection names each piece after its own
    // image offset, so one range covers every piece of one archive entry.
    static inline std::vector<PatchRange> RangeBlacklist = {};
    // Ranges to REPORT rather than filter: a patch landing here is logged with
    // its length and leading bytes, then applied as normal. For watching what
    // the collection does to a region you care about - the thing you want when
    // a patch of theirs turns out to matter and you cannot see its contents.
    typedef struct { uint64_t start, end; std::string label; } PatchWatch;
    static inline std::vector<PatchWatch> PatchWatches = {};
    static inline std::vector<std::vector<unsigned char>> DataBlacklist = {};
    static inline std::vector<unsigned int> TextureWhitelist = {};
    static inline unsigned int ThreadCount           = 0;
    static inline unsigned int InitializeFinishCount = 0;
    static inline unsigned int ScreenWidth  = 0;
    static inline unsigned int ScreenHeight = 0;
    static inline unsigned int ScreenScaleX = 0;
    static inline unsigned int ScreenScaleY = 0;
    static inline unsigned int ScreenMode   = 0;
    static inline bool LaunchIntent    = true;
    static inline SQInteger StartPadId = 4;
    static inline bool CdRomShellOpen  = false;
    static inline bool Smoothing       = false;
    static inline std::vector<std::string> LoadScript = {};

    static std::vector<std::string> ClassNames;
    static inline std::map<std::string, HSQOBJECT<Q>> InstanceTable;

private:
    static std::vector<std::pair<std::string, SQFUNCTION<Q>>> CallTable;
    static std::vector<std::pair<std::string, SQFUNCTION<Q>>> ReturnTable;
    static std::vector<std::pair<std::string, SQInteger(*)(HSQUIRRELVM<Q>, HSQOBJECT<Q>)>> ConstructorTable;
    static std::vector<std::pair<std::string, SQFUNCTION<Q>>> LoadScriptTable;
    static std::vector<std::pair<const SQChar *, SQFUNCTION<Q>>> NativeTable;
};
