#pragma once

#include "stdafx.h"

class M2Hook
{
public:
    M2Hook(HMODULE module = nullptr)
        : m_module(module)
    {
        new (&m_vmtHooks) decltype(m_vmtHooks) {};
        new (&m_vmHooks)  decltype(m_vmHooks)  {};
    }

    virtual ~M2Hook() {}

    static auto & GetInstance(std::optional<std::string> name = std::nullopt, HMODULE module = nullptr)
    {
        static std::map<std::optional<std::string>, M2Hook> instances;
        if (name && name->empty()) name = std::nullopt;
        if (instances.count(name) == 0) {
            if (!module) module = GetModuleHandle(name ? name->c_str() : nullptr);
            instances.try_emplace(name, module);
        }
        return instances[name];
    }

    static void Attach(HINSTANCE hinstDLL)
    {
        GetInstance(".", reinterpret_cast<HMODULE>(hinstDLL));
    }

    template<typename Function>
    bool Hook(const char *signature, std::ptrdiff_t offset, Function && function, const char *label = nullptr)
    {
        void *addr = reinterpret_cast<void *>(Scan(signature, offset, label));
        if (!addr) return false;

        return Hook(addr, function, label);
    }

    template<typename Function>
    bool Hook(void *addr, Function && function, const char *label = nullptr)
    {
        auto && hook = safetyhook::InlineHook::create(addr, function);

        if (label && hook)  spdlog::info("{} hook succeeded.", label);
        if (label && !hook) spdlog::warn("{} hook failed.", label);
        if (!hook) return false;

        m_hooks[function] = std::move(*hook);
        return true;
    }

    bool MidHook(const char *signature, std::ptrdiff_t offset, safetyhook::MidHookFn function, const char *label = nullptr)
    {
        void *addr = reinterpret_cast<void *>(Scan(signature, offset, label));
        if (!addr) return false;

        return MidHook(addr, function, label);
    }

    bool MidHook(void *addr, safetyhook::MidHookFn function, const char *label = nullptr)
    {
        auto && hook = safetyhook::MidHook::create(addr, function);

        if (label && hook)  spdlog::info("{} hook succeeded.", label);
        if (label && !hook) spdlog::warn("{} hook failed.", label);
        if (!hook) return false;

        m_midHooks[function] = std::move(*hook);
        return true;
    }

    template<typename Object>
    bool VirtualTableHook(Object && object, const char *label = nullptr)
    {
        auto && hook = safetyhook::VmtHook::create(object);

        if (label && hook)  spdlog::info("{} hook succeeded.", label);
        if (label && !hook) spdlog::warn("{} hook failed.", label);
        if (!hook) return false;

        m_vmtHooks[object] = std::move(*hook);
        return true;
    }

    template<typename Object, typename Method, typename Function>
    bool VirtualHook(Object && object, Method && method, Function && function, const char *label = nullptr)
    {
        if (m_vmtHooks.count(object) == 0) {
            if (label) spdlog::warn("{} hook failed.", label);
            return false;
        }

        ZydisDecoder decoder {};
        ZyanStatus status;

#ifdef _WIN64
        status = ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#else
        status = ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif
        if (!ZYAN_SUCCESS(status)) {
            if (label) spdlog::warn("{} hook failed.", label);
            return false;
        }

        unsigned char *thunk = reinterpret_cast<unsigned char *&>(method);
        ZydisDecodedInstruction instruction = {};
        while (instruction.mnemonic != ZYDIS_MNEMONIC_JMP) {
            if (!ZYAN_SUCCESS(
                ZydisDecoderDecodeInstruction(
                    &decoder, nullptr,
                    thunk, 15,
                    &instruction)
            )) {
                if (label) spdlog::warn("{} hook failed.", label);
                return false;
            }
            thunk += instruction.length;
        }

        unsigned index = static_cast<unsigned>(instruction.raw.disp.value) / sizeof(uintptr_t);
        auto && hook = safetyhook::create_vm(m_vmtHooks[object], index, function);

        if (label) spdlog::info("{} hook succeeded.", label);

        m_vmHooks[function] = std::move(hook);
        return true;
    }

