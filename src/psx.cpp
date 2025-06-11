#include "m2fix.h"
#include "psx.h"

M2_EmuPSX *PSX::Emulator = nullptr;

std::map<unsigned, PSXFUNCTION> PSX::ModuleHandlers = {};
std::map<unsigned, PSXFUNCTION> PSX::LibraryHandlers = {};

std::map<unsigned, const char *> PSX::Libraries = {
    {0x0, "kernel"},
    {0x1, "libc2"},
    {0x2, "libcard"},
    {0x3, "libcd"},
    {0x4, "libetc"},
    {0x5, "libgpu"},
    {0x6, "libgte"},
    {0x7, "libpad"},
    {0x8, "libpress"},
    {0x9, "libsnd"},
    {0xA, "libspu"},
};

std::vector<std::pair<unsigned int, PSXFUNCTION>> PSX::ModuleTable_Kernel = {
    {0xA0, Kernel_Function},
    {0xB0, Kernel_Function},
    {0xC0, Kernel_Function},
    {0xF000, Kernel_Call},
    {0xFFF0, Kernel_Vector},
};

std::map<M2_EmuPSX_Module *, M2_EmuPSX_Module *> PSX::ModuleMap = {};

unsigned int PSX::ScreenWidth = 0;
unsigned int PSX::ScreenHeight = 0;
unsigned int PSX::ScreenScaleX = 0;
unsigned int PSX::ScreenScaleY = 0;
unsigned int PSX::ScreenMode = 0;
unsigned int PSX::VideoMode = 0;

std::map<std::string, PSX::R3000_InstructionRecord> PSX::R3000_InstructionRecords = {
    {"BcondZ",  {0x04000000,  0}},
    {"SCP",     {0x42000010, -1}},
    {"COP0",    {0x40000000,  0}},
    {"GTE",     {0x4A000000, -1}},
    {"COP2",    {0x48000000,  0}},
    {"LBx",     {0x80000000, -1}},
    {"LWx",     {0x8C000000, -1}},
    {"BASE",    {0x08000000,  2}},
    {"SPECIAL", {0x00000040,  0}},
    {"NOP",     {0x00000000, -1}},
};

std::map<PSXFUNCTION, PSXFUNCTION> PSX::R3000_HookTable;

std::function<PSXFUNCTION(uint32_t)> PSX::R3000_Decode;

M2_EmuPSX_Module *PSX::LoadSystemModule(M2_EmuPSX_Module *mod)
{
    M2_EmuPSX_SystemModule *system = mod->system;
    spdlog::info("[PSX] Loading system module {} at {}.", mod->name, fmt::ptr(mod));

    return mod;
}

M2_EmuPSX_Module *PSX::LoadKernelModule(M2_EmuPSX_Module *mod)
{
    M2_EmuPSX_KernelModule *kernel = mod->kernel;
    if (ModuleMap.count(mod) != 0) {
        M2_EmuPSX_Module *_mod = ModuleMap[mod];
        spdlog::info("[PSX] Already hooked kernel module {} from {} to {}.", _mod->name, fmt::ptr(mod), fmt::ptr(_mod));
        return _mod;
    }

    M2_EmuPSX_TableEntry *_table = new M2_EmuPSX_TableEntry[kernel->count];
    memcpy(_table, kernel->table, sizeof(M2_EmuPSX_TableEntry) * kernel->count);

    M2_EmuPSX_KernelModule *_kernel = new M2_EmuPSX_KernelModule;
    memcpy(_kernel, kernel, sizeof(M2_EmuPSX_KernelModule));
    _kernel->table = _table;

    M2_EmuPSX_Module *_mod = new M2_EmuPSX_Module;
    memcpy(_mod, mod, sizeof(M2_EmuPSX_Module));
    _mod->kernel = _kernel;

    ModuleMap[mod] = _mod;
    spdlog::info("[PSX] Hooked kernel module {} from {} to {}.", _mod->name, fmt::ptr(mod), fmt::ptr(_mod));
    return _mod;
}

