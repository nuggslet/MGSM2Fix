#pragma once

#include "m2hook.h"
#include "m2game.h"

#include "m2utils.h"
#include "m2config.h"

#include "sqhook.h"
#include "epi.h"

#include "config.h"
#include "borderless.h"
#include "d3d11.h"
#include "versionchecker.h"

enum class M2FixGame
{
    Unknown,
    MGS1,
    MGSR,
    Contra,
    Dracula,
    DraculaAdvance,
    DraculaDominus,
    Ray,
    Darius,
    Darius101,
    Gradius,
    NightStrikers,
};

struct M2FixInfo
{
    int id;
    std::string_view M2classname;
    std::string_view title;
    M2Game & game;
};

class M2Fix
{
public:
    M2Fix() {}

    static auto & GetInstance()
    {
        static M2Fix instance;
        return instance;
    }

    static std::string FixName()
    {
        return std::string(GetInstance().m_sFixName);
    }

    static std::string FixVersion()
    {
        return std::string(GetInstance().m_sFixVer);
    }

    static std::string LogFile()
    {
        return std::string(GetInstance().m_sLogFile);
    }

    static std::string ConfigFile()
    {
        return std::string(GetInstance().m_sConfigFile);
    }

    static M2FixGame Game()
    {
        return GetInstance().m_eGame;
    }

    static M2FixInfo *GameInfo()
    {
        return GetInstance().m_kGame;
    }

    static M2Game & GameInstance()
    {
        return GameInfo()->game;
    }

    static void Main(HINSTANCE hinstDLL)
    {
        M2Hook::Attach(hinstDLL);

        M2Fix::CheckModules();
        M2Fix::Logging();
        M2Utils::LogSystemInfo();
        M2Utils::CompatibilityWarnings();

        M2Config::LoadInstance();
        if (M2Config::bConsole) M2Fix::Console();
        if (M2Config::bBreak) __debugbreak();

        if (M2Fix::DetectGame())
        {
            spdlog::info("{} v{} started for {}.",
                M2Fix::FixName(),
                M2Fix::FixVersion(),
                M2Fix::GameInfo()->title
            );
            spdlog::info("----------");

            EPI::LoadInstance();
            SQHook<>::LoadInstance();

            if (M2Config::bExternalEnabled) Config::LoadInstance();
            if (M2Config::bBorderlessMode) Borderless::LoadInstance();

            auto & Game = M2Fix::GameInstance();
            for (auto & Machine : Game.MachineInstances()) {
                Machine.get().Load();
            }
            Game.Load();

            spdlog::info("----------");
        }

        if (M2Config::bShouldCheckForUpdates)
        {
            LatestVersionChecker checker;
            checker.checkForUpdates();
        }
    }