    template<typename Object, typename Function>
    bool VirtualHook(Object && object, unsigned index, Function && function, const char *label = nullptr)
    {
        if (m_vmtHooks.count(object) == 0) {
            if (label) spdlog::warn("{} hook failed.", label);
            return false;
        }
        auto && hook = safetyhook::create_vm(m_vmtHooks[object], index, function);

        if (label) spdlog::info("{} hook succeeded.", label);

        m_vmHooks[function] = std::move(hook);
        return true;
    }

    template<typename Return, typename Function, typename ... Args>
    Return VirtualInvoke(Function && function, Args ... args)
    {
        auto & hook = m_vmHooks[function];
        if constexpr (CallingConvention_v<Function> == CallingConvention::Fastcall) {
            return hook.fastcall<Return>(std::forward<Args>(args) ...);
        }
        else if constexpr (CallingConvention_v<Function> == CallingConvention::Stdcall) {
            return hook.stdcall<Return>(std::forward<Args>(args) ...);
        }
        return hook.call<Return>(std::forward<Args>(args) ...);
    }

    template<typename Return, typename Function, typename ... Args>
    Return Invoke(Function && function, Args ... args)
    {
        auto & hook = m_hooks[function];
        if constexpr (CallingConvention_v<Function> == CallingConvention::Fastcall) {
            return hook.fastcall<Return>(std::forward<Args>(args) ...);
        }
        else if constexpr (CallingConvention_v<Function> == CallingConvention::Stdcall) {
            return hook.stdcall<Return>(std::forward<Args>(args) ...);
        }
        return hook.call<Return>(std::forward<Args>(args) ...);
    }

    uintptr_t Scan(const char *signature, std::ptrdiff_t offset, const char *label = nullptr) const
    {
        uint8_t *result = PatternScan(m_module, signature);
        if (!result) {
            if (label) spdlog::warn("{} pattern scan failed.", label);
            return 0;
        }

        result += offset;
        if (label) spdlog::info("{} is {}.", label, fmt::ptr(result));
        return (uintptr_t) result;
    }

    uintptr_t Scan(void *data, size_t size, std::ptrdiff_t offset, const char *label = nullptr) const
    {
        auto signature = BufferToPattern(data, size);
        return Scan(signature.c_str(), offset, label);
    }

    template <typename ... Types>
    std::tuple<Types ...> Unpack(void *data, std::array<std::ptrdiff_t, sizeof...(Types)> offsets, const char *label = nullptr) const
    {
        std::tuple<Types ...> values;
        UnpackValue<offsets.size(), Types ...>::impl(values, offsets, data, label);
        return values;
    }

    template <typename ... Types>
    std::tuple<Types ...> Unpack(const char *signature, std::ptrdiff_t offset, std::array<std::ptrdiff_t, sizeof...(Types)> offsets, const char *label = nullptr) const
    {
        void *addr = reinterpret_cast<void *>(Scan(signature, offset, label));
        return Unpack<Types ...>(addr, offsets, label);
    }

    uintptr_t ScanBuffer(const char *signature, std::ptrdiff_t offset, void* buffer, size_t length, const char *label = nullptr) const
    {
        uint8_t *result = PatternScanBuffer(buffer, length, signature);
        if (!result) {
            if (label) spdlog::warn("{} pattern scan failed.", label);
            return 0;
        }

        result += offset;
        if (label) spdlog::info("{} is {}.", label, fmt::ptr(result));
        return (uintptr_t) result;
    }

    uintptr_t ScanBuffer(void *data, size_t size, std::ptrdiff_t offset, void* buffer, size_t length, const char *label = nullptr) const
    {
        auto signature = BufferToPattern(data, size);
        return ScanBuffer(signature.c_str(), offset, buffer, length, label);
    }

    bool Patch(const char *signature, std::ptrdiff_t offset, const char *data, const char *label = nullptr) const
    {
        void *addr = reinterpret_cast<void *>(Scan(signature, offset, label));
        if (!addr) return false;

        Patch(addr, data, label);
        return true;
    }

