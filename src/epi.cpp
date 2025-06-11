#include "m2fix.h"
#include "epi.h"

void EPI::Print(const char *fmt, ...)
{
    static std::string buffer;

    va_list va;
    va_start(va, fmt);
    std::vector<char> buf(_vscprintf(fmt, va) + 1, 0);
    char *data = buf.data();
    vsprintf(data, fmt, va);
    va_end(va);

    if (data[strcspn(data, "\r\n")] == 0) {
        buffer += std::string(data);
        return;
    }

    data[strcspn(data, "\r\n")] = 0;
    buffer += std::string(data);
    if (buffer.length() == 0) return;

    spdlog::info("[EPI] [printf] {}", buffer);
    buffer.clear();
}

void EPI::Load()
{
    std::string module;
    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        case M2FixGame::Contra:
        case M2FixGame::Dracula:
        case M2FixGame::DraculaAdvance:
        case M2FixGame::Darius:
        case M2FixGame::Darius101:
        {
            M2Hook::GetInstance(module).Hook(
                "8B 4C 24 04 8D 54 24 08 E8 ?? ?? FF FF 85 C0 74",
                0, Print, "[EPI-32] printf"
            );

            break;
        }

        case M2FixGame::DraculaDominus:
            module = "emu_integration";
            [[fallthrough]];
        case M2FixGame::MGSR:
        case M2FixGame::Ray:
        {
            M2Hook::GetInstance(module).Hook(
                "48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C "
                "89 4C 24 20 48 83 EC 28 48 8D 54 24 38 E8 ?? ?? "
                "?? ?? 48 85 C0 74 08 48 8B C8 E8 ?? ??",
                0, Print, "[EPI-64] printf"
            );

            break;
        }

        default: break;
    }
}
