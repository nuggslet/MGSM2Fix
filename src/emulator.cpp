#include "stdafx.h"

#include "emulator.h"

using namespace std;

extern MgsGame eGameType;
extern int iEmulatorLevel;

typedef struct {
    unsigned int address;
    M2FUNCTION handler;
} M2_TableEntry;

typedef struct {
    const char *name;
    void *create;
} M2_SystemModule;

typedef struct {
    const char *name;
    int id;
    M2_TableEntry *table;
    int count;
} M2_KernelModule;

typedef struct {
    const char *name;
    int id;
    M2_TableEntry *table;
    int count;
    void *deps;
} M2_UserModule;

typedef struct {
    const char *name;
    int type;
    union {
        M2_SystemModule *system;
        M2_KernelModule *kernel;
        M2_UserModule *user;
    };
} M2_Module;

map<M2_Module *, M2_Module *> M2_ModuleMap;

void *M2LoadSystemModule(M2_Module *mod)
{
    M2_SystemModule *system = mod->system;
    LOG_F(INFO, "Emulator: Loading system module %s at 0x%" PRIxPTR ".", mod->name, mod);

    return mod;
}

void *M2LoadKernelModule(M2_Module *mod)
{
    M2_KernelModule *kernel = mod->kernel;
    LOG_F(INFO, "Emulator: Loading kernel module %s at 0x%" PRIxPTR ".", mod->name, mod);

    if (M2_ModuleMap.count(mod) != 0) {
        M2_Module *_mod = M2_ModuleMap[mod];
        LOG_F(INFO, "Emulator: Already hooked kernel module %s at 0x%" PRIxPTR ".", _mod->name, _mod);
        return _mod;
    }

    M2_TableEntry *_table = new M2_TableEntry[kernel->count];
    memcpy(_table, kernel->table, sizeof(M2_TableEntry) * kernel->count);

    M2_KernelModule *_kernel = new M2_KernelModule;
    memcpy(_kernel, kernel, sizeof(M2_KernelModule));
    _kernel->table = _table;

    M2_Module *_mod = new M2_Module;
    memcpy(_mod, mod, sizeof(M2_Module));
    _mod->kernel = _kernel;

    M2_ModuleMap[mod] = _mod;
    LOG_F(INFO, "Emulator: Hooked kernel module %s at 0x%" PRIxPTR ".", _mod->name, _mod);
    return _mod;
}

void *M2LoadUserModule(M2_Module *mod)
{
    M2_UserModule *user = mod->user;
    LOG_F(INFO, "Emulator: Loading user module %s at 0x%" PRIxPTR ".", mod->name, mod);

    if (M2_ModuleMap.count(mod) != 0) {
        M2_Module *_mod = M2_ModuleMap[mod];
        LOG_F(INFO, "Emulator: Already hooked user module %s at 0x%" PRIxPTR ".", _mod->name, _mod);
        return _mod;
    }

    M2_TableEntry *_table = new M2_TableEntry[user->count];
    memcpy(_table, user->table, sizeof(M2_TableEntry) * user->count);

    M2_UserModule *_user = new M2_UserModule;
    memcpy(_user, user, sizeof(M2_UserModule));
    _user->table = _table;

    M2_Module *_mod = new M2_Module;
    memcpy(_mod, mod, sizeof(M2_Module));
    _mod->user = _user;

    M2_ModuleMap[mod] = _mod;
    LOG_F(INFO, "Emulator: Hooked user module %s at 0x%" PRIxPTR ".", _mod->name, _mod);
    return _mod;
}

void *M2LoadModule(void *module, void *)
{
    M2_Module *mod = (M2_Module *)module;

    switch (mod->type) {
    case 1:
        return M2LoadKernelModule(mod);
    case 3:
        return M2LoadSystemModule(mod);
    case 4:
        return M2LoadUserModule(mod);
    default:
        break;
    }

    LOG_F(INFO, "Emulator: Loading module %s with type %d at 0x%" PRIxPTR ".", mod->name, mod->type, mod);
    return mod;
}

void M2ListInsert(void *list, void *object)
{
    // Didn't end up using this but the detour might be useful for something else in future...
    return;
}

map<unsigned int, M2FUNCTION> M2_ModuleHandlers;

