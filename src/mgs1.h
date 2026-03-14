#pragma once

#include "m2game.h"
#include "m2hook.h"
#include "m2config.h"

#include "psx.h"
#ifndef _WIN64
#include "analog.h"
#include "d3d11.h"
#endif

#include "sqemutask.h"
#include "sqtitleprof.h"

#include "sqhook.h"

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
        DisableWindowsFullscreenOptimization();

#ifndef _WIN64
        static D3D11 d3d11;
        D3D11::LoadInstance(&d3d11);

        if (M2Config::bAnalogMode) {
            Analog::LoadInstance();
        }
#endif

        SQHook<Squirk::Standard>::SetReturnHook("set_playside_mgs", SQReturn_set_playside_mgs);

        if (M2Config::bPatchesRemoveUnderpants) {
            for (auto & MGS1_FileBlacklist_Underpant : MGS1_FileBlacklist_Underpants) {
                SQHook<Squirk::Standard>::SetPatchFileBlacklist(MGS1_FileBlacklist_Underpant);
            }
        }

        if (M2Config::bPatchesRestoreGhosts) {
            SQHook<Squirk::Standard>::SetPatchFileBlacklist(MGS1_FileBlacklist_Ghosts);
        }

        if (M2Config::bPatchesRestoreMedicine) {
            SQHook<Squirk::Standard>::SetPatchDataBlacklist(MGS1_DataBlacklist_Medicine);
        }

        if (M2Config::bPatchesDisableFont) {
            for (auto & MGS1_TextureWhitelist_Font : MGS1_TextureWhitelist_Fonts) {
                SQHook<Squirk::Standard>::SetTextureWhitelist(MGS1_TextureWhitelist_Font);
            }
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
                    std::filesystem::create_directories(path, ec);
                }
            }
        }
    }

    virtual std::any EPIModuleHook() override
    {
        return MGS1_ModuleTables;
    }


    virtual std::vector<Ketchup_TitleInfo> *SQKetchupHook() override
    {
        return &MGS1_Ketchup;
    }

