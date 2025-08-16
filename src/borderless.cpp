#include "borderless.h"

#include "m2fix.h"

void Borderless::Load()
{
    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        {
            M2Hook::GetInstance().Patch(
                "B8 00 00 CE 02 BE 00 00 CA 02", 0,
                "B8 00 00 00 90 BE 00 00 00 90",
                "[Borderless-32A] MWin::CreateWindow"
            );

            break;
        }

        case M2FixGame::Contra:
        case M2FixGame::Dracula:
        case M2FixGame::DraculaAdvance:
        case M2FixGame::Darius:
        case M2FixGame::Darius101:
        {
            bool ret = false;

            ret = M2Hook::GetInstance().Patch(
                "B8 00 00 0B 02 C7 85 70 FF FF FF 00 00 CB 00 74 "
                "0F B8 00 00 0F 02 C7 85 70 FF FF FF 00 00 CF 00", 0,
                "B8 00 00 00 90 C7 85 70 FF FF FF 00 00 CB 00 74 "
                "0F B8 00 00 00 90 C7 85 70 FF FF FF 00 00 CF 00",
                "[Borderless-32B] MWin::CreateWindow"
            );

            if (!ret) {
                ret = M2Hook::GetInstance().Patch(
                    "BA 00 00 CA 12 EB 02 33 D2", 0,
                    "BA 00 00 00 90 90 90 90 90",
                    "[Borderless-32C] MWin::CreateWindow"
                );
            }

            break;
        }

        case M2FixGame::NightStrikers:
        {
            M2Hook::GetInstance().Patch(
                "BE 00 00 CB 02 B9 00 00 CB 00 74 0A BE 00 00 CF "
                "02 B9 00 00 CF 00", 0,
                "BE 00 00 00 90 B9 00 00 CB 00 74 0A BE 00 00 00 "
                "90 B9 00 00 CF 00",
                "[Borderless-32D] MWin::CreateWindow"
            );

            break;
        }

        case M2FixGame::MGSR:
        case M2FixGame::DraculaDominus:
        case M2FixGame::Ray:
        case M2FixGame::Gradius:
        {
            M2Hook::GetInstance().Patch(
                "BE 00 00 CB 02 41 BE 00 00 CB 00 44 ?? ?? ?? ?? "
                "?? ?? 74 0B BE 00 00 CF 02 41 BE 00 00 CF 00", 0,
                "BE 00 00 00 90 41 BE 00 00 00 00 90 90 90 90 90 "
                "90 90 90 90 90 90 90 90 90 90 90 90 90 90 90",
                "[Borderless-64A] MWin::CreateWindow"
            );

            break;
        }

        default: break;
    }
}