    void Patch(void *addr, const char *data, const char *label = nullptr) const
    {
        auto patch = PatternToBuffer(data);
        Patch(addr, patch.data(), patch.size(), label);
    }

    void Patch(void *addr, void *data, size_t size, const char *label = nullptr) const
    {
        PatchBytes((uintptr_t) addr, reinterpret_cast<const char *>(data), size);
        if (label) spdlog::info("{} patched.", label);
    }

    std::string BufferToPattern(void *data, size_t size) const
    {
        std::ostringstream stream;
        stream << std::setfill('0') << std::hex;
        for (size_t i = 0; i < size; i++) {
            stream << " " << std::setw(2) << static_cast<int>((reinterpret_cast<unsigned char *>(data))[i]);
        }
        std::string pattern = stream.str();
        pattern.erase(pattern.begin());
        return pattern;
    }

    std::vector<char> PatternToBuffer(const char *pattern) const
    {
        auto buffer = PatternToByte(pattern);
        std::vector<char> data;
        for (auto i : buffer) data.push_back(i);
        return data;
    }

    void *ModuleAddress() const
    {
        return m_module;
    }

    uint32_t ModuleTimestamp() const
    {
        return ModuleTimestamp(m_module);
    }

    std::string ModuleVersion() const
    {
        return ModuleVersion(m_module);
    }

    std::filesystem::path ModuleLocation() const
    {
        char path[MAX_PATH] = { 0 };
        GetModuleFileName(m_module, path, MAX_PATH);
        return path;
    }

    std::string ModulePath() const
    {
        return ModuleLocation().string();
    }

    std::string ModuleName() const
    {
        return ModuleLocation().filename().string();
    }

    std::string ModuleIdentifier() const
    {
        auto path = ModuleLocation().parent_path().filename() / ModuleName();
        return path.string();
    }

    void *ModuleResource(unsigned int id, const char *type, size_t *size = nullptr) const
    {
        HRSRC resource = FindResource(m_module, MAKEINTRESOURCE(id), type);
        if (!resource) return nullptr;

        HGLOBAL global = LoadResource(m_module, resource);
        if (!global) return nullptr;

        void *p = LockResource(global);
        if (!p) return nullptr;

        if (size) *size = SizeofResource(m_module, resource);
        return p;
    }

    static std::string EnvironmentVariable(const std::string &name)
    {
        static std::map<std::string, std::string> variables;
        if (variables.size() == 0)
        {
            auto envFree = [](char *p) { FreeEnvironmentStrings(p); };
            auto envBlock = std::unique_ptr<char, decltype(envFree)>{
                GetEnvironmentStrings(), envFree
            };

            for (char *i = envBlock.get(); *i != 0; ++i)
            {
                std::string key, value;
                for (; *i != '='; ++i) key += *i;
                ++i;
                for (; *i != 0; ++i) value += *i;
                variables[key] = value;
            }
        }

        return variables[name];
    }

    static std::string_view ReadUntilTabOrCRLF(uintptr_t address)
    {
        const char* ptr = reinterpret_cast<const char*>(address);
        const char* start = ptr;

        while (true)
        {
            unsigned char b = *ptr;
            if (b == 0x09) break;                      // tab
            if (b == 0x0D && *(ptr + 1) == 0x0A) break; // CRLF
            ++ptr;
        }

        return std::string_view(start, ptr - start);
    }


