#pragma once

#include "m2game.h"
#include "m2hook.h"
#include "m2utils.h"
#include "m2config.h"

#include "psx.h"
#include "analog.h"
#ifndef _WIN64
#include "dx11.h"
#endif

#include "sqemutask.h"
#include "sqtitleprof.h"

#include "sqhook.h"

class MGS1 : public M2Game
{
public:
    MGS1() {}

    static auto & GetInstance()
    {
        static MGS1 instance;
        return instance;
    }

    virtual std::vector<std::reference_wrapper<M2Machine>> MachineInstances() override
    {
        return { PSX::GetInstance() };
    }

    virtual void Load() override
    {
        M2Utils::DisableWindowsFullscreenOptimization();

#ifndef _WIN64
        static DX11 dx11;
        DX11::LoadInstance(&dx11);
#endif

        Analog::LoadInstance();

        SQHook<Squirk::Standard>::SetReturnHook("set_playside_mgs", SQReturn_set_playside_mgs);

        if (M2Config::bPatchesRemoveUnderpants) {
            for (auto & MGS1_FileBlacklist_Underpant : MGS1_FileBlacklist_Underpants) {
                SQHook<Squirk::Standard>::SetPatchFileBlacklist(MGS1_FileBlacklist_Underpant);
            }
        }

        if (M2Config::bPatchesRestoreGhosts) {
            SQHook<Squirk::Standard>::SetPatchFileBlacklist(MGS1_FileBlacklist_Ghosts);
        }

        if (M2Config::bPatchesRestoreMedicine) {
            SQHook<Squirk::Standard>::SetPatchDataBlacklist(MGS1_DataBlacklist_Medicine);
        }

        if (M2Config::eBrightnessText != M2BrightnessText::Collection) {
            // Scope the collection replacement to USA and the selected disc.
            for (unsigned disk = 0; disk < MGS1_RangeBlacklist_BrightnessText.size(); ++disk) {
                const auto &range = MGS1_RangeBlacklist_BrightnessText[disk];
                SQHook<Squirk::Standard>::SetPatchRangeBlacklist(981, disk, range.first, range.second);
            }
        }

        // `fixed` also writes back the two lines the collection was right to
        // drop, blanked - see MGS1_BrightnessTextData.
        if (M2Config::eBrightnessText == M2BrightnessText::Fixed) {
            MGS1_KetchupPatches = MGS1_BrightnessTextPatches();
        }

        // Report what the collection writes into Integral's `option` stage. It
        // patches the stage's GCL script at one place - disc 1's is
        // disc1_16F8E024_patch.bin, stage offset 152460, i.e. tag 6 (the script
        // chunk) + 908 - and that patch is what redirects KEY CONFIG to the
        // collection's own Control Settings panel. The English port relocates
        // the stage, so the patch lands on sectors the game no longer reads and
        // the interception is lost. Watching it means a change on their side is
        // visible in the log rather than silent.
        //     span = the retail `option` stage, sectors 27136..27210, through
        //     (lba + fo / 2048) * 2352 + 24 + fo % 2048, with STAGE.DIR at LBA
        //     136654 on disc 1 and 105178 on disc 2.
        SQHook<Squirk::Standard>::SetPatchWatch(0x16F634B8ull, 0x16F8E5C8ull,
            "Integral disc 1 option stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x128C92F8ull, 0x128F4408ull,
            "Integral disc 2 option stage");
        // The MISSION LOG stage (`abst`, sectors 139..218 on both discs) was
        // ported on 2026-09-05 (en_abst) and relocated into DUMMY3M, so the
        // collection's patch below now lands on a stage the game no longer
        // reads. The watch stays to show what it wanted to write: the one
        // patch the collection offers there is disc1_132F2716_patch_PS5.bin,
        // stage sector +51 - the disc-change abstract's block in the script
        // chunk, which the port now fills with USA's eight strings. Whether a
        // _PS5-suffixed patch is applied on Windows at all is still unseen.
        SQHook<Squirk::Standard>::SetPatchWatch(0x132D51C8ull, 0x133030C8ull,
            "Integral disc 1 abst stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x0EF3F538ull, 0x0EF6D438ull,
            "Integral disc 2 abst stage");
        // The collection also patches the three other disc-swap text copies
        // (`change`, `demosel`, `title` - named files disc1_18345E07,
        // disc1_18412A95, disc1_18412BD8, disc1_1822B55D) two bytes before the
        // records en_menu / en_menu2 write there, and six places in `camera`
        // (en_camsave). Mapped 2026-09-05 from a filtered log; the watches show
        // what they write and whether the port's records are overlapped.
        SQHook<Squirk::Standard>::SetPatchWatch(0x18341268ull, 0x18346518ull,
            "Integral disc 1 change stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x183F6078ull, 0x18416C28ull,
            "Integral disc 1 demosel stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x181E7788ull, 0x1825C9C8ull,
            "Integral disc 1 title stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x16F8E5C8ull, 0x16FA42E8ull,
            "Integral disc 1 camera stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x13CA70A8ull, 0x13CAC358ull,
            "Integral disc 2 change stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x13D5BEB8ull, 0x13D7CA68ull,
            "Integral disc 2 demosel stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x13B4D5C8ull, 0x13BC2808ull,
            "Integral disc 2 title stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x128F4408ull, 0x1290A128ull,
            "Integral disc 2 camera stage");
        // Integral's VR-DISC (title 99, version VR-DISK; a separate image, so
        // these offsets are small). The English port (tools/integral-english/
        // vr_*.py, 2026-09-06) patches these stages IN PLACE; the option stage
        // in particular keeps its sectors so the collection's KEY CONFIG
        // interception keeps landing - these watches show where that patch is
        // and whether any collection patch overlaps the port's records.
        SQHook<Squirk::Standard>::SetPatchWatch(0x0005E7EB0ull, 0x000611D60ull,
            "Integral VR-DISK option stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x000611D60ull, 0x000627A80ull,
            "Integral VR-DISK camera stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x00315B1C0ull, 0x0031B5430ull,
            "Integral VR-DISK vrtitle stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x004C26BA0ull, 0x004C6D5B0ull,
            "Integral VR-DISK movie stage");
        SQHook<Squirk::Standard>::SetPatchWatch(0x000CFA6E0ull, 0x000D5D320ull,
            "Integral VR-DISK vrsave stage");

        if (M2Config::bPatchesDisableFont) {
            for (auto & MGS1_TextureWhitelist_Font : MGS1_TextureWhitelist_Fonts) {
                SQHook<Squirk::Standard>::SetTextureWhitelist(MGS1_TextureWhitelist_Font);
            }
        }

        for (Ketchup_TitleInfo & title : MGS1_Ketchup) {
            for (Ketchup_VersionInfo & version : title.versions) {
                for (Ketchup_DiskInfo & disk : version.disks) {
                    auto path = M2Hook::GetInstance().ModuleLocation().parent_path();
                    path /= "mods";
                    path /= title.name;
                    if (title.versions.size() > 1) {
                        path /= version.name;
                    }
                    if (version.disks.size() > 1) {
                        path /= std::to_string(disk.id);
                    }
                    std::error_code ec;
                    std::filesystem::create_directories(path, ec);
                }
            }
        }
    }

