#pragma once

#include "stdafx.h"

#include "m2fixbase.h"

// What to do about the collection's replacement for MGS1's Option -> SCREEN
// help texture. See [Patches] BrightnessText in the ini.
enum class M2BrightnessText
{
    Collection,   // leave it alone
    Fixed,        // its four lines, at the position the game draws them
    Original,     // the game's own six lines, at that same position
};

class M2Config : public M2FixBase
{
public:
    M2Config()
    {
        bPatchesRemoveUnderpants = true;
        bPatchesEnableMosaic = true;
        bPatchesRestoreGhosts = true;
        bPatchesRestoreMedicine = true;
        bPatchesThinTexturedQuads = true;
        eBrightnessText = M2BrightnessText::Fixed;
        bPatchesPreserveConfiguration = true;
    }

    static auto & GetInstance()
    {
        static M2Config instance;
        return instance;
    }

    static void LoadInstance()
    {
        GetInstance().Load();
    }

    virtual void Load() override;

public:
    static inline bool bDebuggerEnabled;
    static inline int iDebuggerPort;
    static inline bool bDebuggerAutoUpdate;
    static inline bool bDebuggerExclusive;
    static inline std::optional<bool> bSmoothing;
    static inline std::optional<bool> bScanline;
    static inline std::optional<bool> bDotMatrix;
    static inline bool bBreak;
    static inline bool bConsole;
    static inline bool bError;
    static inline int iLevel;
    static inline int iNativeLevel;
    static inline int iEmulatorLevel;
    static inline int iRendererLevel;
    static inline bool bExternalEnabled;
    static inline int iExternalWidth;
    static inline int iExternalHeight;
    static inline bool bExternalWindowed;
    static inline bool bExternalBorderless;
    static inline bool bInternalEnabled;
    static inline bool bInternalBorderless;
    static inline int iInternalHeight;
    static inline bool bInternalWidescreen;
    static inline std::optional<bool> bAnalog;
    static inline std::optional<bool> bSwapSticks;
    static inline bool bRemoveDeadzone;
    static inline bool bLauncherSkipNotice;
    static inline bool bLauncherStartGame;
    static inline bool bGameStageSelect;
    static inline bool bGameEnglishText;
    static inline bool bGameUnlockVRMissions = false;
    static inline bool bGameUnlockVRExtras = false;
    static inline bool bGameUnlockVRMovies = false;
    static inline bool bGameUnlockTitleBonuses = false;
    static inline bool bPatchesIntegralEnglish = true;
    static inline bool bPatchesIntegralVREnglish = true;
    static inline bool bPatchesGrenadeDelay = true;
    static inline bool bPatchesDisableRAM;
    static inline bool bPatchesDisableCDROM;
    static inline bool bPatchesDisableFont;
    static inline bool bPatchesRemoveUnderpants;
    static inline bool bPatchesEnableMosaic;
    static inline bool bPatchesRestoreGhosts;
    static inline bool bPatchesRestoreMedicine;
    // Given a value here as well: the GPU hook reads it on every polygon.
    static inline bool bPatchesThinTexturedQuads = true;
    // Given a value here, not just in the constructor: an enum's zero is
    // Collection, so a read before the singleton is first constructed would
    // otherwise see the wrong default.
    static inline M2BrightnessText eBrightnessText = M2BrightnessText::Fixed;
    static inline bool bPatchesPreserveConfiguration;
    static inline bool bShouldCheckForUpdates;
    static inline bool bConsoleUpdateNotifications;
    static inline bool bDisableWindowsFullscreenOptimization;
    static inline bool bDisableWindowsSlideshowWarning;

    static inline std::string sFullscreenMode;
    static inline std::string sExternalWidth;
    static inline std::string sExternalHeight;

private:
    inipp::Ini<char> m_ini;
};
