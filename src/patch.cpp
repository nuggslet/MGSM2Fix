#include "stdafx.h"

#include "helper.hpp"

using namespace std;

extern HMODULE gBaseModule;

enum class M2FixGame
{
    Unknown,
    MGS1,
    MGSR,
    Contra,
    Dracula,
    DraculaAdvance,
    Ray,
    Darius,
    DariusHD,
};

extern M2FixGame eGameType;

void ScanFunctions()
{
    extern uintptr_t M2_mallocAddress;
    extern uintptr_t M2_reallocAddress;
    extern uintptr_t M2_freeAddress;

    // MGS 1: Squirrel call
    if (eGameType == M2FixGame::MGS1    || eGameType == M2FixGame::Contra ||
        eGameType == M2FixGame::Dracula || eGameType == M2FixGame::DraculaAdvance ||
        eGameType == M2FixGame::Darius  || eGameType == M2FixGame::DariusHD)
    {
        uint8_t* M2_mallocScanResult = Memory::PatternScan(gBaseModule,
            "8B FF 55 8B EC 56 8B 75 08 83 FE E0 77 30 85 F6");
        if (M2_mallocScanResult)
        {
            M2_mallocAddress = (uintptr_t)M2_mallocScanResult;
            LOG_F(INFO, "M2-32: malloc is 0x%" PRIxPTR ".", M2_mallocAddress);
        }
        else if (!M2_mallocScanResult)
        {
            LOG_F(INFO, "M2-32: malloc scan failed.");
        }

        uint8_t* M2_reallocScanResult = Memory::PatternScan(gBaseModule,
            "8B FF 55 8B EC 57 8B 7D 08 85 FF 75 0B FF 75 0C");
        if (M2_reallocScanResult)
        {
            M2_reallocAddress = (uintptr_t)M2_reallocScanResult;
            LOG_F(INFO, "M2-32: realloc is 0x%" PRIxPTR ".", M2_reallocAddress);
        }
        else if (!M2_reallocScanResult)
        {
            LOG_F(INFO, "M2-32: realloc scan failed.");
        }

        uint8_t* M2_freeScanResult = Memory::PatternScan(gBaseModule,
            "8B FF 55 8B EC 83 7D 08 00 74 2D FF 75 08 6A 00");
        if (M2_freeScanResult)
        {
            M2_freeAddress = (uintptr_t)M2_freeScanResult;
            LOG_F(INFO, "M2-32: free is 0x%" PRIxPTR ".", M2_freeAddress);
        }
        else if (!M2_freeScanResult)
        {
            LOG_F(INFO, "M2-32: free scan failed.");
        }
    }

    // MG | SR: Squirrel call
    if (eGameType == M2FixGame::MGSR || eGameType == M2FixGame::Ray)
    {
        uint8_t* M2_mallocScanResult = Memory::PatternScan(gBaseModule,
            "40 53 48 83 EC 20 48 8B D9 48 83 F9 E0 77 3C 48");
        if (M2_mallocScanResult)
        {
            M2_mallocAddress = (uintptr_t)M2_mallocScanResult;
            LOG_F(INFO, "M2-64: malloc is 0x%" PRIxPTR ".", M2_mallocAddress);
        }
        else if (!M2_mallocScanResult)
        {
            LOG_F(INFO, "M2-64: malloc scan failed.");
        }

        uint8_t* M2_reallocScanResult = Memory::PatternScan(gBaseModule,
            "48 89 5C 24 08 57 48 83 EC 20 48 8B DA 48 8B F9 48 85 C9 75 0A 48 8B CA E8 ?? ?? ?? ?? EB 1F 48");
        if (M2_reallocScanResult)
        {
            M2_reallocAddress = (uintptr_t)M2_reallocScanResult;
            LOG_F(INFO, "M2-64: realloc is 0x%" PRIxPTR ".", M2_reallocAddress);
        }
        else if (!M2_reallocScanResult)
        {
            LOG_F(INFO, "M2-64: realloc scan failed.");
        }

        uint8_t* M2_freeScanResult = Memory::PatternScan(gBaseModule,
            "48 85 C9 74 37 53 48 83 EC 20 4C 8B C1 33 D2 48");
        if (M2_freeScanResult)
        {
            M2_freeAddress = (uintptr_t)M2_freeScanResult;
            LOG_F(INFO, "M2-64: free is 0x%" PRIxPTR ".", M2_freeAddress);
        }
        else if (!M2_freeScanResult)
        {
            LOG_F(INFO, "M2-64: free scan failed.");
        }
    }
}

extern void _SQNew_Standard(HSQUIRRELVM<Squirk::Standard> v);
extern void _SQNew_AlignObject(HSQUIRRELVM<Squirk::AlignObject> v);

// MGS 1: Squirrel hook
uintptr_t MGS1_SQVMReturnJMP;
void __declspec(naked) MGS1_SQVM_CC()
{
#ifndef _WIN64
    __asm
    {
        push esi
        push eax
        push ecx

        push ebp
        mov ebp, esp

        push esi
        call _SQNew_Standard

        mov esp, ebp
        pop ebp

        pop ecx
        pop eax
        pop esi

        mov dword ptr[esi + 0A4h], 0FFFFFFFFh
        jmp[MGS1_SQVMReturnJMP]
    }
#endif
}

uintptr_t COCA_SQVMReturnJMP;
void __declspec(naked) COCA_SQVM_CC()
{
#ifndef _WIN64
    __asm
    {
        push esi
        push eax
        push ecx

        push ebp
        mov ebp, esp

        push esi
        call _SQNew_AlignObject

        mov esp, ebp
        pop ebp

        pop ecx
        pop eax
        pop esi

        mov dword ptr[esi + 0D4h], 0FFFFFFFFh
        jmp[COCA_SQVMReturnJMP]
    }
#endif
}

// MG | SR: Squirrel hook
uintptr_t MGSR_SQVMReturnJMP;
void __declspec(naked) MGSR_SQVM_CC()
{
#ifdef _WIN64
    __asm
    {
        push rcx
        push rdx
        push r8
        push r9

        call _SQNew_Standard

        pop r9
        pop r8
        pop rdx
        pop rcx

        mov qword ptr[rcx + 0FCh], 0FFFFFFFFFFFFFFFFh
        mov[rcx + 0E0h], rsi
        jmp[MGSR_SQVMReturnJMP]
    }
#endif
}

