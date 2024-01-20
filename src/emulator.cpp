#include "stdafx.h"

using namespace std;

extern int iEmulatorLevel;

#ifndef _WIN64

extern bool bPatchesMosaic;

typedef int (*M2FUNCTION)(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);

typedef struct M2_EmuPSX {
    struct M2_MethodsPSX *Methods;
    void *Components;
    void *Archive;
    void *ImageBIOS;
    void *ImageDRAM;
    void *MemoryDRAM;
    void *MemoryTCM;
    struct M2_EmuBusPSX *Bus;
    struct M2_EmuR3000 *DevR3000;
    void *DevCDROM;
    void *DevDMAC;
    void *DevGPU;
    void *DevINTC;
    void *DevMDEC;
    void *DevRTC;
    void *DevSIO;
    void *DevSPU;
    union {
        struct {
            // No more, but space for up to 16.
            unsigned char Wide;
            unsigned char HighP;
        } Flag;
        unsigned char Flags[16]; // "InfoIntegers".
    };

} M2_EmuPSX;

static_assert(sizeof(M2_EmuPSX) == 0x54);

typedef struct M2_MethodsPSX {
    void (*Destroy)     (struct M2_EmuPSX *machine);
    void (*_Stub)       (struct M2_EmuPSX *machine);
    void (*Reload)      (struct M2_EmuPSX *machine, void *param);
    void (*Command)     (struct M2_EmuPSX *machine, int cmd, void *param);
    void (*Update)      (struct M2_EmuPSX *machine, int op);
} M2_MethodsPSX;

static_assert(sizeof(M2_MethodsPSX) == 0x14);

typedef struct M2_EmuBusPSX {
    unsigned char  (*Read8)  (struct M2_EmuBusPSX *bus, unsigned int address);
    unsigned short (*Read16) (struct M2_EmuBusPSX *bus, unsigned int address);
    unsigned int   (*Read32) (struct M2_EmuBusPSX *bus, unsigned int address);
    void           (*Write8) (struct M2_EmuBusPSX *bus, unsigned int address, unsigned char value);
    void           (*Write16)(struct M2_EmuBusPSX *bus, unsigned int address, unsigned short value);
    void           (*Write32)(struct M2_EmuBusPSX *bus, unsigned int address, unsigned int value);
    unsigned int _Spare[2];
    struct M2_EmuPSX *Machine;
} M2_EmuBusPSX;

static_assert(sizeof(M2_EmuBusPSX) == 0x24);

typedef struct M2_EmuCoprocGTE {
    unsigned int Reg[64];
    unsigned int SR;
    int (*Command)(struct M2_EmuCoprocGTE *gte, int id);
    int (*Result) (struct M2_EmuCoprocGTE *gte, int id);
} M2_EmuCoprocGTE;

static_assert(sizeof(M2_EmuCoprocGTE) == 0x10C);

typedef struct M2_EmuR3000 {
    struct M2_MethodsR3000 *Methods;
    void *Components;
    struct M2_EmuBusPSX *Bus;
    void *DevINTC;
    struct M2_EmuCoprocGTE *CoprocGTE;
    M2FUNCTION *Segment; // The main/non-accelerated segment.
    bool           (*Execute)(struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    unsigned char  (*Read8)  (struct M2_EmuR3000 *cpu, unsigned int address);
    unsigned short (*Read16) (struct M2_EmuR3000 *cpu, unsigned int address);
    unsigned int   (*Read32) (struct M2_EmuR3000 *cpu, unsigned int address);
    void           (*Write8) (struct M2_EmuR3000 *cpu, unsigned int address, unsigned char value);
    void           (*Write16)(struct M2_EmuR3000 *cpu, unsigned int address, unsigned short value);
    void           (*Write32)(struct M2_EmuR3000 *cpu, unsigned int address, unsigned int value);
    unsigned int Accelerator;
    bool           (*Step)   (struct M2_EmuR3000 *cpu, int cycle, unsigned int address);
    unsigned int Cycle;
    unsigned int _Spare; // Unused, leaks uninitialised memory.
    unsigned int Target;
    unsigned int Instruction;
    unsigned int State;
    unsigned int Hi;
    unsigned int Lo;
    unsigned int PC0;
    unsigned int PC1;
    unsigned int Reg[32];
    unsigned int SR;
    unsigned int Cause;
    unsigned int EPC;
    unsigned int OffsetDRAM;
    unsigned int SizeDRAM;
    unsigned char *BufferDRAM;
    M2FUNCTION *Segments[0x80000000 / 0x80000];
    void *MemoryVRAM; // What the fuck?
    unsigned int Cycles;
} M2_EmuR3000;

static_assert(sizeof(M2_EmuR3000) == 0x4100);

typedef struct M2_MethodsR3000 {
    void (*Destroy)(struct M2_EmuR3000 *cpu);
    void (*_Stub)  (struct M2_EmuR3000 *cpu);
    void (*Reload) (struct M2_EmuR3000 *cpu);
    void (*Boot)   (struct M2_EmuR3000 *cpu, int op);
    void (*Command)(struct M2_EmuR3000 *cpu, int cmd, void *param);
    void (*Dump)   (struct M2_EmuR3000 *cpu);
    void (*Error)  (struct M2_EmuR3000 *cpu);
} M2_MethodsR3000;

static_assert(sizeof(M2_MethodsR3000) == 0x1C);

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

map<unsigned int, M2FUNCTION> M2_ModuleHandlers;

int M2Hook_main(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
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
    LOG_F(INFO, "MGS 1: %s s03a_disable_mosaic().", bPatchesMosaic ? "Allowing" : "Blocking");
    if (bPatchesMosaic) {
        M2FUNCTION s03a_disable_mosaic = M2_ModuleHandlers[address];
        return s03a_disable_mosaic(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

int M2Hook_s03d_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    LOG_F(INFO, "MGS 1: %s s03d_disable_mosaic().", bPatchesMosaic ? "Allowing" : "Blocking");
    if (bPatchesMosaic) {
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
    FixUserModules();
    FixKernelModules();
}

#else

void FixModules()
{
    return;
}

#endif
