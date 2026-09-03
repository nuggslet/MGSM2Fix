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

#ifdef _WIN64
        M2Hook::GetInstance().MidHook(
            "4C 89 7C 24 58 48 89 74 24 50 4C 89 7C 24 48 4C", 0x2B,
            MGS4_CreateWindow, "[MGS 4] MGS4::CreateWindow"
        );
#endif

        if (M2Config::bInternalEnabled)
        {
#ifdef _WIN64
            M2Hook::GetInstance().MidHook(
                "BA 01 80 00 00 C7 44 24 70 40 01 00 00 48 8B CE C7 "
                "44 24 74 F0 00 00 00", 0x18,
                MGS4_Draw, "[MGS 4] MGS4::Draw"
            );

            M2Hook::GetInstance().MidHook(
                "41 C7 47 30 40 01 00 00 41 C7 47 34 F0 00 00 00", 0x10,
                MGS4_DisplaySize, "[MGS 4] MGS4::DisplaySize"
            );

            M2Hook::GetInstance().MidHook(
                "8B 84 24 08 01 00 00 45 0F 57 DB F3 4C 0F 2A D8", 0,
                MGS4_DriverPosX, "[MGS 4] MGS4::DriverPosX"
            );

            M2Hook::GetInstance().MidHook(
                "8B 84 24 10 01 00 00 45 0F 57 D2 F3 4C 0F 2A D0", 0,
                MGS4_DriverPosY, "[MGS 4] MGS4::DriverPosY"
            );

            M2Hook::GetInstance().MidHook(
                "66 48 0F 7E C2 FF 50 38 49 8B 96 30 01 00 00 49", 0,
                MGS4_ViewPos, "[MGS 4] MGS4::ViewPos"
            );

            M2Hook::GetInstance().MidHook(
                "49 8B 86 18 01 00 00 F3 0F 11 48 50 F3 0F 11 48", 0,
                MGS4_ViewSize, "[MGS 4] MGS4::ViewSize"
            );
#endif
        }
    }

    virtual std::any EPIModuleHook() override
    {
        return MGS4_ModuleTables;
    }

    virtual void EPIOnLoadImage(void *image, unsigned int size) override;
    virtual bool EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args) override;

    static int MGS4_main(M2_EmuR3000 *cpu, int cycle, unsigned int address);

private:
#ifdef _WIN64
    static void MGS4_CreateWindow(safetyhook::Context & ctx) {
        ctx.r8 = reinterpret_cast<uintptr_t>("METAL GEAR SOLID 4 GUNS OF THE PATRIOTS");
    };

    static void MGS4_Draw(safetyhook::Context & ctx) {
        auto *rsp = reinterpret_cast<unsigned int *>(ctx.rsp);
		rsp[28] = (M2Config::iInternalHeight * 320) / 240;
		rsp[29] =  M2Config::iInternalHeight;
    };

    static void MGS4_DisplaySize(safetyhook::Context & ctx) {
        auto *r15 = reinterpret_cast<unsigned int *>(ctx.r15);
        r15[12] = (M2Config::iInternalHeight * 320) / 240;
        r15[13] = M2Config::iInternalHeight;
    };

    static void MGS4_DriverPosX(safetyhook::Context & ctx) {
        ctx.xmm9.f32[0] += 160.0f;
        ctx.xmm9.f32[0] -= ((M2Config::iInternalHeight * 320) / 240) * 0.5f;
    };

    static void MGS4_DriverPosY(safetyhook::Context & ctx) {
        ctx.xmm7.f32[0] += 120.0f;
        ctx.xmm7.f32[0] -= M2Config::iInternalHeight * 0.5f;
    };

    static void MGS4_ViewPos(safetyhook::Context & ctx) {
        ctx.xmm0.f32[0] = ((M2Config::iInternalHeight * 320) / 240) * -0.5f;
        ctx.xmm0.f32[1] = M2Config::iInternalHeight * -0.5f;
        ctx.xmm1.f32[0] = M2Config::iInternalHeight * -0.5f;
    };

    static void MGS4_ViewSize(safetyhook::Context & ctx) {
        ctx.xmm1.f32[0] *= 240.0f;
        ctx.xmm1.f32[0] /= M2Config::iInternalHeight;
    };
#endif

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
