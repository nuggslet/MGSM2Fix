#include "analog.h"

#include "m2fix.h"

#include "sqemutask.h"
#include "mgs1.h"

#include "sqinput.h"
#include "sqinputhub.h"

unsigned char *Analog_PadPTR;
#ifndef _WIN64
uintptr_t __fastcall Analog_ReadPad(unsigned char *addr, unsigned int id, unsigned char *state)
#else
uintptr_t __fastcall Analog_ReadPad(unsigned char *state, unsigned int id, unsigned char *addr)
#endif
{
    if (Analog_PadPTR != state) {
        spdlog::info("[Emulator] Pad state is {}.", fmt::ptr(state));
    }

    Analog_PadPTR = state;
#ifndef _WIN64
    return M2Hook::GetInstance().Invoke<uintptr_t>(Analog_ReadPad, addr, id, state);
#else
    return M2Hook::GetInstance().Invoke<uintptr_t>(Analog_ReadPad, state, id, addr);
#endif
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

#ifndef _WIN64
    unsigned base = 0x44;
#else
    unsigned base = 0x58;
#endif

    // Normalize an axis from (-1, 1) to (0, 255) with 128 = center
    // https://github.com/grumpycoders/pcsx-redux/blob/a072e38d78c12a4ce1dadf951d9cdfd7ea59220b/src/core/pad.cc#L664-L673
    const auto axisToUint8 = [](float axis) {
        constexpr float scale = 1.3f;
        const float scaledValue = std::clamp<float>(axis * scale, -1.0f, 1.0f);
        return (uint8_t)(std::clamp<float>(std::round(((scaledValue + 1.0f) / 2.0f) * 255.0f), 0.0f, 255.0f));
    };

    if (SQSystemData<Squirk::Standard>::SettingPad::GetPlaySide_MGS1() == 0) {
        Analog_PadPTR[base + 0] = axisToUint8(xL);
        Analog_PadPTR[base + 1] = axisToUint8(yL);
        Analog_PadPTR[base + 2] = axisToUint8(xR);
        Analog_PadPTR[base + 3] = axisToUint8(yR);

        for (unsigned int i = base + 4; i < base + 16; i++) {
            Analog_PadPTR[i] = 128;
        }
    }
    else
    {
        for (unsigned int i = base + 0; i < base + 8; i++) {
            Analog_PadPTR[i] = 128;
        }

        Analog_PadPTR[base + 8] = axisToUint8(xL);
        Analog_PadPTR[base + 9] = axisToUint8(yL);
        Analog_PadPTR[base + 10] = axisToUint8(xR);
        Analog_PadPTR[base + 11] = axisToUint8(yR);

        for (unsigned int i = base + 12; i < base + 16; i++) {
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
#ifndef _WIN64
            if (M2Config::bAnalog.has_value() && M2Config::bAnalog.value())
            {
                M2Hook::GetInstance().Hook(
                    "C7 44 24 08 F3 5A 00 00 C7 44 24 0C 00 00 00 00",
                    0, Analog_ReadPad, "[Analog] sio_read_pad"
                );

                M2Hook::GetInstance().Patch(
                    "88 4C 10 44 83 FB 06 7C 86 8B 44 24 18 45 89 6C", 0,
                    "90 90 90 90",
                    "[Analog] sio_update_pad"
                );

                if (M2Config::bSwapSticks.has_value() && M2Config::bSwapSticks.value())
                {
                    M2Hook::GetInstance().Patch(
                        "66 89 4F 0C F3 0F 2C C0 0F B7 C0 66 89 47 0E 75", 0,
                        "66 89 4F 08 F3 0F 2C C0 0F B7 C0 66 89 47 0A 75",
                        "[Analog] MInputHubDM::GetWin"
                    );
                }
            }

            if (M2Config::bRemoveDeadzone)
            {
                M2Hook::GetInstance().Patch(
                    "C7 44 24 1C 52 3D 00 00", 0,
                    "C7 44 24 1C 00 00 00 00",
                    "[Analog] MInputSteam::DeadzoneAxis1"
                );

                M2Hook::GetInstance().Patch(
                    "C7 44 24 1C 9A 40 00 00", 0,
                    "C7 44 24 1C 00 00 00 00",
                    "[Analog] MInputSteam::DeadzoneAxis2"
                );
            }
#else
            if (M2Config::bAnalog.has_value() && M2Config::bAnalog.value())
            {
                M2Hook::GetInstance().Hook(
                    "48 C7 45 F0 F3 5A 00 00 41 0F B6 54 09 45 44 0F",
                    -0x30, Analog_ReadPad, "[Analog] sio_read_pad"
                );

                M2Hook::GetInstance().Patch(
                    "88 44 2F 58 83 FE 02 0F 8C 77 FD FF FF 48 8B 5C", 0,
                    "90 90 90 90",
                    "[Analog] sio_update_pad"
                );
            }

            if (M2Config::bRemoveDeadzone)
            {
                M2Hook::GetInstance().Patch(
                    "41 BB 52 3D 00 00", 0,
                    "41 BB 00 00 00 00",
                    "[Analog] MInputSteam::DeadzoneAxis1"
                );

                M2Hook::GetInstance().Patch(
                    "41 BB E2 43 00 00", 0,
                    "41 BB 00 00 00 00",
                    "[Analog] MInputSteam::DeadzoneAxis2"
                );
            }
#endif

            break;
        }

        default: break;
    }
}
