#pragma once

#include "m2machine.h"
#include "m2/psx.h"
#include "m2hook.h"
#include "stdafx.h"

using PSX_ModuleTables = std::map<const char *, const std::vector<std::pair<unsigned int, PSXFUNCTION>> *, std::function<bool(const char *x, const char *y)>>;

class PSX : public M2Machine
{
public:
    PSX() {}

    static auto & GetInstance()
    {
        static PSX instance;
        return instance;
    }

    virtual void Load() override;
    virtual void BindModules() override;
    virtual void UpdateScreenGeometry(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int) override;

    static void main(struct M2_EmuR3000 *cpu);

private:
    static M2_EmuPSX_Module *LoadSystemModule(M2_EmuPSX_Module *mod);
    static M2_EmuPSX_Module *LoadKernelModule(M2_EmuPSX_Module *mod);
    static M2_EmuPSX_Module *LoadUserModule(M2_EmuPSX_Module *mod);

    static void * __fastcall LoadModule(M2_EmuPSX_Module *mod, void *list);

    static int Kernel_Function(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int Kernel_Vector(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int Kernel_Call(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int Kernel_Event(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);

    static int Event_VBlank(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);

    static void BindKernelModules(std::vector<std::pair<unsigned int, PSXFUNCTION>> &table = ModuleTable_Kernel);
    static void BindUserModules(PSX_ModuleTables & tables, const char *basename);

    static void __cdecl CommandR3000(struct M2_EmuR3000 *cpu, int cmd, unsigned int **args);
    static void __cdecl CommandPSX(struct M2_EmuPSX *psx, int cmd, unsigned int **args);
    static void __cdecl CommandR3000101(struct M2_EmuR3000 *cpu, int cmd, unsigned int **args);
    static void __cdecl CommandPSX101(struct M2_EmuPSX *psx, int cmd, unsigned int **args);
    static void __cdecl LoadImage(void *image, unsigned int size);

    static int R3000_Execute(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int R3000_Step(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);

    static void GTE_RotTransPersSX2(safetyhook::Context & ctx);
#ifdef _WIN64
    static void GTE_RotTransPersSX2P(safetyhook::Context & ctx);
    static void GPU_SetResolution(safetyhook::Context & ctx);
#endif

public:
    static inline std::map<unsigned, PSXFUNCTION> ModuleHandlers = {};

    static inline unsigned int VideoMode = 0;
    static inline M2_EmuPSX *Emulator = nullptr;

private:

    static std::map<unsigned, PSXFUNCTION> KernelHandlers;
    static std::map<unsigned, PSXFUNCTION> EventHandlers;
    static std::map<unsigned, const char *> Libraries;

    static std::vector<std::pair<unsigned int, PSXFUNCTION>> ModuleTable_Kernel;

    static inline std::map<M2_EmuPSX_Module *, M2_EmuPSX_Module *> ModuleMap = {};

    static inline unsigned int ScreenWidth  = 0;
    static inline unsigned int ScreenHeight = 0;
    static inline unsigned int ScreenScaleX = 0;
    static inline unsigned int ScreenScaleY = 0;
    static inline unsigned int ScreenMode   = 0;

    typedef struct {
        unsigned int spec;
        int index;
        std::variant<PSXFUNCTION, PSXFUNCTION *> table;
    } R3000_InstructionRecord;

    typedef struct
    {
        std::string table;
        int index;
        PSXFUNCTION function;
    } R3000_InstructionHook;

    static std::map<std::string, R3000_InstructionRecord> R3000_InstructionRecords;
    static std::map<PSXFUNCTION, R3000_InstructionHook> R3000_HookTable;
    static inline std::function<PSXFUNCTION(uint32_t)> R3000_Decode = {};
    static inline int (*R3000_ExecuteFunc)(struct M2_EmuR3000 *cpu, int cycle, unsigned int address) = nullptr;
};
