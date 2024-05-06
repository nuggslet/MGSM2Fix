#include "stdafx.h"

#include "emulator.h"

#include "helper.hpp"

using namespace std;

extern HMODULE baseModule;
extern M2FixGame eGameType;

uintptr_t R3000_DecodeAddress;

typedef struct {
    unsigned int spec;
    int index;
    variant<M2FUNCTION, M2FUNCTION *> table;
} R3000_InstructionRecord;

std::map<string, R3000_InstructionRecord> R3000_InstructionRecords = {
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

std::map<M2FUNCTION, M2FUNCTION> R3000_HookTable;

M2FUNCTION R3000_Decode(uint32_t instruction)
{
    M2FUNCTION (__fastcall * _R3000_Decode)(uint32_t) = (M2FUNCTION (__fastcall *)(uint32_t)) R3000_DecodeAddress;
    return _R3000_Decode(instruction);
}

void R3000_Hook(const char *table, int index, M2FUNCTION func)
{
    if (R3000_InstructionRecords.count(table) == 0) return;
    auto &rec = R3000_InstructionRecords[table];
    if (rec.index < 0) return;

    M2FUNCTION *funcs = get<M2FUNCTION *>(rec.table);

    R3000_HookTable[func] = funcs[index];
    Memory::PatchBytes(reinterpret_cast<uintptr_t>(&funcs[index]), reinterpret_cast<const char*>(&func), sizeof(M2FUNCTION));

    LOG_F(INFO, "R3000: Hooked %s[%d] at 0x%" PRIxPTR ".", table, index, &func);
}

int R3000_GTE(struct M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    struct M2_EmuCoprocGTE *gte = cpu->CoprocGTE;

    gte->Instruction = cpu->Instruction;
    gte->Reg[63] = 0;

    extern int GTE_execute(struct M2_EmuCoprocGTE *gte, int op);
    int res = GTE_execute(gte, gte->Instruction);

    if (res != 0) {
        return cpu->Step(cpu, cycle + res, address + sizeof(uint32_t));
    }

    cpu->Methods->Error(cpu);
    return cycle;
}

int R3000_CommandGTE(struct M2_EmuCoprocGTE *gte, int op)
{
    gte->Instruction = op;
    gte->Reg[63] = 0;

    extern int GTE_execute(struct M2_EmuCoprocGTE *gte, int op);
    return GTE_execute(gte, gte->Instruction);
}

void R3000Hook()
{
    switch (eGameType) {
        case M2FixGame::MGS1: break;
        default: return;
    }

    uint8_t* R3000_DecodeResult = Memory::PatternScan(baseModule, "8B C1 C1 E8 1A 85 C0 75 15 85 C9 75 06 B8");
    if (R3000_DecodeResult)
    {
        R3000_DecodeAddress = (uintptr_t)R3000_DecodeResult;
        LOG_F(INFO, "R3000: r3000_decode is 0x%" PRIxPTR ".", R3000_DecodeAddress);
    }
    else if (!R3000_DecodeResult)
    {
        LOG_F(INFO, "R3000: r3000_decode scan failed.");
    }

    for (auto &rec : R3000_InstructionRecords) {
        auto &label = rec.first;
        R3000_InstructionRecord & record = rec.second;

        M2FUNCTION func = R3000_Decode(record.spec);
        if (record.index < 0) {
            LOG_F(INFO, "R3000: %s is 0x%" PRIxPTR ".", label.c_str(), func);
            record.table = func;
            continue;
        }

        // Sad!
        ostringstream funcstream;
        funcstream << setfill('0') << hex;
        for (unsigned int i = 0; i < sizeof(M2FUNCTION); i++) {
            funcstream << " " << setw(2) << static_cast<int>((reinterpret_cast<unsigned char *>(&func))[i]);
        }
        string funcaddr = funcstream.str();
        funcaddr.erase(funcaddr.begin());

        uint8_t *table = Memory::PatternScan(baseModule, funcaddr.c_str());
        if (table) {
            table -= record.index * sizeof(M2FUNCTION);
            record.table = reinterpret_cast<M2FUNCTION *>(table);
            LOG_F(INFO, "R3000: %s at 0x%" PRIxPTR ".", label.c_str(), record.table);
        } else if (!table) {
            LOG_F(INFO, "R3000: %s scan failed.", label.c_str());
        }
    }

    M2FUNCTION GTE = get<M2FUNCTION>(R3000_InstructionRecords["GTE"].table);
    Memory::DetourFunction(GTE, R3000_GTE, 1 + sizeof(uintptr_t));
}
