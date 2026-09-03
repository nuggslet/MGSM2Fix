#include "stdafx.h"
#include "patriots.hpp"
#include "m2fix.h"

void Patriots::HDFix()
{
    static std::string sFixName = "MGSHDFix";
    static std::string sFixRepo = "https://github.com/ShizCalev/MGSHDFix/releases";

    std::string sExeName;
    std::filesystem::path sExePath;
    bool bIsLauncher = false;
    HMODULE baseModule = GetModuleHandle(NULL);

    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    if (_stricmp(sExeName.c_str(), "launcher.exe") == 0) {
        bIsLauncher = true;
    }
    sExePath = sExePath.remove_filename();
    sExePath = sExePath.parent_path();

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
    };
    MgsGame eGameType = NONE;

    static const std::map<MgsGame, GameInfo> kGames = {
        { MGS2, { "METAL GEAR SOLID 2",              "METAL GEAR SOLID2.exe", 2131640 } },
        { MGS3, { "METAL GEAR SOLID 3",              "METAL GEAR SOLID3.exe", 2131650 } },
        { MG,   { "METAL GEAR / METAL GEAR 2 (MSX)", "METAL GEAR.exe",        2131680 } },
    };

    if (bIsLauncher)
    {
        for (const auto& [type, info] : kGames)
        {
            auto gamePath = sExePath / info.ExeName;
            if (std::filesystem::exists(gamePath))
            {
                spdlog::info("Detected game: {} (Steam app {}).", info.GameTitle, info.SteamAppId);
                game = &info;
                break;
            }
        }
    }

    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            spdlog::info("Detected game: {} (Steam app {}).", info.GameTitle, info.SteamAppId);
            eGameType = type;
            game = &info;
            break;
        }
    }

    if (eGameType != NONE)
    {
        spdlog::critical("Failed to detect supported game, {} isn't supported by {}.",
            M2Hook::GetInstance().ModuleIdentifier(),
            M2Fix::GetInstance().FixName()
        );

        std::string title = fmt::format(
            "{}: Unsupported Game ({})",
            M2Fix::GetInstance().FixName(),
            sFixName
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

void Patriots::PatriotFix()
{
    static std::string sFixName = "MGSPatriotFix";
    static std::string sFixRepo = "https://github.com/ShizCalev/MGSPatriotFix/releases";

    std::filesystem::path sRootPath;
    std::string sExeName;
    std::filesystem::path sExePath;
    bool bIsLauncher = false;
    bool bisDatabase = false;
    HMODULE baseModule = GetModuleHandle(NULL);

    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    if (_stricmp(sExeName.c_str(), "launcher.exe") == 0) {
        bIsLauncher = true;
    }
    if (_stricmp(sExeName.c_str(), "MGS4DB.exe") == 0) {
        bisDatabase = true;
    }
    sExePath = sExePath.remove_filename();
    sExePath = sExePath.parent_path();
    sRootPath = sExePath.parent_path();

    struct GameInfo
    {
        std::string GameTitle;
        std::string ExeName;
        std::string GameFolder;
        int SteamAppId;
    };
    const GameInfo* game = nullptr;

    enum MgsGame : std::uint8_t
    {
        NONE     = 0,
        MGS4     = 1 << 0,
        MGSPW    = 1 << 1,
    };
    MgsGame eGameType = NONE;

    static const std::map<MgsGame, GameInfo> kGames = {
        { MGS4,  { "METAL GEAR SOLID 4",             "mgs4.exe",                          "MGS4",  2492670 } },
        { MGSPW, { "METAL GEAR SOLID: PEACE WALKER", "METAL GEAR SOLID PEACE WALKER.exe", "mgspw", 2492660 } },
    };

    if (bIsLauncher || bisDatabase)
    {
        for (const auto& [type, info] : kGames)
        {
            auto gamePath = sRootPath / info.GameFolder / info.ExeName;
            if (std::filesystem::exists(gamePath))
            {
                spdlog::info("Detected game: {} (Steam app {}).", info.GameTitle, info.SteamAppId);
                eGameType = type;
                game = &info;
                break;
            }
        }
    }

    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            spdlog::info("Detected game: {} (Steam app {}).", info.GameTitle, info.SteamAppId);
            eGameType = type;
            game = &info;
            break;
        }
    }

    if (eGameType != NONE)
    {
        spdlog::critical("Failed to detect supported game, {} isn't supported by {}.",
            M2Hook::GetInstance().ModuleIdentifier(),
            M2Fix::GetInstance().FixName()
        );

        std::string title = fmt::format(
            "{}: Unsupported Game ({})",
            M2Fix::GetInstance().FixName(),
            sFixName
        );

        std::string message = fmt::format(
            "{} is not supported by {}, please delete {} and use our sister project, {} instead."
            "\n\n",
            game->GameTitle,
            M2Fix::GetInstance().FixName(),
            M2Fix::GetInstance().FixName(),
            sFixName
        );

        if (eGameType & MGS4) {
            message += fmt::format(
                "{} only supports the Metal Gear Solid flashback sequence in {} and must be installed to the MGS1 sub-folder."
                "\n\n",
                M2Fix::GetInstance().FixName(),
                game->GameTitle
            );
        }

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

void Patriots::Check()
{
    HDFix();
    PatriotFix();
}
