#include "mgs1.h"
#include "psx.h"
#include "sqhook.h"

int MGS1::MGS1_main(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    PSX::main(cpu);

    unsigned int ra = cpu->Reg[31];
    spdlog::info("[MGS 1] __main: 0x{:08x} -> 0x{:08x}.", address, ra);

    return cpu->Step(cpu, 0, ra);
}

int MGS1::MGS1_s03a_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    static bool oneshot = false;
    if (!oneshot) {
        spdlog::info("[MGS 1] {} s03a_disable_mosaic().", M2Config::bPatchesEnableMosaic ? "Blocking" : "Allowing");
        oneshot = true;
    }
    if (!M2Config::bPatchesEnableMosaic) {
        PSXFUNCTION s03a_disable_mosaic = PSX::ModuleHandlers[address];
        return s03a_disable_mosaic(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

int MGS1::MGS1_s03d_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    static bool oneshot = false;
    if (!oneshot) {
        spdlog::info("[MGS 1] {} s03d_disable_mosaic().", M2Config::bPatchesEnableMosaic ? "Blocking" : "Allowing");
        oneshot = true;
    }
    if (!M2Config::bPatchesEnableMosaic) {
        PSXFUNCTION s03d_disable_mosaic = PSX::ModuleHandlers[address];
        return s03d_disable_mosaic(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

SQInteger MGS1::SQReturn_set_playside_mgs(HSQUIRRELVM<Squirk::Standard> v)
{
    spdlog::info("[MGS 1] Set play side to {}.", SQSystemData<Squirk::Standard>::SettingPad::GetPlaySide_MGS1());
    return 0;
};

void MGS1::SQOnMemoryDefine()
{
    MGS1_GlobalsPTR = SQTitleProf<Squirk::Standard>::GetMemoryDefine("scene_name");
    spdlog::info("[MGS 1] mgs_stage is 0x{:x}.", MGS1_GlobalsPTR);
}

void MGS1::SQOnUpdateGadgets()
{
    if (MGS1_GlobalsPTR != 0 && MGS1_LoaderPTR != 0) {
        SQInteger MGS1_StageNamePTR = MGS1_GlobalsPTR;

        char MGS1_StageName[8] = { 0 };
        char MGS1_LoaderName[8] = { 0 };
        SQEmuTask<Squirk::Standard>::RamCopy(MGS1_StageName, MGS1_StageNamePTR, sizeof(MGS1_StageName));
        SQEmuTask<Squirk::Standard>::RamCopy(MGS1_LoaderName, MGS1_LoaderPTR, sizeof(MGS1_LoaderName));

        if (M2Config::bGameStageSelect) {
            if (strcmp(MGS1_LoaderName, "title") == 0 && strcmp(MGS1_StageName, "select") != 0) {
                strcpy(MGS1_LoaderName, "select");
                SQEmuTask<Squirk::Standard>::RamCopy(MGS1_LoaderPTR, MGS1_LoaderName, sizeof(MGS1_LoaderName));
                spdlog::info("[MGS 1] Set mgs_loader_stage to \"{}\".", MGS1_LoaderName);
            }
        }
    }

    if (M2Config::bAnalogMode) AnalogLoop();
}

void MGS1::EPIOnLoadImage(void *image, size_t size)
{
    MGS1_LoaderPTR = M2Hook::GetInstance().ScanBuffer(
        "00 00 00 00 69 6E 69 74 00 00 00 00",
        -0x30 - reinterpret_cast<uintptr_t>(image),
        image, size, "[MGS 1] mgs_loader_stage"
    );
}

bool MGS1::GWBlank()
{
    if (!PSX::Emulator) return false;
    struct M2_EmuGPU *gpu = PSX::Emulator->DevGPU;
    if (gpu->VideoMode && (gpu->Status & 0x10000)) {
        if (MGS1_Blank < 5) {
            ++MGS1_Blank;
            return true;
        }
        return false;
    } else {
        if (MGS1_Blank > 0) {
            --MGS1_Blank;
            return true;
        }
        return false;
    }
}

bool MGS1::EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args)
{
    struct M2_EmuPSX *psx = std::any_cast<M2_EmuPSX *>(machine);
    struct M2_EmuGPU *gpu = psx->DevGPU;
    bool ret = true;

    switch (cmd)
    {
        case 0x8002: // GET_POSITION
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            unsigned int w =  ((gpu->ScreenRangeW >> 12) & 0xFFF) - (gpu->ScreenRangeW & 0xFFF);
            unsigned int h = (((gpu->ScreenRangeH >> 10) & 0x3FF) - (gpu->ScreenRangeH & 0x3FF)) << ((gpu->Status >> 22) & 1);
            unsigned int x = (gpu->VideoMode ? 256 : 240) << ((gpu->Status >> 22) & 1);
            unsigned int y = std::min(h, x);

            *(args[0]) = (2560 - w) >> 1;
            *(args[1]) = (M2Config::iInternalHeight * ((x - y) >> 1)) / x;
            *(args[2]) = (w * M2Config::iInternalHeight) / 256;
            *(args[3]) = (y * M2Config::iInternalHeight) / 256;

            ret = false;
            break;
        }

        case 0x8003: // GET_DIMENSION
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            *(args[0]) = (M2Config::iInternalHeight * ((gpu->VideoMode && (gpu->Status & 0x10000)) ? 384 : 320)) / 256;
            *(args[1]) = ((M2Config::iInternalHeight * (gpu->VideoMode ? 256 : 224)) / (gpu->VideoMode ? 256 : 240));

            ret = false;
            break;
        }

        case 0x8004: // SET_DEVICE
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            unsigned int *res = args[0];

            res[1] = ((M2Config::iInternalHeight * 320) / 256) * 2;
            res[2] =   M2Config::iInternalHeight * 2;
            break;
        }

        case 0x8007: // SET_VIDEO_MODE
        {
            PSX::VideoMode = reinterpret_cast<unsigned int>(args[0]);
            break;
        }

        default: break;
    }

    return ret;
}