M2_EmuPSX_Module *PSX::LoadUserModule(M2_EmuPSX_Module *mod)
{
    M2_EmuPSX_UserModule *user = mod->user;

    if (ModuleMap.count(mod) != 0) {
        M2_EmuPSX_Module *_mod = ModuleMap[mod];
        spdlog::info("[PSX] Already hooked user module {} from {} to {}.", _mod->name, fmt::ptr(mod), fmt::ptr(_mod));
        return _mod;
    }

    M2_EmuPSX_TableEntry *_table = new M2_EmuPSX_TableEntry[user->count];
    memcpy(_table, user->table, sizeof(M2_EmuPSX_TableEntry) * user->count);

    M2_EmuPSX_UserModule *_user = new M2_EmuPSX_UserModule;
    memcpy(_user, user, sizeof(M2_EmuPSX_UserModule));
    _user->table = _table;

    M2_EmuPSX_Module *_mod = new M2_EmuPSX_Module;
    memcpy(_mod, mod, sizeof(M2_EmuPSX_Module));
    _mod->user = _user;

    ModuleMap[mod] = _mod;
    spdlog::info("[PSX] Hooked user module {} from {} to {}.", _mod->name, fmt::ptr(mod), fmt::ptr(_mod));
    return _mod;
}

void * __fastcall PSX::LoadModule(M2_EmuPSX_Module *mod, void *list)
{
    Emulator = nullptr;
    switch (mod->type)
    {
        case 1:
            mod = LoadKernelModule(mod);
            break;
        case 3:
            mod = LoadSystemModule(mod);
            break;
        case 4:
            mod = LoadUserModule(mod);
            break;
        default:
            spdlog::info("[PSX] Loading module {} with type {} at {}.", mod->name, mod->type, fmt::ptr(mod));
            break;
    }

    return M2Hook::GetInstance().Invoke<void *>(PSX::LoadModule, mod, list);
}

void * __fastcall PSX::ListInsert(void *list, void *object)
{
    // Didn't end up using this but the detour might be useful for something else in future...
    return M2Hook::GetInstance().Invoke<void *>(PSX::ListInsert, list, object);
}

void PSX::main(M2_EmuR3000 *cpu)
{
    Emulator = cpu->Bus->Machine;
    spdlog::info("[PSX] Machine at {}.", fmt::ptr(cpu->Bus->Machine));
    spdlog::info("[PSX] Archive at {}.", cpu->Bus->Machine->Archive);
    spdlog::info("[PSX] BIOS at {}.", fmt::ptr(cpu->Bus->Machine->ImageBIOS));
    spdlog::info("[PSX] Image at {}.", fmt::ptr(cpu->Bus->Machine->ImageDRAM));
    spdlog::info("[PSX] DRAM at {}.", cpu->Bus->Machine->MemoryDRAM);
    spdlog::info("[PSX] TCM at {}.", cpu->Bus->Machine->MemoryTCM);
    spdlog::info("[PSX] Bus at {}.", fmt::ptr(cpu->Bus->Machine->Bus));
    spdlog::info("[PSX] R3000 at {}.", fmt::ptr(cpu->Bus->Machine->DevR3000));
    spdlog::info("[PSX] GTE at {}.", fmt::ptr(cpu->CoprocGTE));
    spdlog::info("[PSX] CDROM at {}.", cpu->Bus->Machine->DevCDROM);
    spdlog::info("[PSX] DMAC at {}.", cpu->Bus->Machine->DevDMAC);
    spdlog::info("[PSX] GPU at {}.", fmt::ptr(cpu->Bus->Machine->DevGPU));
    spdlog::info("[PSX] VRAM at {}.", fmt::ptr(cpu->Bus->Machine->DevGPU->MemoryVRAM));
    spdlog::info("[PSX] INTC at {}.", cpu->Bus->Machine->DevINTC);
    spdlog::info("[PSX] MDEC at {}.", cpu->Bus->Machine->DevMDEC);
    spdlog::info("[PSX] RTC at {}.", cpu->Bus->Machine->DevRTC);
    spdlog::info("[PSX] SIO at {}.", cpu->Bus->Machine->DevSIO);
    spdlog::info("[PSX] SPU at {}.", cpu->Bus->Machine->DevSPU);
}

int PSX::Kernel_Function(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    PSXFUNCTION Kernel_Function = ModuleHandlers[address];

    if (M2Config::iEmulatorLevel >= 2) {
        char type = (((address & 0xF0) - 0xA0) >> 4) + 'A';
        unsigned int ra = cpu->Reg[31];
        unsigned int r9 = cpu->Reg[9];
        spdlog::info("[PSX] Kernel_Function{}(0x{:x}): 0x{:x}.", type, r9, ra);
    }

    return Kernel_Function(cpu, cycle, address);
}

