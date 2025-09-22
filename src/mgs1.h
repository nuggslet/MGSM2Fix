#pragma once

#include "m2game.h"
#include "m2hook.h"
#include "m2config.h"

#include "psx.h"
#include "analog.h"
#include "d3d11.h"

#include "sqhook.h"

#include "sqemutask.h"
#include "sqtitleprof.h"

class MGS1 : public M2Game
{
public:
    MGS1() {}

    static auto & GetInstance()
    {
        static MGS1 instance;
        return instance;
    }

    virtual std::vector<std::reference_wrapper<M2Machine>> MachineInstances() override
    {
        return { PSX::GetInstance() };
    }

    virtual void Load() override
    {
        D3D11::LoadInstance();

        DisableWindowsFullscreenOptimization();

        if (M2Config::bAnalogMode) {
            Analog::LoadInstance();
        }

        SQHook<Squirk::Standard>::SetReturnHook("set_playside_mgs", SQReturn_set_playside_mgs);

        if (M2Config::bPatchesRemoveUnderpants) {
            for (auto & MGS1_FileFilter_Underpant : MGS1_FileFilter_Underpants) {
                SQHook<Squirk::Standard>::SetPatchFileFilter(MGS1_FileFilter_Underpant);
            }
        }

        if (M2Config::bPatchesRestoreGhosts) {
            SQHook<Squirk::Standard>::SetPatchFileFilter(MGS1_FileFilter_Ghosts);
        }

        if (M2Config::bPatchesRestoreMedicine) {
            SQHook<Squirk::Standard>::SetPatchDataFilter(MGS1_DataFilter_Medicine);
        }

        for (Ketchup_TitleInfo & title : MGS1_Ketchup) {
            for (Ketchup_VersionInfo & version : title.versions) {
                for (Ketchup_DiskInfo & disk : version.disks) {
                    auto path = M2Hook::GetInstance().ModuleLocation().parent_path();
                    path /= "mods";
                    path /= title.name;
                    if (title.versions.size() > 1) {
                        path /= version.name;
                    }
                    if (version.disks.size() > 1) {
                        path /= std::to_string(disk.id);
                    }
                    std::error_code ec;
                    bool success = std::filesystem::create_directories(path, ec);
                }
            }
        }
    }

    virtual std::pair<std::any, const char *> EPIModuleHook() override
    {
        return { MGS1_ModuleTables, "mgs_r3000_int" };
    }


    virtual std::vector<Ketchup_TitleInfo> *SQKetchupHook() override
    {
        return &MGS1_Ketchup;
    }

    virtual void GWRenderGeometry(int & gw_width, int & gw_height, int & fb_width, int & fb_height, int & img_width, int & img_height) override
    {
        gw_width   = 1024;
        gw_height  = 1024;
        fb_width   = 320;
        fb_height  = 256;
        img_width  = 320;
        img_height = PSX::VideoMode ? 256 : 224;
    }

    virtual bool GWBlank() override;

    virtual void SQOnMemoryDefine() override;
    virtual void SQOnUpdateGadgets() override;
    virtual void EPIOnLoadImage(void *image, size_t size) override;
    virtual bool EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args) override;

    static int MGS1_main(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_s03a_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_s03d_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address);

    static SQInteger SQReturn_set_playside_mgs(HSQUIRRELVM<Squirk::Standard> v);

    static void AnalogLoop();

private:
    uintptr_t MGS1_GlobalsPTR = 0;
    uintptr_t MGS1_LoaderPTR = 0;

    int MGS1_Blank = 0;

    static void DisableWindowsFullscreenOptimization();

    const std::vector<std::string> MGS1_FileFilter_Underpants = {
        "0046a5", "0046a6",
        "0057c3", "0057c4", "0057c5",
        "0099fc", "0099fd",
    };

    const std::string MGS1_FileFilter_Ghosts = "shinrei";

    const std::vector<unsigned char> MGS1_DataFilter_Medicine = {
        0, 152, 0, 72, 152, 72, 152, 152, 152
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_ES = {
        {0x800D322C, MGS1_s03a_disable_mosaic},
        {0x800D9D80, MGS1_s03d_disable_mosaic},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_DE = {
        {0x800D3618, MGS1_s03a_disable_mosaic},
        {0x800DA16C, MGS1_s03d_disable_mosaic},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_IT = {
        {0x800D31A8, MGS1_s03a_disable_mosaic},
        {0x800D9CFC, MGS1_s03d_disable_mosaic},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_FR = {
        {0x800D36D0, MGS1_s03a_disable_mosaic},
        {0x800DA224, MGS1_s03d_disable_mosaic},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_UK = {
        {0x800D3118, MGS1_s03a_disable_mosaic},
        {0x800D9C6C, MGS1_s03d_disable_mosaic},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_US = {
        {0x800D4AC8, MGS1_s03a_disable_mosaic},
        {0x800DB61C, MGS1_s03d_disable_mosaic},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_Integral = {
        {0x80098F14, MGS1_main},
    };

    const PSX_ModuleTables MGS1_ModuleTables { {
        { "mgs_r3000_es",  &MGS1_ModuleTable_ES },
        { "mgs_r3000_de",  &MGS1_ModuleTable_DE },
        { "mgs_r3000_it",  &MGS1_ModuleTable_IT },
        { "mgs_r3000_fr",  &MGS1_ModuleTable_FR },
        { "mgs_r3000_uk",  &MGS1_ModuleTable_UK },
        { "mgs_r3000_us",  &MGS1_ModuleTable_US },
        { "mgs_r3000_int", &MGS1_ModuleTable_Integral },
        },
        std::bind([](const char *x, const char *y) {
                return strcmp(x, y) < 0;
            },
            std::placeholders::_1,
            std::placeholders::_2
        )
    };

    std::vector<Ketchup_TitleInfo> MGS1_Ketchup = {
        {99, "INTEGRAL", {
            {"INTEGRAL", {
                {0, 0x131D2238, Ketchup<>::PSX_DiskRange(0x9C000)},
                {1, 0x0EB38078, Ketchup<>::PSX_DiskRange(0x9C000)},
            }},
            {"VR-DISK", {
                {0, 0x000865F8, Ketchup<>::PSX_DiskRange(0x99800)},
            }},
        }},
        {101, "VR-DISK_US", {
            {"USA", {
                {0, 0x0000E5C8, Ketchup<>::PSX_DiskRange(0x9C800)},
            }},
        }},
        {102, "VR-DISK_EU", {
            {"EUROPE", {
                {0, 0x0018CCA8, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {980, "MGS1_JP", {
            {"JAPAN", {
                {0, 0x127F1478, Ketchup<>::PSX_DiskRange(0x9C800)},
                {1, 0x0E0B4AA8, Ketchup<>::PSX_DiskRange(0x9C800)},
            }},
        }},
        {981, "MGS1_US", {
            {"USA", {
                {0, 0x0000E5C8, Ketchup<>::PSX_DiskRange(0x9E800)},
                {1, 0x0000E5C8, Ketchup<>::PSX_DiskRange(0x9E800)},
            }},
        }},
        {982, "MGS1_UK", {
            {"UK", {
                {0, 0x119BF0C8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {983, "MGS1_DE", {
            {"GERMANY", {
                {0, 0x119BE798, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {984, "MGS1_FR", {
            {"FRANCE", {
                {0, 0x119BF9F8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {985, "MGS1_IT", {
            {"ITALY", {
                {0, 0x119BF0C8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {986, "MGS1_ES", {
            {"SPAIN", {
                {0, 0x119BF9F8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D441C08, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
    };
};