int M2Hook_main(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    LOG_F(INFO, "Emulator: Machine at 0x%" PRIxPTR ".", cpu->Bus->Machine);
    LOG_F(INFO, "Emulator: Archive at 0x%" PRIxPTR ".", cpu->Bus->Machine->Archive);
    LOG_F(INFO, "Emulator: BIOS at 0x%" PRIxPTR ".", cpu->Bus->Machine->ImageBIOS);
    LOG_F(INFO, "Emulator: Image at 0x%" PRIxPTR ".", cpu->Bus->Machine->ImageDRAM);
    LOG_F(INFO, "Emulator: DRAM at 0x%" PRIxPTR ".", cpu->Bus->Machine->MemoryDRAM);
    LOG_F(INFO, "Emulator: TCM at 0x%" PRIxPTR ".", cpu->Bus->Machine->MemoryTCM);
    LOG_F(INFO, "Emulator: Bus at 0x%" PRIxPTR ".", cpu->Bus->Machine->Bus);
    LOG_F(INFO, "Emulator: R3000 at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevR3000);
    LOG_F(INFO, "Emulator: GTE at 0x%" PRIxPTR ".", cpu->CoprocGTE);
    LOG_F(INFO, "Emulator: CDROM at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevCDROM);
    LOG_F(INFO, "Emulator: DMAC at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevDMAC);
    LOG_F(INFO, "Emulator: GPU at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevGPU);
    LOG_F(INFO, "Emulator: INTC at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevINTC);
    LOG_F(INFO, "Emulator: MDEC at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevMDEC);
    LOG_F(INFO, "Emulator: RTC at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevRTC);
    LOG_F(INFO, "Emulator: SIO at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevSIO);
    LOG_F(INFO, "Emulator: SPU at 0x%" PRIxPTR ".", cpu->Bus->Machine->DevSPU);

    extern int R3000_CommandGTE(struct M2_EmuCoprocGTE *gte, int op);
    cpu->CoprocGTE->Command = R3000_CommandGTE;

    unsigned int ra = cpu->Reg[31];
    LOG_F(INFO, "MGS 1: __main: 0x%" PRIxPTR " -> 0x%" PRIxPTR ".", address, ra);

    return cpu->Step(cpu, 0, ra);
}

int M2Hook_Kernel_Function(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    M2FUNCTION Kernel_Function = M2_ModuleHandlers[address];

    if (iEmulatorLevel >= 1) {
        char type = (((address & 0xF0) - 0xA0) >> 4) + 'A';
        unsigned int ra = cpu->Reg[31];
        unsigned int r9 = cpu->Reg[9];
        LOG_F(INFO, "PSX: Kernel_Function%c(0x%" PRIxPTR "): 0x%" PRIxPTR ".", type, r9, ra);
    }

    return Kernel_Function(cpu, cycle, address);
}

int M2Hook_Kernel_Vector(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    M2FUNCTION Kernel_Vector = M2_ModuleHandlers[address];

    if (iEmulatorLevel >= 1) {
        unsigned int ra = cpu->Reg[31];
        unsigned int r9 = cpu->Reg[9];
        LOG_F(INFO, "PSX: Kernel_Vector(0x%" PRIxPTR "): 0x%" PRIxPTR ".", r9, ra);
    }

    return Kernel_Vector(cpu, cycle, address);
}

int M2Hook_Kernel_Call(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    M2FUNCTION Kernel_Call = M2_ModuleHandlers[address];

    if (iEmulatorLevel >= 1) {
        unsigned int ra = cpu->Reg[31];
        unsigned int r9 = cpu->Reg[9];
        LOG_F(INFO, "PSX: Kernel_Call(0x%" PRIxPTR "): 0x%" PRIxPTR ".", r9, ra);
    }

    return Kernel_Call(cpu, cycle, address);
}

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_Kernel = {
    make_pair(0xA0, M2Hook_Kernel_Function),
    make_pair(0xB0, M2Hook_Kernel_Function),
    make_pair(0xC0, M2Hook_Kernel_Function),
    make_pair(0xF000, M2Hook_Kernel_Call),
    make_pair(0xFFF0, M2Hook_Kernel_Vector),
};

int M2Hook_s03a_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    extern bool bPatchesEnableMosaic;
    static bool oneshot = false;
    if (!oneshot) {
        LOG_F(INFO, "MGS 1: %s s03a_disable_mosaic().", bPatchesEnableMosaic ? "Blocking" : "Allowing");
        oneshot = true;
    }
    if (!bPatchesEnableMosaic) {
        M2FUNCTION s03a_disable_mosaic = M2_ModuleHandlers[address];
        return s03a_disable_mosaic(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

int M2Hook_s03d_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    extern bool bPatchesEnableMosaic;
    static bool oneshot = false;
    if (!oneshot) {
        LOG_F(INFO, "MGS 1: %s s03d_disable_mosaic().", bPatchesEnableMosaic ? "Blocking" : "Allowing");
        oneshot = true;
    }
    if (!bPatchesEnableMosaic) {
        M2FUNCTION s03d_disable_mosaic = M2_ModuleHandlers[address];
        return s03d_disable_mosaic(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_ES = {
    make_pair(0x800D322C, M2Hook_s03a_disable_mosaic),
    make_pair(0x800D9D80, M2Hook_s03d_disable_mosaic),
};

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_DE = {
    make_pair(0x800D3618, M2Hook_s03a_disable_mosaic),
    make_pair(0x800DA16C, M2Hook_s03d_disable_mosaic),
};

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_IT = {
    make_pair(0x800D31A8, M2Hook_s03a_disable_mosaic),
    make_pair(0x800D9CFC, M2Hook_s03d_disable_mosaic),
};

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_FR = {
    make_pair(0x800D36D0, M2Hook_s03a_disable_mosaic),
    make_pair(0x800DA224, M2Hook_s03d_disable_mosaic),
};

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_UK = {
    make_pair(0x800D3118, M2Hook_s03a_disable_mosaic),
    make_pair(0x800D9C6C, M2Hook_s03d_disable_mosaic),
};

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_US = {
    make_pair(0x800D4AC8, M2Hook_s03a_disable_mosaic),
    make_pair(0x800DB61C, M2Hook_s03d_disable_mosaic),
};

vector<pair<unsigned int, M2FUNCTION>> M2_ModuleTable_Integral = {
    make_pair(0x80098F14, M2Hook_main),
};

map<const char *, vector<pair<unsigned int, M2FUNCTION>> *, function<bool(const char *x, const char *y)>> M2_ModuleTables { {
    {"mgs_r3000_es",  &M2_ModuleTable_ES},
    {"mgs_r3000_de",  &M2_ModuleTable_DE},
    {"mgs_r3000_it",  &M2_ModuleTable_IT},
    {"mgs_r3000_fr",  &M2_ModuleTable_FR},
    {"mgs_r3000_uk",  &M2_ModuleTable_UK},
    {"mgs_r3000_us",  &M2_ModuleTable_US},
    {"mgs_r3000_int", &M2_ModuleTable_Integral},
    }, bind([](const char *x, const char *y) { return strcmp(x, y) < 0; }, placeholders::_1, placeholders::_2)
};

void FixKernelModules(vector<pair<unsigned int, M2FUNCTION>> &table = M2_ModuleTable_Kernel)
{
    for (auto const &i : M2_ModuleMap) {
        M2_Module *module = i.second;
        if (module->type != 1) continue;
        M2_KernelModule *kernel = module->kernel;

        for (auto &func : table) {
            for (int i = 0; i < kernel->count; i++) {
                if (kernel->table[i].address != func.first) continue;
                M2_ModuleHandlers[kernel->table[i].address] = kernel->table[i].handler;
                kernel->table[i].handler = func.second;
                break;
            }
        }
    }

    LOG_F(INFO, "Emulator: Applied hooks to kernel modules.");
}

void FixUserModules(const char *basename = "mgs_r3000_int")
{
    // To avoid repetitively populating the tables for each game version, we can exploit certain hooks being 
    // assigned to the same native function. This means if you're lucky only e.g. the Integral table needs 
    // to be populated for a given hook. Integral has been chosen as it tends to receive the most attention.
    while (basename && M2_ModuleTables.count(basename) != 0) {
        auto basetable = M2_ModuleTables[basename];

        M2_Module *base = NULL;
        for (auto const &i : M2_ModuleMap) {
            M2_Module *module = i.second;
            if (module->type != 4) continue;
            if (strcmp(module->name, basename) == 0) {
                base = module;
                break;
            }
        }

        if (!base) {
            LOG_F(INFO, "Emulator: Couldn't find %s base module.", basename);
            break;
        }

        map<M2FUNCTION, M2FUNCTION> basemap;

        // Now the base module is located, we can create the mapping between handler and hook.
        M2_UserModule *user = base->user;
        for (auto &func : *basetable) {
            for (int i = 0; i < user->count; i++) {
                if (user->table[i].address != func.first) continue;
                basemap[func.second] = user->table[i].handler;
                M2_ModuleHandlers[user->table[i].address] = user->table[i].handler;
                user->table[i].handler = func.second;
                break;
            }
        }

        LOG_F(INFO, "Emulator: Applied hooks to %s.", base->name);

        // With the mapping created, hook each module's handler.
        for (auto const &i : M2_ModuleMap) {
            M2_Module *module = i.second;
            if (module->type != 4) continue;
            M2_UserModule *user = module->user;

            if (strcmp(basename, module->name) == 0)
                continue;

            for (auto &func : basemap) {
                for (int i = 0; i < user->count; i++) {
                    if (user->table[i].handler != func.second) continue;
                    M2_ModuleHandlers[user->table[i].address] = user->table[i].handler;
                    user->table[i].handler = func.first;
                    break;
                }
            }

            LOG_F(INFO, "Emulator: Applied hooks from %s to %s.", base->name, module->name);
        }

        break;
    }

    // Fall-back for cases where the native functions are essentially duplicated with e.g.
    // only changes to ReadN/WriteN/Step parameters but are otherwise conceptually the same.
    // These require populating the table for each game version.
    for (auto const &i : M2_ModuleMap) {
        M2_Module *module = i.second;
        if (module->type != 4) continue;
        M2_UserModule *user = module->user;

        if (basename && strcmp(basename, module->name) == 0)
            continue;
        if (M2_ModuleTables.count(module->name) == 0)
            continue;

        auto table = M2_ModuleTables[module->name];
        for (auto &func : *table) {
            for (int i = 0; i < user->count; i++) {
                if (user->table[i].address != func.first) continue;
                M2_ModuleHandlers[user->table[i].address] = user->table[i].handler;
                user->table[i].handler = func.second;
                break;
            }
        }

        LOG_F(INFO, "Emulator: Applied hooks to %s.", module->name);
    }
}

void FixModules()
{
    switch (eGameType) {
        case MgsGame::MGS1:
            FixUserModules();
            FixKernelModules();
        default: break;
    }
}

bool M2MachineCommand(unsigned int *args)
{
    extern bool bInternalEnabled;
    extern int iInternalWidth;
    extern int iInternalHeight;
    extern int iLayerWidth;
    extern int iLayerHeight;
    extern SQInteger M2_ScreenWidth;
    extern SQInteger M2_ScreenHeight;

    struct M2_EmuPSX *psx = reinterpret_cast<struct M2_EmuPSX *>(args[1]);
    unsigned int cmd = args[2];

    struct M2_EmuGPU *gpu = psx->DevGPU;

    switch (cmd)
    {
        case 0x8001: // DRAW
            break;

        case 0x8002: // GET_LAYER
        {
            if (!bInternalEnabled) return false;

            unsigned int w = (gpu->ScreenRangeW >> 12 & 0xFFF) - (gpu->ScreenRangeW & 0xFFF);
            unsigned int h = ((gpu->ScreenRangeH >> 10 & 0x3FF) - (gpu->ScreenRangeH & 0x3FF)) << ((gpu->Status >> 22) & 1);
            unsigned int x = (gpu->VideoMode ? 256 : 240) << ((gpu->Status >> 22) & 1);
            unsigned int y = min(h, x);

            (* (unsigned int *) args[3]) = (2560 - w) >> 1; // X
            (* (unsigned int *) args[4]) = (x - y) >> 1;    // Y
            (* (unsigned int *) args[5]) = iLayerWidth;     // W
            (* (unsigned int *) args[6]) = iLayerHeight;    // H

            return true;
        }

        case 0x8003: // GET_DIMENSION
        {
            if (!bInternalEnabled) return false;

            if ((gpu->Status >> 16) & 1) {
                return false;
            }

            (* (unsigned int *) args[3]) = iInternalWidth;

            static const unsigned int h[] = { 256, 320, 512, 640 };
            static const unsigned int hx = 368;

            (* (unsigned int *) args[3]) *= h[(gpu->Status >> 17) & 3];
            (* (unsigned int *) args[3]) /= h[1];

            if ((gpu->Status >> 16) & 1) {
                (* (unsigned int *) args[3]) *= hx;
                (* (unsigned int *) args[3]) /= h[(gpu->Status >> 17) & 3];
            }

            (* (unsigned int *) args[4]) = iInternalHeight;

            static const unsigned int w[] = { 240, 480 };

            (* (unsigned int *) args[4]) *= w[(gpu->Status >> 19) & 1];
            (* (unsigned int *) args[4]) /= w[0];

            /*
            LOG_F(INFO, "Emulator: GET_DIMENSION: %ux%u -> %ux%u.",
                ((gpu->Status >> 16) & 1) ? hx : h[(gpu->Status >> 17) & 3],
                w[(gpu->Status >> 19) & 1],
                (* (unsigned int *) args[3]),
                (* (unsigned int *) args[4]));
            */

            return true;
        }

        case 0x8004: // SUBMIT
        {
            if (!bInternalEnabled) return false;

            if ((gpu->Status >> 16) & 1) {
                return false;
            }

            unsigned int *res = (unsigned int *) args[3];

            res[1] = iInternalWidth;

            static const unsigned int h[] = { 256, 320, 512, 640 };
            static const unsigned int hx = 368;

            res[1] *= h[1];
            res[1] /= h[(gpu->Status >> 17) & 3];

            if ((gpu->Status >> 16) & 1) {
                res[1] *= h[(gpu->Status >> 17) & 3];
                res[1] /= hx;
            }

            res[2] = iInternalHeight;

            static const unsigned int w[] = { 240, 480 };

            res[2] *= w[0];
            res[2] /= w[(gpu->Status >> 19) & 1];

            return false;
        }

        default: break;
    }

    return false;
}