int PSX::Kernel_Vector(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    PSXFUNCTION Kernel_Vector = ModuleHandlers[address];

    unsigned int ra = cpu->Reg[31];
    unsigned int r9 = cpu->Reg[9];

    if (M2Config::iEmulatorLevel >= 1) {
        spdlog::info("[PSX] Vector({}: 0x{:x}): 0x{:x}.", Libraries[r9 >> 8], r9 & 0xFF, ra);
    }

    if (LibraryHandlers.count(r9)) {
        cpu->Reg[9] &= 0xFF;
        auto Library_Handler = LibraryHandlers[r9];
        return Library_Handler(cpu, cycle + 1, address);
    }

    return Kernel_Vector(cpu, cycle, address);
}

int PSX::Kernel_Call(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    PSXFUNCTION Kernel_Call = ModuleHandlers[address];

    if (M2Config::iEmulatorLevel >= 3) {
        unsigned int ra = cpu->Reg[31];
        unsigned int r9 = cpu->Reg[9];
        spdlog::info("[PSX] Kernel_Call(0x{:x}): 0x{:x}.", r9, ra);
    }

    return Kernel_Call(cpu, cycle, address);
}

void PSX::BindKernelModules(std::vector<std::pair<unsigned int, PSXFUNCTION>> &table)
{
    for (auto const & i : ModuleMap) {
        M2_EmuPSX_Module *module = i.second;
        if (module->type != 1) continue;
        M2_EmuPSX_KernelModule *kernel = module->kernel;

        for (auto & func : table) {
            for (int i = 0; i < kernel->count; i++) {
                if (kernel->table[i].address != func.first) continue;
                ModuleHandlers[kernel->table[i].address] = kernel->table[i].handler;
                kernel->table[i].handler = func.second;
                break;
            }
        }
    }

    spdlog::info("[PSX] Applied hooks to kernel modules.");
}

void PSX::BindUserModules(PSX_ModuleTables & tables, const char *basename)
{
    // To avoid repetitively populating the tables for each game version, we can exploit certain hooks being 
    // assigned to the same native function. This means if you're lucky only e.g. the Integral table needs 
    // to be populated for a given hook. Integral has been chosen as it tends to receive the most attention.
    while (basename && tables.count(basename) != 0) {
        auto basetable = tables[basename];

        M2_EmuPSX_Module *base = NULL;
        for (auto const &i : ModuleMap) {
            M2_EmuPSX_Module *module = i.second;
            if (module->type != 4) continue;
            if (strcmp(module->name, basename) == 0) {
                base = module;
                break;
            }
        }

        if (!base) {
            spdlog::info("[PSX] Couldn't find {} base module.", basename);
            break;
        }

        std::map<PSXFUNCTION, PSXFUNCTION> basemap;

        // Now the base module is located, we can create the mapping between handler and hook.
        M2_EmuPSX_UserModule *user = base->user;
        for (auto & func : *basetable) {
            for (int i = 0; i < user->count; i++) {
                if (user->table[i].address != func.first) continue;
                basemap[func.second] = user->table[i].handler;
                ModuleHandlers[user->table[i].address] = user->table[i].handler;
                user->table[i].handler = func.second;
                break;
            }
        }

        spdlog::info("[PSX] Applied hooks to {}.", base->name);

        // With the mapping created, hook each module's handler.
        for (auto const & i : ModuleMap) {
            M2_EmuPSX_Module *module = i.second;
            if (module->type != 4) continue;
            M2_EmuPSX_UserModule *user = module->user;

            if (strcmp(basename, module->name) == 0)
                continue;

            for (auto & func : basemap) {
                for (int i = 0; i < user->count; i++) {
                    if (user->table[i].handler != func.second) continue;
                    ModuleHandlers[user->table[i].address] = user->table[i].handler;
                    user->table[i].handler = func.first;
                    break;
                }
            }

            spdlog::info("[PSX] Applied hooks from {} to {}.", base->name, module->name);
        }

        break;
    }

    // Fall-back for cases where the native functions are essentially duplicated with e.g.
    // only changes to ReadN/WriteN/Step parameters but are otherwise conceptually the same.
    // These require populating the table for each game version.
    for (auto const & i : ModuleMap) {
        M2_EmuPSX_Module *module = i.second;
        if (module->type != 4) continue;
        M2_EmuPSX_UserModule *user = module->user;

        if (basename && strcmp(basename, module->name) == 0)
            continue;
        if (tables.count(module->name) == 0)
            continue;

        auto table = tables[module->name];
        for (auto & func : *table) {
            for (int i = 0; i < user->count; i++) {
                if (user->table[i].address != func.first) continue;
                ModuleHandlers[user->table[i].address] = user->table[i].handler;
                user->table[i].handler = func.second;
                break;
            }
        }

        spdlog::info("[PSX] Applied hooks to {}.", module->name);
    }
}

