# Changes worth a separate upstream pull request

The `integral-english-text` branch contains the Integral English port and
MGSM2Fix changes that also help players without the port. This ledger tracks
the latter. Commit IDs identify implementations; test status is evidence from
the recorded sessions, not a claim that every supported title was tested.

| Commits | Change and scope | Verification / remaining work |
|---|---|---|
| `5921ec9` | Ketchup defers executable writes to the RAM mirror. General mechanism for Ketchup titles. | In use since August 2026; not a test of every title. |
| `640f359`, `eccecb9` | `[Game] EnglishText` holds Integral's initial language through title setup. | In use since 2026-08-28. |
| `a57859e`, `d386a09` | `[Game] UnlockBriefing` reveals all 16 briefing entries in Integral and USA; addresses derive from `scene_name`. | In use. It also seeds a new game's `var_buf` with those flags. |
| `6fb21a5`, `4528f91` | Squirrel RAM read/write hooks and call-stack logging; Integral language guard follows option-screen choices. | Trace identified the collection's rewrite. Japanese/English choices and title round-trip tested 2026-09-03. |
| `a5209ce` | `[Patches] PreserveConfiguration` carries only the bits changed by the collection into the current configuration word. All MGS1 versions. | Option toggles and 1P MODE passed 2026-09-03. A logged intervention in the intermittent race is still needed. |
| `6308afb`, `4757fab`, `10661d3` | `[Patches] BrightnessText`: `fixed` uses four lines at the original position, `original` restores six, `collection` leaves collection behavior. **USA/title 981 only.** | Both changed modes verified 2026-09-03 with achievements active. `fixed` matched the Integral port pixel-for-pixel. Disc 2 and the range filter's independent contribution remain unobserved. Integral uses its own PPF, independent of this setting. |
| `4757fab` | Built-in disc patches: a title ships disc-image bytes of its own through `Ketchup_DiskPatch` / `SQKetchupPatches()`, and `SQHook::SetPatchRangeBlacklist` drops collection CD-ROM patches by offset range. General mechanism; `BrightnessText` is its first user. | Shipping since 2026-09-03 inside the BrightnessText fix. Upstream the mechanism on its own; the ranges it is handed are per title. |
| `81de8c0`, `8d7a2e7` | `SQHook::SetPatchWatch` logs collection writes to selected CD-ROM ranges, including inline patches, before filtering. | Caught the 24-byte KEY CONFIG doorbell patch; on 2026-09-05 it mapped the collection's patches to six Integral stages (named-file patches register with no inline data, so only their offset and size are visible). Upstream the general mechanism separately from the Integral stage ranges `mgs1.h` registers â€” which since 2026-09-06 also cover five VR-disc stages. |
| `9bcca8f` | `Ketchup::Audit` reports whether RAM still contains the bytes Ketchup wrote. Read-only diagnostic. | Used in the save-message investigation; on 2026-09-05 it caught two of the three item-text faults in the act (a code offset writing into a ported string, and a rewrite landing in another). It does not repair foreign writes. |
| `5fa2cf5` | `[Game] GiveItems` grants specified items once per gameplay stage where Snake does not have them. | **Exercised for the first time 2026-09-07 on MGS1 USA disc 1, and it granted nothing.** An absent item is stored as **-1**, not 0 (`IT_None = -1`), so the "count is zero" guard could never fire. Fixed the same day, along with two related faults: it no longer writes `GM_ItemsMax` (for consumables the game reads `GM_Items[id + 11]` instead, and elsewhere the max is not consulted), and it leaves owned-but-disabled entries (`IT_TYPE_DISABLED`, 0x8000) alone. Granted items persist in subsequent saves, and the corrected version was confirmed on screen the same evening: all 24 in the inventory. |
| (this branch) | `[Game] GiveWeapons`: the same for the ten weapons, which are a separate pair of arrays (ammo in `GM_Weapons`, capacity in `GM_WeaponsMax`) in MGS1's own `WP_` order. Ownership is `GM_Weapons[i] >= 0`, per the game's own menu code. | Added 2026-09-07 because the items alone were not what was wanted. A granted weapon receives the full magazine the game already records and an empty one where no capacity is recorded; no ammo count is invented and `GM_WeaponsMax` is never written. **Confirmed on screen**: used together with `GiveItems` to reach and photograph the weapon and item descriptions on both MGS1 USA and Integral for the port's comparison work. |
| (this branch) | `[Patches] ThinTexturedQuads`: a mid-hook on the emulator's GP0 polygon dispatch (`PSX::GPU_PolygonCommand`). For a textured polygon whose Y (or X) extent is exactly one pixel and whose leading-edge vertices agree on V (or U), every vertex takes that texel, which is the row the PlayStation GPU rasterises; the collection's renderer sampled a different row and drew MGS1's briefing connector lines faint or not at all, for USA and Integral alike. PSX machine, all titles that run it; default on. | Found 2026-09-10 by comparing SwanStation and collection shots of the same disc bytes; the sampling difference was measured (+25 over background on the collection, the dark texel's value). Hook site unique in the MGS1 executable; the same signature must be checked on the other titles' executables before enabling there. **Seen on screen 2026-09-11 00:07**: hook logged at start-up, the briefing's outline connector measures +205 over the background where it measured +0 the night before, and nothing else on the screen changed. |
| (this branch) | `build_zydis.cmd` takes the main project's `$(PlatformToolset)` as a second argument and builds Zydis with it. Zydis.vcxproj hardcodes v143; a machine with only a newer Visual Studio (here VS 18, toolset v145) failed the prebuild with MSB8020. No effect where v143 is installed. | Built 2026-09-11. |
| `3f04e4a`, `d988e1d` | `[Game] StageSelect` accepts `select1` through `select4` directly; `true` retains the developer top menu. | `select3` reached `s11a` on disc 1. Direct `s14e` hung with missing story state. This proves menu selection, not disc-2 loading or safe entry into every stage. |

