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
    static void * __fastcall ListInsert(void *list, void *object);

    static int Kernel_Function(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int Kernel_Vector(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int Kernel_Call(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);

    static void BindKernelModules(std::vector<std::pair<unsigned int, PSXFUNCTION>> &table = ModuleTable_Kernel);
    static void BindUserModules(PSX_ModuleTables & tables, const char *basename);

    static void CommandR3000(struct M2_EmuR3000 *cpu, int cmd, unsigned int **args);
    static void CommandPSX(struct M2_EmuPSX *psx, int cmd, unsigned int **args);
    static void CommandR3000101(struct M2_EmuR3000 *cpu, int cmd, unsigned int **args);
    static void CommandPSX101(struct M2_EmuPSX *psx, int cmd, unsigned int **args);
    static void LoadImage(void *image, size_t size);

    static void R3000_Hook(const char *table, int index, PSXFUNCTION func);

#ifndef _WIN64
    static void GTE_RTP(safetyhook::Context & ctx);
#endif

public:
    static std::map<unsigned, PSXFUNCTION> ModuleHandlers;

    static unsigned int VideoMode;
    static M2_EmuPSX *Emulator;

private:

    static std::map<unsigned, PSXFUNCTION> LibraryHandlers;
    static std::map<unsigned, const char *> Libraries;

    static std::vector<std::pair<unsigned int, PSXFUNCTION>> ModuleTable_Kernel;

    static std::map<M2_EmuPSX_Module *, M2_EmuPSX_Module *> ModuleMap;

    static unsigned int ScreenWidth;
    static unsigned int ScreenHeight;
    static unsigned int ScreenScaleX;
    static unsigned int ScreenScaleY;
    static unsigned int ScreenMode;

    typedef struct {
        unsigned int spec;
        int index;
        std::variant<PSXFUNCTION, PSXFUNCTION *> table;
    } R3000_InstructionRecord;

    static std::map<std::string, R3000_InstructionRecord> R3000_InstructionRecords;
    static std::map<PSXFUNCTION, PSXFUNCTION> R3000_HookTable;
    static std::function<PSXFUNCTION(uint32_t)> R3000_Decode;
};
