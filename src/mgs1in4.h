#pragma once

#include "m2game.h"
#include "m2hook.h"
#include "m2utils.h"
#include "m2config.h"

#include "psx.h"
#include "analog.h"

class MGS1in4 : public M2Game
{
public:
    MGS1in4() {}

    static auto & GetInstance()
    {
        static MGS1in4 instance;
        return instance;
    }

    virtual std::vector<std::reference_wrapper<M2Machine>> MachineInstances() override
    {
        return { PSX::GetInstance() };
    }

    virtual void Load() override
    {
        M2Utils::DisableWindowsFullscreenOptimization();

        Analog::LoadInstance();
    }

    virtual std::any EPIModuleHook() override
    {
        return MGS4_ModuleTables;
    }

    virtual void EPIOnLoadImage(void *image, unsigned int size) override;

    static int MGS4_main(M2_EmuR3000 *cpu, int cycle, unsigned int address);

private:
    uintptr_t MGS4_GlobalsPTR = 0;
    uintptr_t MGS4_LoaderPTR = 0;
    unsigned int MGS4_LoadImageCount = 0;

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS4_ModuleTable_JP = {
        {0x80097598, MGS4_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS4_ModuleTable_US = {
        {0x80097504, MGS4_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS4_ModuleTable_FR = {
        {0x80097658, MGS4_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS4_ModuleTable_IT = {
        {0x800974F4, MGS4_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS4_ModuleTable_GR = {
        {0x800975F4, MGS4_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS4_ModuleTable_SP = {
        {0x800975C0, MGS4_main},
    };

    const PSX_ModuleTables MGS4_ModuleTables { {
        { "mgs4_jp_r3000",             &MGS4_ModuleTable_JP },
        { "mgs4_us_r3000",             &MGS4_ModuleTable_US },
        { "mgs4_fr_r3000",             &MGS4_ModuleTable_FR },
        { "mgs4_it_r3000",             &MGS4_ModuleTable_IT },
        { "mgs4_gr_r3000",             &MGS4_ModuleTable_GR },
        { "mgs4_sp_r3000",             &MGS4_ModuleTable_SP },
        },
        std::bind([](const char *x, const char *y) {
                return strcmp(x, y) < 0;
            },
            std::placeholders::_1,
            std::placeholders::_2
        )
    };
};