    virtual std::any EPIModuleHook() override
    {
        return MGS1_ModuleTables;
    }


    virtual std::vector<Ketchup_TitleInfo> *SQKetchupHook() override
    {
        return &MGS1_Ketchup;
    }

    virtual std::vector<Ketchup_DiskPatch> *SQKetchupPatches() override
    {
        return &MGS1_KetchupPatches;
    }

#ifndef _WIN64
    virtual void GWRenderGeometry(int & gw_width, int & gw_height, int & fb_width, int & fb_height, int & img_width, int & img_height) override
    {
        gw_width   = 1024;
        gw_height  = 1024;
        fb_width   = 320;
        fb_height  = 256;
        img_width  = 320;
        img_height = PSX::VideoMode ? 256 : 224;
    }

    virtual bool GWBlank() override;
#endif

    static SQInteger MGS1_EmuTask_getHeight(HSQUIRRELVM<Squirk::Standard> v)
    {
        auto id = SQGlobals<Squirk::Standard>::GetTitle();
        static std::set<int> NTSC = { 980, 981, 99, 101 };

        SQObjectPtr<Squirk::Standard> func = SQ_EmuTask_getHeight;
        SQObjectPtr<Squirk::Standard> res = {};
        v->Call(func, 1, v->_stackbase, res, false);

        SQFloat height = _float(res);
        if (M2Config::bInternalBorderless && NTSC.contains(id)) {
            height = (height * 224.0f) / 240.0f;
        }

        sq_pushfloat(v, height);
        return 1;
    }

