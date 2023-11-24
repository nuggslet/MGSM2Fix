#include "stdafx.h"

#include "helper.hpp"

using namespace std;

extern HMODULE baseModule;

extern string sExeName;

void ScanFunctions()
{
    extern uintptr_t M2_mallocAddress;
    extern uintptr_t M2_reallocAddress;
    extern uintptr_t M2_freeAddress;

    // MGS 1: Squirrel call
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* M2_mallocScanResult = Memory::PatternScan(baseModule, "8B FF 55 8B EC 56 8B 75 08 83 FE E0 77 30 85 F6");
        if (M2_mallocScanResult)
        {
            M2_mallocAddress = (uintptr_t)M2_mallocScanResult;
            LOG_F(INFO, "M2: malloc is 0x%" PRIxPTR ".", M2_mallocAddress);
        }
        else if (!M2_mallocScanResult)
        {
            LOG_F(INFO, "M2: malloc scan failed.");
        }

        uint8_t* M2_reallocScanResult = Memory::PatternScan(baseModule, "8B FF 55 8B EC 57 8B 7D 08 85 FF 75 0B FF 75 0C");
        if (M2_reallocScanResult)
        {
            M2_reallocAddress = (uintptr_t)M2_reallocScanResult;
            LOG_F(INFO, "M2: realloc is 0x%" PRIxPTR ".", M2_reallocAddress);
        }
        else if (!M2_reallocScanResult)
        {
            LOG_F(INFO, "M2: realloc scan failed.");
        }

        uint8_t* M2_freeScanResult = Memory::PatternScan(baseModule, "8B FF 55 8B EC 83 7D 08 00 74 2D FF 75 08 6A 00");
        if (M2_freeScanResult)
        {
            M2_freeAddress = (uintptr_t)M2_freeScanResult;
            LOG_F(INFO, "M2: free is 0x%" PRIxPTR ".", M2_freeAddress);
        }
        else if (!M2_freeScanResult)
        {
            LOG_F(INFO, "M2: free scan failed.");
        }
    }

    // MG | SR: Squirrel call
    if (sExeName == "MGS MC1 Bonus Content.exe")
    {
        uint8_t* M2_mallocScanResult = Memory::PatternScan(baseModule, "40 53 48 83 EC 20 48 8B D9 48 83 F9 E0 77 3C 48");
        if (M2_mallocScanResult)
        {
            M2_mallocAddress = (uintptr_t)M2_mallocScanResult;
            LOG_F(INFO, "M2: malloc is 0x%" PRIxPTR ".", M2_mallocAddress);
        }
        else if (!M2_mallocScanResult)
        {
            LOG_F(INFO, "M2: malloc scan failed.");
        }

        uint8_t* M2_reallocScanResult = Memory::PatternScan(baseModule, "48 89 5C 24 08 57 48 83 EC 20 48 8B DA 48 8B F9 48 85 C9 75 0A 48 8B CA E8 ?? ?? ?? ?? EB 1F 48");
        if (M2_reallocScanResult)
        {
            M2_reallocAddress = (uintptr_t)M2_reallocScanResult;
            LOG_F(INFO, "M2: realloc is 0x%" PRIxPTR ".", M2_reallocAddress);
        }
        else if (!M2_reallocScanResult)
        {
            LOG_F(INFO, "M2: realloc scan failed.");
        }

        uint8_t* M2_freeScanResult = Memory::PatternScan(baseModule, "48 85 C9 74 37 53 48 83 EC 20 4C 8B C1 33 D2 48");
        if (M2_freeScanResult)
        {
            M2_freeAddress = (uintptr_t)M2_freeScanResult;
            LOG_F(INFO, "M2: free is 0x%" PRIxPTR ".", M2_freeAddress);
        }
        else if (!M2_freeScanResult)
        {
            LOG_F(INFO, "M2: free scan failed.");
        }
    }
}

// MGS 1: Squirrel hook
uintptr_t MGS1_SQVMReturnJMP;
extern void SquirrelNew(HSQUIRRELVM v);
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
        call SquirrelNew

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

        call SquirrelNew

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

SQInteger HookNative(SQFUNCTION func, HSQUIRRELVM v);
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
        call HookNative

        mov esp, ebp
        pop ebp

        mov ecx, [ebp + 18h]
        jmp[MGS1_SQVMCallNativeReturnJMP]
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
        call HookNative

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

