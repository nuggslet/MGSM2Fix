#include "m2utils.h"
#include "m2fix.h"

#include <setupapi.h>
#include <devguid.h>

#include "warning_background_shuffle.hpp"
#pragma comment(lib, "setupapi.lib")

std::filesystem::path M2Utils::EnsureAppData()
{
    std::filesystem::path path = std::getenv("APPDATA");
    path = path / "M2Fix" / M2Fix::GameInfo()->classname;
    std::filesystem::create_directories(path);
    return path;
}

bool M2Utils::IsSteamOS()
{
    static bool bCheckedSteamDeck = false;
    static bool bIsSteamDeck = false;
    if (bCheckedSteamDeck) {
        return bIsSteamDeck;
    }
    bCheckedSteamDeck = true;
    // Check for Proton/Steam Deck environment variables
    if (std::getenv("STEAM_COMPAT_CLIENT_INSTALL_PATH") || std::getenv("STEAM_COMPAT_DATA_PATH") || std::getenv("XDG_SESSION_TYPE")) {
        bIsSteamDeck = true;
    }
    return bIsSteamDeck;
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

    cpu.erase(cpu.find_last_not_of(" \t\n\r\f\v") + 1);
    if (!cpu.empty()) spdlog::info("[System] CPU: {}", cpu);

    SP_DEVINFO_DATA devInfo = {};
    devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_DISPLAY,
        nullptr, nullptr,
        DIGCF_PRESENT
    );
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo); ++i)
    {
        char deviceName[256] = {};
        bool deviceResult = SetupDiGetDeviceRegistryPropertyA(
            hDevInfo, &devInfo,
            SPDRP_DEVICEDESC, nullptr,
            reinterpret_cast<PBYTE>(deviceName),
            sizeof(deviceName), nullptr
        );
        if (!deviceResult) continue;
        std::string gpu = std::string(deviceName);

        HKEY key = SetupDiOpenDevRegKey(
            hDevInfo, &devInfo,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DRV,
            KEY_READ
        );
        if (!key || key == INVALID_HANDLE_VALUE) {
            gpu.erase(gpu.find_last_not_of(" \t\n\r\f\v") + 1);
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
            gpu.erase(gpu.find_last_not_of(" \t\n\r\f\v") + 1);
            if (!gpu.empty()) spdlog::info("[System] GPU: {}", gpu);
            continue;
        }

        std::string drv = driverVersion;
        if (!drv.empty()) gpu += " (driver " + drv + ")";
        gpu.erase(gpu.find_last_not_of(" \t\n\r\f\v") + 1);
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

        DWORD ubr = 0;

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

            // Get UBR (Update Build Revision)
            size = sizeof(DWORD);
            LSTATUS ubrResult = RegQueryValueExA(
                key, "UBR",
                nullptr, nullptr,
                reinterpret_cast<LPBYTE>(&ubr), &size
            );
            if (ubrResult != ERROR_SUCCESS)
            {
                ubr = 0; // fallback if not present
            }

            RegCloseKey(key);
        }

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
            os += " (build " + std::to_string(info.dwBuildNumber);
            if (ubr != 0)
            {
                os += "." + std::to_string(ubr);
            }
            os += ")";

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

