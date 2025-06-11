#pragma once

#include "stdafx.h"

#include "m2fixbase.h"

class M2Config : public M2FixBase
{
public:
    M2Config()
    {
        bPatchesRemoveUnderpants = true;
        bPatchesEnableMosaic = true;
        bPatchesRestoreGhosts = true;
        bPatchesRestoreMedicine = true;
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
    static inline bool bWindowedMode;
    static inline bool bBorderlessMode;
    static inline bool bInternalEnabled;
    static inline int iInternalHeight;
    static inline bool bInternalWidescreen;
    static inline bool bAnalogMode;
    static inline bool bLauncherSkipNotice;
    static inline bool bLauncherStartGame;
    static inline bool bGameStageSelect;
    static inline bool bPatchesDisableRAM;
    static inline bool bPatchesDisableCDROM;
    static inline bool bPatchesRemoveUnderpants;
    static inline bool bPatchesEnableMosaic;
    static inline bool bPatchesRestoreGhosts;
    static inline bool bPatchesRestoreMedicine;

    static inline std::string sFullscreenMode;
    static inline std::string sExternalWidth;
    static inline std::string sExternalHeight;

private:
    inipp::Ini<char> m_ini;
};