Current maintenance update: the RAM repair loop verifies every byte and writes
only differences, retaining the bounded retry/back-off. Overlapping RAM runs are
normalized in application order so the last mod wins without an internal repair
fight. Native tests cover interior corruption and overlapping runs; runtime
regression remains open. Brightness range filtering now keys USA title 981 and
the selected disc. The former unscoped range behavior is historical.

Three further Ketchup changes were made on 2026-09-07, all general and all
independent of the Integral port. They are in `src/m2fix/ketchup.{cpp,h}` and belong
in the same pull request as the rest:

* **`ApplyBlock` warns when a record is not fully mirrored.** A write that runs
  into a raw sector's 304-byte tail reaches the disc image and never RAM, which
  used to be silent while the log said the patch loaded â€” it once cost a patch
  142 of 442 bytes. It now names the offset and the counts.
* **`ReportOverlaps` warns when two PPFs in one folder write the same byte with
  different values**, naming both files, once the folder has been processed. A
  mod that works until another is dropped beside it is otherwise invisible;
  agreeing writes are not reported, since order cannot matter for those.
* **`ApplyPPF3` validates the whole record chain before applying any of it.** A
  malformed file used to be walked to a byte count that never reached zero,
  applying garbage in a loop â€” a 306 MB log, once. A patch is now either wholly
  applied or wholly refused, with a reason.

The PPF3 **block check is still skipped**, and now says so in the log rather than
implying it was honoured: verifying it means comparing 1024 bytes of the image at
0x9320, and the emulator interface this fix has only writes. Reading the image
back is what that would need.

Port scripts, PPFs and port-specific README sections remain outside these
upstream changes. Current Integral build and audit status live in
[BUILDING.md](tools/integral-english/BUILDING.md) and
[COVERAGE.md](tools/integral-english/COVERAGE.md).


2026-09-11: optional PPF controls were added at the user's request. Ketchup now
obtains a deterministic per-disc loading plan and can apply validated per-file
byte overrides; the MGS1 mapping lives in `src/games/mgs1_patch_options.h`.
Seven INI settings cover the main/VR Integral English groups, the grenade delay
correction, VR missions/extras/movies unlocks and story title bonuses. Test aids
default off. The grenade control selects Japanese/English assets and restores
the original English numeral when disabled; companions pin the exact PPF layout.
No game assets are embedded in this C++ implementation. The file-selection and
configuration mechanism can be reviewed independently; the named Integral
assets and companion generation belong with the patch toolchain. Native tests
cover selection and metadata failures; four plans were applied to real disc
stages and verified. Release x64 built and was deployed; in-game toggle testing
is still pending. Historical details: tools/integral-english/HANDOFF-ARCHIVE.md
sections 5.15-5.16; current testing needs: tools/integral-english/NextSteps.md.
