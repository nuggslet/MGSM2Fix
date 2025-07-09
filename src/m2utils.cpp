#include "m2utils.h"

#include <setupapi.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")


bool M2Utils::IsSteamOS()
{
    // Check for Proton/Steam Deck environment variables
    return std::getenv("STEAM_COMPAT_CLIENT_INSTALL_PATH") ||
        std::getenv("STEAM_COMPAT_DATA_PATH") ||
        std::getenv("XDG_SESSION_TYPE"); 
}

std::string M2Utils::GetSteamOSVersion()
{
    std::ifstream os_release("/etc/os-release");
    std::string line;
    while (std::getline(os_release, line))
    {
        if (line.find("PRETTY_NAME=") == 0)
        {
            // Remove quotes if present
            size_t first_quote = line.find('"');
            size_t last_quote = line.rfind('"');
            if (first_quote != std::string::npos && last_quote != std::string::npos && last_quote > first_quote)
            {
                return line.substr(first_quote + 1, last_quote - first_quote - 1);
            }
            return line.substr(13); // fallback
        }
    }
    return "SteamOS (Unknown Version)";
}

void M2Utils::LogSystemInfo()
{
    std::string cpu;

    std::array<std::uint32_t, 3> functionIds = {
        0x80000002, // Manufacturer
        0x80000003, // Model
        0x80000004  // Clock-speed
    };

    for (auto id : functionIds)
    {
        char buffer[64] = {};
        __cpuid(reinterpret_cast<int *>(buffer), id);
        cpu += std::string(buffer);
    }

    cpu.erase(std::remove(cpu.begin(), cpu.end(), '\n'), cpu.end());
    if (!cpu.empty()) spdlog::info("[System] CPU: {}", cpu);

    std::string gpu;

    SP_DEVINFO_DATA devInfo = {};
    devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_DISPLAY,
        nullptr, nullptr,
        DIGCF_PRESENT
    );
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo); ++i) {
        char deviceName[256] = {};
        bool deviceResult = SetupDiGetDeviceRegistryPropertyA(
            hDevInfo, &devInfo,
            SPDRP_DEVICEDESC, nullptr,
            reinterpret_cast<PBYTE>(deviceName),
            sizeof(deviceName), nullptr
        );
        if (!deviceResult) continue;
        gpu = std::string(deviceName);

        HKEY key = SetupDiOpenDevRegKey(
            hDevInfo, &devInfo,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DRV,
            KEY_READ
        );
        if (!key || key == INVALID_HANDLE_VALUE) {
            gpu.erase(std::remove(gpu.begin(), gpu.end(), '\n'), gpu.end());
            if (!gpu.empty()) spdlog::info("[System] GPU: {}", gpu);
            continue;
        }

        char driverVersion[256];
        DWORD driverLength = sizeof(driverVersion);
        LSTATUS versionResult = RegQueryValueExA(
            key, "DriverVersion",
            nullptr, nullptr,
            reinterpret_cast<LPBYTE>(driverVersion),
            &driverLength
        );

        RegCloseKey(key);
        if (versionResult != ERROR_SUCCESS) {
            gpu.erase(std::remove(gpu.begin(), gpu.end(), '\n'), gpu.end());
            if (!gpu.empty()) spdlog::info("[System] GPU: {}", gpu);
            continue;
        }

        std::string drv = driverVersion;
        if (!drv.empty()) gpu += " (driver " + drv + ")";
        gpu.erase(std::remove(gpu.begin(), gpu.end(), '\n'), gpu.end());
        if (!gpu.empty()) spdlog::info("[System] GPU: {}", gpu);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);

    double memory = static_cast<double>(status.ullTotalPhys) / 1024.0f / 1024.0f;
    spdlog::info("[System] RAM: {:.{}f} GB ({:.{}f} MB)",
        std::ceil((memory / 1024.0f) * 100.0f) / 100.0f, 2, memory, 0
    );

    std::string os;

    if (IsSteamOS())
    {
        os = GetSteamOSVersion();
    }
    else
    {
        HKEY key;
        LSTATUS versionResult = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0, KEY_READ | KEY_WOW64_64KEY, &key
        );

        if (versionResult == ERROR_SUCCESS)
        {
            char buffer[256]; DWORD size = sizeof(buffer);
            LSTATUS nameResult = RegQueryValueExA(
                key, "ProductName",
                nullptr, nullptr,
                reinterpret_cast<LPBYTE>(buffer), &size
            );
            if (nameResult == ERROR_SUCCESS)
            {
                os = buffer;
            }
        }

        RegCloseKey(key);

        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        while (ntdll)
        {
            typedef LONG(WINAPI* RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
            RtlGetVersion_t RtlGetVersion =
                reinterpret_cast<RtlGetVersion_t>(GetProcAddress(ntdll, "RtlGetVersion"));
            if (!RtlGetVersion) break;

            RTL_OSVERSIONINFOW info = {};
            info.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

            if (RtlGetVersion(&info) != 0) break;
            os += " (build " + std::to_string(info.dwBuildNumber) + ")";

            if (info.dwBuildNumber < 22000) break;
            std::size_t pos = os.find("Windows 10");

            if (pos == std::string::npos) break;
            os.replace(pos, 10, "Windows 11");
            break;
        }
    }

    if (!os.empty()) spdlog::info("[System] OS: {}", os);

    spdlog::info("----------");
}

bool M2Utils::memsetHookCalled = false;
bool M2Utils::mainThreadFinished = false;
std::mutex M2Utils::memsetHookMutex = {};
std::mutex M2Utils::mainThreadFinishedMutex = {};
std::condition_variable M2Utils::mainThreadFinishedVar = {};

// Thanks emoose!
void *M2Utils::memsetWait(void *str, int c, size_t n)
{
    std::lock_guard lock(memsetHookMutex);
    if (!memsetHookCalled)
    {
        memsetHookCalled = true;

        // Wait for our main thread to finish before we return to the game
        if (!mainThreadFinished)
        {
            std::unique_lock finishedLock(mainThreadFinishedMutex);
            mainThreadFinishedVar.wait(finishedLock, [] { return mainThreadFinished; });
        }
    }

    return M2Hook::GetInstance().Invoke<void *>(memsetWait, str, c, n);
}

void M2Utils::memsetRelease()
{
    // Signal any threads which might be waiting for us before continuing
    std::lock_guard lock(mainThreadFinishedMutex);
    mainThreadFinished = true;
    mainThreadFinishedVar.notify_all();
}

void M2Utils::memsetHook()
{
#ifndef _WIN64
    M2Hook::GetInstance().Hook("8B 4C 24 0C 0F B6 44 24 08 8B D7 8B 7C 24 04 85", 0, memsetWait);
#else
    M2Hook::GetInstance().Hook("4C 8B D9 0F B6 D2 49 B9 01 01 01 01 01 01 01 01", 0, memsetWait);
#endif
}
