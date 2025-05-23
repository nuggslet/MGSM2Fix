#include "stdafx.h"
#include <stdio.h>
#pragma comment(lib,"Version.lib")

using namespace std;

namespace Memory
{
    template<typename T>
    static void Write(uintptr_t writeAddress, T value)
    {
        DWORD oldProtect;
        VirtualProtect((LPVOID)(writeAddress), sizeof(T), PAGE_EXECUTE_WRITECOPY, &oldProtect);
        *(reinterpret_cast<T*>(writeAddress)) = value;
        VirtualProtect((LPVOID)(writeAddress), sizeof(T), oldProtect, &oldProtect);
    }

    static void PatchBytes(uintptr_t address, const char* pattern, unsigned int numBytes)
    {
        DWORD oldProtect;
        VirtualProtect((LPVOID)address, numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy((LPVOID)address, pattern, numBytes);
        VirtualProtect((LPVOID)address, numBytes, oldProtect, &oldProtect);
    }

    static void ReadBytes(const uintptr_t address, void* const buffer, const SIZE_T size)
    {
        memcpy(buffer, reinterpret_cast<const void*>(address), size); 
    }

    static uintptr_t ReadMultiLevelPointer(uintptr_t base, const std::vector<uint32_t>& offsets)
    {
        MEMORY_BASIC_INFORMATION mbi;
        for (auto& offset : offsets)
        {
            if (!VirtualQuery(reinterpret_cast<LPCVOID>(base), &mbi, sizeof(MEMORY_BASIC_INFORMATION)) || mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
                return 0;

            base = *reinterpret_cast<uintptr_t*>(base) + offset;
        }

        return base;
    }

    static int GetHookLength(char* src, int minLen)
    {
        int lengthHook = 0;
        const int size = 15;
        char buffer[size];

        memcpy(buffer, src, size);

        // must be >= 14
        while (lengthHook <= minLen)
            lengthHook += ldisasm(&buffer[lengthHook], true);

        return lengthHook;
    }

    static bool DetourFunction32(void* src, void* dst, int len)
    {
        if (len < 5) return false;

        DWORD curProtection;
        VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

        memset(src, 0x90, len);

        uintptr_t relativeAddress = ((uintptr_t)dst - (uintptr_t)src) - 5;

        *(BYTE*)src = 0xE9;
        *(uintptr_t*)((uintptr_t)src + 1) = relativeAddress;

        DWORD temp;
        VirtualProtect(src, len, curProtection, &temp);

        return true;
    }

    static void* DetourFunction64(void* pSource, void* pDestination, int dwLen)
    {
        DWORD MinLen = 14;

        if (dwLen < MinLen) return NULL;

        BYTE stub[] = {
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp qword ptr [$+6]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // ptr
        };

        void* pTrampoline = VirtualAlloc(0, dwLen + sizeof(stub), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        DWORD dwOld = 0;
        VirtualProtect(pSource, dwLen, PAGE_EXECUTE_READWRITE, &dwOld);

        DWORD64 retto = (DWORD64)pSource + dwLen;

        // trampoline
        memcpy(stub + 6, &retto, 8);
        memcpy((void*)((DWORD_PTR)pTrampoline), pSource, dwLen);
        memcpy((void*)((DWORD_PTR)pTrampoline + dwLen), stub, sizeof(stub));

        // orig
        memcpy(stub + 6, &pDestination, 8);
        memcpy(pSource, stub, sizeof(stub));

        for (int i = MinLen; i < dwLen; i++)
        {
            *(BYTE*)((DWORD_PTR)pSource + i) = 0x90;
        }

        VirtualProtect(pSource, dwLen, dwOld, &dwOld);
        return (void*)((DWORD_PTR)pTrampoline);
    }

    static void DetourFunction(void* src, void* dst, int len)
    {
#ifdef _WIN64
        DetourFunction64(src, dst, len);
#else
        DetourFunction32(src, dst, len);
#endif
    }

    static HMODULE GetThisDllHandle()
    {
        MEMORY_BASIC_INFORMATION info;
        size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)GetThisDllHandle, &info, sizeof(info));
        assert(len == sizeof(info));
        return len ? (HMODULE)info.AllocationBase : NULL;
    }

    std::string GetModuleVersion(HMODULE module)
    {
        if (!module)
            return "0.0.0";

        char modulePath[MAX_PATH] = { 0 };
        if (!GetModuleFileNameA(module, modulePath, MAX_PATH))
            return "0.0.0";

        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeA(modulePath, &handle);
        if (size == 0)
            return "0.0.0";

        std::vector<BYTE> versionInfo(size);
        if (!GetFileVersionInfoA(modulePath, handle, size, versionInfo.data()))
            return "0.0.0";

        VS_FIXEDFILEINFO* fileInfo = nullptr;
        UINT fileInfoLen = 0;
        if (!VerQueryValueA(versionInfo.data(), "\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoLen) || !fileInfo)
            return "0.0.0";

        // Extract version numbers
        DWORD verMS = fileInfo->dwFileVersionMS;
        DWORD verLS = fileInfo->dwFileVersionLS;
        int major = HIWORD(verMS);
        int minor = LOWORD(verMS);
        int patch = HIWORD(verLS);

        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    static auto pattern_to_byte = [](const char* pattern) {
        auto bytes = std::vector<int>{};
        auto start = const_cast<char*>(pattern);
        auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
    };

    // CSGOSimple's pattern scan
    // https://github.com/OneshotGH/CSGOSimple-master/blob/master/CSGOSimple/helpers/utils.cpp
    static std::uint8_t* PatternScan(void* module, const char* signature)
    {
        auto dosHeader = (PIMAGE_DOS_HEADER)module;
        auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

        auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        auto patternBytes = pattern_to_byte(signature);
        auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

        auto s = patternBytes.size();
        auto d = patternBytes.data();

        for (auto i = 0ul; i < sizeOfImage - s; ++i) {
            bool found = true;
            for (auto j = 0ul; j < s; ++j) {
                if (scanBytes[i + j] != d[j] && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return &scanBytes[i];
            }
        }
        return nullptr;
    }

    static std::uint8_t* PatternScanBuffer(void* buffer, size_t size, const char* signature)
    {
        auto patternBytes = pattern_to_byte(signature);
        auto scanBytes = reinterpret_cast<std::uint8_t*>(buffer);

        auto s = patternBytes.size();
        auto d = patternBytes.data();

        for (auto i = 0ul; i < size - s; ++i) {
            bool found = true;
            for (auto j = 0ul; j < s; ++j) {
                if (scanBytes[i + j] != d[j] && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return &scanBytes[i];
            }
        }
        return nullptr;
    }

    static uintptr_t GetAbsolute(uintptr_t address) noexcept
    {
        return (address + 4 + *reinterpret_cast<std::int32_t*>(address));
    }
}