SQInteger _HookNative_Standard(SQFUNCTION<Squirk::Standard> func, HSQUIRRELVM<Squirk::Standard> v);
SQInteger _HookNative_AlignObject(SQFUNCTION<Squirk::AlignObject> func, HSQUIRRELVM<Squirk::AlignObject> v);

uintptr_t MGS1_SQVMCallNativeReturnJMP;
void __declspec(naked) MGS1_SQVMCallNative_CC()
{
#ifndef _WIN64
    __asm
    {
        push ebp
        mov ebp, esp

        push esi
        push eax
        call _HookNative_Standard

        mov esp, ebp
        pop ebp

        mov ecx, [ebp + 18h]
        jmp[MGS1_SQVMCallNativeReturnJMP]
    }
#endif
}

uintptr_t COCA_SQVMCallNativeReturnJMP;
void __declspec(naked) COCA_SQVMCallNative_CC()
{
#ifndef _WIN64
    __asm
    {
        push ebp
        mov ebp, esp

        push esi
        push eax
        call _HookNative_AlignObject

        mov esp, ebp
        pop ebp

        mov ecx, [ebp + 18h]
        jmp[COCA_SQVMCallNativeReturnJMP]
    }
#endif
}

uintptr_t MGSR_SQVMCallNativeReturnJMP;
void __declspec(naked) MGSR_SQVMCallNative_CC()
{
#ifdef _WIN64
    __asm
    {
        push rcx
        push rdx
        push r8
        push r9

        mov rdx, rcx
        mov rcx, qword ptr[rbx + 68h]
        call _HookNative_Standard

        pop r9
        pop r8
        pop rdx
        pop rcx

        dec dword ptr[rdi + 0F0h]
        mov rcx, [rsp + 0F8h]
        jmp[MGSR_SQVMCallNativeReturnJMP]
    }
#endif
}

extern SQRESULT _sq_setnativeclosurename_Standard(HSQUIRRELVM<Squirk::Standard> v, SQInteger idx, const SQChar *name);
extern SQRESULT _sq_setnativeclosurename_AlignObject(HSQUIRRELVM<Squirk::AlignObject> v, SQInteger idx, const SQChar *name);

uintptr_t MGS1_SqratBindFuncReturnJMP;
void __declspec(naked) MGS1_SqratBindFunc_CC()
{
#ifndef _WIN64
    __asm
    {
        push[esp + 10h]
        push -1
        push[edi + 4]
        call _sq_setnativeclosurename_Standard
        add esp, 12

        movzx eax, [esp + 20h]
        jmp[MGS1_SqratBindFuncReturnJMP]
    }
#endif
}

uintptr_t COCA_SqratBindFuncReturnJMP;
void __declspec(naked) COCA_SqratBindFunc_CC()
{
#ifndef _WIN64
    __asm
    {
        push[esp + 10h]
        push -1
        push[edi + 8]
        call _sq_setnativeclosurename_AlignObject
        add esp, 12

        movzx eax, [esp + 20h]
        jmp[COCA_SqratBindFuncReturnJMP]
    }
#endif
}

uintptr_t MGSR_SqratBindFuncReturnJMP;
void __declspec(naked) MGSR_SqratBindFunc_CC()
{
#ifdef _WIN64
    __asm
    {
        mov rcx, [r14 + 8]
        mov rdx, -1
        mov r8, [rcx + 30h] // bah
        mov r8, [r8 + 18h]
        add r8, 38h

        call _sq_setnativeclosurename_Standard

        movzx r8d, [rsp + 78h]
        mov edx, 0FFFFFFFDh
        mov rcx, [r14 + 8]
        jmp[MGSR_SqratBindFuncReturnJMP]
    }
#endif
}

