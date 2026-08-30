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