void M2Utils::DisableWindowsFullscreenOptimization()
{
    if (IsSteamOS()) {
        return;
    }

    const std::filesystem::path path = M2Hook::GetInstance().ModuleLocation();
    assert(path.has_extension() && path.extension() == ".exe" && "ModuleLocation() didn't return a .exe!"); //Just an extra sanity check since we're messing with the registry.
    std::string sExePath = path.string();

    const bool shouldApply = M2Config::bDisableWindowsFullscreenOptimization;
    const auto markerFile = EnsureAppData() / "fullscreen_optimization.bin"; // Marker file to track if we're the one who applied the fix, or if the user did it manually.
    const bool markerExists = std::filesystem::exists(markerFile);
    const bool shouldRemove = !shouldApply && markerExists; // Only remove if we're the ones who initially applied the compatibility setting.
    if (!shouldApply && !shouldRemove) {
        return;
    }
    spdlog::info("[Registry] {} fullscreen optimization registry fix for {}.", shouldApply ? "Applying" : "Reverting", path.filename().string());
    HKEY hKey;
    const char* subKey = R"(Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)";
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, subKey, 0, KEY_READ | KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        spdlog::error("[Registry] Failed to open registry key: {}.", subKey);
        return;
    }

    // Query existing value
    DWORD type = 0, dataSize = 0;
    result = RegQueryValueExA(hKey, sExePath.c_str(), nullptr, &type, nullptr, &dataSize);

    std::string value;
    if (result == ERROR_SUCCESS && dataSize > 0)
    {
        std::vector<char> data(dataSize);
        if (RegQueryValueExA(hKey, sExePath.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(data.data()), &dataSize) == ERROR_SUCCESS)
        {
            value.assign(data.begin(), data.end());
            while (!value.empty() && value.back() == '\0')
                value.pop_back();
        }
    }

    bool modified = false;

    if (shouldApply)
    {
        if (!value.empty() && value[0] != '~') {
            value = "~ " + value;
            modified = true;
        }
        if (value.find("DISABLEDXMAXIMIZEDWINDOWEDMODE") == std::string::npos) {
            if (!value.empty() && value.back() != ' ')
                value.push_back(' ');
            value += "DISABLEDXMAXIMIZEDWINDOWEDMODE";
            modified = true;
        }
    }
    else if (shouldRemove)
    {
        size_t pos = value.find("DISABLEDXMAXIMIZEDWINDOWEDMODE");
        if (pos != std::string::npos)
        {
            value.erase(pos, strlen("DISABLEDXMAXIMIZEDWINDOWEDMODE"));
            while (!value.empty() && value.back() == ' ')
                value.pop_back();
            if (value == "~")
                value.clear();
            modified = true;
        }
    }

    if (modified)
    {
        if (value.empty())
        {
            if (RegDeleteValueA(hKey, sExePath.c_str()) == ERROR_SUCCESS)
                spdlog::info("[Registry] Deleted registry entry for {}.", path.filename().string());
            else
                spdlog::error("[Registry] Failed to delete registry entry for {}.", path.filename().string());
        }
        else
        {
            DWORD valueSize = static_cast<DWORD>(value.size() + 1);
            if (RegSetValueExA(hKey, sExePath.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), valueSize) == ERROR_SUCCESS)
                spdlog::info("[Registry] Wrote registry entry for {}: {}.", path.filename().string(), value);
            else
                spdlog::error("[Registry] Failed to write registry entry for {}.", path.filename().string());
        }
    }
    else
    {
        spdlog::info("[Registry] No registry changes required for {}.", path.filename().string());
    }

    RegCloseKey(hKey);

    if (shouldApply)
    {
        if (!markerExists)
        {
            try
            {
                std::ofstream out(markerFile, std::ios::trunc);
                if (out)
                {
                    out << "  ...A surveillance camera?!\n";
                    out << "MGSM2Fix wrote this file to track fullscreen optimization registry state.\n";
                    out.close();
                    spdlog::info("[Registry] Created marker file: {}.", markerFile.string());
                }
            }
            catch (const std::exception& e)
            {
                spdlog::error("[Registry] Failed to create marker file: {} - {}.", markerFile.string(), e.what());
            }
        }
    }
    else if (shouldRemove)
    {
        std::error_code ec;
        std::filesystem::remove(markerFile, ec);
        if (!ec)
            spdlog::info("[Registry] Removed marker file: {}.", markerFile.string());
        else
            spdlog::warn("[Registry] Failed to remove marker file: {}.", markerFile.string());
    }
}

// Thanks emoose!
void * __cdecl M2Utils::memsetWait(void *str, int c, size_t n)
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

void M2Utils::CompatibilityWarnings()
{
    BackgroundShuffleWarning::Check();
}

void M2Utils::nullsub()
{
}