    static void Logging()
    {
        auto path = M2Hook::GetInstance().ModuleLocation().parent_path();

        try {
            spdlog::init_thread_pool(8192, 1);
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((path / LogFile()).string(), true);
            auto logger = std::make_shared<spdlog::async_logger>(FixName(), sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            spdlog::set_default_logger(logger);
        }
        catch (const spdlog::spdlog_ex & ex) {
            std::string message = fmt::format("Log initialisation failed: {}.", ex.what());
            MessageBox(NULL, message.c_str(), m_sFixName.data(), MB_ICONERROR | MB_OK);
            std::exit(1);
        }

        spdlog::flush_on(spdlog::level::debug);
        spdlog::info("{} v{} loaded.", FixName(), FixVersion());
        spdlog::info("----------");
    }

    static void CheckModules()
    {
        auto path = M2Hook::GetInstance(".").ModuleLocation().parent_path();
        auto name = M2Hook::GetInstance(".").ModuleLocation().filename();
#ifndef _WIN64
        std::string name_double = fmt::format("{}32.asi", m_sFixName);
#else
        std::string name_double = fmt::format("{}64.asi", m_sFixName);
#endif
        bool has_single = std::filesystem::exists(path / fmt::format("{}.asi", m_sFixName));
        bool has_double = std::filesystem::exists(path / name_double);
        if (has_single && has_double) {
            if (name == name_double) {
                std::string message = fmt::format(
                    "{}.asi and {} both exist. One of them must be deleted as the current configuration is loading the mod twice.",
                    m_sFixName, name_double
                );
                MessageBox(NULL, message.c_str(), m_sFixName.data(), MB_ICONERROR | MB_OK);
                std::exit(1);
            }
            Sleep(INFINITE);
        }
    }

    static BOOL WINAPI ConsoleHandler(DWORD dwCtrlType)
    {
        switch (dwCtrlType) {
            case CTRL_C_EVENT:
                __debugbreak();
                return TRUE;
            default: break;
        }
        return FALSE;
    }

    static void Console()
    {
        FILE *dummy = {};
        AllocConsole();
        freopen_s(&dummy, "CONOUT$", "w", stdout);

        auto title = fmt::format("{} v{} - {}", m_sFixName, m_sFixVer, m_kGame->title);
        SetConsoleTitle(title.c_str());

        auto console = CreateFileA("CONOUT$", GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
        );

        auto window = GetConsoleWindow();
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
        );
        ShowWindow(window, SW_NORMAL);

        CONSOLE_FONT_INFOEX font = { sizeof(font) };
        GetCurrentConsoleFontEx(console, false, &font);
        font.dwFontSize.X = 10;
        font.dwFontSize.Y = 10;
        SetCurrentConsoleFontEx(console, false, &font);

        RECT rect = {};
        GetWindowRect(window, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        MoveWindow(window, rect.left, rect.top, width, height, true);

        COLORREF key = {}; BYTE alpha = {}; DWORD flags = {};
        GetLayeredWindowAttributes(window, &key, &alpha, &flags);
        alpha = (256 / 4) * 3;
        flags = LWA_ALPHA;
        SetLayeredWindowAttributes(window, key, alpha, flags);

        SetConsoleCtrlHandler(ConsoleHandler, true);

        auto logger = spdlog::default_logger();
        auto sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
        logger->sinks().push_back(sink);
    }

    static bool DetectGame()
    {
        auto & hook = M2Hook::GetInstance();
        spdlog::info("[Module] Name: {}", hook.ModuleName());
        spdlog::info("[Module] Path: {}", hook.ModulePath());
        spdlog::info("[Module] Address: {}", hook.ModuleAddress());
        spdlog::info("[Module] Timestamp: {} ({})",
            hook.ModuleVersion(),
            hook.ModuleTimestamp()
        );
        spdlog::info("----------");

        std::string_view classname = M2Hook::ReadUntilTabOrCRLF(M2Hook::GetInstance().Scan("43 4C 41 53 53 4E 41 4D 45 20 3D 20", 0xC)); // CLASSNAME
        for (auto & [type, info] : M2Fix::GetInstance().m_kGames)
        {
            if (info.M2classname == classname)
            {
                m_eGame = type;
                m_kGame = &info;
                m_kGame->title = M2Hook::ReadUntilTabOrCRLF(M2Hook::GetInstance().Scan("43 41 50 54 49 4F 4E 20 3D 20", 0xA)); // CAPTION
                if (const std::string_view backup_base_path = M2Hook::ReadUntilTabOrCRLF(M2Hook::GetInstance().Scan("42 41 43 4B 55 50 5F 42 41 53 45 5F 50 41 54 48 20 3D 20", 0x13)); backup_base_path.find("?Epic_AccountID?") != std::string::npos)
                {
                    m_kGame->id = -1;
                    spdlog::info("Detected game: {} (Epic Games).", info.title);
                }
                else
                {
                    spdlog::info("Detected game: {} (Steam App {}).", info.title, info.id);
                }
                return true;
            }
        }

        spdlog::critical("Failed to detect supported game, {} isn't supported by {}.",
            M2Hook::GetInstance().ModuleIdentifier(),
            m_sFixName
        );
        return false;
    }

private:
    static constexpr std::string_view m_sFixName    = FIX_NAME;
    static constexpr std::string_view m_sFixVer     = VERSION_STRING;
    static constexpr std::string_view m_sLogFile    = FIX_NAME ".log";
    static constexpr std::string_view m_sConfigFile = FIX_NAME ".ini";

    std::multimap<M2FixGame, M2FixInfo> m_kGames = {
        { M2FixGame::MGS1,               { 2131630, "MGS1",                           "", MGS1::GetInstance()   } },
        { M2FixGame::MGSR,               { 2306740, "MGSBC",                          "", M2Game::GetInstance() } },
        { M2FixGame::Contra,             { 1018020, "CONTRALLECTION",                 "", M2Game::GetInstance() } },
        { M2FixGame::Dracula,            { 1018010, "CASTLEVANIACOLLECTION",          "", M2Game::GetInstance() } },
        { M2FixGame::DraculaAdvance,     { 1552550, "CASTLEVANIAADVANCECOLLECTION",   "", M2Game::GetInstance() } },
        { M2FixGame::DraculaDominus,     { 2369900, "CASTLEVANIADOMINUSCOLLECTION",   "", M2Game::GetInstance() } },
        { M2FixGame::Ray,                { 2478020, "RAC",                            "", M2Game::GetInstance() } },
        { M2FixGame::Darius,             { 1638330, "DariusCozmicCollection",         "", M2Game::GetInstance() } },
        { M2FixGame::Darius101,          { 1640160, "GDarius",                        "", M2Game::GetInstance() } },
        { M2FixGame::Gradius,            { 2897590, "GRADIUSORIGINCOLLECTION",        "", M2Game::GetInstance() } },
        { M2FixGame::NightStrikers,      { 3099790, "OPERATIONNIGHTSTRIKERS",         "", M2Game::GetInstance() } },
    };

    static inline M2FixInfo *m_kGame;
    static inline M2FixGame m_eGame;
};