#ifndef _WIN64
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
#endif

    static SQInteger MGS1_EmuTask_getHeight(HSQUIRRELVM<Squirk::Standard> v)
    {
        auto id = SQGlobals<Squirk::Standard>::GetTitle();
        static std::set<int> NTSC = { 980, 981, 99, 101 };

        SQObjectPtr<Squirk::Standard> func = SQ_EmuTask_getHeight;
        SQObjectPtr<Squirk::Standard> res = {};
        v->Call(func, 1, v->_stackbase, res, false);

        SQFloat height = _float(res);
        if (M2Config::bInternalBorderless && NTSC.contains(id)) {
            height = (height * 224.0f) / 240.0f;
        }

        sq_pushfloat(v, height);
        return 1;
    }

    virtual void SQOnInitSystemFirst() override
    {
        SQHook<Squirk::Standard>::HookMethod(
            Sqrat::DefaultVM<Squirk::Standard>::Get(),
            "EmuTask",
            "getHeight",
            MGS1_EmuTask_getHeight,
            &SQ_EmuTask_getHeight
        );
    }

    virtual void SQOnMemoryDefine() override;
    virtual void SQOnUpdateGadgets() override;
    virtual void EPIOnLoadImage(void *image, unsigned int size) override;
    virtual bool EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args) override;

    static int MGS1_main(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_s03a_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_s03d_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_font(M2_EmuR3000 *cpu, int cycle, unsigned int address);

    static SQInteger SQReturn_set_playside_mgs(HSQUIRRELVM<Squirk::Standard> v);

#ifndef _WIN64
    static void AnalogLoop();
#endif

private:
    uintptr_t MGS1_GlobalsPTR = 0;
    uintptr_t MGS1_LoaderPTR = 0;
    static inline HSQOBJECT<Squirk::Standard> SQ_EmuTask_getHeight = {};

#ifndef _WIN64
    int MGS1_Blank = 0;
#endif

    static void DisableWindowsFullscreenOptimization();

    const std::vector<std::string> MGS1_FileBlacklist_Underpants = {
        "0046a5", "0046a6",
        "0057c3", "0057c4", "0057c5",
        "0099fc", "0099fd",
    };

    const std::string MGS1_FileBlacklist_Ghosts = "shinrei";

    const std::vector<unsigned char> MGS1_DataBlacklist_Medicine = {
        0, 152, 0, 72, 152, 72, 152, 152, 152
    };

    const std::vector<unsigned int> MGS1_TextureWhitelist_Fonts = {
        0x0026f0, 0x00c1bd, 0x00e786,
        0x003528, 0x00352e, 0x00515b, 0x00c76a,
        0x00061d, 0x00c148,
        0x00cc46, 0x00cc47, 0x00fbf1, 0x00fbf7,
        0x00ac92
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_VR_EU = {
        {0x800196FC, MGS1_main},
        {0x80047D6C, MGS1_font},
        {0x800498E0, MGS1_font},
        {0x800499A8, MGS1_font},
        {0x80049B40, MGS1_font},
        {0x80048B68, MGS1_font},
        {0x80049CA0, MGS1_font},
        {0x80049D00, MGS1_font},
        {0x485E8,    MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_VR_US = {
        {0x800196F4, MGS1_main},
        {0x80047C68, MGS1_font},
        {0x800497DC, MGS1_font},
        {0x800498A4, MGS1_font},
        {0x80049A3C, MGS1_font},
        {0x80048A64, MGS1_font},
        {0x80049B9C, MGS1_font},
        {0x80049BFC, MGS1_font},
        {0x484E4,    MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_VR_JP = {
        {0x80016630, MGS1_main},
        {0x80044C88, MGS1_font},
        {0x80046664, MGS1_font},
        {0x8004670C, MGS1_font},
        {0x80046864, MGS1_font},
        {0x80045514, MGS1_font},
        {0x80045A78, MGS1_font},
        {0x800469C4, MGS1_font},
        {0x80046A24, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_Integral = {
        {0x80098F14, MGS1_main},
        {0x80044BC0, MGS1_font},
        {0x8004659C, MGS1_font},
        {0x80046644, MGS1_font},
        {0x8004679C, MGS1_font},
        {0x8004544C, MGS1_font},
        {0x800459B0, MGS1_font},
        {0x800468FC, MGS1_font},
        {0x8004695C, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_ES = {
        {0x8001682C, MGS1_main},
        {0x800D322C, MGS1_s03a_disable_mosaic},
        {0x800D9D80, MGS1_s03d_disable_mosaic},
        {0x80045D18, MGS1_font},
        {0x800477FC, MGS1_font},
        {0x800478B4, MGS1_font},
        {0x80047A28, MGS1_font},
        {0x800465BC, MGS1_font},
        {0x80046B28, MGS1_font},
        {0x80047B8C, MGS1_font},
        {0x80047BEC, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_DE = {
        {0x800167C8, MGS1_main},
        {0x800D3618, MGS1_s03a_disable_mosaic},
        {0x800DA16C, MGS1_s03d_disable_mosaic},
        {0x80045CAC, MGS1_font},
        {0x80047790, MGS1_font},
        {0x80047848, MGS1_font},
        {0x800479BC, MGS1_font},
        {0x80046550, MGS1_font},
        {0x80046ABC, MGS1_font},
        {0x80047B20, MGS1_font},
        {0x80047B80, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_IT = {
        {0x80016764, MGS1_main},
        {0x800D31A8, MGS1_s03a_disable_mosaic},
        {0x800D9CFC, MGS1_s03d_disable_mosaic},
        {0x80045C48, MGS1_font},
        {0x8004772C, MGS1_font},
        {0x800477E4, MGS1_font},
        {0x80047958, MGS1_font},
        {0x800464EC, MGS1_font},
        {0x80046A58, MGS1_font},
        {0x80047ABC, MGS1_font},
        {0x80047B1C, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_FR = {
        {0x80016890, MGS1_main},
        {0x800D36D0, MGS1_s03a_disable_mosaic},
        {0x800DA224, MGS1_s03d_disable_mosaic},
        {0x80045D58, MGS1_font},
        {0x8004783C, MGS1_font},
        {0x800478F4, MGS1_font},
        {0x80047A68, MGS1_font},
        {0x800465FC, MGS1_font},
        {0x80046B68, MGS1_font},
        {0x80047BCC, MGS1_font},
        {0x80047C2C, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_UK = {
        {0x800165F4, MGS1_main},
        {0x800D3118, MGS1_s03a_disable_mosaic},
        {0x800D9C6C, MGS1_s03d_disable_mosaic},
        {0x80045ABC, MGS1_font},
        {0x80047448, MGS1_font},
        {0x800474E4, MGS1_font},
        {0x80047620, MGS1_font},
        {0x8004636C, MGS1_font},
        {0x800468C0, MGS1_font},
        {0x80047784, MGS1_font},
        {0x800477E4, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_US = {
        {0x80016D48, MGS1_main},
        {0x800D4AC8, MGS1_s03a_disable_mosaic},
        {0x800DB61C, MGS1_s03d_disable_mosaic},
        {0x800471D0, MGS1_font},
        {0x80048B5C, MGS1_font},
        {0x80048BF8, MGS1_font},
        {0x80048D34, MGS1_font},
        {0x80047A80, MGS1_font},
        {0x80047FD4, MGS1_font},
        {0x80048E98, MGS1_font},
        {0x80048EF8, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_JP = {
        {0x80016818, MGS1_main},
        {0x80045978, MGS1_font},
        {0x80047304, MGS1_font},
        {0x800473A0, MGS1_font},
        {0x800474DC, MGS1_font},
        {0x8004622C, MGS1_font},
        {0x8004677C, MGS1_font},
        {0x80047640, MGS1_font},
        {0x800476A0, MGS1_font},
    };

    const PSX_ModuleTables MGS1_ModuleTables { {
        { "mgs_r3000_vr_eu", &MGS1_ModuleTable_VR_EU },
        { "mgs_r3000_vr_us", &MGS1_ModuleTable_VR_US },
        { "mgs_r3000_vr_jp", &MGS1_ModuleTable_VR_JP },
        { "mgs_r3000_int",   &MGS1_ModuleTable_Integral },
        { "mgs_r3000_es",    &MGS1_ModuleTable_ES },
        { "mgs_r3000_de",    &MGS1_ModuleTable_DE },
        { "mgs_r3000_it",    &MGS1_ModuleTable_IT },
        { "mgs_r3000_fr",    &MGS1_ModuleTable_FR },
        { "mgs_r3000_uk",    &MGS1_ModuleTable_UK },
        { "mgs_r3000_us",    &MGS1_ModuleTable_US },
        { "mgs_r3000_jp",    &MGS1_ModuleTable_JP },
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
