#include "analog.h"

#include "m2fix.h"

#include "sqemutask.h"
#include "mgs1.h"

#include "sqinput.h"
#include "sqinputhub.h"

unsigned char *Analog_PadPTR;
unsigned int __fastcall Analog_ReadPad(unsigned int addr, unsigned int id, unsigned char *state)
{
    if (Analog_PadPTR != state) {
        spdlog::info("[Emulator] Pad state is {}.", fmt::ptr(state));
    }

    Analog_PadPTR = state;
    return M2Hook::GetInstance().Invoke<unsigned int>(Analog_ReadPad, addr, id, state);
}

void MGS1::AnalogLoop()
{
    if (!Analog_PadPTR) return;

    SQInputHub<Squirk::Standard>::SetDirectionMerge(0);
    SQEmuTask<Squirk::Standard>::SetInputDirectionMerge(0);

    SQInputHub<Squirk::Standard>::SetDeadzone(0.0);
    SQEmuTask<Squirk::Standard>::SetInputDeadzone(0.0);

    SQFloat xL = SQInput<Squirk::Standard>::GetAnalogStickX();
    SQFloat yL = SQInput<Squirk::Standard>::GetAnalogStickY();

    SQFloat xR = SQInput<Squirk::Standard>::GetRightAnalogStickX();
    SQFloat yR = SQInput<Squirk::Standard>::GetRightAnalogStickY();

    // Normalize an axis from (-1, 1) to (0, 255) with 128 = center
    // https://github.com/grumpycoders/pcsx-redux/blob/a072e38d78c12a4ce1dadf951d9cdfd7ea59220b/src/core/pad.cc#L664-L673
    const auto axisToUint8 = [](float axis) {
        constexpr float scale = 1.3f;
        const float scaledValue = std::clamp<float>(axis * scale, -1.0f, 1.0f);
        return (uint8_t)(std::clamp<float>(std::round(((scaledValue + 1.0f) / 2.0f) * 255.0f), 0.0f, 255.0f));
    };

    if (SQSystemData<Squirk::Standard>::SettingPad::GetPlaySide_MGS1() == 0) {
        Analog_PadPTR[0x44] = axisToUint8(xL);
        Analog_PadPTR[0x45] = axisToUint8(yL);
        Analog_PadPTR[0x46] = axisToUint8(xR);
        Analog_PadPTR[0x47] = axisToUint8(yR);

        for (unsigned int i = 0x48; i < 0x54; i++) {
            Analog_PadPTR[i] = 128;
        }
    }
    else
    {
        for (unsigned int i = 0x44; i < 0x4C; i++) {
            Analog_PadPTR[i] = 128;
        }

        Analog_PadPTR[0x4C] = axisToUint8(xL);
        Analog_PadPTR[0x4D] = axisToUint8(yL);
        Analog_PadPTR[0x4E] = axisToUint8(xR);
        Analog_PadPTR[0x4F] = axisToUint8(yR);

        for (unsigned int i = 0x50; i < 0x54; i++) {
            Analog_PadPTR[i] = 128;
        }
    }
}

void Analog::Load()
{
    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        {
            M2Hook::GetInstance().Hook(
                "C7 44 24 08 F3 5A 00 00 C7 44 24 0C 00 00 00 00",
                0, Analog_ReadPad, "[Analog] sio_read_pad"
            );

            M2Hook::GetInstance().Patch(
                "66 89 4F 0C F3 0F 2C C0 0F B7 C0 66 89 47 0E 75", 0,
                "66 89 4F 08 F3 0F 2C C0 0F B7 C0 66 89 47 0A 75",
                "[Analog] MInputHubDM::GetWin"
            );

            M2Hook::GetInstance().Patch(
                "88 4C 10 44 83 FB 06 7C 86 8B 44 24 18 45 89 6C", 0,
                "90 90 90 90",
                "[Analog] sio_update_pad"
            );

            break;
        }

        default: break;
    }
}