void MGS1::DisableWindowsFullscreenOptimization()
{
    spdlog::info("[Registry Compat Fix] Checking registry for {} compatibility flags.", M2Hook::GetInstance().ModuleName());
    HKEY hKey;
    const char* subKey = R"(Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)";
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, subKey, 0, KEY_READ | KEY_WRITE, &hKey);

    if (result != ERROR_SUCCESS)
    {
        spdlog::error("[Registry Compat Fix] Failed to open registry key: {}", subKey);
        return;
    }

    std::string exePath = M2Hook::GetInstance().ModulePath();

    DWORD type = 0;
    DWORD dataSize = 0;

    result = RegQueryValueExA(hKey, exePath.c_str(), nullptr, &type, nullptr, &dataSize);
    if (result != ERROR_SUCCESS)
    {
        // Key not found, create with default
        const char* defaultValue = "~ DISABLEDXMAXIMIZEDWINDOWEDMODE";
        DWORD valueSize = static_cast<DWORD>(strlen(defaultValue) + 1);

        result = RegSetValueExA(hKey, exePath.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(defaultValue), valueSize);
        if (result == ERROR_SUCCESS)
        {
            spdlog::info("[Registry Compat Fix] Created registry entry for {} with value: {}", exePath, defaultValue);
        }
        else
        {
            spdlog::error("[Registry Compat Fix] Failed to create registry value for {}", exePath);
        }

        RegCloseKey(hKey);
        return;
    }

    // Value exists
    std::vector<char> data(dataSize);
    result = RegQueryValueExA(hKey, exePath.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(data.data()), &dataSize);

    if (result != ERROR_SUCCESS)
    {
        spdlog::error("[Registry Compat Fix] Failed to read registry value for {}", exePath);
        RegCloseKey(hKey);
        return;
    }

    std::string value(data.begin(), data.end());
    // Trim trailing nulls
    while (!value.empty() && value.back() == '\0')
    {
        value.pop_back();
    }

    bool modified = false;

    if (!value.empty() && value[0] != '~')
    {
        value = "~ " + value;
        modified = true;
    }

    if (value.find("DISABLEDXMAXIMIZEDWINDOWEDMODE") == std::string::npos)
    {
        if (!value.empty() && value.back() != ' ')
        {
            value.push_back(' ');
        }
        value += "DISABLEDXMAXIMIZEDWINDOWEDMODE";
        modified = true;
    }

    if (modified)
    {
        DWORD valueSize = static_cast<DWORD>(value.size() + 1);
        result = RegSetValueExA(hKey, exePath.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), valueSize);

        if (result == ERROR_SUCCESS)
        {
            spdlog::info("[Registry Compat Fix] Updated registry entry for {}: {}", exePath, value);
        }
        else
        {
            spdlog::error("[Registry Compat Fix] Failed to update registry value for {}", exePath);
        }
    }
    else
    {
        spdlog::info("[Registry Compat Fix] Registry entry for {} already contains: {}", exePath, value);
    }

    RegCloseKey(hKey);
}
