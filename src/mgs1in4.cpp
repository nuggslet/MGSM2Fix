#include "mgs1in4.h"
#include "psx.h"

int MGS1in4::MGS4_main(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    PSX::main(cpu);
    unsigned int ra = cpu->Reg[31];
    spdlog::info("[MGS 4] __main: 0x{:08x} -> 0x{:08x}.", address, ra);

    PSXFUNCTION main = PSX::UserHandler(cpu, address);
    return main(cpu, cycle, address);
}

void MGS1in4::EPIOnLoadImage(void *image, unsigned int size)
{
    if (MGS4_LoadImageCount == 0) {
        for (auto & Machine : MachineInstances()) {
            Machine.get().BindModules();
        }
    }
    MGS4_LoadImageCount++;

    MGS4_LoaderPTR = M2Hook::GetInstance().ScanBuffer(
        "00 00 00 00 69 6E 69 74 00 00 00 00",
        -0x30 - reinterpret_cast<uintptr_t>(image),
        image, size, "[MGS 4] mgs_loader_stage"
    );
}

bool MGS1in4::EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args)
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
            unsigned int x = 240 << 1;
            unsigned int y = std::min(h, x);

            *(args[0]) = (2560 - w) >> 1;
            *(args[1]) = (M2Config::iInternalHeight * ((x - y) >> 1)) / x;
            *(args[2]) = (w * M2Config::iInternalHeight) / 240;
            *(args[3]) = (y * M2Config::iInternalHeight) / 240;

            ret = false;
            break;
        }

        case 0x8003: // GET_DIMENSION
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            *(args[0]) = (M2Config::iInternalHeight * 320) / 240;
            *(args[1]) =  M2Config::iInternalHeight;

            ret = false;
            break;
        }

        case 0x8004: // SET_DEVICE
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            unsigned int *res = args[0];

            res[2] = ((M2Config::iInternalHeight * 320) / 240) * 2;
            res[3] =   M2Config::iInternalHeight * 2;
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
