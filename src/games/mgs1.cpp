#include "mgs1.h"
#include "psx.h"
#include "sqhook.h"

int MGS1::MGS1_main(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    PSX::main(cpu);
    unsigned int ra = cpu->Reg[31];
    spdlog::info("[MGS 1] __main: 0x{:08x} -> 0x{:08x}.", address, ra);

    PSXFUNCTION main = PSX::UserHandler(cpu, address);
    return main(cpu, cycle, address);
}

int MGS1::MGS1_s03a_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    static bool oneshot = false;
    if (!oneshot) {
        spdlog::info("[MGS 1] {} s03a_disable_mosaic().", M2Config::bPatchesEnableMosaic ? "Blocking" : "Allowing");
        oneshot = true;
    }
    if (!M2Config::bPatchesEnableMosaic) {
        PSXFUNCTION s03a_disable_mosaic = PSX::UserHandler(cpu, address);
        return s03a_disable_mosaic(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

int MGS1::MGS1_s03d_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    static bool oneshot = false;
    if (!oneshot) {
        spdlog::info("[MGS 1] {} s03d_disable_mosaic().", M2Config::bPatchesEnableMosaic ? "Blocking" : "Allowing");
        oneshot = true;
    }
    if (!M2Config::bPatchesEnableMosaic) {
        PSXFUNCTION s03d_disable_mosaic = PSX::UserHandler(cpu, address);
        return s03d_disable_mosaic(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

int MGS1::MGS1_font(M2_EmuR3000 *cpu, int cycle, unsigned int address)
{
    static bool oneshot = false;
    if (!oneshot) {
        spdlog::info("[MGS 1] {} high resolution font system.", M2Config::bPatchesDisableFont ? "Blocking" : "Allowing");
        oneshot = true;
    }
    if (!M2Config::bPatchesDisableFont) {
        PSXFUNCTION font = PSX::UserHandler(cpu, address);
        return font(cpu, cycle, address);
    }

    return cpu->Execute(cpu, cycle, address);
}

SQInteger MGS1::SQReturn_set_playside_mgs(HSQUIRRELVM<Squirk::Standard> v)
{
    spdlog::info("[MGS 1] Set play side to {}.", SQSystemData<Squirk::Standard>::SettingPad::GetPlaySide_MGS1());
    return 0;
};

void MGS1::SQOnMemoryDefine()
{
    MGS1_GlobalsPTR = SQTitleProf<Squirk::Standard>::GetMemoryDefine("scene_name");
    spdlog::info("[MGS 1] mgs_stage is 0x{:x}.", MGS1_GlobalsPTR);

    // Only Integral and the VR disk define these; everything else reads back 0.
    MGS1_LanguagePTR = SQTitleProf<Squirk::Standard>::GetMemoryDefine("language_setting");
    MGS1_LanguageMask = SQTitleProf<Squirk::Standard>::GetMemoryDefine("language_setting_mask");
    MGS1_LanguageDone = false;
    MGS1_LanguageHeld = 0;
    MGS1_LanguageLast = -1;
    MGS1_LanguageWanted = false;
    MGS1_LanguageLogs = 0;
    MGS1_LanguageWriteLogs = 0;
    MGS1_LanguageReadLogs = 0;

    // GM_Configuration, for [Patches] PreserveConfiguration (see mgs1.h). Only a
    // collection write to exactly this address is ever touched, so a version
    // where the relation did not hold would simply be left alone.
    MGS1_ConfigPTR = MGS1_GlobalsPTR != 0 ? MGS1_GlobalsPTR + 0x14 : 0;
    MGS1_ConfigSeenValid = false;
    MGS1_ConfigPreserved = 0;
    if (MGS1_ConfigPTR != 0) {
        spdlog::info("[MGS 1] GM_Configuration is 0x{:x}.", MGS1_ConfigPTR);
        if (MGS1_LanguagePTR != 0 && MGS1_LanguagePTR != MGS1_ConfigPTR + 1) {
            spdlog::warn("[MGS 1] language_setting (0x{:x}) is not GM_Configuration's high byte; check the layout.", MGS1_LanguagePTR);
        }
    }
    if (MGS1_LanguagePTR != 0) {
        spdlog::info("[MGS 1] language_setting is 0x{:x}, mask is 0x{:x}.",
            MGS1_LanguagePTR, MGS1_LanguageMask);
    }

    MGS1_LastStageName[0] = 0;

}

void MGS1::SQOnUpdateGadgets()
{
    if (MGS1_GlobalsPTR != 0 && MGS1_LoaderPTR != 0) {
        SQInteger MGS1_StageNamePTR = MGS1_GlobalsPTR;

        char MGS1_StageName[8] = { 0 };
        char MGS1_LoaderName[8] = { 0 };
        SQEmuTask<Squirk::Standard>::RamCopy(MGS1_StageName, MGS1_StageNamePTR, sizeof(MGS1_StageName));
        SQEmuTask<Squirk::Standard>::RamCopy(MGS1_LoaderName, MGS1_LoaderPTR, sizeof(MGS1_LoaderName));

        if (strncmp(MGS1_StageName, MGS1_LastStageName, sizeof(MGS1_StageName)) != 0) {
            spdlog::info("[MGS 1] scene_name \"{}\" -> \"{}\".", MGS1_LastStageName, MGS1_StageName);
            memcpy(MGS1_LastStageName, MGS1_StageName, sizeof(MGS1_StageName));
        }


        if (M2Config::bGameStageSelect) {
            if (strcmp(MGS1_LoaderName, "title") == 0 && strcmp(MGS1_StageName, "select") != 0) {
                strcpy(MGS1_LoaderName, "select");
                SQEmuTask<Squirk::Standard>::RamCopy(MGS1_LoaderPTR, MGS1_LoaderName, sizeof(MGS1_LoaderName));
                spdlog::info("[MGS 1] Set mgs_loader_stage to \"{}\".", MGS1_LoaderName);
            }
        }

        // Integral selects Japanese text in its own option screen by default.
        // Hold the language bit set until it survives on its own, then stop, so
        // English is what a new game starts with while the in-game option stays
        // free to choose Japanese afterwards.
        //
        // Waiting for the title screen specifically matters: the setting is
        // restored from the memory card as the title loads, which overwrites an
        // earlier write. Anything set during "init" is gone by the time a player
        // could see it, so the hold has to span every pre-gameplay stage and end
        // only once the value has stayed put for a while on the title itself.
        //
        // After that the bit is only guarded. The option screen (scene "option")
        // is the one place the player can change it, so a change seen there is
        // the player's choice and is followed; a change seen in any other scene
        // was made by something else and is undone. Every change is logged with
        // its scene, and Squirrel writes to the word are traced (SQOnRamWrite):
        // starting Integral's 1P MODE has been seen to leave English selected on
        // the title and yet play in Japanese, and neither the game's scripts nor
        // its code clear the bit on that path, so the writer is being looked for.
        if (MGS1_LanguagePTR != 0 && MGS1_LanguageMask != 0) {
            SQInteger setting = SQEmuTask<Squirk::Standard>::GetRamValue(CHAR_BIT, MGS1_LanguagePTR);
            int english = (setting & MGS1_LanguageMask) != 0;
            if (english != MGS1_LanguageLast) {
                if (MGS1_LanguageLast >= 0 && ++MGS1_LanguageLogs <= 64) {
                    spdlog::info("[MGS 1] language_setting is now 0x{:02x} ({}) in scene \"{}\".",
                        setting, english ? "English" : "Japanese", MGS1_StageName);
                }
                MGS1_LanguageLast = english;
            }
            if (M2Config::bGameEnglishText) {
                if (!MGS1_LanguageDone) {
                    if (!english) {
                        SQEmuTask<Squirk::Standard>::SetRamValue(CHAR_BIT, MGS1_LanguagePTR,
                            setting | MGS1_LanguageMask);
                        spdlog::info("[MGS 1] Selected English text (0x{:x}: 0x{:02x} -> 0x{:02x}).",
                            MGS1_LanguagePTR, setting, setting | MGS1_LanguageMask);
                        MGS1_LanguageHeld = 0;
                        MGS1_LanguageLast = 1;
                    }
                    else if (!strcmp(MGS1_StageName, "title")
                        && ++MGS1_LanguageHeld >= MGS1_LanguageHoldFrames) {
                        MGS1_LanguageDone = true;
                        MGS1_LanguageWanted = true;
                        spdlog::info("[MGS 1] English text is set; leaving language_setting alone.");
                    }
                }
                else if (!strcmp(MGS1_StageName, "option")) {
                    if (MGS1_LanguageWanted != (english != 0)) {
                        MGS1_LanguageWanted = english != 0;
                        spdlog::info("[MGS 1] The option screen selected {} text; following it.",
                            english ? "English" : "Japanese");
                    }
                }
                else if (MGS1_LanguageWanted && !english) {
                    SQEmuTask<Squirk::Standard>::SetRamValue(CHAR_BIT, MGS1_LanguagePTR,
                        setting | MGS1_LanguageMask);
                    MGS1_LanguageLast = 1;
                    if (++MGS1_LanguageLogs <= 64) {
                        spdlog::info("[MGS 1] Restored English text: something cleared it in scene \"{}\" (0x{:02x} -> 0x{:02x}).",
                            MGS1_StageName, setting, setting | MGS1_LanguageMask);
                    }
                }
            }
        }

    }

    if (M2Config::bAnalog.has_value() && M2Config::bAnalog.value()) {
        AnalogLoop();
    }
}

bool MGS1::SQOnRamWrite(unsigned width, unsigned offset, unsigned &value)
{
    // The collection's per-frame rewrite of GM_Configuration: it read the word
    // a moment ago (SQOnRamRead) and is writing its edited copy back. Anything
    // the game stored in between would be lost, so re-read the word and carry
    // over only the bits the collection changed.
    if (M2Config::bPatchesPreserveConfiguration && MGS1_ConfigPTR != 0
        && width == 16 && offset == MGS1_ConfigPTR && MGS1_ConfigSeenValid) {
        MGS1_ConfigSeenValid = false;
        unsigned now = SQEmuTask<Squirk::Standard>::GetRamValue(16, MGS1_ConfigPTR) & 0xFFFF;
        unsigned changed = (MGS1_ConfigSeen ^ value) & 0xFFFF;
        unsigned merged = (now & ~changed) | (value & changed);
        if (merged != (value & 0xFFFF)) {
            if (++MGS1_ConfigPreserved <= 32) {
                spdlog::info("[MGS 1] Preserved GM_Configuration: the collection read 0x{:04x}, would write 0x{:04x}, the game has 0x{:04x} now; writing 0x{:04x} (scene \"{}\").",
                    MGS1_ConfigSeen, value & 0xFFFF, now, merged, MGS1_LastStageName);
            }
            value = merged;
        }
    }

    // GM_Configuration is the 16-bit word ending at the language_setting byte
    // (the English bit is bit 8, so the define points at its high byte). Report
    // any write, of any width, that overlaps that word.
    if (MGS1_LanguagePTR == 0) return false;
    unsigned bytes = width / CHAR_BIT;
    if (bytes == 0) bytes = 1;
    if (offset > MGS1_LanguagePTR || offset + bytes <= MGS1_LanguagePTR - 1) return false;
    // The collection rewrites this word every frame (see the header); a write
    // that leaves it as it is only counts, a write that changes it is reported.
    SQInteger current = SQEmuTask<Squirk::Standard>::GetRamValue(bytes * CHAR_BIT, offset);
    unsigned mask = bytes >= 4 ? 0xFFFFFFFFu : ((1u << (bytes * CHAR_BIT)) - 1);
    bool changes = ((unsigned)current & mask) != (value & mask);
    if (!changes && ++MGS1_LanguageWriteLogs > 4) return false;
    spdlog::info("[MGS 1] Squirrel is writing the language word: setRamValue({}, 0x{:x}, 0x{:x}) over 0x{:x} in scene \"{}\"{}.",
        width, offset, value, (unsigned)current & mask, MGS1_LastStageName,
        changes ? " - CHANGES IT" : "");
    return true;
}

bool MGS1::SQOnRamRead(unsigned width, unsigned offset)
{
    // Remember what the collection is about to read from GM_Configuration; the
    // read itself follows this hook on the same thread, so the two agree.
    if (M2Config::bPatchesPreserveConfiguration && MGS1_ConfigPTR != 0
        && width == 16 && offset == MGS1_ConfigPTR) {
        MGS1_ConfigSeen = SQEmuTask<Squirk::Standard>::GetRamValue(16, MGS1_ConfigPTR) & 0xFFFF;
        MGS1_ConfigSeenValid = true;
    }

    if (MGS1_LanguagePTR == 0) return false;
    unsigned bytes = width / CHAR_BIT;
    if (bytes == 0) bytes = 1;
    if (offset > MGS1_LanguagePTR || offset + bytes <= MGS1_LanguagePTR - 1) return false;
    if (++MGS1_LanguageReadLogs > 4) return false;
    spdlog::info("[MGS 1] Squirrel is reading the language word: getRamValue({}, 0x{:x}) in scene \"{}\".",
        width, offset, MGS1_LastStageName);
    return true;
}

void MGS1::EPIOnLoadImage(void *image, unsigned int size)
{
    MGS1_LoaderPTR = M2Hook::GetInstance().ScanBuffer(
        "00 00 00 00 69 6E 69 74 00 00 00 00",
        -0x30 - reinterpret_cast<uintptr_t>(image),
        image, size, "[MGS 1] mgs_loader_stage"
    );
}

#ifndef _WIN64
bool MGS1::GWBlank()
{
    if (!PSX::Emulator) return false;
    struct M2_EmuGPU *gpu = PSX::Emulator->DevGPU;
    if (gpu->VideoMode && (gpu->Status & 0x10000)) {
        if (MGS1_Blank < 5) {
            ++MGS1_Blank;
            return true;
        }
        return false;
    } else {
        if (MGS1_Blank > 0) {
            --MGS1_Blank;
            return true;
        }
        return false;
    }
}
#endif

bool MGS1::EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args)
{
    struct M2_EmuPSX *psx = std::any_cast<M2_EmuPSX *>(machine);
    struct M2_EmuGPU *gpu = psx->DevGPU;
    bool ret = true;

    switch (cmd)
    {
#ifndef _WIN64
        case 0x8002: // GET_POSITION
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            unsigned int w =  ((gpu->ScreenRangeW >> 12) & 0xFFF) - (gpu->ScreenRangeW & 0xFFF);
            unsigned int h = (((gpu->ScreenRangeH >> 10) & 0x3FF) - (gpu->ScreenRangeH & 0x3FF)) << ((gpu->Status >> 22) & 1);
            unsigned int x = (gpu->VideoMode ? 256 : 240) << ((gpu->Status >> 22) & 1);
            unsigned int y = std::min(h, x);

            *(args[0]) = (2560 - w) >> 1;
            *(args[1]) = (M2Config::iInternalHeight * ((x - y) >> 1)) / x;
            *(args[2]) = (w * M2Config::iInternalHeight) / 256;
            *(args[3]) = (y * M2Config::iInternalHeight) / 256;

            ret = false;
            break;
        }

        case 0x8003: // GET_DIMENSION
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            *(args[0]) = (M2Config::iInternalHeight * ((gpu->VideoMode && (gpu->Status & 0x10000)) ? 384 : 320)) / 256;
            *(args[1]) = ((M2Config::iInternalHeight * (gpu->VideoMode ? 256 : 224)) / (gpu->VideoMode ? 256 : 240));

            ret = false;
            break;
        }

        case 0x8004: // SET_DEVICE
        {
            if (!M2Config::bInternalEnabled) {
                break;
            }

            unsigned int *res = args[0];

            res[1] = ((M2Config::iInternalHeight * 320) / 256) * 2;
            res[2] =   M2Config::iInternalHeight * 2;
            break;
        }

        case 0x8007: // SET_VIDEO_MODE
        {
            PSX::VideoMode = reinterpret_cast<unsigned int>(args[0]);
            break;
        }
#endif

        default: break;
    }

    return ret;
}