    static void DumpBytes(uintptr_t address, size_t length)
    {
        if (address == 0 || length == 0)
        {
            spdlog::error("Invalid address or length.");
            return;
        }

        unsigned char* base = reinterpret_cast<unsigned char*>(address);

        spdlog::info("Dumping {} bytes from process memory at address 0x{:X}:",
                     length, address);

        std::string line;
        for (size_t i = 0; i < length; ++i)
        {
            if (i % 16 == 0)
            {
                if (!line.empty())
                {
                    spdlog::info("{}", line);
                    line.clear();
                }
                line = fmt::format("0x{:08X}  ",
                                   static_cast<unsigned int>(address + i));
            }
            line += fmt::format("{:02X} ", base[i]);
        }
        if (!line.empty())
        {
            spdlog::info("{}", line);
        }
    }


private:
    template<size_t N, typename ... Types>
    struct UnpackValue
    {
        static void impl(std::tuple<Types ...> & values, const std::array<std::ptrdiff_t, sizeof...(Types)> & offsets, void *data, const char *label = nullptr)
        {
            auto *buffer = reinterpret_cast<uint8_t *>(data);

            // ???
            std::get<N - 1>(values) = {};
            std::remove_reference_t<decltype(std::get<N - 1>(values))> value = {};
            std::memcpy(&value, &buffer[offsets[N - 1]], sizeof(value));
            std::get<N - 1>(values) = value;

            if (label) {
                spdlog::info("{}[{}] = {}.", label, N - 1, fmt::ptr(value));
            }

            return UnpackValue<N - 1, Types ...>::impl(values, offsets, data, label);
        }
    };

    template<typename ... Types>
    struct UnpackValue<0, Types ...>
    {
        static void impl(std::tuple<Types ...> & values, const std::array<std::ptrdiff_t, sizeof...(Types)> & offsets, void *data, const char *label = nullptr)
        {}
    };

    template<typename T>
    static void Write(uintptr_t address, T value)
    {
        DWORD oldProtect;
        VirtualProtect(reinterpret_cast<LPVOID>(address), sizeof(T), PAGE_EXECUTE_WRITECOPY, &oldProtect);
        *(reinterpret_cast<T*>(address)) = value;
        VirtualProtect(reinterpret_cast<LPVOID>(address), sizeof(T), oldProtect, &oldProtect);
    }

    static void PatchBytes(uintptr_t address, const char* pattern, unsigned int numBytes)
    {
        DWORD oldProtect;
        VirtualProtect(reinterpret_cast<LPVOID>(address), numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(reinterpret_cast<LPVOID>(address), pattern, numBytes);
        VirtualProtect(reinterpret_cast<LPVOID>(address), numBytes, oldProtect, &oldProtect);
    }

    static uint32_t ModuleTimestamp(HMODULE module)
    {
        auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
        auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<unsigned char *>(module) + dosHeader->e_lfanew
        );
        return ntHeaders->FileHeader.TimeDateStamp;
    }

    static std::string ModuleVersion(HMODULE module)
    {
        uint32_t timestamp = ModuleTimestamp(module);
        // Convert the timestamp to a human-readable date format (YYYY-MM-DD)
        std::time_t time = static_cast<std::time_t>(timestamp);
        std::tm* tm = std::gmtime(&time); // Convert to UTC time
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d");
        return oss.str();
    }

    static std::vector<int> PatternToByte(const char *pattern)
    {
        auto bytes = std::vector<int>{};
        auto start = const_cast<char *>(pattern);
        auto end = const_cast<char *>(pattern) + strlen(pattern);

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
    static unsigned char *PatternScanBuffer(void *buffer, size_t size, const char *signature)
    {
        auto patternBytes = PatternToByte(signature);
        auto scanBytes = reinterpret_cast<unsigned char *>(buffer);

        auto s = patternBytes.size();
        auto d = patternBytes.data();

        for (unsigned i = 0; i < size - s; ++i) {
            bool found = true;
            for (unsigned j = 0; j < s; ++j) {
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

    static unsigned char *PatternScan(void *module, const char *signature)
    {
        auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
        auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<unsigned char *>(module) + dosHeader->e_lfanew
        );
        auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        return PatternScanBuffer(module, sizeOfImage, signature);
    }

    HMODULE m_module;
    std::map<void *, safetyhook::InlineHook> m_hooks;
    std::map<void *, safetyhook::MidHook>    m_midHooks;
    union {
        struct {
            std::map<void *, safetyhook::VmtHook> m_vmtHooks;
            std::map<void *, safetyhook::VmHook>  m_vmHooks;
        };
    };
};