    virtual void SQOnInitSystemFirst() override
    {
        SQHook<Squirk::Standard>::HookMethod(
            Sqrat::DefaultVM<Squirk::Standard>::Get(),
            "EmuTask",
            "getHeight",
            MGS1_EmuTask_getHeight,
            &SQ_EmuTask_getHeight
        );
    }

    virtual void SQOnMemoryDefine() override;
    virtual void SQOnUpdateGadgets() override;
    virtual bool SQOnRamWrite(unsigned width, unsigned offset, unsigned &value) override;
    virtual bool SQOnRamRead(unsigned width, unsigned offset) override;
    virtual void EPIOnLoadImage(void *image, unsigned int size) override;
    virtual bool EPIOnMachineCommand(std::any machine, int cmd, unsigned int **args) override;

    static int MGS1_main(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_s03a_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_s03d_disable_mosaic(M2_EmuR3000 *cpu, int cycle, unsigned int address);
    static int MGS1_font(M2_EmuR3000 *cpu, int cycle, unsigned int address);

    static SQInteger SQReturn_set_playside_mgs(HSQUIRRELVM<Squirk::Standard> v);

    static void AnalogLoop();

private:
    uintptr_t MGS1_GlobalsPTR = 0;
    uintptr_t MGS1_LoaderPTR = 0;
    uintptr_t MGS1_LanguagePTR = 0;
    unsigned MGS1_LanguageMask = 0;
    unsigned MGS1_LanguageHeld = 0;
    bool MGS1_LanguageDone = false;
    constexpr static unsigned MGS1_LanguageHoldFrames = 120;
    // Last value of the English bit seen (-1: not read yet), the language the
    // player is taken to want once the hold is over, and a cap on the log lines
    // the change tracking may emit.
    int MGS1_LanguageLast = -1;
    bool MGS1_LanguageWanted = false;
    unsigned MGS1_LanguageLogs = 0;
    // The Master Collection's _update_option_button_setting rewrites the whole
    // GM_Configuration word every frame; only report the first few of those
    // writes and reads, and every write that actually changes the word.
    unsigned MGS1_LanguageWriteLogs = 0;
    unsigned MGS1_LanguageReadLogs = 0;
    // GM_Configuration (libgcl's linkvarbuf[2]): linkvarbuf sits 0x10 above the
    // "scene_name" define (variable.c's stage_name[16]), so the word is at
    // scene_name + 0x14 in every MGS1 executable. The collection's
    // _update_option_button_setting reads it (getRamValue) and writes the whole
    // word back (setRamValue) once per frame; [Patches] PreserveConfiguration
    // remembers what it read and, at the write, re-applies only the bits it
    // changed on top of the word as the game has it by then.
    uintptr_t MGS1_ConfigPTR = 0;
    unsigned MGS1_ConfigSeen = 0;
    bool MGS1_ConfigSeenValid = false;
    unsigned MGS1_ConfigPreserved = 0;
    // libgcl's var_buf (variable.c) - the GCL `$f:`/`$w:` variable memory. Not a
    // memory define; a static whose address is read off GCL_GetVar's constants
    // in each executable. The briefing menu's sixteen items are gated by the
    // flags at var_buf+0x4C..0x4E (bits 0x4C.1-7, 0x4D.0-7, 0x4E.0), which the
    // brf stage script passes to the menu as its `-f` option.
    uintptr_t MGS1_VarBufPTR = 0;
    unsigned MGS1_UnlockWrites = 0;
    char MGS1_GaveItemsIn[16] = {};   // stage the GiveItems grant last ran in
    char MGS1_LastStageName[8] = { 0 };
    constexpr static unsigned MGS1_BriefingFlagsOffset = 0x4C;
    constexpr static unsigned char MGS1_BriefingFlagsMask[3] = { 0xFE, 0xFF, 0x01 };
    static inline HSQOBJECT<Squirk::Standard> SQ_EmuTask_getHeight = {};

#ifndef _WIN64
    int MGS1_Blank = 0;
#endif

    const std::vector<std::string> MGS1_FileBlacklist_Underpants = {
        "0046a5", "0046a6",
        "0057c3", "0057c4", "0057c5",
        "0099fc", "0099fd",
    };

    const std::string MGS1_FileBlacklist_Ghosts = "shinrei";

    // The collection replaces the Option -> SCREEN help texture (`sc_text` in
    // the `option` stage's archive) with a four-line version on the same 232x70
    // canvas. Dropping "Press the O button to return to the option screen." is
    // fair - that button's name is not the same on every platform - but it also
    // re-centres the four lines it keeps, and that is a bug: the whole canvas is
    // opaque (its quad is drawn with abe = 0), and the game's art paints canvas
    // rows 0..3 one step off black, at (8,8,8), so they vanish into the grey
    // ramp's own 8 band and hide the backdrop's top edge. Re-centring carries
    // those four rows down with the text, leaving flat black over the 8 band.
    // Measured on the collection's USA: 0.00 luminance inside the canvas
    // against 8.89 beside it, four rows, with the text one full line pitch
    // (12 game rows) below where the game draws it.
    //
    // These are the four pieces of that one archive entry on MGS1 (USA) disc 1,
    // addressed by image offset, which is how the collection names them.
    // Integral needs none: it has no `sc_text` of its own for them to replace.
    const std::vector<std::string> MGS1_FileBlacklist_BrightnessText = {
        "disc1_165A34CC", "disc1_165A3BD8", "disc1_165A4508", "disc1_165A4E38",
    };

    // The same entry as a disc-image range, per disk: `option` is STAGE.DIR
    // sector 27023 on both of MGS1 (USA)'s disks, and the entry's payload is
    // 5852 bytes at file offset 55499300, so the span follows from each disk's
    // STAGE.DIR LBA (132344 and 100801). Tight enough to be safe, checked two
    // ways against every patch candidate in the log archive: of the 107 the
    // collection offers this title, exactly four fall anywhere inside the
    // 81-sector option stage and they are the four above; and across titles
    // 99 (Integral) and 980 (the Japanese MGS1), whose disc images do span
    // these offsets, nothing falls inside either window. Registered with USA title 981 and a disc index; checked against the
    // active title/disc when each collection patch is submitted.
    const std::vector<std::pair<uint64_t, uint64_t>> MGS1_RangeBlacklist_BrightnessText = {
        { 0x165A34CCull, 0x165A4F38ull },   // disk 0
        { 0x11EE2B7Cull, 0x11EE45E8ull },   // disk 1
    };

    // Rows 46..69 of that texture, re-encoded blank. Splicing these over the
    // game's own payload leaves rows 0..45 - the four lines the collection also
    // keeps, and the (8,8,8) filler - byte for byte as the game has them, and
    // drops lines 5 and 6. The payload's declared size does not change: PCX
    // here is four bit planes run-length encoded one row at a time, so rows are
    // independent in the byte stream, this encoding is shorter than the one it
    // replaces, and the decoder stops after 70 rows without reading the rest.
    //
    // Why not move the quad instead: the texture's own size supplies the UVs
    // (SetPacketTexture reads them from DG_TEX), so the quad cannot crop, and
    // shifting it up 11 rows would drag the backdrop's top edge onto the ramp's
    // brighter 16 band - a worse notch than the one being fixed.
    // Kept as a plain array in .rdata, not an initializer_list: the bytes are
    // then greppable in the shipped binary, which is how this build verified
    // that what ends up on disc is what the tools produced.
    static inline const unsigned char MGS1_BrightnessTextData[426] = {
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0x40, 0xDB, 0x00, 0x10, 0xDF, 0x00,
        0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00, 0xBF, 0xDB, 0xFF, 0xC1, 0xEF, 0x00,
        0xDC, 0x00, 0x10, 0xDF, 0x00, 0xDC, 0xFF, 0xC1, 0xEF, 0x00, 0xDC, 0xFF,
        0xC1, 0xEF, 0x00, 0xDC, 0x00, 0x10, 0xDF, 0x00, 0xDC, 0xFF, 0xC1, 0xEF,
        0x00, 0xDC, 0xFF, 0xC1, 0xEF, 0x00,
    };

    // Where those bytes go: row 46 of the payload, i.e. file offset 55503496,
    // mapped through (lba + fo / 2048) * 2352 + 24 + fo % 2048. Both writes
    // land inside one sector, so neither needs splitting.
    static std::vector<Ketchup_DiskPatch> MGS1_BrightnessTextPatches()
    {
        std::vector<unsigned char> data(std::begin(MGS1_BrightnessTextData),
                                        std::end(MGS1_BrightnessTextData));
        return {
            {981, "USA", 0, 0x165A4790ull, data, "sc_text rows 46-69 blanked"},
            {981, "USA", 1, 0x11EE3E40ull, data, "sc_text rows 46-69 blanked"},
        };
    }

    std::vector<Ketchup_DiskPatch> MGS1_KetchupPatches = {};

    const std::vector<unsigned char> MGS1_DataBlacklist_Medicine = {
        0, 152, 0, 72, 152, 72, 152, 152, 152
    };

    const std::vector<unsigned int> MGS1_TextureWhitelist_Fonts = {
        0x0026f0, 0x00c1bd, 0x00e786,
        0x003528, 0x00352e, 0x00515b, 0x00c76a,
        0x00061d, 0x00c148,
        0x00cc46, 0x00cc47, 0x00fbf1, 0x00fbf7,
        0x00ac92
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_VR_EU = {
        {0x80099C18, MGS1_main},
        {0x80047D6C, MGS1_font},
        {0x800498E0, MGS1_font},
        {0x800499A8, MGS1_font},
        {0x80049B40, MGS1_font},
        {0x80048B68, MGS1_font},
        {0x80049CA0, MGS1_font},
        {0x80049D00, MGS1_font},
        {0x485E8,    MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_VR_US = {
        {0x800999B0, MGS1_main},
        {0x80047C68, MGS1_font},
        {0x800497DC, MGS1_font},
        {0x800498A4, MGS1_font},
        {0x80049A3C, MGS1_font},
        {0x80048A64, MGS1_font},
        {0x80049B9C, MGS1_font},
        {0x80049BFC, MGS1_font},
        {0x484E4,    MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_VR_JP = {
        {0x80096BD0, MGS1_main},
        {0x80044C88, MGS1_font},
        {0x80046664, MGS1_font},
        {0x8004670C, MGS1_font},
        {0x80046864, MGS1_font},
        {0x80045514, MGS1_font},
        {0x80045A78, MGS1_font},
        {0x800469C4, MGS1_font},
        {0x80046A24, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_Integral = {
        {0x80098F14, MGS1_main},
        {0x80044BC0, MGS1_font},
        {0x8004659C, MGS1_font},
        {0x80046644, MGS1_font},
        {0x8004679C, MGS1_font},
        {0x8004544C, MGS1_font},
        {0x800459B0, MGS1_font},
        {0x800468FC, MGS1_font},
        {0x8004695C, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_ES = {
        {0x8009A4DC, MGS1_main},
        {0x800D322C, MGS1_s03a_disable_mosaic},
        {0x800D9D80, MGS1_s03d_disable_mosaic},
        {0x80045D18, MGS1_font},
        {0x800477FC, MGS1_font},
        {0x800478B4, MGS1_font},
        {0x80047A28, MGS1_font},
        {0x800465BC, MGS1_font},
        {0x80046B28, MGS1_font},
        {0x80047B8C, MGS1_font},
        {0x80047BEC, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_DE = {
        {0x8009A870, MGS1_main},
        {0x800D3618, MGS1_s03a_disable_mosaic},
        {0x800DA16C, MGS1_s03d_disable_mosaic},
        {0x80045CAC, MGS1_font},
        {0x80047790, MGS1_font},
        {0x80047848, MGS1_font},
        {0x800479BC, MGS1_font},
        {0x80046550, MGS1_font},
        {0x80046ABC, MGS1_font},
        {0x80047B20, MGS1_font},
        {0x80047B80, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_IT = {
        {0x8009A404, MGS1_main},
        {0x800D31A8, MGS1_s03a_disable_mosaic},
        {0x800D9CFC, MGS1_s03d_disable_mosaic},
        {0x80045C48, MGS1_font},
        {0x8004772C, MGS1_font},
        {0x800477E4, MGS1_font},
        {0x80047958, MGS1_font},
        {0x800464EC, MGS1_font},
        {0x80046A58, MGS1_font},
        {0x80047ABC, MGS1_font},
        {0x80047B1C, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_FR = {
        {0x8009A91C, MGS1_main},
        {0x800D36D0, MGS1_s03a_disable_mosaic},
        {0x800DA224, MGS1_s03d_disable_mosaic},
        {0x80045D58, MGS1_font},
        {0x8004783C, MGS1_font},
        {0x800478F4, MGS1_font},
        {0x80047A68, MGS1_font},
        {0x800465FC, MGS1_font},
        {0x80046B68, MGS1_font},
        {0x80047BCC, MGS1_font},
        {0x80047C2C, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_UK = {
        {0x8009A3A4, MGS1_main},
        {0x800D3118, MGS1_s03a_disable_mosaic},
        {0x800D9C6C, MGS1_s03d_disable_mosaic},
        {0x80045ABC, MGS1_font},
        {0x80047448, MGS1_font},
        {0x800474E4, MGS1_font},
        {0x80047620, MGS1_font},
        {0x8004636C, MGS1_font},
        {0x800468C0, MGS1_font},
        {0x80047784, MGS1_font},
        {0x800477E4, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_US = {
        {0x8009BA50, MGS1_main},
        {0x800D4AC8, MGS1_s03a_disable_mosaic},
        {0x800DB61C, MGS1_s03d_disable_mosaic},
        {0x800471D0, MGS1_font},
        {0x80048B5C, MGS1_font},
        {0x80048BF8, MGS1_font},
        {0x80048D34, MGS1_font},
        {0x80047A80, MGS1_font},
        {0x80047FD4, MGS1_font},
        {0x80048E98, MGS1_font},
        {0x80048EF8, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_JP = {
        {0x80099CC4, MGS1_main},
        {0x80045978, MGS1_font},
        {0x80047304, MGS1_font},
        {0x800473A0, MGS1_font},
        {0x800474DC, MGS1_font},
        {0x8004622C, MGS1_font},
        {0x8004677C, MGS1_font},
        {0x80047640, MGS1_font},
        {0x800476A0, MGS1_font},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_SM_R3000 = {
        {0x80099C18, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_VRM_R3000 = {
        {0x800999B0, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_Integral_Disc1_R3000 = {
        {0x80098F14, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_Integral_Disc3_R3000 = {
        {0x80096BD0, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_ES_Disc1_R3000 = {
        {0x8009A4DC, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_DE_Disc1_R3000 = {
        {0x8009A870, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_IT_Disc1_R3000 = {
        {0x8009A404, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_FR_Disc1_R3000 = {
        {0x8009A91C, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_EN_Disc1_R3000 = {
        {0x8009A3A4, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_US_Disc1_R3000 = {
        {0x8009BA50, MGS1_main},
    };

    const std::vector<std::pair<unsigned int, PSXFUNCTION>> MGS1_ModuleTable_JP_Disc1_R3000 = {
        {0x80099CC4, MGS1_main},
    };

    const PSX_ModuleTables MGS1_ModuleTables { {
        { "mgs_r3000_vr_eu",          &MGS1_ModuleTable_VR_EU },
        { "mgs_r3000_vr_us",          &MGS1_ModuleTable_VR_US },
        { "mgs_r3000_vr_jp",          &MGS1_ModuleTable_VR_JP },
        { "mgs_r3000_int",            &MGS1_ModuleTable_Integral },
        { "mgs_r3000_es",             &MGS1_ModuleTable_ES },
        { "mgs_r3000_de",             &MGS1_ModuleTable_DE },
        { "mgs_r3000_it",             &MGS1_ModuleTable_IT },
        { "mgs_r3000_fr",             &MGS1_ModuleTable_FR },
        { "mgs_r3000_uk",             &MGS1_ModuleTable_UK },
        { "mgs_r3000_us",             &MGS1_ModuleTable_US },
        { "mgs_r3000_jp",             &MGS1_ModuleTable_JP },
        { "mgs_sm_r3000",             &MGS1_ModuleTable_SM_R3000 },
        { "mgs_vrm_r3000",            &MGS1_ModuleTable_VRM_R3000 },
        { "mgs_integral_disc1_r3000", &MGS1_ModuleTable_Integral_Disc1_R3000 },
        { "mgs_integral_disc3_r3000", &MGS1_ModuleTable_Integral_Disc3_R3000 },
        { "mgs_es_disc1_r3000",       &MGS1_ModuleTable_ES_Disc1_R3000 },
        { "mgs_de_disc1_r3000",       &MGS1_ModuleTable_DE_Disc1_R3000 },
        { "mgs_it_disc1_r3000",       &MGS1_ModuleTable_IT_Disc1_R3000 },
        { "mgs_fr_disc1_r3000",       &MGS1_ModuleTable_FR_Disc1_R3000 },
        { "mgs_en_disc1_r3000",       &MGS1_ModuleTable_EN_Disc1_R3000 },
        { "mgs_us_disc1_r3000",       &MGS1_ModuleTable_US_Disc1_R3000 },
        { "mgs_jp_disc1_r3000",       &MGS1_ModuleTable_JP_Disc1_R3000 },
        },
        std::bind([](const char *x, const char *y) {
                return strcmp(x, y) < 0;
            },
            std::placeholders::_1,
            std::placeholders::_2
        )
    };

    std::vector<Ketchup_TitleInfo> MGS1_Ketchup = {
        {99, "INTEGRAL", {
            {"INTEGRAL", {
                {0, 0x131D2238, Ketchup<>::PSX_DiskRange(0x9C000)},
                {1, 0x0EB38078, Ketchup<>::PSX_DiskRange(0x9C000)},
            }},
            {"VR-DISK", {
                {0, 0x000865F8, Ketchup<>::PSX_DiskRange(0x99800)},
            }},
        }},
        {101, "VR-DISK_US", {
            {"USA", {
                {0, 0x0000E5C8, Ketchup<>::PSX_DiskRange(0x9C800)},
            }},
        }},
        {102, "VR-DISK_EU", {
            {"EUROPE", {
                {0, 0x0018CCA8, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {980, "MGS1_JP", {
            {"JAPAN", {
                {0, 0x127F1478, Ketchup<>::PSX_DiskRange(0x9C800)},
                {1, 0x0E0B4AA8, Ketchup<>::PSX_DiskRange(0x9C800)},
            }},
        }},
        {981, "MGS1_US", {
            {"USA", {
                {0, 0x0000E5C8, Ketchup<>::PSX_DiskRange(0x9E800)},
                {1, 0x0000E5C8, Ketchup<>::PSX_DiskRange(0x9E800)},
            }},
        }},
        {982, "MGS1_UK", {
            {"UK", {
                {0, 0x119BF0C8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {983, "MGS1_DE", {
            {"GERMANY", {
                {0, 0x119BE798, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {984, "MGS1_FR", {
            {"FRANCE", {
                {0, 0x119BF9F8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {985, "MGS1_IT", {
            {"ITALY", {
                {0, 0x119BF0C8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D442538, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
        {986, "MGS1_ES", {
            {"SPAIN", {
                {0, 0x119BF9F8, Ketchup<>::PSX_DiskRange(0x9D000)},
                {1, 0x0D441C08, Ketchup<>::PSX_DiskRange(0x9D000)},
            }},
        }},
    };
};
