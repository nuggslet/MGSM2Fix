#include "stdafx.h"
#include "patriots.hpp"
#include "m2fix.h"
#include <shellapi.h>

void Patriots::Check()
{
    static std::string sFixName = "MGSHDFix";
    static std::string sFixRepo = "https://github.com/Lyall/MGSHDFix/releases";

    std::string sExeName;
    std::filesystem::path sExePath;
    bool bIsLauncher = false;
    HMODULE baseModule = GetModuleHandle(NULL);

    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    if (_stricmp(sExeName.c_str(), "launcher.exe") == 0)
    {
        bIsLauncher = true;
    }
    sExePath = sExePath.remove_filename();

    struct GameInfo
    {
        std::string GameTitle;
        std::string ExeName;
        int SteamAppId;
    };
    const GameInfo* game = nullptr;

    enum MgsGame : std::uint8_t
    {
        NONE     = 0,
        MGS2     = 1 << 0,
        MGS3     = 1 << 1,
        MG       = 1 << 2,
        LAUNCHER = 1 << 3,
        UNKNOWN  = 1 << 4
    };
    MgsGame eGameType = UNKNOWN;

    static const std::map<MgsGame, GameInfo> kGames = {
        { MGS2, { "Metal Gear Solid 2",              "METAL GEAR SOLID2.exe", 2131640 } },
        { MGS3, { "Metal Gear Solid 3",              "METAL GEAR SOLID3.exe", 2131650 } },
        { MG,   { "Metal Gear / Metal Gear 2 (MSX)", "METAL GEAR.exe",        2131680 } },
    };

    eGameType = UNKNOWN;
    if (bIsLauncher)
    {
        for (const auto& [type, info] : kGames)
        {
            auto gamePath = sExePath.parent_path() / info.ExeName;
            if (std::filesystem::exists(gamePath))
            {
                spdlog::info("Detected launcher for game: {} (Steam app {}).", info.GameTitle.c_str(), info.SteamAppId);
                eGameType = LAUNCHER;
                game = &info;
                break;
            }
        }
    }

    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            spdlog::info("Detected game: {} (Steam app {}).", info.GameTitle.c_str(), info.SteamAppId);
            eGameType = type;
            game = &info;
            break;
        }
    }

    if (eGameType != UNKNOWN) {
        std::string title = fmt::format(
            "{}: Unsupported Game",
            M2Fix::GetInstance().FixName()
        );

        std::string message = fmt::format(
            "{} is not supported by {}, please delete {} and use our sister project, {} instead."
            "\n\n",
            game->GameTitle,
            M2Fix::GetInstance().FixName(),
            M2Fix::GetInstance().FixName(),
            sFixName
        );

        if (!M2Utils::IsSteamOS()) {
            message += fmt::format(
                "Want to visit the {} download page?",
                sFixName
            );
            if (MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_ICONERROR | MB_OKCANCEL) == IDOK) {
                ShellExecuteA(0, NULL, sFixRepo.c_str(), NULL, NULL, SW_SHOWDEFAULT);
            }
        }
        else {
            message += fmt::format(
                "Visit {} for {} downloads.",
                sFixRepo,
                sFixName
            );
            MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
        }

        std::exit(1);
    }
}