void PSX::BindModules()
{
    auto [_tables, basename] = M2Fix::GameInstance().EPIModuleHook();
    if (_tables.has_value()) {
        auto tables = std::any_cast<PSX_ModuleTables>(_tables);
        BindUserModules(tables, basename);
    }
    BindKernelModules();
}

void PSX::UpdateScreenGeometry(unsigned int width, unsigned int height, unsigned int scale_x, unsigned int scale_y, unsigned int mode)
{
    ScreenWidth = width;
    ScreenHeight = height;
    ScreenScaleX = scale_x;
    ScreenScaleY = scale_y;
    ScreenMode = mode;
}

void PSX::CommandR3000(M2_EmuR3000 *cpu, int cmd, unsigned int **args)
{
    struct M2_EmuPSX *psx = cpu->Bus->Machine;
    switch (cmd)
    {
        case 6:
        {
            LoadImage(psx->MemoryDRAM, psx->ImageDRAM[2]);
            break;
        }

        default: break;
    }

    bool ret = M2Fix::GameInstance().EPIOnCommandCPU(cpu, cmd, args);
    if (ret) {
        M2Hook::GetInstance().Invoke<void>(CommandR3000, cpu, cmd, args);
    }
}

void PSX::CommandPSX(M2_EmuPSX *psx, int cmd, unsigned int **args)
{
    bool ret = M2Fix::GameInstance().EPIOnMachineCommand(psx, cmd, args);
    if (ret) {
        M2Hook::GetInstance().Invoke<void>(CommandPSX, psx, cmd, args);
    }
}

void PSX::LoadImage(void *image, size_t size)
{
    spdlog::info("[PSX] Loading image at {} with size {} bytes.", image, size);
    M2Fix::GameInstance().EPIOnLoadImage(image, size);
}

void PSX::R3000_Hook(const char *table, int index, PSXFUNCTION func)
{
    if (R3000_InstructionRecords.count(table) == 0) return;
    auto & rec = R3000_InstructionRecords[table];
    if (rec.index < 0) return;

    PSXFUNCTION *funcs = std::get<PSXFUNCTION *>(rec.table);

    R3000_HookTable[func] = funcs[index];
    M2Hook::GetInstance().Patch(&funcs[index], &func, sizeof(PSXFUNCTION), nullptr);

    spdlog::info("[PSX] R3000 hooked {}[{}] at {}.", table, index, fmt::ptr(&func));
}

#ifndef _WIN64
void PSX::GTE_RTP(safetyhook::Context & ctx)
{
    int64_t sx2 = 0;

    int64_t x = 1;
    int64_t y = 1;

    if (M2Config::bInternalWidescreen)
    {
        switch (ScreenMode)
        {
            case 3:
                x = ScreenScaleX;
                y = ScreenScaleY;
                break;
            case 7:
                x = 4;
                y = 3;
                break;
            default: break;
        }
    }

    sx2 = ctx.edx;
    sx2 <<= 32;
    sx2 |= ctx.eax;

    sx2 = (sx2 * y) / x;

    ctx.eax = sx2 & UINT32_MAX;
    ctx.edx = sx2 >> 32;
}
#endif

void PSX::CommandR3000101(M2_EmuR3000 *cpu, int cmd, unsigned int **args)
{
    struct M2_EmuPSX *psx = cpu->Bus->Machine;
    switch (cmd)
    {
        case 6:
        {
            LoadImage(psx->MemoryDRAM, psx->ImageDRAM[2]);
            break;
        }

        default: break;
    }

    bool ret = M2Fix::GameInstance().EPIOnCommandCPU(cpu, cmd, args);
    if (ret) {
        M2Hook::GetInstance().Invoke<void>(CommandR3000101, cpu, cmd, args);
    }
}