void SquirrelHook()
{
    // MGS 1: Squirrel hook
    if (eGameType == M2FixGame::MGS1)
    {
        uint8_t* MGS1_SQVMScanResult = Memory::PatternScan(gBaseModule,
            "C7 86 A4 00 00 00 FF FF FF FF 89 86 A0 00 00 00");
        if (MGS1_SQVMScanResult)
        {
            uintptr_t MGS1_SQVMAddress = (uintptr_t)MGS1_SQVMScanResult;
            int MGS1_SQVMHookLength = Memory::GetHookLength((char*)MGS1_SQVMAddress, 4);
            MGS1_SQVMReturnJMP = MGS1_SQVMAddress + MGS1_SQVMHookLength;
            Memory::DetourFunction((void*)MGS1_SQVMAddress, MGS1_SQVM_CC, MGS1_SQVMHookLength);

            LOG_F(INFO, "SQ-32<Standard>: SQVM::SQVM hook length is %d bytes.", MGS1_SQVMHookLength);
            LOG_F(INFO, "SQ-32<Standard>: SQVM::SQVM hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SQVMAddress);
        }
        else if (!MGS1_SQVMScanResult)
        {
            LOG_F(INFO, "SQ-32<Standard>: SQVM::SQVM pattern scan failed.");
        }

        uint8_t* MGS1_SQVMCallNativeScanResult = Memory::PatternScan(gBaseModule,
            "FF D0 8B 4D 18 83 C4 04 FF 8E 98 00 00 00 C6 01");
        if (MGS1_SQVMCallNativeScanResult)
        {
            uintptr_t MGS1_SQVMCallNativeAddress = (uintptr_t)MGS1_SQVMCallNativeScanResult;
            int MGS1_SQVMCallNativeHookLength = Memory::GetHookLength((char*)MGS1_SQVMCallNativeAddress, 4);
            MGS1_SQVMCallNativeReturnJMP = MGS1_SQVMCallNativeAddress + MGS1_SQVMCallNativeHookLength;
            Memory::DetourFunction((void*)MGS1_SQVMCallNativeAddress, MGS1_SQVMCallNative_CC, MGS1_SQVMCallNativeHookLength);

            LOG_F(INFO, "SQ-32<Standard>: SQVM::CallNative hook length is %d bytes.", MGS1_SQVMCallNativeHookLength);
            LOG_F(INFO, "SQ-32<Standard>: SQVM::CallNative hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SQVMCallNativeAddress);
        }
        else if (!MGS1_SQVMCallNativeScanResult)
        {
            LOG_F(INFO, "SQ-32<Standard>: SQVM::CallNative pattern scan failed.");
        }

        uint8_t* MGS1_SqratBindFuncScanResult = Memory::PatternScan(gBaseModule,
            "0F B6 44 24 20 83 C4 04 8B 4F 04 BA FD FF FF FF");
        if (MGS1_SqratBindFuncScanResult)
        {
            uintptr_t MGS1_SqratBindFuncAddress = (uintptr_t)MGS1_SqratBindFuncScanResult;
            int MGS1_SqratBindFuncHookLength = Memory::GetHookLength((char*)MGS1_SqratBindFuncAddress, 4);
            MGS1_SqratBindFuncReturnJMP = MGS1_SqratBindFuncAddress + MGS1_SqratBindFuncHookLength;
            Memory::DetourFunction((void*)MGS1_SqratBindFuncAddress, MGS1_SqratBindFunc_CC, MGS1_SqratBindFuncHookLength);

            LOG_F(INFO, "SQ-32<Standard>: Sqrat::BindFunc hook length is %d bytes.", MGS1_SqratBindFuncHookLength);
            LOG_F(INFO, "SQ-32<Standard>: Sqrat::BindFunc hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SqratBindFuncAddress);
        }
        else if (!MGS1_SqratBindFuncScanResult)
        {
            LOG_F(INFO, "SQ-32<Standard>: Sqrat::BindFunc pattern scan failed.");
        }
    }

    if (eGameType == M2FixGame::Contra || eGameType == M2FixGame::Dracula || eGameType == M2FixGame::DraculaAdvance ||
        eGameType == M2FixGame::Darius || eGameType == M2FixGame::DariusHD)
    {
        uint8_t* COCA_SQVMScanResult = Memory::PatternScan(gBaseModule,
            "C7 86 D4 00 00 00 FF FF FF FF 89 86 D0 00 00 00");
        if (COCA_SQVMScanResult)
        {
            uintptr_t COCA_SQVMAddress = (uintptr_t)COCA_SQVMScanResult;
            int COCA_SQVMHookLength = Memory::GetHookLength((char*)COCA_SQVMAddress, 4);
            COCA_SQVMReturnJMP = COCA_SQVMAddress + COCA_SQVMHookLength;
            Memory::DetourFunction((void*)COCA_SQVMAddress, COCA_SQVM_CC, COCA_SQVMHookLength);

            LOG_F(INFO, "SQ-32<AlignObject>: SQVM::SQVM hook length is %d bytes.", COCA_SQVMHookLength);
            LOG_F(INFO, "SQ-32<AlignObject>: SQVM::SQVM hook address is 0x%" PRIxPTR ".", (uintptr_t)COCA_SQVMAddress);
        }
        else if (!COCA_SQVMScanResult)
        {
            LOG_F(INFO, "SQ-32<AlignObject>: SQVM::SQVM pattern scan failed.");
        }

        uint8_t* COCA_SQVMCallNativeScanResult = Memory::PatternScan(gBaseModule,
            "FF D0 8B 4D 18 83 C4 04 FF 8E C8 00 00 00 C6 01");
        if (COCA_SQVMCallNativeScanResult)
        {
            uintptr_t COCA_SQVMCallNativeAddress = (uintptr_t)COCA_SQVMCallNativeScanResult;
            int COCA_SQVMCallNativeHookLength = Memory::GetHookLength((char*)COCA_SQVMCallNativeAddress, 4);
            COCA_SQVMCallNativeReturnJMP = COCA_SQVMCallNativeAddress + COCA_SQVMCallNativeHookLength;
            Memory::DetourFunction((void*)COCA_SQVMCallNativeAddress, COCA_SQVMCallNative_CC, COCA_SQVMCallNativeHookLength);

            LOG_F(INFO, "SQ-32<AlignObject>: SQVM::CallNative hook length is %d bytes.", COCA_SQVMCallNativeHookLength);
            LOG_F(INFO, "SQ-32<AlignObject>: SQVM::CallNative hook address is 0x%" PRIxPTR ".", (uintptr_t)COCA_SQVMCallNativeAddress);
        }
        else if (!COCA_SQVMCallNativeScanResult)
        {
            LOG_F(INFO, "SQ-32<AlignObject>: SQVM::CallNative pattern scan failed.");
        }

        uint8_t* COCA_SqratBindFuncScanResult = Memory::PatternScan(gBaseModule,
            "0F B6 44 24 20 83 C4 04 8B 4F 08 BA FD FF FF FF");
        if (COCA_SqratBindFuncScanResult)
        {
            uintptr_t COCA_SqratBindFuncAddress = (uintptr_t)COCA_SqratBindFuncScanResult;
            int COCA_SqratBindFuncHookLength = Memory::GetHookLength((char*)COCA_SqratBindFuncAddress, 4);
            COCA_SqratBindFuncReturnJMP = COCA_SqratBindFuncAddress + COCA_SqratBindFuncHookLength;
            Memory::DetourFunction((void*)COCA_SqratBindFuncAddress, COCA_SqratBindFunc_CC, COCA_SqratBindFuncHookLength);

            LOG_F(INFO, "SQ-32<AlignObject>: Sqrat::BindFunc hook length is %d bytes.", COCA_SqratBindFuncHookLength);
            LOG_F(INFO, "SQ-32<AlignObject>: Sqrat::BindFunc hook address is 0x%" PRIxPTR ".", (uintptr_t)COCA_SqratBindFuncAddress);
        }
        else if (!COCA_SqratBindFuncScanResult)
        {
            LOG_F(INFO, "SQ-32<AlignObject>: Sqrat::BindFunc pattern scan failed.");
        }
    }

    // MG | SR: Squirrel hook
    if (eGameType == M2FixGame::MGSR || eGameType == M2FixGame::Ray)
    {
        uint8_t* MGSR_SQVMScanResult = Memory::PatternScan(gBaseModule,
            "48 C7 81 FC 00 00 00 FF FF FF FF 48 89 B1 E0 00");
        if (MGSR_SQVMScanResult)
        {
            uintptr_t MGSR_SQVMAddress = (uintptr_t)MGSR_SQVMScanResult;
            int MGSR_SQVMHookLength = Memory::GetHookLength((char*)MGSR_SQVMAddress, 13) + 3;
            MGSR_SQVMReturnJMP = MGSR_SQVMAddress + MGSR_SQVMHookLength;
            Memory::DetourFunction((void*)MGSR_SQVMAddress, MGSR_SQVM_CC, MGSR_SQVMHookLength);

            LOG_F(INFO, "SQ-64<Standard>: SQVM::SQVM hook length is %d bytes.", MGSR_SQVMHookLength);
            LOG_F(INFO, "SQ-64<Standard>: SQVM::SQVM hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_SQVMAddress);
        }
        else if (!MGSR_SQVMScanResult)
        {
            LOG_F(INFO, "SQ-64<Standard>: SQVM::SQVM pattern scan failed.");
        }

        uint8_t* MGSR_SQVMCallNativeScanResult = Memory::PatternScan(gBaseModule,
            "FF 53 68 FF 8F F0 00 00 00 48 8B 8C 24 F8 00 00");
        if (MGSR_SQVMCallNativeScanResult)
        {
            uintptr_t MGSR_SQVMCallNativeAddress = (uintptr_t)MGSR_SQVMCallNativeScanResult;
            int MGSR_SQVMCallNativeHookLength = Memory::GetHookLength((char*)MGSR_SQVMCallNativeAddress, 13);
            MGSR_SQVMCallNativeReturnJMP = MGSR_SQVMCallNativeAddress + MGSR_SQVMCallNativeHookLength;
            Memory::DetourFunction((void*)MGSR_SQVMCallNativeAddress, MGSR_SQVMCallNative_CC, MGSR_SQVMCallNativeHookLength);

            LOG_F(INFO, "SQ-64<Standard>: SQVM::CallNative hook length is %d bytes.", MGSR_SQVMCallNativeHookLength);
            LOG_F(INFO, "SQ-64<Standard>: SQVM::CallNative hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_SQVMCallNativeAddress);
        }
        else if (!MGSR_SQVMCallNativeScanResult)
        {
            LOG_F(INFO, "SQ-64<Standard>: SQVM::CallNative pattern scan failed.");
        }

        uint8_t* MGSR_SqratBindFuncScanResult = Memory::PatternScan(gBaseModule,
            "44 0F B6 44 24 78 BA FD FF FF FF 49 8B 4E 08 E8");
        if (MGSR_SqratBindFuncScanResult)
        {
            uintptr_t MGSR_SqratBindFuncAddress = (uintptr_t)MGSR_SqratBindFuncScanResult;
            int MGSR_SqratBindFuncHookLength = Memory::GetHookLength((char*)MGSR_SqratBindFuncAddress, 13);
            MGSR_SqratBindFuncReturnJMP = MGSR_SqratBindFuncAddress + MGSR_SqratBindFuncHookLength;
            Memory::DetourFunction((void*)MGSR_SqratBindFuncAddress, MGSR_SqratBindFunc_CC, MGSR_SqratBindFuncHookLength);

            LOG_F(INFO, "SQ-64<Standard>: Sqrat::BindFunc hook length is %d bytes.", MGSR_SqratBindFuncHookLength);
            LOG_F(INFO, "SQ-64<Standard>: Sqrat::BindFunc hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_SqratBindFuncAddress);
        }
        else if (!MGSR_SqratBindFuncScanResult)
        {
            LOG_F(INFO, "SQ-64<Standard>: Sqrat::BindFunc pattern scan failed.");
        }
    }
}

void M2Hook()
{
    // MGS 1: M2 hook
    extern void M2Print(const char *fmt, ...);

    if (eGameType == M2FixGame::MGS1    || eGameType == M2FixGame::Contra ||
        eGameType == M2FixGame::Dracula || eGameType == M2FixGame::DraculaAdvance ||
        eGameType == M2FixGame::Darius  || eGameType == M2FixGame::DariusHD)
    {
        uint8_t* MGS1_M2PrintScanResult = Memory::PatternScan(gBaseModule,
            "8B 4C 24 04 8D 54 24 08 E8 ?? ?? FF FF 85 C0 74");
        if (MGS1_M2PrintScanResult)
        {
            uintptr_t MGS1_M2PrintAddress = (uintptr_t)MGS1_M2PrintScanResult;
            int MGS1_M2PrintHookLength = Memory::GetHookLength((char*)MGS1_M2PrintAddress, 4);
            Memory::DetourFunction((void*)MGS1_M2PrintAddress, M2Print, MGS1_M2PrintHookLength);

            LOG_F(INFO, "M2-32: printf hook length is %d bytes.", MGS1_M2PrintHookLength);
            LOG_F(INFO, "M2-32: printf hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_M2PrintAddress);
        }
        else if (!MGS1_M2PrintScanResult)
        {
            LOG_F(INFO, "M2-32: printf pattern scan failed.");
        }
    }

    // MG | SR: M2 hook
    if (eGameType == M2FixGame::MGSR || eGameType == M2FixGame::Ray)
    {
        uint8_t* MGSR_M2PrintScanResult = Memory::PatternScan(gBaseModule,
            "48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 48 83 EC 28 48 8D 54 24 38 E8 ?? ?? ?? ?? 48 85 C0 74 08 48 8B C8 E8 ?? ??");
        if (MGSR_M2PrintScanResult)
        {
            uintptr_t MGSR_M2PrintAddress = (uintptr_t)MGSR_M2PrintScanResult;
            int MGSR_M2PrintHookLength = Memory::GetHookLength((char*)MGSR_M2PrintAddress, 13);
            Memory::DetourFunction((void*)MGSR_M2PrintAddress, M2Print, MGSR_M2PrintHookLength);

            LOG_F(INFO, "M2-64: printf hook length is %d bytes.", MGSR_M2PrintHookLength);
            LOG_F(INFO, "M2-64: printf hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_M2PrintAddress);
        }
        else if (!MGSR_M2PrintScanResult)
        {
            LOG_F(INFO, "M2-64: printf pattern scan failed.");
        }
    }
}

uintptr_t MGS1_MWinResCfgGetValueReturnJMP;
extern const char* M2_GetCfgValue(string *key);
void __declspec(naked) MGS1_MWinResCfgGetValue_CC()
{
#ifndef _WIN64
    __asm
    {
        push ebp
        mov ebp, esp

        push eax
        push ecx

        mov eax, esp
        add eax, 10h

        push eax
        call M2_GetCfgValue
        add esp, 4
        cmp eax, 0

        pop ecx

        je MGS1_MWinResCfgGetValue_RET
        mov esp, ebp
        pop ebp
        retn 1Ch

    MGS1_MWinResCfgGetValue_RET:
        pop eax
        mov esp, ebp
        pop ebp

        push ebp
        mov ebp, esp
        push 0FFFFFFFFh
        jmp[MGS1_MWinResCfgGetValueReturnJMP]
    }
#endif
}

uintptr_t MGSR_MWinResCfgGetValueReturnJMP;
void __declspec(naked) MGSR_MWinResCfgGetValue_CC()
{
#ifdef _WIN64
    __asm
    {
        push rcx
        push rdx
        push r8
        push r9

        mov rcx, rdx
        call M2_GetCfgValue

        pop r9
        pop r8
        pop rdx
        pop rcx

        cmp rax, 0

        je MGSR_MWinResCfgGetValue_RET
        mov rbx, [rsp + 80h]
        add rsp, 60h
        pop rdi
        retn

    MGSR_MWinResCfgGetValue_RET:
        mov rdi, rdx
        mov rbx, rcx
        mov[rsp + 48h], rdx
        lea rcx, [rsp + 28h]
        jmp[MGSR_MWinResCfgGetValueReturnJMP]
    }
#endif
}

void ConfigHook()
{
    // MGS 1: Configuration hook
    if (eGameType == M2FixGame::MGS1    || eGameType == M2FixGame::Contra ||
        eGameType == M2FixGame::Dracula || eGameType == M2FixGame::DraculaAdvance ||
        eGameType == M2FixGame::Darius  || eGameType == M2FixGame::DariusHD)
    {
        uint8_t* MGS1_MWinResCfgGetValueScanResult = Memory::PatternScan(gBaseModule,
            "50 C6 01 00 E8 ?? ?? ?? FF 8B CF E8 ?? ?? ?? ?? 85 C0 78 1E 8B 57 04 8D 34 C0 8B 45 20 8D 0C 40");
        if (MGS1_MWinResCfgGetValueScanResult)
        {
            uintptr_t MGS1_MWinResCfgGetValueAddress = (uintptr_t)(MGS1_MWinResCfgGetValueScanResult - 0x48);
            int MGS1_MWinResCfgGetValueHookLength = Memory::GetHookLength((char*)MGS1_MWinResCfgGetValueAddress, 4);
            MGS1_MWinResCfgGetValueReturnJMP = MGS1_MWinResCfgGetValueAddress + MGS1_MWinResCfgGetValueHookLength;
            Memory::DetourFunction((void*)MGS1_MWinResCfgGetValueAddress, MGS1_MWinResCfgGetValue_CC, MGS1_MWinResCfgGetValueHookLength);

            LOG_F(INFO, "M2-32: MWinResCfg::GetValue hook length is %d bytes.", MGS1_MWinResCfgGetValueHookLength);
            LOG_F(INFO, "M2-32: MWinResCfg::GetValue hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_MWinResCfgGetValueAddress);
        }
        else if (!MGS1_MWinResCfgGetValueScanResult)
        {
            LOG_F(INFO, "M2-32: MWinResCfg::GetValue pattern scan failed.");
        }
    }

    // MG | SR: Configuration hook
    if (eGameType == M2FixGame::MGSR || eGameType == M2FixGame::Ray)
    {
        uint8_t* MGSR_MWinResCfgGetValueScanResult = Memory::PatternScan(gBaseModule,
            "48 33 C4 48 89 44 24 50 48 8B FA 48 8B D9 48 89 54 24 48 48 8D 4C 24 28 E8 ?? ?? ?? ?? 48");
        if (MGSR_MWinResCfgGetValueScanResult)
        {
            uintptr_t MGSR_MWinResCfgGetValueAddress = (uintptr_t)(MGSR_MWinResCfgGetValueScanResult + 8);
            int MGSR_MWinResCfgGetValueHookLength = Memory::GetHookLength((char*)MGSR_MWinResCfgGetValueAddress, 13);
            MGSR_MWinResCfgGetValueReturnJMP = MGSR_MWinResCfgGetValueAddress + MGSR_MWinResCfgGetValueHookLength;
            Memory::DetourFunction((void*)MGSR_MWinResCfgGetValueAddress, MGSR_MWinResCfgGetValue_CC, MGSR_MWinResCfgGetValueHookLength);

            LOG_F(INFO, "M2-64: MWinResCfg::GetValue hook length is %d bytes.", MGSR_MWinResCfgGetValueHookLength);
            LOG_F(INFO, "M2-64: MWinResCfg::GetValue hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_MWinResCfgGetValueAddress);
        }
        else if (!MGSR_MWinResCfgGetValueScanResult)
        {
            LOG_F(INFO, "M2-64: MWinResCfg::GetValue pattern scan failed.");
        }
    }
}

void BorderlessPatch()
{
    // MGS 1: Borderless patch
    if (eGameType == M2FixGame::MGS1 || eGameType == M2FixGame::Darius || eGameType == M2FixGame::DariusHD)
    {
        uint8_t* MGS1_MWinResCfgSetWindowScanResult = Memory::PatternScan(gBaseModule,
            "B8 00 00 CE 02 BE 00 00 CA 02");
        if (MGS1_MWinResCfgSetWindowScanResult)
        {
            uint8_t* MGS1_MWinResCfgSetWindowPTR = (uint8_t*)MGS1_MWinResCfgSetWindowScanResult;
            uint8_t MGS1_MWinResCfgSetWindowFlags[] = { "\xB8\x00\x00\x00\x90\xBE\x00\x00\x00\x90" };
            Memory::PatchBytes((uintptr_t)MGS1_MWinResCfgSetWindowPTR, (const char*)MGS1_MWinResCfgSetWindowFlags, sizeof(MGS1_MWinResCfgSetWindowFlags) - 1);
            LOG_F(INFO, "M2-32<Standard>: Borderless: MWinResCfg::SetWindow patched.");
        }
        else if (!MGS1_MWinResCfgSetWindowScanResult)
        {
            LOG_F(INFO, "M2-32<Standard>: Borderless: MWinResCfg::SetWindow pattern scan failed.");
        }
    }

    if (eGameType == M2FixGame::Contra || eGameType == M2FixGame::Dracula || eGameType == M2FixGame::DraculaAdvance)
    {
        uint8_t* COCA_MWinResCfgSetWindowScanResult = Memory::PatternScan(gBaseModule,
            "B8 00 00 0B 02 C7 85 70 FF FF FF 00 00 CB 00 74 0F B8 00 00 0F 02 C7 85 70 FF FF FF 00 00 CF 00");
        if (COCA_MWinResCfgSetWindowScanResult)
        {
            uint8_t* COCA_MWinResCfgSetWindowPTR = (uint8_t*)COCA_MWinResCfgSetWindowScanResult;
            uint8_t COCA_MWinResCfgSetWindowFlags[] = { "\xB8\x00\x00\x00\x90\xC7\x85\x70\xFF\xFF\xFF\x00\x00\xCB\x00\x74\x0F\xB8\x00\x00\x00\x90\xC7\x85\x70\xFF\xFF\xFF\x00\x00\xCF\x00" };
            Memory::PatchBytes((uintptr_t)COCA_MWinResCfgSetWindowPTR, (const char*)COCA_MWinResCfgSetWindowFlags, sizeof(COCA_MWinResCfgSetWindowFlags) - 1);
            LOG_F(INFO, "M2-32<Legacy>: Borderless: MWinResCfg::SetWindow patched.");
        }
        else if (!COCA_MWinResCfgSetWindowScanResult)
        {
            LOG_F(INFO, "M2-32<Legacy>: Borderless: MWinResCfg::SetWindow pattern scan failed.");
        }
    }

    // MG | SR: Borderless patch
    if (eGameType == M2FixGame::MGSR || eGameType == M2FixGame::Ray)
    {
        uint8_t* MGSR_MWinResCfgSetWindowScanResult = Memory::PatternScan(gBaseModule,
            "BE 00 00 CB 02 41 BE 00 00 CB 00 44 ?? ?? ?? ?? ?? ?? 74 0B BE 00 00 CF 02 41 BE 00 00 CF 00");
        if (MGSR_MWinResCfgSetWindowScanResult)
        {
            uint8_t* MGSR_MWinResCfgSetWindowPTR = (uint8_t*)MGSR_MWinResCfgSetWindowScanResult;
            uint8_t MGSR_MWinResCfgSetWindowFlags[] = { "\xBE\x00\x00\x00\x90\x41\xBE\x00\x00\x00\x00\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90" };
            Memory::PatchBytes((uintptr_t)MGSR_MWinResCfgSetWindowPTR, (const char*)MGSR_MWinResCfgSetWindowFlags, sizeof(MGSR_MWinResCfgSetWindowFlags) - 1);
            LOG_F(INFO, "M2-64<Standard>: Borderless: MWinResCfg::SetWindow patched.");
        }
        else if (!MGSR_MWinResCfgSetWindowScanResult)
        {
            LOG_F(INFO, "M2-64<Standard>: Borderless: MWinResCfg::SetWindow pattern scan failed.");
        }
    }
}

void AnalogPatch()
{
    // MGS 1: Analog patch
    if (eGameType == M2FixGame::MGS1)
    {
        uint8_t* MGS1_MInputHubDMGetWinScanResult = Memory::PatternScan(gBaseModule,
            "66 89 4F 0C F3 0F 2C C0 0F B7 C0 66 89 47 0E 75");
        if (MGS1_MInputHubDMGetWinScanResult)
        {
            uint8_t* MGS1_MInputHubDMGetWinPTR = (uint8_t*)MGS1_MInputHubDMGetWinScanResult;
            uint8_t MGS1_MInputHubDMGetWin[] = { "\x66\x89\x4F\x08\xF3\x0F\x2C\xC0\x0F\xB7\xC0\x66\x89\x47\x0A\x75" };
            Memory::PatchBytes((uintptr_t)MGS1_MInputHubDMGetWinPTR, (const char*)MGS1_MInputHubDMGetWin, sizeof(MGS1_MInputHubDMGetWin) - 1);
            LOG_F(INFO, "MGS 1: Analog: MInputHubDM::GetWin patched.");
        }
        else if (!MGS1_MInputHubDMGetWinScanResult)
        {
            LOG_F(INFO, "MGS 1: Analog: MInputHubDM::GetWin pattern scan failed.");
        }

        uint8_t* MGS1_SIOUpdatePadScanResult = Memory::PatternScan(gBaseModule,
            "88 4C 10 44 83 FB 06 7C 86 8B 44 24 18 45 89 6C");
        if (MGS1_SIOUpdatePadScanResult)
        {
            uint8_t* MGS1_SIOUpdatePadPTR = (uint8_t*)MGS1_SIOUpdatePadScanResult;
            uint8_t MGS1_SIOUpdatePad[] = { "\x90\x90\x90\x90" };
            Memory::PatchBytes((uintptr_t)MGS1_SIOUpdatePadPTR, (const char*)MGS1_SIOUpdatePad, sizeof(MGS1_SIOUpdatePad) - 1);
            LOG_F(INFO, "MGS 1: Analog: sio_update_pad patched.");
        }
        else if (!MGS1_SIOUpdatePadScanResult)
        {
            LOG_F(INFO, "MGS 1: Analog: sio_update_pad pattern scan failed.");
        }
    }
}

uintptr_t MGS1_SIOReadPadReturnJMP;
extern void M2_ReadPad(unsigned int addr, unsigned int id, unsigned char* state);
void __declspec(naked) MGS1_SIOReadPad_CC()
{
#ifndef _WIN64
    __asm
    {
        push ecx
        push edx
        push[esp + 8]

        call M2_ReadPad

        add esp, 4
        pop edx
        pop ecx

        mov[esp + 8], 0xF3
        mov[esp + 9], 0x5A
        jmp[MGS1_SIOReadPadReturnJMP]
    }
#endif
}

void AnalogHook()
{
    // MGS 1: Analog hook
    if (eGameType == M2FixGame::MGS1)
    {
        uint8_t* MGS1_SIOReadPadScanResult = Memory::PatternScan(gBaseModule,
            "C7 44 24 08 F3 5A 00 00 C7 44 24 0C 00 00 00 00");
        if (MGS1_SIOReadPadScanResult)
        {
            uintptr_t MGS1_SIOReadPadAddress = (uintptr_t)MGS1_SIOReadPadScanResult;
            int MGS1_SIOReadPadHookLength = Memory::GetHookLength((char*)MGS1_SIOReadPadAddress, 4);
            MGS1_SIOReadPadReturnJMP = MGS1_SIOReadPadAddress + MGS1_SIOReadPadHookLength;
            Memory::DetourFunction((void*)MGS1_SIOReadPadAddress, MGS1_SIOReadPad_CC, MGS1_SIOReadPadHookLength);

            LOG_F(INFO, "MGS 1: Analog: sio_read_pad hook length is %d bytes.", MGS1_SIOReadPadHookLength);
            LOG_F(INFO, "MGS 1: Analog: sio_read_pad hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SIOReadPadAddress);
        }
        else if (!MGS1_SIOReadPadScanResult)
        {
            LOG_F(INFO, "MGS 1: Analog: sio_read_pad pattern scan failed.");
        }
    }
}

extern void *M2LoadImage(void *dst, void *src, size_t num);

uintptr_t MGS1_PSXLoadImageReturnJMP;
void __declspec(naked) MGS1_PSXLoadImage_CC()
{
#ifndef _WIN64
    __asm
    {
        push dword ptr[ebx]
        call M2LoadImage
        jmp[MGS1_PSXLoadImageReturnJMP]
    }
#endif
}

uintptr_t MGS1_PSXReloadImageReturnJMP;
void __declspec(naked) MGS1_PSXReloadImage_CC()
{
#ifndef _WIN64
    __asm
    {
        push dword ptr[esi + 14h]
        call M2LoadImage
        jmp[MGS1_PSXReloadImageReturnJMP]
    }
#endif
}

extern bool M2MachineCommand(unsigned int *args);

uintptr_t MGS1_PSXMachineCommandReturnJMP;
void __declspec(naked) MGS1_PSXMachineCommand_CC()
{
#ifndef _WIN64
    __asm
    {
        mov eax, esp

        push ebp
        mov ebp, esp

        push eax
        call M2MachineCommand

        mov esp, ebp
        pop ebp

        cmp eax, 0
        je MGS1_PSXMachineCommand_RET

        retn

    MGS1_PSXMachineCommand_RET:
        mov eax, [esp + 4]
        lea edx, [esp + 12]
        jmp[MGS1_PSXMachineCommandReturnJMP]
    }
#endif
}

extern void *M2LoadModule(void *module, void *);

uintptr_t MGS1_M2EPILoadModuleReturnJMP;
void __declspec(naked) MGS1_M2EPILoadModule_CC()
{
#ifndef _WIN64
    __asm
    {
        push edx
        push ebp
        mov ebp, esp

        push edx
        push ecx

        call M2LoadModule
        mov ecx, eax

        mov esp, ebp
        pop ebp
        pop edx

        sub esp, 8
        push ebx
        push ebp
        jmp[MGS1_M2EPILoadModuleReturnJMP]
    }
#endif
}

extern void M2ListInsert(void *list, void *object);

uintptr_t MGS1_M2EPIListInsertReturnJMP;
void __declspec(naked) MGS1_M2EPIListInsert_CC()
{
#ifndef _WIN64
    __asm
    {
        push edx
        push ecx

        push ebp
        mov ebp, esp

        push edx
        push ecx
        call M2ListInsert

        mov esp, ebp
        pop ebp

        pop ecx
        pop edx

        sub esp, 12
        mov[esp + 4], ecx
        jmp[MGS1_M2EPIListInsertReturnJMP]
    }
#endif
}

void EmuHook()
{
    if (eGameType == M2FixGame::MGS1 || eGameType == M2FixGame::DariusHD)
    {
        uint8_t* MGS1_PSXLoadImageScanResult = Memory::PatternScan(gBaseModule,
            "51 FF 70 04 FF 33 E8 ?? ?? ?? ?? 83 C4 0C 6A 06");
        if (MGS1_PSXLoadImageScanResult)
        {
            uintptr_t MGS1_PSXLoadImageAddress = (uintptr_t)(MGS1_PSXLoadImageScanResult + 4);
            int MGS1_PSXLoadImageHookLength = Memory::GetHookLength((char*)MGS1_PSXLoadImageAddress, 4);
            MGS1_PSXLoadImageReturnJMP = MGS1_PSXLoadImageAddress + MGS1_PSXLoadImageHookLength;
            Memory::DetourFunction((void*)MGS1_PSXLoadImageAddress, MGS1_PSXLoadImage_CC, MGS1_PSXLoadImageHookLength);

            LOG_F(INFO, "PSX: psx_load_image hook length is %d bytes.", MGS1_PSXLoadImageHookLength);
            LOG_F(INFO, "PSX: psx_load_image hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_PSXLoadImageAddress);
        }
        else if (!MGS1_PSXLoadImageScanResult)
        {
            LOG_F(INFO, "PSX: psx_load_image pattern scan failed.");
        }

        uint8_t* MGS1_PSXReloadImageScanResult = Memory::PatternScan(gBaseModule,
            "74 1B FF 70 08 FF 70 04 FF 76 14 E8 ?? ?? ?? ??");
        if (MGS1_PSXReloadImageScanResult)
        {
            uintptr_t MGS1_PSXReloadImageAddress = (uintptr_t)(MGS1_PSXReloadImageScanResult + 8);
            int MGS1_PSXReloadImageHookLength = Memory::GetHookLength((char*)MGS1_PSXReloadImageAddress, 4);
            MGS1_PSXReloadImageReturnJMP = MGS1_PSXReloadImageAddress + MGS1_PSXReloadImageHookLength;
            Memory::DetourFunction((void*)MGS1_PSXReloadImageAddress, MGS1_PSXReloadImage_CC, MGS1_PSXReloadImageHookLength);

            LOG_F(INFO, "PSX: psx_reload_image hook length is %d bytes.", MGS1_PSXReloadImageHookLength);
            LOG_F(INFO, "PSX: psx_reload_image hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_PSXReloadImageAddress);
        }
        else if (!MGS1_PSXReloadImageScanResult)
        {
            LOG_F(INFO, "PSX: psx_reload_image pattern scan failed.");
        }

        uint8_t* MGS1_PSXMachineCommandResult = Memory::PatternScan(gBaseModule,
            "8B 44 24 04 8D 54 24 0C 52 FF 74 24 0C 8B 08 50 8B 41 0C FF D0 83 C4 0C C3");
        if (MGS1_PSXMachineCommandResult)
        {
            uintptr_t MGS1_PSXMachineCommandAddress = (uintptr_t)MGS1_PSXMachineCommandResult;
            int MGS1_PSXMachineCommandHookLength = Memory::GetHookLength((char*)MGS1_PSXMachineCommandAddress, 4);
            MGS1_PSXMachineCommandReturnJMP = MGS1_PSXMachineCommandAddress + MGS1_PSXMachineCommandHookLength;
            Memory::DetourFunction((void*)MGS1_PSXMachineCommandAddress, MGS1_PSXMachineCommand_CC, MGS1_PSXMachineCommandHookLength);

            LOG_F(INFO, "PSX: psx_machine_cmd hook length is %d bytes.", MGS1_PSXMachineCommandHookLength);
            LOG_F(INFO, "PSX: psx_machine_cmd hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_PSXMachineCommandAddress);
        }
        else if (!MGS1_PSXMachineCommandResult)
        {
            LOG_F(INFO, "PSX: psx_machine_cmd pattern scan failed.");
        }

        uint8_t* MGS1_M2EPILoadModuleScanResult = Memory::PatternScan(gBaseModule,
            "83 EC 08 53 55 56 8B 35 ?? ?? ?? ?? 8B DA 8B E9");
        if (MGS1_M2EPILoadModuleScanResult)
        {
            uintptr_t MGS1_M2EPILoadModuleAddress = (uintptr_t)MGS1_M2EPILoadModuleScanResult;
            int MGS1_M2EPILoadModuleHookLength = Memory::GetHookLength((char*)MGS1_M2EPILoadModuleAddress, 4);
            MGS1_M2EPILoadModuleReturnJMP = MGS1_M2EPILoadModuleAddress + MGS1_M2EPILoadModuleHookLength;
            Memory::DetourFunction((void*)MGS1_M2EPILoadModuleAddress, MGS1_M2EPILoadModule_CC, MGS1_M2EPILoadModuleHookLength);

            LOG_F(INFO, "Emulator: m2epi_load_module hook length is %d bytes.", MGS1_M2EPILoadModuleHookLength);
            LOG_F(INFO, "Emulator: m2epi_load_module hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_M2EPILoadModuleAddress);
        }
        else if (!MGS1_M2EPILoadModuleScanResult)
        {
            LOG_F(INFO, "Emulator: m2epi_load_module pattern scan failed.");
        }

        uint8_t* MGS1_M2EPIListInsertResult = Memory::PatternScan(gBaseModule,
            "83 EC 0C 89 4C 24 04 56 8D 74 24 04 57 8B FA 85");
        if (MGS1_M2EPIListInsertResult)
        {
            uintptr_t MGS1_M2EPIListInsertAddress = (uintptr_t)MGS1_M2EPIListInsertResult;
            int MGS1_M2EPIListInsertHookLength = Memory::GetHookLength((char*)MGS1_M2EPIListInsertAddress, 4);
            MGS1_M2EPIListInsertReturnJMP = MGS1_M2EPIListInsertAddress + MGS1_M2EPIListInsertHookLength;
            Memory::DetourFunction((void*)MGS1_M2EPIListInsertAddress, MGS1_M2EPIListInsert_CC, MGS1_M2EPIListInsertHookLength);

            LOG_F(INFO, "Emulator: m2epi_list_insert hook length is %d bytes.", MGS1_M2EPIListInsertHookLength);
            LOG_F(INFO, "Emulator: m2epi_list_insert hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_M2EPIListInsertAddress);
        }
        else if (!MGS1_M2EPIListInsertResult)
        {
            LOG_F(INFO, "Emulator: m2epi_list_insert pattern scan failed.");
        }
    }
}

uintptr_t memsetReturnJMP;
extern void memsetWait();
void __declspec(naked) memset_CC()
{
#ifndef _WIN64
    __asm
    {
        call memsetWait
        mov ecx, [esp + 12]
        movzx eax, byte ptr[esp + 8]
        jmp[memsetReturnJMP]
    }
#else
    __asm
    {
        push rcx
        push rdx
        push r8
        push r9

        push rbp
        mov rbp, rsp

        call memsetWait

        mov rsp, rbp
        pop rbp

        pop r9
        pop r8
        pop rdx
        pop rcx

        mov r11, rcx
        movzx edx, dl
        mov r9, 101010101010101h
        jmp[memsetReturnJMP]
    }
#endif
}

void memsetHook()
{
#ifndef _WIN64
    uint8_t* memsetResult = Memory::PatternScan(gBaseModule,
        "8B 4C 24 0C 0F B6 44 24 08 8B D7 8B 7C 24 04 85");
    int memsetMinHookLength = 4;
#else
    uint8_t* memsetResult = Memory::PatternScan(gBaseModule,
        "4C 8B D9 0F B6 D2 49 B9 01 01 01 01 01 01 01 01");
    int memsetMinHookLength = 13;
#endif
    if (memsetResult)
    {
        uintptr_t memsetAddress = (uintptr_t)memsetResult;
        int memsetHookLength = Memory::GetHookLength((char*)memsetAddress, memsetMinHookLength);
        memsetReturnJMP = memsetAddress + memsetHookLength;
        Memory::DetourFunction((void*)memsetAddress, memset_CC, memsetHookLength);
    }
}