uintptr_t MGS1_SqratBindFuncReturnJMP;
void __declspec(naked) MGS1_SqratBindFunc_CC()
{
#ifndef _WIN64
    __asm
    {
        push[esp + 10h]
        push - 1
        push[edi + 4]
        call sq_setnativeclosurename
        add esp, 12

        movzx eax, [esp + 20h]
        jmp[MGS1_SqratBindFuncReturnJMP]
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

        call sq_setnativeclosurename

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
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_SQVMScanResult = Memory::PatternScan(baseModule, "C7 86 A4 00 00 00 FF FF FF FF 89 86 A0 00 00 00");
        if (MGS1_SQVMScanResult)
        {
            uintptr_t MGS1_SQVMAddress = (uintptr_t)MGS1_SQVMScanResult;
            int MGS1_SQVMHookLength = Memory::GetHookLength((char*)MGS1_SQVMAddress, 4);
            MGS1_SQVMReturnJMP = MGS1_SQVMAddress + MGS1_SQVMHookLength;
            Memory::DetourFunction((void*)MGS1_SQVMAddress, MGS1_SQVM_CC, MGS1_SQVMHookLength);

            LOG_F(INFO, "MGS 1: M2: SQVM::SQVM hook length is %d bytes.", MGS1_SQVMHookLength);
            LOG_F(INFO, "MGS 1: M2: SQVM::SQVM hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SQVMAddress);
        }
        else if (!MGS1_SQVMScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: SQVM::SQVM pattern scan failed.");
        }

        uint8_t* MGS1_SQVMCallNativeScanResult = Memory::PatternScan(baseModule, "FF D0 8B 4D 18 83 C4 04 FF 8E 98 00 00 00 C6 01");
        if (MGS1_SQVMCallNativeScanResult)
        {
            uintptr_t MGS1_SQVMCallNativeAddress = (uintptr_t)MGS1_SQVMCallNativeScanResult;
            int MGS1_SQVMCallNativeHookLength = Memory::GetHookLength((char*)MGS1_SQVMCallNativeAddress, 4);
            MGS1_SQVMCallNativeReturnJMP = MGS1_SQVMCallNativeAddress + MGS1_SQVMCallNativeHookLength;
            Memory::DetourFunction((void*)MGS1_SQVMCallNativeAddress, MGS1_SQVMCallNative_CC, MGS1_SQVMCallNativeHookLength);

            LOG_F(INFO, "MGS 1: M2: SQVM::CallNative hook length is %d bytes.", MGS1_SQVMCallNativeHookLength);
            LOG_F(INFO, "MGS 1: M2: SQVM::CallNative hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SQVMCallNativeAddress);
        }
        else if (!MGS1_SQVMCallNativeScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: SQVM::CallNative pattern scan failed.");
        }

        uint8_t* MGS1_SqratBindFuncScanResult = Memory::PatternScan(baseModule, "0F B6 44 24 20 83 C4 04 8B 4F 04 BA FD FF FF FF");
        if (MGS1_SqratBindFuncScanResult)
        {
            uintptr_t MGS1_SqratBindFuncAddress = (uintptr_t)MGS1_SqratBindFuncScanResult;
            int MGS1_SqratBindFuncHookLength = Memory::GetHookLength((char*)MGS1_SqratBindFuncAddress, 4);
            MGS1_SqratBindFuncReturnJMP = MGS1_SqratBindFuncAddress + MGS1_SqratBindFuncHookLength;
            Memory::DetourFunction((void*)MGS1_SqratBindFuncAddress, MGS1_SqratBindFunc_CC, MGS1_SqratBindFuncHookLength);

            LOG_F(INFO, "MGS 1: M2: Sqrat::BindFunc hook length is %d bytes.", MGS1_SqratBindFuncHookLength);
            LOG_F(INFO, "MGS 1: M2: Sqrat::BindFunc hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_SqratBindFuncAddress);
        }
        else if (!MGS1_SqratBindFuncScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: Sqrat::BindFunc pattern scan failed.");
        }
    }

    // MG | SR: Squirrel hook
    if (sExeName == "MGS MC1 Bonus Content.exe")
    {
        uint8_t* MGSR_SQVMScanResult = Memory::PatternScan(baseModule, "48 C7 81 FC 00 00 00 FF FF FF FF 48 89 B1 E0 00");
        if (MGSR_SQVMScanResult)
        {
            uintptr_t MGSR_SQVMAddress = (uintptr_t)MGSR_SQVMScanResult;
            int MGSR_SQVMHookLength = Memory::GetHookLength((char*)MGSR_SQVMAddress, 13) + 3;
            MGSR_SQVMReturnJMP = MGSR_SQVMAddress + MGSR_SQVMHookLength;
            Memory::DetourFunction((void*)MGSR_SQVMAddress, MGSR_SQVM_CC, MGSR_SQVMHookLength);

            LOG_F(INFO, "MG | SR: M2: SQVM::SQVM hook length is %d bytes.", MGSR_SQVMHookLength);
            LOG_F(INFO, "MG | SR: M2: SQVM::SQVM hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_SQVMAddress);
        }
        else if (!MGSR_SQVMScanResult)
        {
            LOG_F(INFO, "MG | SR: M2: SQVM::SQVM pattern scan failed.");
        }

        uint8_t* MGSR_SQVMCallNativeScanResult = Memory::PatternScan(baseModule, "FF 53 68 FF 8F F0 00 00 00 48 8B 8C 24 F8 00 00");
        if (MGSR_SQVMCallNativeScanResult)
        {
            uintptr_t MGSR_SQVMCallNativeAddress = (uintptr_t)MGSR_SQVMCallNativeScanResult;
            int MGSR_SQVMCallNativeHookLength = Memory::GetHookLength((char*)MGSR_SQVMCallNativeAddress, 13);
            MGSR_SQVMCallNativeReturnJMP = MGSR_SQVMCallNativeAddress + MGSR_SQVMCallNativeHookLength;
            Memory::DetourFunction((void*)MGSR_SQVMCallNativeAddress, MGSR_SQVMCallNative_CC, MGSR_SQVMCallNativeHookLength);

            LOG_F(INFO, "MG | SR: M2: SQVM::CallNative hook length is %d bytes.", MGSR_SQVMCallNativeHookLength);
            LOG_F(INFO, "MG | SR: M2: SQVM::CallNative hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_SQVMCallNativeAddress);
        }
        else if (!MGSR_SQVMCallNativeScanResult)
        {
            LOG_F(INFO, "MG | SR: M2: SQVM::CallNative pattern scan failed.");
        }

        uint8_t* MGSR_SqratBindFuncScanResult = Memory::PatternScan(baseModule, "44 0F B6 44 24 78 BA FD FF FF FF 49 8B 4E 08 E8");
        if (MGSR_SqratBindFuncScanResult)
        {
            uintptr_t MGSR_SqratBindFuncAddress = (uintptr_t)MGSR_SqratBindFuncScanResult;
            int MGSR_SqratBindFuncHookLength = Memory::GetHookLength((char*)MGSR_SqratBindFuncAddress, 13);
            MGSR_SqratBindFuncReturnJMP = MGSR_SqratBindFuncAddress + MGSR_SqratBindFuncHookLength;
            Memory::DetourFunction((void*)MGSR_SqratBindFuncAddress, MGSR_SqratBindFunc_CC, MGSR_SqratBindFuncHookLength);

            LOG_F(INFO, "MG | SR: M2: Sqrat::BindFunc hook length is %d bytes.", MGSR_SqratBindFuncHookLength);
            LOG_F(INFO, "MG | SR: M2: Sqrat::BindFunc hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_SqratBindFuncAddress);
        }
        else if (!MGSR_SqratBindFuncScanResult)
        {
            LOG_F(INFO, "MG | SR: M2: Sqrat::BindFunc pattern scan failed.");
        }
    }
}

void M2Hook()
{
    // MGS 1: M2 hook
    extern void M2Print(const char *fmt, ...);

    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_M2PrintScanResult = Memory::PatternScan(baseModule, "8B 4C 24 04 8D 54 24 08 E8 ?? ?? FF FF 85 C0 74");
        if (MGS1_M2PrintScanResult)
        {
            uintptr_t MGS1_M2PrintAddress = (uintptr_t)MGS1_M2PrintScanResult;
            int MGS1_M2PrintHookLength = Memory::GetHookLength((char*)MGS1_M2PrintAddress, 4);
            Memory::DetourFunction((void*)MGS1_M2PrintAddress, M2Print, MGS1_M2PrintHookLength);

            LOG_F(INFO, "MGS 1: M2: printf hook length is %d bytes.", MGS1_M2PrintHookLength);
            LOG_F(INFO, "MGS 1: M2: printf hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_M2PrintAddress);
        }
        else if (!MGS1_M2PrintScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: printf pattern scan failed.");
        }
    }

    // MG | SR: M2 hook
    if (sExeName == "MGS MC1 Bonus Content.exe")
    {
        uint8_t* MGSR_M2PrintScanResult = Memory::PatternScan(baseModule, "48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 48 83 EC 28 48 8D 54 24 38 E8 ?? ?? ?? ?? 48 85 C0 74 08 48 8B C8 E8 ?? ??");
        if (MGSR_M2PrintScanResult)
        {
            uintptr_t MGSR_M2PrintAddress = (uintptr_t)MGSR_M2PrintScanResult;
            int MGSR_M2PrintHookLength = Memory::GetHookLength((char*)MGSR_M2PrintAddress, 13);
            Memory::DetourFunction((void*)MGSR_M2PrintAddress, M2Print, MGSR_M2PrintHookLength);

            LOG_F(INFO, "MG | SR: M2: printf hook length is %d bytes.", MGSR_M2PrintHookLength);
            LOG_F(INFO, "MG | SR: M2: printf hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_M2PrintAddress);
        }
        else if (!MGSR_M2PrintScanResult)
        {
            LOG_F(INFO, "MG | SR: M2: printf pattern scan failed.");
        }
    }
}

uintptr_t MGS1_MWinResCfgGetReturnJMP;
extern const char* ConfigOverride(string *key);
void __declspec(naked) MGS1_MWinResCfgGet_CC()
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
        call ConfigOverride
        add esp, 4
        cmp eax, 0

        pop ecx

        je MGS1_MWinResCfgGet_RET
        mov esp, ebp
        pop ebp
        retn 1Ch

        MGS1_MWinResCfgGet_RET :
        pop eax
            mov esp, ebp
            pop ebp

            push ebp
            mov ebp, esp
            push 0FFFFFFFFh
            jmp[MGS1_MWinResCfgGetReturnJMP]
    }
#endif
}

uintptr_t MGSR_MWinResCfgGetReturnJMP;
void __declspec(naked) MGSR_MWinResCfgGet_CC()
{
#ifdef _WIN64
    __asm
    {
        push rcx
        push rdx
        push r8
        push r9

        mov rcx, rdx
        call ConfigOverride

        pop r9
        pop r8
        pop rdx
        pop rcx

        cmp rax, 0

        je MGSR_MWinResCfgGet_RET
        mov rbx, [rsp + 80h]
        add rsp, 60h
        pop rdi
        retn

        MGSR_MWinResCfgGet_RET :
        mov rdi, rdx
            mov rbx, rcx
            mov[rsp + 48h], rdx
            lea rcx, [rsp + 28h]
            jmp[MGSR_MWinResCfgGetReturnJMP]
    }
#endif
}

void ConfigHook()
{
    // MGS 1: Configuration hook
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_MWinResCfgGetScanResult = Memory::PatternScan(baseModule, "50 C6 01 00 E8 ?? ?? ?? FF 8B CF E8 78 0B 00 00");
        if (MGS1_MWinResCfgGetScanResult)
        {
            uintptr_t MGS1_MWinResCfgGetAddress = (uintptr_t)(MGS1_MWinResCfgGetScanResult - 0x48);
            int MGS1_MWinResCfgGetHookLength = Memory::GetHookLength((char*)MGS1_MWinResCfgGetAddress, 4);
            MGS1_MWinResCfgGetReturnJMP = MGS1_MWinResCfgGetAddress + MGS1_MWinResCfgGetHookLength;
            Memory::DetourFunction((void*)MGS1_MWinResCfgGetAddress, MGS1_MWinResCfgGet_CC, MGS1_MWinResCfgGetHookLength);

            LOG_F(INFO, "MGS 1: M2: MWinResCfg::Get hook length is %d bytes.", MGS1_MWinResCfgGetHookLength);
            LOG_F(INFO, "MGS 1: M2: MWinResCfg::Get hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_MWinResCfgGetAddress);
        }
        else if (!MGS1_MWinResCfgGetScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: MWinResCfg::Get pattern scan failed.");
        }
    }

    // MG | SR: Configuration hook
    if (sExeName == "MGS MC1 Bonus Content.exe")
    {
        uint8_t* MGSR_MWinResCfgGetScanResult = Memory::PatternScan(baseModule, "48 33 C4 48 89 44 24 50 48 8B FA 48 8B D9 48 89 54 24 48 48 8D 4C 24 28 E8 ?? ?? ?? ?? 48");
        if (MGSR_MWinResCfgGetScanResult)
        {
            uintptr_t MGSR_MWinResCfgGetAddress = (uintptr_t)(MGSR_MWinResCfgGetScanResult + 8);
            int MGSR_MWinResCfgGetHookLength = Memory::GetHookLength((char*)MGSR_MWinResCfgGetAddress, 13);
            MGSR_MWinResCfgGetReturnJMP = MGSR_MWinResCfgGetAddress + MGSR_MWinResCfgGetHookLength;
            Memory::DetourFunction((void*)MGSR_MWinResCfgGetAddress, MGSR_MWinResCfgGet_CC, MGSR_MWinResCfgGetHookLength);

            LOG_F(INFO, "MG | SR: M2: MWinResCfg::Get hook length is %d bytes.", MGSR_MWinResCfgGetHookLength);
            LOG_F(INFO, "MG | SR: M2: MWinResCfg::Get hook address is 0x%" PRIxPTR ".", (uintptr_t)MGSR_MWinResCfgGetAddress);
        }
        else if (!MGSR_MWinResCfgGetScanResult)
        {
            LOG_F(INFO, "MG | SR: M2: MWinResCfg::Get pattern scan failed.");
        }
    }
}

void BorderlessPatch()
{
    // MGS 1: Borderless patch
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_MWinResCfgSetWindowScanResult = Memory::PatternScan(baseModule, "B8 00 00 CE 02 BE 00 00 CA 02");
        if (MGS1_MWinResCfgSetWindowScanResult)
        {
            uint8_t* MGS1_MWinResCfgSetWindowPTR = (uint8_t*)MGS1_MWinResCfgSetWindowScanResult;
            uint8_t MGS1_MWinResCfgSetWindowFlags[] = { "\xB8\x00\x00\x00\x90\xBE\x00\x00\x00\x90" };
            Memory::PatchBytes((uintptr_t)MGS1_MWinResCfgSetWindowPTR, (const char*)MGS1_MWinResCfgSetWindowFlags, sizeof(MGS1_MWinResCfgSetWindowFlags) - 1);
            LOG_F(INFO, "MGS 1: M2: Borderless: MWinResCfg::SetWindow patched.");
        }
        else if (!MGS1_MWinResCfgSetWindowScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: Borderless: MWinResCfg::SetWindow pattern scan failed.");
        }
    }

    // MG | SR: Borderless patch
    if (sExeName == "MGS MC1 Bonus Content.exe")
    {
        uint8_t* MGSR_MWinResCfgSetWindowScanResult = Memory::PatternScan(baseModule, "BE 00 00 CB 02 41 BE 00 00 CB 00 44 ?? ?? ?? ?? ?? ?? 74 0B BE 00 00 CF 02 41 BE 00 00 CF 00");
        if (MGSR_MWinResCfgSetWindowScanResult)
        {
            uint8_t* MGSR_MWinResCfgSetWindowPTR = (uint8_t*)MGSR_MWinResCfgSetWindowScanResult;
            uint8_t MGSR_MWinResCfgSetWindowFlags[] = { "\xBE\x00\x00\x00\x90\x41\xBE\x00\x00\x00\x00\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90" };
            Memory::PatchBytes((uintptr_t)MGSR_MWinResCfgSetWindowPTR, (const char*)MGSR_MWinResCfgSetWindowFlags, sizeof(MGSR_MWinResCfgSetWindowFlags) - 1);
            LOG_F(INFO, "MG | SR: M2: Borderless: MWinResCfg::SetWindow patched.");
        }
        else if (!MGSR_MWinResCfgSetWindowScanResult)
        {
            LOG_F(INFO, "MG | SR: M2: Borderless: MWinResCfg::SetWindow pattern scan failed.");
        }
    }
}

void AnalogPatch()
{
    // MGS 1: Analog patch
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_MInputHubDMGetWinScanResult = Memory::PatternScan(baseModule, "66 89 4F 0C F3 0F 2C C0 0F B7 C0 66 89 47 0E 75");
        if (MGS1_MInputHubDMGetWinScanResult)
        {
            uint8_t* MGS1_MInputHubDMGetWinPTR = (uint8_t*)MGS1_MInputHubDMGetWinScanResult;
            uint8_t MGS1_MInputHubDMGetWin[] = { "\x66\x89\x4F\x08\xF3\x0F\x2C\xC0\x0F\xB7\xC0\x66\x89\x47\x0A\x75" };
            Memory::PatchBytes((uintptr_t)MGS1_MInputHubDMGetWinPTR, (const char*)MGS1_MInputHubDMGetWin, sizeof(MGS1_MInputHubDMGetWin) - 1);
            LOG_F(INFO, "MGS 1: M2: Analog: MInputHubDM::GetWin patched.");
        }
        else if (!MGS1_MInputHubDMGetWinScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: Analog: MInputHubDM::GetWin pattern scan failed.");
        }

        uint8_t* MGS1_M2EpiPadUpdateAxisScanResult = Memory::PatternScan(baseModule, "88 4C 10 44 83 FB 06 7C 86 8B 44 24 18 45 89 6C");
        if (MGS1_M2EpiPadUpdateAxisScanResult)
        {
            uint8_t* MGS1_M2EpiPadUpdateAxisPTR = (uint8_t*)MGS1_M2EpiPadUpdateAxisScanResult;
            uint8_t MGS1_M2EpiPadUpdateAxis[] = { "\x90\x90\x90\x90" };
            Memory::PatchBytes((uintptr_t)MGS1_M2EpiPadUpdateAxisPTR, (const char*)MGS1_M2EpiPadUpdateAxis, sizeof(MGS1_M2EpiPadUpdateAxis) - 1);
            LOG_F(INFO, "MGS 1: M2: Analog: M2Epi::PadUpdateAxis patched.");
        }
        else if (!MGS1_M2EpiPadUpdateAxisScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: Analog: M2Epi::PadUpdateAxis pattern scan failed.");
        }
    }
}

uintptr_t MGS1_M2EpiPadUpdateReturnJMP;
extern void M2_EpiPadState(unsigned int addr, unsigned int id, void* state);
void __declspec(naked) MGS1_M2EpiPadUpdate_CC()
{
#ifndef _WIN64
    __asm
    {
        push ecx
        push edx
        push[esp + 8]

        call M2_EpiPadState

        add esp, 4
        pop edx
        pop ecx

        mov[esp + 8], 0xF3
        mov[esp + 9], 0x5A
        jmp[MGS1_M2EpiPadUpdateReturnJMP]
    }
#endif
}

void AnalogHook()
{
    // MGS 1: Analog hook
    if (sExeName == "METAL GEAR SOLID.exe")
    {
        uint8_t* MGS1_M2EpiPadUpdateScanResult = Memory::PatternScan(baseModule, "C7 44 24 08 F3 5A 00 00 C7 44 24 0C 00 00 00 00");
        if (MGS1_M2EpiPadUpdateScanResult)
        {
            uintptr_t MGS1_M2EpiPadUpdateAddress = (uintptr_t)MGS1_M2EpiPadUpdateScanResult;
            int MGS1_M2EpiPadUpdateHookLength = Memory::GetHookLength((char*)MGS1_M2EpiPadUpdateAddress, 4);
            MGS1_M2EpiPadUpdateReturnJMP = MGS1_M2EpiPadUpdateAddress + MGS1_M2EpiPadUpdateHookLength;
            Memory::DetourFunction((void*)MGS1_M2EpiPadUpdateAddress, MGS1_M2EpiPadUpdate_CC, MGS1_M2EpiPadUpdateHookLength);

            LOG_F(INFO, "MGS 1: M2: Analog: M2Epi::PadUpdate hook length is %d bytes.", MGS1_M2EpiPadUpdateHookLength);
            LOG_F(INFO, "MGS 1: M2: Analog: M2Epi::PadUpdate hook address is 0x%" PRIxPTR ".", (uintptr_t)MGS1_M2EpiPadUpdateAddress);
        }
        else if (!MGS1_M2EpiPadUpdateScanResult)
        {
            LOG_F(INFO, "MGS 1: M2: Analog: M2Epi::PadUpdate pattern scan failed.");
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
    uint8_t* memsetResult = Memory::PatternScan(baseModule, "8B 4C 24 0C 0F B6 44 24 08 8B D7 8B 7C 24 04 85");
    int memsetMinHookLength = 4;
#else
    uint8_t* memsetResult = Memory::PatternScan(baseModule, "4C 8B D9 0F B6 D2 49 B9 01 01 01 01 01 01 01 01");
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