void PSX::CommandPSX101(M2_EmuPSX *psx, int cmd, unsigned int **args)
{
    bool ret = M2Fix::GameInstance().EPIOnMachineCommand(psx, cmd, args);
    if (ret) {
        M2Hook::GetInstance().Invoke<void>(CommandPSX101, psx, cmd, args);
    }
}

void PSX::Load()
{
    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        {
            M2Hook::GetInstance().Hook(
                "8B 44 24 ?? 05 ?? ?? ?? ?? 83 F8 ?? 0F 87 ?? ?? "
                "?? ?? 53",
                0, PSX::CommandPSX, "[PSX] machine_psx_cmd"
            );

            M2Hook::GetInstance().Hook(
                "8B 44 24 ?? 48 83 F8 ?? 0F 87 ?? ?? ?? ?? FF 24 "
                "85 ?? ?? ?? ?? 8B 44 24",
                0, PSX::CommandR3000, "[PSX] cpu_r3000_cmd"
            );

            M2Hook::GetInstance().Hook(
                "83 EC 08 53 55 56 8B 35 ?? ?? ?? ?? 8B DA 8B E9",
                0, PSX::LoadModule, "[PSX] m2epi_load_module"
            );

            M2Hook::GetInstance().Hook(
                "83 EC 0C 89 4C 24 04 56 8D 74 24 04 57 8B FA 85",
                0, PSX::ListInsert, "[PSX] m2epi_list_insert"
            );

            break;
        }

        case M2FixGame::Darius101:
        {
            M2Hook::GetInstance().Hook(
                "8B 44 24 08 3D 04 90 00 00 0F 8F 5C 01 00 00 56",
                0, PSX::CommandPSX, "[PSX] machine_psx_cmd"
            );

            M2Hook::GetInstance().Hook(
                "55 8B EC 83 E4 F8 8B 45 0C 05 FF 7F FF FF 83 F8 "
                "04 77 64",
                0, PSX::CommandPSX101, "[PSX] machine_psx_101_cmd"
            );

            M2Hook::GetInstance().Hook(
                "8B 44 24 08 48 83 F8 09 0F 87 CE 00 00 00 FF 24",
                0, PSX::CommandR3000, "[PSX] cpu_r3000_cmd"
            );

            M2Hook::GetInstance().Hook(
                "8B 44 24 08 48 83 F8 08 0F 87 CE 00 00 00 FF 24",
                0, PSX::CommandR3000101, "[PSX] cpu_r3000_101_cmd"
            );

            M2Hook::GetInstance().Hook(
                "83 EC 08 53 55 56 8B 35 ?? ?? ?? ?? 8B DA 8B E9",
                0, PSX::LoadModule, "[PSX] m2epi_load_module"
            );

            M2Hook::GetInstance().Hook(
                "83 EC 0C 89 4C 24 04 56 8D 74 24 04 57 8B FA 85",
                0, PSX::ListInsert, "[PSX] m2epi_list_insert"
            );

            break;
        }

        default: break;
    }

    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        {
            PSX::R3000_Decode = reinterpret_cast<PSXFUNCTION(__fastcall *)(uint32_t)>(
                M2Hook::GetInstance().Scan(
                    "8B C1 C1 E8 1A 85 C0 75 15 85 C9 75 06 B8",
                    0, "[PSX] r3000_decode"
                ));

            for (auto & rec : PSX::R3000_InstructionRecords) {
                auto & [label, record] = rec;

                PSXFUNCTION func = PSX::R3000_Decode(record.spec);
                if (record.index < 0) {
                    spdlog::info("[PSX] R3000 {} is {}.", label, fmt::ptr(func));
                    record.table = func;
                    continue;
                }

                record.table = reinterpret_cast<PSXFUNCTION *>(
                    M2Hook::GetInstance().Scan(
                        &func, sizeof(PSXFUNCTION),
                        - ((std::ptrdiff_t) sizeof(PSXFUNCTION) * record.index),
                        ("[PSX] R3000 " + label).c_str()
                    ));
            }

    #ifndef _WIN64
            M2Hook::GetInstance().MidHook(
                "8B D8 8B CA 8B 86 E0 00 00 00 99 03 D8 13 CA 0F",
                0, PSX::GTE_RTP, "[PSX] GTE RTP"
            );
    #endif

            break;
        }

        default: break;
    }
}
