> Historical/reference record. For current instructions see [README.md](README.md),
> [BUILDING.md](BUILDING.md), and [NextSteps.md](NextSteps.md). Dated counts,
> machine paths, deployment state, and completed-work estimates below are not current instructions.

# Next steps — MGS Integral English text port

Written for whoever picks this up cold: a later session of the same assistant,
a different model, or a person. It says where everything is, what the user's
rules are (verbatim), how far each piece is verified, what remains and in what
order, and which decisions are the user's to make. The technical record — byte
formats, mechanisms, every gotcha with its evidence — is `REFERENCE.md` beside
this file; section names are quoted below so they can be found.
[`BUILD-HISTORY.md`](BUILD-HISTORY.md) is the reproducible build and packaging
procedure; [`COVERAGE-RECORD.md`](COVERAGE-RECORD.md) is the text-coverage inventory and its
limits; [`SCRIPTS.md`](SCRIPTS.md) is a one-line-per-script index. `UPSTREAM.md`
at the repo root tracks the MGSM2Fix changes that deserve their own upstream
pull request. **These repo documents are authoritative** over anything in the
assistant's private memory files, which may still exist but can be stale.

The dated, session-by-session account of how this port was built — first
written 2026-09-04 and updated through 2026-09-11 — now lives in
[`HISTORY.md`](HISTORY.md). References to §9 and above point there; this file's
§1–8 hold the current state of each item.

**Latest handoff, 2026-09-11:** the English sets and grenade fix are enabled.
All four new unlock controls and the separate `UnlockBriefing` are **false** in
the active INI. The eight unlock PPFs remain installed behind those controls;
parked copies are backups. The user confirmed that turning `UnlockBriefing`
off fixed the still-unlocked Integral briefing menu. No save was edited or
deleted. This verifies the existing briefing control, not the seven new PPF
toggles; their full in-game matrix remains pending (§5.16).

---

## 1. Where everything is

| what | where | notes |
|---|---|---|
| MGSM2Fix repo | `C:\Users\Tideg\My Drive\Development\MGSM2Fix`, branch **`integral-english-text`** | based on MGSM2Fix **3.6.0**; upstream is now **3.7.2** — a rebase is needed before any upstream PR |
| remotes | `origin` = `https://github.com/TideGear/MGSM2Fix.git` (push here); `upstream` = nuggslet's MGSM2Fix — **never push to upstream** | |
| decompilation | `D:\mgsbuild\d`, branch `integral-english-text`, origin `FoxdieTeam/mgs_reversing` — **do not push there** | our source changes are captured as `tools/integral-english/decomp-overlay-changes.patch` (= `git diff 7964de7`); regenerate it after any decomp edit. Local decomp commits exist (e.g. `0534934` for the doorbell in `opt.c`) |
| working data | `D:\mgsbuild\integral-english-work\` — `work\` (extracted STAGE.DIRs, the four retail executables, built binaries, baselines), `unlocks_parked\` (backup unlock PPFs; active copies are now INI-gated), `keyconfig_test\`, `map_pristine.map` (the pristine exe's symbol map), ini/log/`opt.c` snapshots | every tool imports `WORK` from `workdir.py`: `INTEGRAL_ENGLISH_WORK` env var → `D:\mgsbuild\integral-english-work` → cwd. `workdir.py` also exports `GAME` (`INTEGRAL_ENGLISH_GAME`, default the Steam folder) and `DECOMP` (`INTEGRAL_ENGLISH_DECOMP`, default `D:\mgsbuild\d`); the builders and `rebuild.py` take every path from it. As of 2026-09-11 every standalone helper does too — `ppfcheck.py`'s and `unlock_title.py`'s `MODS`, `optbright.py`'s and `preope_both.py`'s `OVL`, `audit_text.py`'s argparse defaults, `verify_usa_brightness.py`'s ASI/`mgs1.h`/`alldata.bin` paths, and the sibling-module `sys.path` inserts in `jpsweep.py`, `kcplace.py`, `kcquads.py`, `kcrects.py` and `verify_integral_option.py` no longer require the author's paths (the ASI finder retains a last-resort local fallback). `py workdir.py` prints what it resolved |
| VR working data | `work\vrint_stage.dir`, `work\vrus_stage.dir` (the two VR STAGE.DIRs), `work\vrint.exe` (rebuilt from the decomp, `build.py --variant vr_exe`, SHA-256 `c370f8e4…`), `work\vrus.exe` (real `SLUS-00957`), `work\INTEGRAL_vr_*.ppf` | `vrlib.py` finds the two VR ISOs inside the containers itself (`0x57592000` and `0xD39B7000`) and computes stage LBAs from STAGE.DIR |
| retail executables | `work\int1.exe`, `int2.exe` (641,024 bytes each), `us1.exe`, `us2.exe` (651,264) — hashes in `BUILD-HISTORY.md`; `rebuild.py` rejects any other | **the collection's ISO executable extents are zero-filled**, so extracting an exe from `alldata.bin`/`dlc_japan.bin` yields no code — the first clean-build attempt failed on exactly that. These four files are the only source of executable bytes |
| reproducible build | `py rebuild.py --output <fresh dir> [--variant raw] [--compare-deployed]` (see `BUILD-HISTORY.md`) | never installs anything. Builds **everything**: ten families × two main discs plus the VR disc's seven. Last artefact `D:/mgsbuild/repro20/Integral-English-collection.zip`, SHA-256 `9dff4849…bce3` (2026-09-08), **all 27 PPFs equal to the deployed set's effective bytes**. `--variant raw` builds the raw-disc variant instead (§5.4) |
| game | `D:\Steam\SteamApps\common\MGS1` (Master Collection Vol. 1, Steam app **2131630**) | launch: `Start-Process steam://rungameid/2131630`; process name `METAL GEAR SOLID`; **kill by PID only, never `taskkill /IM`** |
| Ketchup mods | `D:\Steam\SteamApps\common\MGS1\mods\INTEGRAL\INTEGRAL\0` (disc 1) and `\1` (disc 2); the VR disc is `mods\INTEGRAL\VR-DISK\` and the USA VR disc `mods\VR-DISK_US\` | Ketchup loads every PPF in the folder, so each patch is its own file and can be removed individually. Its `RootPath` adds a version folder only when a title has more than one version and a disk folder only when a version has more than one disk, which is why the two VR folders have no numbered subdirectory |
| deployed ini | `D:\Steam\SteamApps\common\MGS1\MGSM2Fix.ini` is a **Vortex symlink**; edit the target: `%APPDATA%\Vortex\metalgearsolidmc\mods\MGSM2Fix-5-3-6-0-1774482213\MGSM2Fix.ini` | edit with Python or via `realpath`; `sed -i` on the link would replace the link with a file. The repo's `MGSM2Fix.ini` is the committed default, not what the game reads |
| deployed ASI | same Vortex folder, `MGSM2Fix64.asi` | |
| build | `"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" MGSM2Fix.sln /p:Configuration=Release /p:Platform=x64` → `x64\Release\MGSM2Fix.asi` → copy to the Vortex folder as `MGSM2Fix64.asi`; compare hashes | **Run it from PowerShell**: Git Bash rewrites `/p:` into paths (MSB1008). Since the 2026-09-10 rebase the Zydis prebuild needs the toolset passed through (`build_zydis.cmd` second argument, done 2026-09-11). The post-build step calls `python M2Install.py`, which is not on this PATH (`py` is), so MSBuild reports exit 1 with error 9009 **after** the ASI is linked - the `.asi` is good; deploy by hand. If MSBuild times out it can leave `cl.exe` processes behind — stop them by PID |
| log | `D:\Steam\SteamApps\common\MGS1\MGSM2Fix.log` (rotates to `.prev`) | **Corrected 2026-09-07.** The boot-time `Error parsing ini file ... at these lines: 12` was recorded here as a harmless inipp quirk on the `[Internal Resolution]` header at line 12. It was neither. inipp prints the offending line's **content**, not its number, and the content really was a bare `12` — the deployed ini's `[Update Notifications]` section header had been overwritten by it at some point. It was not harmless: with the header gone, `CheckForUpdates = true` fell into `[Game]` and the fix read `bShouldCheckForUpdates: false`. Header restored; the file now has all ten sections and no unparseable line. **If that message comes back, read the line it quotes as text and go find it.** |
| screenshots | `C:\Program Files (x86)\Steam\userdata\7924217\760\remote\2131630\screenshots` | 3840×2160; 9 display px per game px, x offset 480 |
| USA source data | `work\usa1_stage.dir` / `usa2_stage.dir` (real USA discs, extracted from `windata\alldata.bin`); `work\us1_stage.dir` is **European** despite its name — do not source text from it | README "Toolchain and environment" and the source-discs note |
| credit and provenance | [`CREDITS.md`](CREDITS.md) | whose work this is built on, what of it is in this repository, and the one thing that is **not settled**: the decomp states no licence |
| git identity | `git -c user.name=TideGear -c user.email=tidegear@gmail.com commit` | **no `Co-Authored-By: Claude` or any AI attribution in commit messages** |
| title ids | from `MGS1_Ketchup` in `src/mgs1.h`: **99** INTEGRAL, **980** MGS1_JP, **981** MGS1_US, 101/102 VR, 982–986 EU | attribute a collection patch to a title by its id, never by the order lines appear in a log |

---

## 2. The user's standing rules — verbatim

These were given during the work and govern everything. Quote them, do not
paraphrase them away.

1. **No translation.** "Btw I'm not authorizing you to translate (yet) anything only in english with no port should be left in Japanese." — Any string without an English counterpart in a released build stays Japanese exactly as it is: not blanked, not paraphrased, not abridged. When English is longer than Integral's slot, grow the slot; never shorten the text.
2. **Previous Operations:** "For Previous Operations, change the page count. Do not edit."
3. **Scope:** "the goal is verbatim text placed identically, but integral could have relevant adjustsments (since it was a later release) that are worth keeping I'm ok with keeping Integrals differences if they aren't text translation and appropriate positioning." — Fix text and the chrome that positions text (rules, connectors, highlight boxes, row spacing); leave Integral's colour, brightness, blend and background-art differences alone and note them in the README's Scope table.
4. **Amendment (2026-09-03):** "Where Integral's art/gui/hud/etc. is intentionally different, consider moving the English text to fix it, but ask me first." — Measure the relationship USA has between text and the art it relates to, reproduce that relationship against Integral's art, do not port the art — and **ask before doing it**, case by case. Worked examples: `key_syukan` on KEY CONFIG, and the VR MOVIE EXIT box (§5.4a), where the chrome moved rather than the text. **The default, in the user's words (2026-09-07): "Usually we skew toward the Integral visuals."** So every such move is an exception that has to be asked for and written down.
4b. **Amendment (2026-09-07): Integral's own ENGLISH may be replaced by USA's,
   when asked.** The no-translation rule and the scope rule were both written
   for Japanese text: port English where English exists, keep what Integral did
   differently. Neither says what to do when **Integral already has English and
   it differs from USA's**. The first case forced the question and the user
   settled it, of the item side-column abbreviation: *"this is a case where we
   are replacing existing english in integral instead of japanese, so it's an
   exception to our rule."*

   **Applied twice so far, and each time it was asked first.** `SCARF` ->
   `HANDKER` (2026-09-07, §5.9) and the four `abst` location names (2026-09-08,
   §5.9). The amendment is a licence to ask, not a default: where Integral's own
   English merely reads differently and nobody has asked, it stays.

   So it is an exception, not a new default: an Integral string that is already
   English is left alone unless it is asked for, case by case, exactly like the
   art amendment above. Done so far: **one**, `SCARF` -> `HANDKER` (§5.9).
   Still outstanding under the same heading and NOT done: the `abst` location
   spellings (four of them — §5.9).

5. **In the collection:** "in MC i prefer the circle message suppressed and key config intercepted" — the four-line brightness text (no ○-button line) and the collection's own Control Settings panel for KEY CONFIG.
6. **The raw disc matters:** "\"They'd still matter for a raw PSX disc patch.\" that was the point of porting the text. I want the intercept still in mc." — Text the collection hides (KEY CONFIG labels, disc-swap prompts) is still ported for a future raw-PSX-disc patch, while the collection keeps its interception.
7. **Documentation:** "Make sure you're remembering to document and gotchas worth documenting" — and, 2026-09-04: nothing important may live only in a conversation or a memory file.
8. **Upstream tracking:** anything changed in MGSM2Fix that benefits players who do not use the port goes into `UPSTREAM.md` for a separate PR; port-only changes stay out.
9. **Be careful:** "I need you to be more careful. Stop guessing when the hard data is available to you." — see §3.

---

## 3. How to work here (distilled from the mistakes)

- **Measure before theorising.** The decompiled source, both games' data, the retail binaries, the font tables and a relaunchable game are all available. Before proposing why something breaks, ask what single check rules it in or out, and run it.
- **Bisect against a stock run first.** Establish the fault is in our change before reasoning about mechanisms. The option-screen freeze implicated both overlay growth and `f924[12]`; reverting both together confounded the size-only diagnosis. Keep the conservative size guard and retail's `f924[8]`. KEY CONFIG interception was found by bisection plus `SetPatchWatch`.
- **Check the cheap invariant.** Sizes, counts, hashes against the known-good before logic.
- **Absence of observation is not a negative result.** Before saying "there is no X", establish you would have *seen* an X. Three wrong conclusions in one day came from this (README "The collection's KEY CONFIG interception").
- **Re-run every static check on the artefact you actually deploy** — a check on build N says nothing about build N+1.
- **Do not "fix" retail's quirks in an overlay.** `f924[8]` must stay `[8]`; growing it caused the very freeze it was meant to cure.
- **Read the README section for a stage before touching it.** Each stage has its own traps; the README records them with evidence.
- **Run `ppfcheck.py` before any PPF goes near the game.** A 60-byte description in the 50-byte field once crashed the game and produced a 306 MB log.
- **Keep a GCL chain's byte delta at exactly zero when you can** (pad the shortest record with trailing spaces); otherwise use `gclparse.containers_over` to resize every enclosing container. A −8,055-byte shift crashed the script with no exception.
- **Never let an executable PPF record cross a 2048-byte payload boundary** — Ketchup drops the spill silently while logging success.
- **The extracted `int1_stage.dir` is unpatched.** To see what the deployed game shows, apply the deployed PPF first (see the `en_camsave` verification, 2026-09-04) — reading the extraction alone shows Japanese everywhere and proves nothing.
- **Function pointers decode as "text".** `addiu sp,sp,-N` (`c0 ff bd 27`…) in a pointer table is code. Known in `camera` at overlay 0x6E0–0x6E8.
- **Never take executable bytes from the collection's disc images** — their exe extents are zero-filled. Use the four retail executables in `work\` (§1).
- **Builders stage into `WORK` and deploy only with an explicit `--deploy`**; `rebuild.py` never touches the game folder. Compare a rebuild to the deployed set by *effective changed bytes* (`--compare-deployed`), not by PPF file hash — record grouping and the description text can differ without changing game data.
- **A normal disc swap proves only the normal swap path.** The title / wrong-disc, demo-theater and abstract copies of the disc-swap text need their own evidence; do not infer "unreachable" for all four from one silent swap.
- Absolute dates in notes, not "yesterday". Kill by PID. No AI attribution in commits.

---

## 4. State on 2026-09-07: what ships, and how far each is verified

### PPF patches (all deployed for both discs unless noted)

| patch | what | verified |
|---|---|---|
| `en_items` | item and weapon descriptions, the frozen Ration/Ketchup pair, the HARD/EXTREME Mine Detector message (executable) | **Fully verified against USA on screen 2026-09-07**: all 24 items and all 10 weapons photographed in both games and compared side by side, text, line breaks and glyphs - `<<`/`>>`, the O/X/[] button glyphs, apostrophes, the `<Limitless>` single angle quotes. **34 of 34 identical.** The only difference anywhere in those shots was the side column's short name for item 22, `SCARF` against `HANDKER` - a different table the port did not touch. **Asked and changed the same evening** (§5.9, and §2 amendment 4b: the port's first replacement of Integral's own English); redeployed 22:56 and **seen on screen at 23:03**. The two conditional descriptions were photographed the same evening too (§5.1a), so every form of every item and weapon description in this family has now been read on screen. Earlier: in game. Three faults found from the user's shots and fixed 2026-09-05 (card level digit offset, SOCOM suppressor rewrite, a retail-equal byte the collection's RAM patch owned — README "Three item-text faults"); **the fixes were seen on screen at 12:55** (SOCOM, ID Card `level 7 security`, Mine Detector) and the audit is silent. The PPF owns every byte of both arenas |
| `en_menu`, `en_menu2` | menu strings; `en_menu2` includes the `demosel` and `change` disc-swap copies | in game (menus); the disc-swap copies **never seen** (see §5.1) |
| `en_option` | option-screen strings; KEY CONFIG labels (8 textures); brightness paragraph as USA's `sc_text` texture, four lines in the collection build | in game, pixel-measured; SCREEN / KEY CONFIG (collection panel via the doorbell) / EXIT all confirmed 2026-09-04 |
| `en_preope` | Previous Operations, USA's exact pagination (MG1 13 pages, MG2 19) | in game, 29 lines pixel-exact |
| `en_brf` | briefing labels, quads, row arithmetic | in game, 26 shot pairs, 0.00% right-column diff |
| `en_savemsg` | memory-card captions in the executable | in game 2026-09-04: save + load; kept slots idx 1/9 Japanese by rule. Since 2026-09-05 the PPF owns every byte of the pool and tables, so the collection's six writes cannot survive at retail-equal bytes (the mechanism that broke the SOCOM line) |
| `en_camsave` | the PHOTO ALBUM's own captions (`camera` overlay) | **fully verified 2026-09-04**: all 23 English on screen / by slot comparison; the six USA-blank slots stay Japanese (`ロード中です`, `ロードが完了しました`, `変更内容を上書き保存しますか？` are those) |
| `en_abst` | the MISSION LOG: all 122 pages in USA's two-screen model (7 lines a screen, page counter, ◄ ► EXIT, USA's input and slide), plus the disc-change abstract's eight strings — the fourth disc-swap copy | **built 2026-09-05 and seen on screen the same day**: both pages of the Heliport and Comm Tower A logs, the controls and the slide (the one fault, stale-VRAM fragments during the slide, fixed at 13:05 and confirmed clean at 13:50). Statically, pages re-parse and equal USA's byte for byte and the PPF records rebuild the relocated 88-sector stage exactly on both discs. Stage in DUMMY3M slots 462..549. **Since 2026-09-08 it also carries USA's four location-name spellings** (`USA_LOCATION_NAMES`, §5.9): +12 bytes, still 88 sectors, and the verifier re-parses the 31-record list and asserts it equals USA's record for record. Not yet seen: a demo.gcx page (disc-2 saves), a count-7 page, and the four location names |
| `en_menu3` | the `title` disc-swap copy — the fourth and last | **built and verified 2026-09-07, RAW DISC ONLY, not deployed.** The collection patches the same block (`disc1_1822B55D_patch`, at the address of our record 0), and the two layouts do not mix: the title stage dies with a `GCL:WRONG CODE` run. Staged as `INTEGRAL_disc{1,2}_en_menu3_raw.ppf` for the raw variant; `menu3.py --deploy` refuses. §5.3 |
| `en_pad2` | the controller-port subtitle `second.c` draws in the Psycho Mantis room, at all three of its call sites per disc | **built and deployed 2026-09-08**, 159 bytes a disc. Statically verified by effect: the archive keeps its length, both stages re-walk to the same command structure, every changed byte is inside the three slots, and each slot re-parses at length 55 with USA's English at the front. Disjoint from all nine other PPFs on each disc. **Never seen on screen** — it needs the Mantis room *and* a controller in port 2. §5.11 |
| unlock PPFs | title-screen extras | installed but disabled by `UnlockTitleBonuses = false`; parked copies retained as backups (§5.16) |

### VR-DISC patches (deployed 2026-09-06 in `mods\INTEGRAL\VR-DISK\`)

Ported from USA's VR Missions (`SLUS-00957`). README "The VR disc (SLPM-86249)"
is the technical record; `vrlib.py` is the shared library. **Seen on screen by
the end of 2026-09-07:** the option screen, KEY CONFIG (with the two flags on),
the mission menu with every mission unlocked, all three MOVIE captions, the
EXTRA menu with every item, mission title and briefing windows, and an item and
a weapon description. **Still unseen:** a mission RESULT window, three of the
EXTRA help lines, a save and a load message, the PHOTOGRAPHING card messages,
and the EXIT box after its move — §5.5's list 1 and §5.4a.

| patch | what | verified |
|---|---|---|
| `vr_en_missions` | 1808 of 1813 in-mission windows across 92 stages: titles, briefings, results, hints | statically: every stage re-parses, no stage grew (ten padded back to their sector count), 15 031 records / 3 370 955 bytes, fonts merged and every remaining glyph code proved to exist in the new font |
| `vr_en_items` | the VR executable's item, weapon and capture-mode pools | statically; the PPF owns every byte of all three arenas, as the main game's does since the SOCOM fault |
| `vr_en_savemsg` | the VR executable's 12 save and 12 load messages | statically; indices 1 and 9 stay Japanese (USA draws nothing) |
| `vr_en_option` | the option screen's 7 help lines and the whole KEY CONFIG screen | **the option screen is verified on screen 2026-09-06** after three faults, all found by bisecting the PPF: the DAR's entry sizes were not 4-aligned and crashed the stage at `load option`; record 3 doubled the vibration-test sentence; and Integral's colon/values were lit beside the English while the lines sat off-centre. Fixed by padding every DAR payload to 4 (paid for with `pcx4`'s real 63-byte run cap), blanking record 3 as the main game does, unlighting the colon/values via the state switch, and giving each ported entry USA's `{num 1, x 160, y 196}`. All five rows now read as one centred English line, measured within 0.3 game px of centre. **KEY CONFIG verified on screen 2026-09-07** with `DisableRAM`/`DisableCDROM` on: Integral's own screen draws, all eight labels English through all three button types, and `key_syukan`'s +11 clears the curve. One fault found and fixed the same day — the selection highlight on the `first person view` row was 24 px short (88 against USA's 112) because it is drawn by hardcoded `glow(work, x, y, w, h, ...)` calls rather than an `Init_Res` quad, so the transplant never touched it; measured 113 px against USA's 114 after the fix |
| `vr_en_title` | the EXTRA menu's four help lines | statically; record 6 (PocketStation) deliberately kept — USA's `See the staff credits.` is a different feature |
| `vr_en_memcard` | the memory-card captions of the `vrsave` and `selectvr` overlays (SAVE / LOAD / REPLAY DATA screens): 12 save + 12 load each, USA's English index for index, 1/9 and the prompts kept | **built and deployed 2026-09-11 11:29**, 7 records, both pools 390/492 bytes; verified by reading every slot back. Found from the CLEAR DATA screen - whose own copy, in `vrtitle`, is Japanese in USA too and stays. Not yet seen on screen: its captions are the error and edge states (Save failed, No save file, No empty block...), which the 22 shots of 11:11 did not reach |
| `vr_en_camsave` | the PHOTOGRAPHING mode's memory-card messages | statically; 429 of the pool's 492 bytes used. Never seen on screen |
| `vr_en_movie` | the MOVIE selection captions | **all three ported 2026-09-07**, the two TGS ones as USA's two lines. The line count was never data: USA calls the actor's own `highlight(work, i)` twice — for `clip*2` and `clip*2+1` — where Integral calls it once, so the port retargets that one `jal` at a 16-word stub in the overlay's own sector padding. **Verified on screen 2026-09-07**, all three clips: both TGS captions on two rows with correct attribution and real typographic quotes, E3 on one. Line 1's ink then overlapped the EXIT box by 2 rows, because Integral's caption face is taller than USA's; the box moved up 4 px to USA's own y with the user's approval (§5.4a, §6), and **that part is not yet seen on screen** |
| `vr_unlock_movies` | the EXTRA movies unlocked (test aid) | **verified in game 2026-09-06: all three thumbnails appear.** One instruction in the `movie` overlay: its own `count / 3` score gate, separate from the mission one. Writes no progress; disable its INI control and restart to relock |
| `vr_unlock` | the **mission menu** unlocked (test aid) | **verified in game 2026-09-06: every mission unlocked.** Emulation predicted 46 → 361 of 373 items on Integral (45 → 357 on USA) and its three words were verified in place against the deployed PPF. It does **not** open the EXTRA movies — that is a separate retail gate, `???` in USA's VR disc too. Saving with it in place is safe (it writes no progress) and disabling its INI control then restarting relocks; the standing rule still holds — achievements off (`DisableRAM`/`DisableCDROM` true) while any unlock aid is enabled |

`ppfcheck.py --deployed` is clean over all **27** deployed files (20 main, the
ten families × two discs, and the VR disc's seven), and **every one of them is
disjoint from every other** as of 2026-09-08.

That last part is new, and it is what §5.10 item 1 bought. Until then
`vr_en_movie` shared 686 of its 753 bytes with `vr_en_missions` and worked only
because Ketchup applies a folder in name order and `...missions` sorts before
`...movie` — a dependency nothing enforced and nothing would have reported. The
two now own one stage each and share **0 bytes**; the split was proved equal in
effect before it was kept, and `rebuild.py` refuses *any* VR overlap. The
deployed pair was still the old one until 2026-09-08, when the VR seven were
redeployed from `repro17` together. The superseded `vr_en_movie_e3` still must
**not** sit in the folder — it writes the same stage — and `vr_movie.py
--deploy` moves it to `work/` with a `.was-deployed` suffix. `vr_sweep.py` rebuilds every stage as the
game will see it and finds **222 game-encoded records against 10 809 English**
(USA's own disc: 940 against 10 344); 181 of the 222 are English with
local-font glyphs, and the rest are exactly the list the README calls
"Deferred, with reasons". Nothing with a USA English counterpart is still
Japanese.

**Reproducibility:** every one of the ten shipping families is rebuilt from
retail inputs in an isolated directory by `rebuild.py` — stage files extracted
from the collection, the four retail executables as hashed inputs, the decomp
exported at `7964de7` plus `decomp-overlay-changes.patch`, three overlays
recompiled (byte-identical to the shipped ones) — and all 20 PPFs match the
deployed set's effective changed bytes. **Since 2026-09-07 the VR disc's seven
are in the same run** (its executable built from the decomp, not copied), so the
last clean run, `repro20` (2026-09-08), reproduces **27 of 27** against what is deployed. So the deployed patches are
no longer artefacts of a lost scratchpad: they can be regenerated. `BUILD-HISTORY.md`
has the inputs, hashes, command, outputs and the ZIP's hash. This is static
equivalence, not a new gameplay test.

### MGSM2Fix features on this branch (see `UPSTREAM.md` for the upstream view)

| feature | ini | state |
|---|---|---|
| Ketchup RAM-mirror deferral | — | in use daily |
| `[Game] EnglishText` (+hold, +guard restoring English outside scene `option`) | `EnglishText = true` | tested, follow-the-player path tested |
| `[Patches] PreserveConfiguration` | `= true` | three clean runs; the race it guards has not been caught in the act |
| `[Game] UnlockBriefing` | `= false` | active INI now false; user confirmed this relocked Integral's briefing menu on 2026-09-11. When enabled, seeds new-game `var_buf` |
| `[Patches] BrightnessText` (tri-state `fixed` / `original` / `collection`) | `= fixed` | USA only; fixed and original verified on disc 1. Integral's paragraph is built into its PPF independently of this setting |
| `[Patches] ThinTexturedQuads` (2026-09-11) | `= true` | a mid-hook on M2's GP0 polygon dispatch: a textured polygon one pixel tall (or wide) takes the texel of its leading edge, which is what the PlayStation GPU draws; the collection sampled another and drew MGS1's briefing connector lines faint. All PSX titles; seen on screen 00:07 and in twenty unlocked-briefing pairs 00:13-00:17 (§26). Deployed as `MGSM2Fix64.asi` SHA-256 `9ea87429…` |
| Ketchup built-in disc patches + `SetPatchRangeBlacklist` | — | shipping (the USA four-line brightness fix) |
| `SQHook::SetPatchWatch` (logs collection patches landing in a region) | — | in use; watches on `option`, `abst`, `change`, `demosel`, `title` and `camera` spans on both main discs, and since 2026-09-06 on the VR disc's `option`, `camera`, `vrtitle`, `movie` and `vrsave` spans (ASI rebuilt and deployed 2026-09-06 00:25) |
| `Ketchup::Audit` (every byte of every RAM run, read-only, every ~5 s) | — | in use; it caught two of the three item faults on 2026-09-05. Since both exe PPFs own whole regions it now sees every byte of both pools |
| `[Game] GiveItems` (test aid) | `GiveItems =` (empty) | confirmed on screen with `GiveWeapons` for the USA/Integral inventory comparisons; see §5.6 and UPSTREAM.md |
| `[Game] StageSelect` = `true` / menu name / stage name | `StageSelect = false` | works; see README "The disc-swap text" for what it can and cannot reach |

### Deployed ini right now, and the play defaults

**Current live state, 2026-09-11 after the briefing follow-up:**
`UnlockBriefing = false`; `UnlockVRMissions`, `UnlockVRExtras`, `UnlockVRMovies`
and `UnlockTitleBonuses` are also false. The eight unlock PPFs remain installed
but disabled. `IntegralEnglishPatch`, `IntegralVREnglishPatch`,
`GrenadeDelayFix` and `ThinTexturedQuads` are true. The earlier briefing test
session had left `UnlockBriefing` enabled; the user confirmed turning it off
fixed the menu without save edits. Other existing INI settings were preserved;
check the active INI before changing achievement/test settings. See 5.16.

**Historical state, 2026-09-08 22:20 — back at the play defaults, and the test session was over.** `DisableRAM = false`, `DisableCDROM = false` (achievements live), `GiveItems` and `GiveWeapons` both empty, and **no `_unlock_` PPF anywhere under `mods\`** — all four aids (`INTEGRAL_vr_unlock_{missions,movies,extras}.ppf` and `VRUS_unlock_missions.ppf`) were deleted. Remember the ini the game reads is the **Vortex symlink target**, `%APPDATA%\Vortex\...\MGSM2Fix.ini`; write that, not the link. **27 PPFs are deployed and `ppfcheck.py --deployed` is clean over all of them**: 20 main (the ten families × two discs) and the VR disc's seven. The VR seven were redeployed from `repro17` the same evening, which is what finally put the **disjoint** `vr_en_missions` / `vr_en_movie` pair on disk — the deployed pair had still been the old overlapping build, 686 bytes shared and every one of them conflicting, working only because Ketchup applies a folder in name order. They now share **0 bytes**, and the whole VR set was proved equal in effect to what it replaced before it went on (`vr_set_effect_equal`, no differences).

**Play defaults:** `DisableRAM = false`, `DisableCDROM = false` (achievements live), `StageSelect = false`, `GiveItems =`, `EnglishText = true`, `BrightnessText = fixed`, `PreserveConfiguration = true`, `UnlockBriefing = false`, and all four PPF unlock controls false (installed unlock files may remain). The user has real saves: **Heliport** and **Comm Twr A** (both disc 1, the latter made with a full developer-menu inventory and a photo).

---

## 5. What remains — in the order I would do it

**Correction, 2026-09-07 evening.** This section used to say that nothing with
a USA counterpart was still Japanese on any disc. That was believed on the
strength of `vr_sweep.py` for the VR disc and a candidate inventory for the main
discs — and a candidate inventory cannot establish it. `mainsweep.py`, written
this evening to do for discs 1 and 2 what `vr_sweep` does for the VR disc, found
**one**: §5.11, **built and deployed 2026-09-08**. Everything else it flags is
inside a stage a patch family already owns, or is Japanese on the USA disc
too. Its own universe turned out to have a hole as well — it compares only
the 82 stage names both discs share, so the same string in the
Integral-only `s07br` was invisible to it. That is recorded in
`COVERAGE-RECORD.md` now.

**State of play, end of 2026-09-08.** Most of this section is now DONE and kept
only for its reasoning: 5.1a (the six conditional descriptions, all seen), 5.3
(`en_menu3`, raw-disc only), 5.4 (the raw-disc build switch), 5.4a (the MOVIE
captions), 5.5's items 2 and 6, 5.10 (the whole patch-side review), and — all on
2026-09-08 — 5.8 (the census closed, 0 unaccounted), 5.11 (`en_pad2`), 5.14
(swept; a fourth `abst` spelling found), and both of 5.9's decided cases,
`SCARF` -> `HANDKER` and the four `abst` location names.

**Nothing with a USA counterpart is known to be Japanese any more, on any of the
three discs**, and every Japanese GCL string on the main discs is accounted for
by name (5.8). What is left is of five kinds — and none of it is text to port:

| kind | items |
|---|---|
| ~~**housekeeping**~~ | **DONE 2026-09-08 22:20.** The four `_unlock_` PPFs deleted, `GiveItems`/`GiveWeapons` emptied, `DisableRAM`/`DisableCDROM` back to `false`, the disjoint VR pair finally deployed, and the branch committed. §4's "Live at" paragraph is the current state |
| **needs you at the controller**, nothing to build | 5.1, 5.2, 5.5's list 1, the `en_pad2` subtitle (5.11, needs a pad in port 2) and the four `abst` location names (5.9, free with the 5.2 run) |
| ~~**real engineering**~~ | **The briefing: FIXED 2026-09-10 late** - `ROW_H` was an R3000 **load-delay hazard**, `subu` reading `a1` in the slot right after `lbu a1`; one `nop` in place, no stub, and `hazards.py` now scans every rewritten block on every build (§24, top). Build `repro32raw`, images in `D:\mgsbuild\patched`. **Seen on screen 22:38, matching the MC set** for every reachable state; the six flag-gated items and their connectors still want a save with them earned on the raw disc. Still real work: **submit the pull request** (5.6), and look at the other two raw-only screens |
| **needs a fresh pair of eyes** | the briefing's six flag-gated indented items and their L-connectors on the **raw disc** (a save with them earned, or a raw-disc unlock aid that does not exist yet). On the collection they were seen 2026-09-11 00:13-00:17 with `ThinTexturedQuads`, twenty pairs, 0.00% against USA (§26). **English grenade fix confirmed; Japanese visual check remains:** the VR weapon-select grenade model's `DELAY` texture — Integral's own authoring error, confirmed against real gameplay (§5.15, HISTORY.md §28) |
| ~~**the one open task**~~ | **DONE 2026-09-10.** The count was never 1,813 - that figure came from a broken fragment map. 1,214 bank-1 shapes are named, the byte scanner is retired for a walk of the game's own records, and the export is complete, on all three discs: 68,242 lines, 3,923,944 kana/kanji, zero unresolved codes. §18 and §19 |
| **to investigate** | ~~5.14~~ swept and ~~5.8~~ closed on 2026-09-08 — but see §16: on 2026-09-09 both turned out to have been sweeping **one file**. `RADIO.DAT` holds 6.5 MB of Integral-exclusive Japanese developer commentary no tool here could see. That is translation, not porting, so the port's scope is unchanged; what needs redoing is any claim of completeness. Also left: what the 13 Integral-only `*r` stages **are** — new lead 2026-09-12, still not established, see below; and per-family verifiers where they are missing (5.14 step 3) |
| ~~**held open on purpose**~~ | **All four of §6's 2026-09-07 items now decided.** The `abst` location names on 2026-09-08 (use USA's); the READ MISSION LOG? caption, the `1/2` counter, the VR number substitutions and VR EXTRA record 6 all on 2026-09-11, each keeping the current default. The VR number substitutions' underlying measurement was re-checked and resolved 2026-09-12 — see §6 |
| **loose ends** | 5.7's remaining untested runtime features, 5.5's items 4 and 5, and — identified 2026-09-12, still not fully chased — what the Rev 0/Rev 1 pressing difference behind 5.13's `us1.exe` finding actually changes in play |

### 5.1 Still to be seen (needs the user; nothing to build)
Everything built so far has been seen on screen except: a mission-log page from
demo.gcx (a disc-2 save) and a count-7 page (USA's `1/2` with an empty second
screen — reproduced on purpose, §5.9); the other weapon descriptions besides the
SOCOM; the disc-swap screens, which only 5.2 can reach; the controller-port
subtitle of §5.11, which needs the Mantis room and a pad in port 2; and the four
`abst` location names now reading USA's (§5.9), which show in the MISSION LOG's
own location column and so come free with the 5.2 run. If anything looks
wrong, bisect first: move the family's two PPFs out of the mods folders and
confirm the retail text comes back. No debug shortcut exists for any of it —
the collection's launcher waits for a game to be chosen before anything loads
(a `StageSelect = abst` smoke test idled there on 2026-09-05 00:39).

### 5.1a The six conditional descriptions — ALL SEEN ON SCREEN 2026-09-07
Found 2026-09-07 by reading the two functions that decide which description is
printed (README, "Descriptions that change with the game state"). An item's text
is not always the string its table points at, and six slots change with the game
state. Four were already covered by the 34-pair comparison against USA; the two
conditional ones had never been drawn, and `GiveItems` made both reachable
whatever the difficulty. **Both were photographed the same evening and both are
right:**

- **The Mine Detector on HARD** (seen 23:08) draws USA's three lines,
  `《Mine Detector》` / `Cannot be used in` / `HARD or EXTREME mode.` That is also
  the first on-screen proof that this string's **relocation** works - it is 59
  bytes against Integral's 53, so `items.py` moves it and repoints the
  `lui`/`addiu` pair that reaches it.
- **The MP5 SD on VERY EASY** (seen 23:07, 999/999 rounds). On that difficulty
  Integral **replaces the FA-MAS with the MP5 SD** - it is a different weapon,
  not a relabelled one, and it reuses `WP_Famas` as its id, which is why the
  code reads as if it were about the FA-MAS. Three places do it: `check_type`
  (`game/item.c`) returns 0 for that id so the **FA-MAS pickup never spawns**;
  `menu/weapon.c` names the slot from an inline `"MP 5 SD"` literal; and the
  description pointer becomes the MP5's. **Its text is Japanese and correct, not
  a fault** - Integral-only, and USA has neither the weapon nor the difficulty:
  `《MP 5 SD》 サブマシンガン。□ボタンを押すと発砲。押しつづけると、フルオート連射。サプレッサー装備。`

### 5.2 The disc-2 run (needs the user at the controller; nothing to build)
Play through the actual story disc break from a late disc-1 save (no debug).
Where exactly the break falls in the story has not been checked here; do not
take a route from this file. Watch whether the game's own swap flow draws (`Now Checking...` /
`Insert DISC 2.`) or the collection swaps silently; then read the log for
`Disk ID is 1`. This validates disc 2 and the normal swap path. It does **not**
establish reachability of all four copies: title/wrong-disc, demo-theater and
abstract paths require separate evidence. Unseen text still matters for the
raw-disc release. **The developer menu cannot do this** — disc 2 is set only by
`change.c`'s CD check (README "The disc-swap text: four copies"). Once on disc
2, glance at SCREEN / KEY CONFIG (byte-identical to disc 1) and load a disc-2
save to see a mission-log page from demo.gcx and a count-7 page.

Read the log afterwards for `Disk ID is 1`, any `WATCH` line on disc 2's
spans, and any audit line. The collection patches all four disc-swap text
copies with named files that begin two bytes before `en_menu2`'s `change` and
`demosel` records (README "Where the collection's own disc patches land"); the
watches proved on 2026-09-05 that those patches register on Windows but carry
no inline data, so only the swap screens themselves show whose bytes win. If
the game's own prompt draws in English, ours won; if it draws something else,
note exactly what.

### 5.3 `en_menu3` (the `title` copy) — DONE 2026-09-07, raw-disc only
Built, verified, and deliberately **not deployed**. `menu3.py` writes
`INTEGRAL_disc{1,2}_en_menu3_raw.ppf` into `work/` for the raw-disc variant
(§5.4); `--deploy` refuses and prints why.

**Why it cannot go in the collection.** The collection patches that exact block
itself: `disc1_1822B55D_patch` lands at image `0x1822B55D`, the address of
record 0's `07` header, from `099/patch/disc1_1822B55D_patch_PS5.bin` — a named
file, so the watch reports "0 bytes" and its contents stay invisible. It is not
filtered in normal play. Two patches writing the same five strings with
different layouts desynchronise the script walk: the title stage dies on entry
with a run of `GCL:WRONG CODE` reading out of `Press the Start Button`, the same
seventeen bytes on 2026-08-28, 08-29 and again on 09-07, and the log ends
mid-run. **The user's call, 2026-09-07: ship raw-only.** The alternative — an
ini flag blacklisting their patch so ours owns the block, the `BrightnessText`
mechanism — was declined as an ASI change buying a screen the collection cannot
reach.

**The old diagnosis in this section was wrong, and is corrected in the README.**
It said the interpreter resumes at the early NUL and prescribed shrinking four
container sizes. `GCL_GetNextValue` advances a STRING by its length byte, never
by `strlen`; and the artefact that sat in `mods/_disabled/` changed payload bytes
only, re-parsing cleanly with 25 records at retail's offsets. The prescription
was written on 09-03 from the 08-28/29 logs and attached to a build it had never
been tested against. The shape that crashed on 09-07 is the **length-preserving**
one, which leaves retail's layout completely intact — so the record shape was
never the fault.

What the builder does, for whoever picks this up: it rewrites the five records
inside the `-v` option of the title actor's `CMD 9906` (chara `0xCF79`,
CHARA_OPEN, `onoda/open/open.c` — the same generic numbered-text module as
`abst.c` and the VR captions), re-stamps the SCRIPT/ARG/COMMAND sizes that
`containers_over` reports over each edit, and leaves the `-v` option's own u8
alone because it is an overflowed truncation nothing reads (`v` is the last of
the command's eighteen options and `open.c` asks for exactly those eighteen) —
the same call `abst_build.py` makes for the mission log's `-i`. Record count and
order are preserved because `open.c` reads a fixed 24 and indexes each line's
position and colour by n. No text is modified: USA's sentences go in verbatim,
and only the record slot shrinks.

**Still unproven, and cheap when someone wants it:** boot once with
`DisableCDROM = true`, which filters the collection's patches, and the deployed
build should then load a clean title. That would turn "the collision is the
cause" from a strong inference into a measurement. It costs achievements for one
session, which is why it was not run.

### 5.4 The raw-disc variant — DONE 2026-09-07
One switch now builds both, where two constants used to be edited by hand:

    py rebuild.py --output <dir>                 # collection, what mods/ gets
    py rebuild.py --output <dir> --variant raw   # for a real PSX disc image

| | collection | raw |
|---|---|---|
| `SC_KEEP_LINES` (`optsctext.py`) | 4 | 6, USA's own text |
| `OPTION_MC_CONTROL_SETTINGS` (`opt.c`) | 1, the KEY CONFIG doorbell | 0, nothing to intercept |
| `en_menu3` | excluded | included (§5.3) |

It is `INTEGRAL_ENGLISH_VARIANT`, resolved in `workdir.py` beside
`WORK`/`GAME`/`DECOMP`, so a hand-run tool honours it too. `rebuild.py` sets it
for every tool, rewrites the `opt.c` constant in its own isolated decomp export
before compiling, records both values in the report, names the ZIP for the
variant, and refuses `--compare-deployed` with `--variant raw` (what is deployed
is the collection build). `BUILD-HISTORY.md` has the table and the commands.

**One bug the first raw build found, and it is the kind only packaging finds.**
`en_menu3` rewrites the whole title block and shifts every record in it, while
`en_menu` writes `RADAR OFF` at retail's offset 130 bytes in - two patches on the
same bytes with different layouts. `rebuild.py`'s packaged-set overlap check
caught it on the first run, at `0x1822b5df`. The fix moves ownership: under
`--variant raw`, `menu2.py` skips that record and `menu3.py` ports it (index 4,
the one entry in its table with no change/demosel twin). Worth remembering as
the general shape - **a builder that shifts records has to own every patch that
writes into the region it moves**, and the only thing that notices is a check
over the assembled set.

**BOOTED 2026-09-10.** The user ran the patched disc 1 and it worked. That is
the first time anything in this project has been proved to run outside the
Master Collection, and it closes the item this paragraph carried from
2026-09-07. What it establishes is that the image is *valid* - it loads, the
executable runs, the game plays - not that every ported screen is right on it;
the raw-only three (`en_menu3`, the six-line brightness paragraph, Integral's
own KEY CONFIG) still want eyes on them, and they are now reachable for the
first time.

The image tested was built before the language default existed, so it started
in Japanese - correct behaviour for a retail disc, and the reason §23 was
written. Rebuilt with `--english-default yes` the same day.

**Three things used to stand in front of that boot, and all three are now
closed** (§22). Two were closed in code by §5.10 the same evening this
paragraph was written, and the text above them went stale: `rebuild.py
--variant raw` gives every PPF a **block check** (`place()`, from
`blockcheck_of`) and emits a per-disc **`zz_ecc` PPF** carrying the recomputed
EDC/ECC of every touched sector (`raw_tails()`, `rawdisc.py`). The third — show
the collection's embedded images equal a retail dump — was measured on
2026-09-10 against the Redump set and holds byte for byte outside the
executables. **Do not re-plan work that the code already does; read
`rebuild.py` before believing a paragraph in this file.**

### 5.4a The VR movie captions — DONE and VERIFIED ON SCREEN 2026-09-07
All three MOVIE selection captions are ported and deployed as
`INTEGRAL_vr_en_movie.ppf`, and the user's three shots at 01:03 confirm every
one of them:

| clip | on screen |
|---|---|
| TGS ROLL A | `Exhibition clip “A” for` / `the Tokyo Game Show, Spring '98.` on two rows |
| TGS ROLL B | the same with `B` |
| E3 | `Video clip from E3 (6/97)`, one line |

Right text, right clip, two rows where USA has two, and the typographic quotes
render as quotes — so the local-font remap is right as well.

**What the fix is.** The captions are drawn by the engine's generic
numbered-text module — the same one `abst.c` implements in the decomp — which
draws *every* slot that is lit, so the line count is only ever "how many slots
get lit". That is the single place the two discs differ, in the clip-selection
code: USA calls the actor's own `highlight(work, i)` twice, for `clip*2` and
`clip*2+1`, where Integral calls it once with `clip`. USA's sequence is 14 words
and Integral's block has 11, so the port leaves the block alone, retargets its
one `jal` (one word) at a 16-word stub appended to the overlay, and the stub
calls retail's `highlight` twice. Plus USA's six records and USA's position
table, which places the pairs on rows 196/208.

The stub sits at overlay `+1DFB8` (RAM `0x800DF158`) in padding the stage
already carries — the payload is 122,808 bytes inside 60 sectors of 122,880 — so
**no payload moves, the stage keeps its 123 sectors and its LBA, and nothing is
relocated**. The RAM is overlay space by construction: USA's own `movie` overlay
runs 9,576 bytes past Integral's end at the same load base, and Integral's
`init` reaches `0x800EA1EC`. Six lines is what the module is dimensioned for
(its slot array ends exactly where `Act`'s tpage prims sit, at 24 slots; the
builder's cap is 24). The longest line measures **201 px against the 240 px wrap
limit** with Integral VR's own `font.res`, so nothing wraps — that check is a
build-time assertion, because a wrap here lands on the CLUT row and writes past
the buffer.

#### The EXIT box moved up — approved 2026-09-07, and an exception worth naming
Measured off the three shots (9 display px per game px, x offset 480):

| element | game y |
|---|---|
| EXIT box, top border | 190–191 |
| EXIT text | 193–199 |
| EXIT box, **bottom border** | **201–202** |
| caption line 1 | **201–213** — overlapped the border by 2 rows |
| caption line 2 | 215–225 — clear |
| E3's single line | 203–213 — cleared the border by 1 px |

A two-line caption at USA's rows collided with Integral's EXIT box by two pixel
rows. USA avoids it twice over, and the user confirmed both halves: its caption
face is **shorter**, and its **EXIT box sits higher**.

The font stays Integral's — a face is its own art, not text (README "The white
caption font differs between the two VR discs"), and line 1's ink is 12 rows
because it is topped by the two 12x12 script-local quote glyphs where the same
font draws the option screen's `Sound setting.` in 8. So the room comes from the
box, which is **chrome that positions text** under rule 3.

**The decision, in the user's words: "Make sure to document this decision.
Usually we skew toward the Integral visuals."** This is therefore a named
exception to that default, approved on 2026-09-07 after being asked: the box
moves because the English cannot otherwise sit where USA puts it. Nothing else
about the box changes — Integral's art, colour and size all stay.

**Where the position lives, and how much it moved.** Every widget in the stage is
built by `Init_Res(slot, 0, GV_StrCode(name), y)`, centring the texture from its
own header and offsetting it by y from screen centre 120. Listing all 28 calls in
both overlays gives y values **identical between the discs except one**:

    sp_exit    Integral y +70 (screen 190)      USA y +66 (screen 186)

and the measured retail top border is exactly 190, which confirms the model. One
immediate carries it — scanning every `addiu`/`ori`/`slti` in either overlay
finds precisely one instruction holding 70 (Integral) or 66 (USA) — so the port
writes `addiu a3, zero, 66` at overlay `+F128`, USA's own value.

**The highlight moves with it**, which the user asked to be sure of. The widget
is a single object at `work+0xF0`, and both things that light it anchor on that
object rather than on a coordinate of their own: `+10198` attaches the `cur_l`
cursor as `f(work+0xF0, strcode, 1, 1)`, and `+F38C` sets its lit state as
`f(work+0xF0, 0xFF, 1)`. Both call sites — and all ten references to
`work+0xF0` — are byte-identical between the discs. So USA's entire widget, box,
label and selection highlight, sits at 186 because of that one immediate;
nothing else could position the highlight, or USA's own would be wrong.

Deployed 2026-09-07. Expected on the next look: the box 4 px higher, its
highlight with it, and 3 rows of clearance under it instead of a 2-row overlap.

**CONFIRMED ON SCREEN 2026-09-12.** The user verified the moved box works as
intended. The EXIT highlight when selected was not separately called out and
remains an edge worth a glance if anyone is looking anyway, but the box move
itself — the thing that was asked and approved — is done.

**If the whole thing ever has to come out**, the fallback is a file move, not a
rebuild: put `work\INTEGRAL_vr_en_movie_e3.ppf.was-deployed` back in
`mods\INTEGRAL\VR-DISK\` as `INTEGRAL_vr_en_movie_e3.ppf` and delete
`INTEGRAL_vr_en_movie.ppf` (never both — they overlap and `_e3` sorts last).
That returns to the 2026-09-06 state: the E3 caption in English at Integral's own
row, both TGS captions Japanese, and no code change in the overlay at all.

**Two earlier attempts failed, and both are recorded in the README** ("Three
corrections"): the record count alone (six records with one highlight gives one
line per clip and misattributes captions) and USA's position table alone (the
rows moved exactly as predicted and still one line each). The address that made
both look like dead ends was `Act`'s `lw` from `0x800A9580`, read as
`captions[clip]`; the decomp's `abst_sprt` indexes the same table with
**`GV_Clock`** — it is the per-frame ordering table, and the draw is handed an
OT, not a string. Lesson worth carrying: **when a disassembled function indexes
a global, find the same pattern in the decomp before naming the global.**

### 5.5 The VR disc: what has been seen, and the edges left
Ported 2026-09-06, deployed, and **first run on screen 2026-09-06/07**. Seen and
correct: the option screen's seven help lines, centred, after the three faults
§4 records; the mission menu with every mission unlocked; a clip's description
window in English when a TGS video opens; and EXTRA -> MOVIE with all three
thumbnails and, since 2026-09-07, all three captions in English — the two TGS
ones on two rows (§5.4a; the overlap those shots showed between line 1 and the
EXIT box was fixed the same day by moving the box to USA's y, deployed and not
yet seen). What is left, in rough order:

1. **On screen 2026-09-07: mission windows.** Two shots, both correct English -
   `ADVANCED MODE / SOCOM LEVEL 01`, "Eliminate all enemy soldiers and head for
   the goal! / Enemies 2", and `1 MIN. BATTLE VS. TARGET / SOCOM`, "Use Socom to
   destroy targets! / Conditions to clear: 15 targets". That is the title and
   the briefing of the `vr_en_missions` family, the largest one, read in play.

   **Also on screen 2026-09-07**: an item description (`《Diazepam》 Anti-anxiety
   drug. Temporarily stops involuntary trembling.`) and a weapon description
   (`《PSG1》 Sniper rifle. Aim with directional buttons, press □ to fire.`, the
   button glyph rendering correctly) - and the same PSG1 window shot on USA's
   own disc reads **identically**, which is the port matching its donor word for
   word rather than merely looking plausible.

   **Still unseen**: a mission RESULT window, three of the EXTRA menu's four
   help lines (EXIT's was seen; see the aid below), a save and a load message,
   and the PHOTOGRAPHING mode's card messages (the ALBUM path). If something is
   wrong, bisect the same way as the main game: move that one PPF out of
   `mods\INTEGRAL\VR-DISK\` and confirm the Japanese comes back.

   **On reaching the item and weapon descriptions:** there is no "give all" for
   VR and there does not need to be. Every mission fixes its own loadout, so the
   pools are covered by playing the mission that carries each one - the mission
   aid makes them all selectable and that is as far as unlocking can take you.
   Four entries are Japanese **on purpose** and are not faults: Integral's MP5 SD
   (no USA counterpart), the frozen Ration/Ketchup pair (no USA counterpart, and
   unreachable in VR), and the mine-detector HARD/EXTREME line (VR has no
   difficulty level).
2. **KEY CONFIG — DONE 2026-09-07.** Seen with `DisableRAM = true` and
   `DisableCDROM = true`: Integral's own screen, all eight labels English in
   all three button types, `key_syukan`'s +11 shift clearing the connector
   curve. The user spotted the one fault — the row's selection highlight was
   24 px short, because it is drawn by hardcoded `glow()` arguments rather than
   an `Init_Res` quad and the transplant only knew about quads. Fixed, redeployed
   and re-measured within a px of USA (README, "The VR KEY CONFIG on screen").
   The help line under the controller stays Japanese by rule: USA leaves records
   17..25 empty. **The flags are only for looking at it** — the user's rule
   stands that in the collection they prefer the interception, and the
   transplant is for the raw disc.
3. **Four VR unlock aids may stay installed; enable them only for testing.**
   `vr_unlock.py` (Integral's missions), `vr_unlock_movies.py` (the EXTRA
   clips), `vr_unlock_extras.py` (the EXTRA MENU's items) and - for the donor
   disc - `VRUS_unlock_missions.ppf`, which `vr_unlock.py` also builds.

   **There are three separate gates on this disc and each needed its own aid**,
   which is the lesson: the mission menu is gated in `selectvr`, the clips in
   `movie`, and the menu items in `vrtitle`. The last was found 2026-09-07 when
   the user reported only MOVIE, ALBUM and EXIT on the menu, leaving three of
   `vr_en_title`'s four ported help lines unreachable. The EXTRA menu builds a
   visibility bitmask at `work+0x1e` from progress flags at `+0x1a1c`, one item
   per test, each test also bumping the item count - so the aid forces the three
   **tests** (`andi v0, v0, 3` / `0x10` / `0x40` -> `addiu v0, zero, 1`) rather
   than the mask, because a mask forced from outside would leave the count and
   the layout disagreeing. With it, PocketStation appears too, and its help line
   is Japanese **on purpose** (Integral's fifth item is PocketStation where USA's
   is STAFF CREDIT, §6). None writes progress, so saving with them in place is safe; the
   standing rule is achievements **off** first, unlock, test, disable the unlock
   INI controls and restart, achievements back on. With older ASIs that lack
   these controls, remove the PPFs instead.

   **The USA one sat unbuilt-into-place for a day**: it was written 2026-09-06
   and never copied to `mods\VR-DISK_US\`, so USA's missions were still locked
   when the user went to compare against them on 2026-09-07. Deployed then.
   Ketchup's base path for that title really is `mods\VR-DISK_US` with no
   version or disk subdirectory - confirmed in the log, `[Ketchup] base path is
   mods\VR-DISK_US`. There is no USA equivalent of `vr_unlock_movies` yet; USA's
   own EXTRA clips gate the same way and would need their own offsets.
4. **The number substitutions** are decided, and the two previously-unconfirmed
   measurements are now RESOLVED 2026-09-12 (see section 6): both were reading
   inert pooled template windows, not the live mission's own copy.
5. **Deferred edges**, each a small piece of work: the camera's EXORCISE
   textures, and whether anything in the mission windows overflows a line at
   240 px the way the main game's could. (The two TGS MOVIE captions were the
   third item here and are done — §5.4a.)
6. **`rebuild.py` builds the VR patches — DONE 2026-09-07.** All seven come out
   of the same isolated run as the main discs and are packaged under
   `mods/INTEGRAL/VR-DISK/`. Three things were needed: extracting the two VR
   stage dirs, **building** Integral's VR executable from the decomp rather than
   copying it (the generator runs a second time for the `vr_exe` variant, then
   ninja makes `obj_vr/_mgsi.exe`, checked against SHA-256 `c370f8e4…`), and
   giving `vr_movie` an `INTEGRAL_ENGLISH_VR_PPF_DIR` so it composes on the run's
   own output instead of the deployed folder. The VR set's one deliberate
   overlap (`vr_en_movie` over `vr_en_missions`) is allowed by name and any other
   is an error. The two unlock aids are deliberately not built: they are test
   aids and must never ship. Verified on `repro8`: **25 PPFs, 18 main + 7 VR, all
   25 equal to the deployed set's effective bytes.**

### 5.6 Sync with upstream, and the pull request — one job, done once
**REBASED 2026-09-10: the branch now sits on `upstream/master` (`97172f5`).**
The rest of this section is the 2026-09-07 estimate, kept because most of it
still describes the pull request, and because **its cost estimate was wrong in
a way worth recording** - see "What it actually cost" at the end. A
fast-forward was never possible: that needs a branch with no commits of its
own, and this one had 133 then and 161 now.

| | |
|---|---|
| merge base | `8fb944d` (v3.6 + 5 commits) |
| upstream commits we lack | **10**, through `48fe165` "Complete Wamsoft port" (2026-09-07) |
| our commits they lack | **133** |
| our `src/` footprint | 854 lines across **9 files** |

**Every file the port touches has moved upstream**, so any route has to follow
renames:

| ours | upstream | similarity | our lines |
|---|---|---|---|
| `src/mgs1.cpp` | `src/games/mgs1.cpp` | **54%** | +242 |
| `src/mgs1.h` | `src/games/mgs1.h` | 98% | +234 |
| `src/ketchup.{cpp,h}` | `src/m2fix/…` | 100% | +152 / +39 |
| `src/m2config.{cpp,h}` | `src/m2fix/…` | 100% | +62 / +20 |
| `src/m2game.h` | `src/m2fix/m2game.h` | 98% | +11 |
| `src/sqhook.{cpp,h}` | `src/modules/…` | 96 / 97% | +79 / +15 |

Eight of the nine are near-pure moves that git's rename detection will carry;
the work concentrates in `mgs1.cpp`, the one file upstream rewrote and the one
holding most of our lines.

**Why it is deferred rather than done: those ten commits give MGS1 Integral
nothing.** They are Vol. 2 support, MGS1in4 fixes, the Wamsoft port, PATRIOTS
text and the restructure. Upstream's own diff to `mgs1.cpp` is **0 insertions,
130 deletions** - code moved out, no MGS1 behaviour changed - and nothing in
them touches `MGS1_Ketchup`, Integral, the brightness text, the Ketchup
deferral or the patch watches. Against that, merging costs a conflict
resolution in our most-changed file plus a rebuild and a re-test of everything
runtime the ASI carries (`EnglishText`, `BrightnessText`,
`PreserveConfiguration`, the deferral, six patch watches).

**So it is one job, and the right moment is the pull request**, because the PR
needs our changes in upstream's new layout anyway - doing it now would mean
doing it twice. When it happens:

- try `git merge upstream/master` on a throwaway branch first and read the true
  conflict set before touching `integral-english-text`;
- `UPSTREAM.md` lists what goes up; its two omissions are right (789f4a2 is
  superseded by the BrightnessText tri-state, fbb170c is the port-only abst
  watch comment);
- separate two things when submitting: `SetPatchWatch` goes upstream as a
  mechanism *without* the Integral `option`/`abst` ranges `mgs1.h` registers,
  and `BrightnessText` covers title 981 (USA) only;
- re-test the runtime list above afterwards - the port's behaviour in the
  collection depends on all of it.

**What it actually cost, measured 2026-09-10.** The estimate above said a
merge would cost "a conflict resolution in our most-changed file". It did not.
Following this section's own advice - try it on a throwaway branch first -
both routes were run, and both produced **one** conflict, in
`build_zydis.cmd`, where upstream moved the submodule to `src/extern/zydis`
and our commit had quoted the old path. Everything else applied by itself:

| | |
|---|---|
| upstream commits taken | 11, through `97172f5` |
| our commits replayed | 161 |
| conflicts | **1**, at commit 1 of 161 |
| `src/mgs1.cpp`, the feared file | auto-merged |

Git's rename detection carried all nine files into upstream's new layout
(`src/games/`, `src/m2fix/`, `src/modules/`) unaided. The rebased tree and the
merged tree came out with **the same tree hash**, `356d3798` - that is the
check worth copying: if a rebase and a merge of the same work disagree, one of
them lost something.

Verified after: all fourteen feature markers appear the same number of times
as before (`bGameEnglishText`, `SetPatchWatch`, `Ketchup_DiskPatch`,
`BrightnessText`, ...), and `selftest.py` still passes 35/35.
`backup-before-rebase` points at the pre-rebase tip, `b6513f8`.

**One piece of fallout, and it is not git's fault.** Upstream moved six
*submodules* into `src/extern/`, and a submodule's working tree does not move
with a checkout. `git submodule sync --recursive && git submodule update
--init --recursive` populates the new paths; the old
`src/{imgui,inipp,json,safetyhook,spdlog,zydis}` checkouts are left behind as
untracked duplicates - 53 MB, including a built `Zydis.lib` that
`build_zydis.cmd` regenerates at the new path on the next build. **Deleted
2026-09-11 00:25** after they surfaced as six "new" items in GitHub Desktop;
the live submodules under `src/extern/` were verified present first.

**The lesson for the estimate.** Nothing about the 2026-09-07 reasoning was
careless: it read the rename percentages and the line counts and concluded the
work was concentrated in `mgs1.cpp`. What it never did was spend two minutes
running the merge on a throwaway branch - which this very section recommended.
A deferral justified by an unmeasured cost is a guess, and this one was wrong
by about a day of imagined work.

### 5.7 Still untested, low effort when the moment comes
- **The patch watch is blind while `DisableCDROM = true`**: the early return
  in `sqhook.cpp` precedes the watch loop. Answered 2026-09-05 12:57 with the
  flags live: the `_PS5`-suffixed `abst` patch does register on Windows (and is
  orphaned by the `en_abst` relocation); named-file patches carry no inline
  data, so their content stays unknown to the watch.
- `PreserveConfiguration` catching a real stale write (intermittent race).
- ~~`GiveItems` in a stage where the inventory is actually empty~~ **Exercised
  for the first time 2026-09-07, and it did not work. Fixed the same evening.**

  Run on MGS1 **USA** (title 981, disc 1, stage `s01a`) with all 24 ids, it
  logged twenty-four lines and granted nothing: `count 65535 -> 65535`, over and
  over, while the inventory stayed empty. **An item Snake does not have is
  stored as -1, not 0.** `game/g_define.h` says so in one line - `IT_None = -1`
  - and `game/item.c` tests `GM_Items[IT_Ketchup] == -1`. The feature looked for
  a count of zero, which is a state an absent item is never in, so the guard
  that was meant to protect a real inventory silently protected everything.

  Two things it got wrong beyond the sentinel, both now corrected in
  `src/mgs1.cpp`. It also wrote `GM_ItemsMax[id]` wherever that read 0, which
  helps nothing: for the three consumables the max the game actually consults is
  `GM_Items[id + 11]` (`item.c` `add_item`), and for everything else `add_item`
  simply assigns. That write is gone. And an owned-but-**disabled** item is a
  third state - `disable_equipment()` ORs `IT_TYPE_DISABLED` (0x8000) into the
  entry - so `0x8001` is now deliberately left alone rather than being read as
  "absent" and quietly re-enabled.

  The log line says which happened: `granted (65535 -> 1)` or `already held,
  left alone`. **Confirmed working on screen 2026-09-07**: all 24 items in the
  inventory on MGS1 USA disc 1.

- **`[Game] GiveWeapons`, added 2026-09-07** once the items worked and the
  weapons visibly had not. Weapons are a different shape: two arrays, ammo in
  `GM_Weapons` (`linkvarbuf[17..26]`) and capacity in `GM_WeaponsMax`
  (`[27..36]`), ten of each, in MGS1's own `WP_` order - which is **not** the
  menu's: 0 SOCOM, 1 FA-MAS, 2 Grenade, 3 Nikita, 4 Stinger, 5 Claymore, 6 C4,
  7 Stun Grenade, 8 Chaff Grenade, 9 PSG1.

  Ownership is the same convention the items turned out to use, and the menu
  states it outright - `GM_Weapons[i] >= 0` (`menu/weapon.c`), so -1 is "not
  carried". **What goes in the magazine is taken from the game, not invented.**
  Its own `add_weapon` lifts a negative entry to 0 and fills toward
  `GM_WeaponsMax`, so a granted weapon gets the full magazine the game already
  records for it, and an empty one where no capacity is recorded. Nothing writes
  `GM_WeaponsMax`: there is no capacity table anywhere in the decompiled source
  to write a truthful one from, and making up ammo counts is the kind of
  invention this project does not do. If a weapon arrives empty, that is the
  game having no number for it, and the log says so.

  Both keys work on **Integral** as well as USA - the code is in the shared
  `MGS1` handler, keyed on the `scene_name` memory define every version has,
  with no title check. Set both back to empty for normal play.

  **Confirmed on screen.** `GiveWeapons` and `GiveItems` together are how the
  weapon and item description screenshots were captured on both MGS1 USA and
  Integral for this port's comparison work - not a separate, unexercised
  feature.

  **The lesson is the general one this project keeps relearning:** the feature
  was written, built, reviewed and documented as working for five weeks without
  ever being pointed at an empty inventory. "Built" is not "exercised", and a
  guard that never fires looks exactly like a guard that never needs to.
- `Ketchup::Audit` did report two `differs from what was written` lines on
  2026-09-05 — both were the game's own code editing ported strings, fixed in
  `items.py`. Both pools are now owned byte for byte; a future audit line means
  the collection wrote *after* Ketchup's pass, which has not been seen.

### 5.8 Finish the text census — CLOSED FOR GCL STRINGS 2026-09-08
`mainsweep.py --census` accounts for **every** Japanese GCL string rather than
classifying flagged candidates, and the buckets are asserted to sum to the
total so a residue cannot hide in the framing. Both discs, identical: of
**1,360** Japanese strings, 1,260 are inside a stage a patch family owns, 17 are
Japanese on the USA disc too, 83 have no owner on the USA disc at all
(Integral-only content), and **0 are unaccounted**. It exits non-zero if that
last bucket is ever not 0, so it is a regression guard and not just a report.
The Integral-only stages are folded in through `base_stage`, so this covers
every stage on the disc and not only the shared names.

That supersedes the paragraph below, which is kept because it says what the
older tool measures and why its residue was never a defect list. Texture
lettering, executable UI beyond the save-title probes, and runtime language
branches remain outside both tools.

**The older framing:**
`audit_text.py` inventories GCL string candidates and address references
across all three Integral images and USA's, but it is a framing heuristic.
Of disc 1's 1,414 flagged Integral candidates, 1,025 were the mission log (now
ported), 111 are preope's retained unread recap bytes, 51 `rank`'s Integral-only
sentences, 45 the 1P MODE pages, 20 the option screen's Japanese help lines;
about 160 remain across gameplay stages and need verification against their
callers, the font bank (`0x80xx` is Latin, `0x9001` a space) and a reachable
screen — USA itself shows 307 flagged, so a flag is not a Japanese string.
Texture lettering and runtime language branches are outside both tools.

### 5.9 Optional, ask first
**Held open on purpose, 2026-09-07.** Both remaining items here were put to the
user the same evening the `SCARF` case below was decided, and both were kept
open rather than swept along with it. They are decisions still owed, not
oversights; §6 carries the same marker.

- **The caption under READ MISSION LOG? — DECIDED 2026-09-11: keep Japanese
  (the current default).** (作戦記録を参照しますか？): kept because USA draws
  nothing there — blanking it would remove Japanese text with no USA
  counterpart, against the standing rule; `KEEP_PROMPT_CAPTION = False` in
  `abst_build.py` gives USA's empty record and is not used. **Also decided the
  same day:** the `1/2` counter and empty second screen on count-7 pages stay
  USA's own behaviour, reproduced — confirmed as the general rule for this
  shape of case.
- **The `abst` location names: USA's, ASKED AND DONE 2026-09-08.** The second
  application of amendment 4b, and the first one that needed a container to
  grow. `mainsweep.py --diff-english` enumerated the table: 30 English names in
  each game, aligned 1:1, and **four** differ - the three the documents listed
  plus `Cmnd rm`, which was in none of them.

  | Integral | USA |
  |---|---|
  | `Tank Hanger` | `Tank Hangar` |
  | `Medi rm` | `Medi room` |
  | `Cmnder rm` | `Cmnder room` |
  | `Cmnd rm` | `Cmnd room` |

  *What it is.* One `0x9906` command in `scenerio.gcx`'s script body -
  `mainsweep.py` calls it `chara 53C7` after the actor its first STRID spawns -
  carrying 31 records directly in its value list with **no option list at all**,
  which is why `page_of` returns None for it and `rebuild_body` used to skip it.
  The names are **two-byte font codes**, not ASCII (`0x80xx` Latin, `0x9001` a
  space), which is why no ASCII search of the PPFs had ever turned them up. Both
  games use the same encoding, so USA's bytes drop straight in.

  *How it was done.* `USA_LOCATION_NAMES` in `abst_build.py`, beside
  `KEEP_PROMPT_CAPTION`. USA's **whole command** is taken rather than its four
  records, and that is the point: the block carries **two** derived length fields
  - the COMMAND's BE16 size and a **u8 at `start+5`** that `option_starts` uses
  to reach the option list - and both are 12 bytes larger on USA's disc. Patching
  the records and forgetting the u8 would leave a block whose size says one thing
  and whose offset byte says another. Taking the block whole keeps them in step
  by construction. `abst_build.py` already recomputes every enclosing container
  (command size, proc body ARG length, proc table offsets, proclen, script
  length) and relocates the stage, so nothing else was needed.

  *Cost.* One name is length-neutral (`e` -> `a`), three grow by 4 bytes each -
  two extra glyphs at 2 bytes a glyph - so **+12 bytes**, measured both ways:
  the rebuilt chunk is 104,600 bytes with the constant off and 104,612 with it
  on. The stage is still **88 sectors** and still lands in DUMMY3M slots
  462..549, so no budget moved.

  *Verified.* The builder's own verifier now re-parses the location list out of
  the rebuilt script and asserts it equals its source record for record, and that
  the constant is honoured exactly once - `verified: the 31-record location list
  equals USA's exactly; 4 record(s) differ from retail Integral`. Both directions
  were run: with the constant off it reports Integral's own list and 0 differing.
  Deployed to both discs 2026-09-08; `ppfcheck.py --deployed` clean over 27
  files. **Not yet seen on screen.**

  *The Japanese location list is untouched.* `demo.gcx` carries Integral's own,
  31 records USA has no counterpart for, and it is not offered to the
  substitution at all.
- **The item short-name table: `SCARF` -> `HANDKER`, ASKED AND DONE 2026-09-07.**
  **This is the port's first replacement of Integral's own English** rather than
  of its Japanese, and therefore the first case under amendment 4b in §2.

  *What it is.* The abbreviations in the inventory's side column are a different
  string set from the descriptions: NUL-terminated names on an 8-byte stride in
  the executable, items 23 down to 0, **already Latin on both discs**, which is
  why the port had never touched them. Dumping both tables and comparing entry
  by entry, exactly one differs - item 22, `SCARF` on Integral against
  `HANDKER` on USA. Every other item and weapon abbreviation was already
  identical.

  *Why it was worth changing.* Integral's own Japanese description for that item
  reads 《ハンカチ》 - *hankachi*, a handkerchief - and its own description says
  so. Its short label was the only thing calling it a scarf, so the change makes
  Integral agree with itself as well as with USA. Read with `rendertext.py`.

  *What went in, and what could not.* `HANDKER`, which is USA's own string - not
  "Handkerchief". The slot is 8 bytes, so seven characters is the ceiling, and
  any other abbreviation would be invented text rather than ported text. USA's
  is exactly seven, so it lands in the existing slot and **nothing moves**:
  the patch grew by exactly 8 bytes per disc and changed nothing else, which the
  deployed-set comparison confirms byte for byte.

  *Where it lives.* `items.py`, `IN_SHORTNAME = 0x09BCC0` against USA's
  `0x09E448`, added to the regions `en_items` owns outright so Ketchup writes
  and audits all 8 bytes. Both discs, deployed 2026-09-07 22:56. **Not yet seen
  on screen.**

  **Seen on screen 2026-09-07 23:03**: the side column reads `HANDKER`, beside
  the description it already agreed with.

  Two facts about that block worth keeping. **Cold Medicine and Diazepam are not
  in it** - both names are 8 characters, one too many for a slot that must also
  hold a terminator, so both games keep those two elsewhere. And the `MP 5 SD`
  string sitting near the table is **not a table entry** - a first pass with a
  fixed 8-byte stride suggested Integral's table was one longer than USA's, and
  that was wrong: it is a string literal in `menu/weapon.c` that names the FA-MAS
  slot on VERY EASY, emitted into rodata beside the table. Walk this region as
  NUL-terminated strings and compare by content, never by index.
- `rank`: 36 Integral-only Japanese sentences with no USA counterpart → stays
  Japanese unless a USA source turns up. Nothing to do without one.

### 5.10 The patch-side pass — DONE 2026-09-07 (evening)
Seven robustness and completeness gaps were identified in the morning review and
all seven were closed the same evening. None of them changes what a player sees;
what they change is what can go wrong unnoticed.

**1. The two VR patches that wrote the same stage now have one owner each.**
`vr_en_movie` used to share 686 of its 753 bytes with `vr_en_missions` and work
only because Ketchup applies a folder in file-name order — a dependency nothing
enforced and nothing would report. `vr_windows.py` now ports the `movie` stage
as before but writes none of its records, handing the finished stage to
`vr_movie.py` as `work/vr_movie_base.bin` (`vr_windows.HANDOVER`); `vr_movie`
emits against **retail** and owns the stage outright. The two share **0 bytes**,
the deployed-folder dependency (`INTEGRAL_ENGLISH_VR_PPF_DIR`) is gone, and
`rebuild.py` now refuses *any* VR overlap instead of allowing that one pair.

**Verified by effect, which is what made it safe to do at all:** applying the new
pair gives the disc byte for byte what the old pair gave. Of 136 positions where
the two sets differ as *files*, every one is a byte equal to retail either way —
the old set wrote them redundantly inside merged runs. **Positions where the disc
would actually differ: 0.** Both files must be replaced together when this is
deployed; neither works with the other's old copy.

**2. Width checking, and what it turned out the invariant is.** `widths.py`
derives a `vrwindow`'s real budget from the decomp rather than guessing it —
`align4(w) - 16`, to VRAM words, to whole 12-px cells, less the 12 px
`font_draw_string` reserves — so a 256-wide window gives 228 px. Then the
measurement said not to assert it: **retail Integral has 107 lines over that
budget and USA has 19**, so it is not an invariant, it is an estimate. What IS
enforced is exact and cheap: `substitute_numbers` may never make a line render
wider than the USA line it came from (the only width known to be safe, because
USA shipped it), plus the 255-px ceiling that `kcb->max_width` imposes on
anything, over every record the port writes. Both hold today and every VR patch
rebuilds byte-identical, so these are regression guards, not fixes.

`vr_exe.py` deliberately has **no** width check, and the reason is recorded in
it: its pools hold multi-line descriptions, the English ones break on `0x807C`,
and retail Integral's own Japanese entries in the same arena measure 540 px
unsplit — so they break on something not yet established. Asserting there would
fail on bytes the game ships and works with.

**3. The raw disc now carries correct error correction.** A PPF that changes a
payload byte invalidates that sector's EDC and P/Q parity; the collection's
emulator does not care and a real console may. `cdecc.py` implements both sums
and `rawdisc.py` recomputes the tail of every sector a raw patch set touches —
413 sectors per main disc, 2003 on the VR disc. Each PPF also gains a PPF3 block
check so a tool can refuse the wrong image. `py rawdisc.py <package>` applies a
finished set in memory and confirms every touched sector verifies; it does.

**The pass rests on one invariant, checked per sector: before anything is
applied, the sector as we believe retail has it must verify against its own
stored parity.** That is a 280-byte sum over 2048 bytes, so it cannot pass by
accident, and it is what makes the executables safe — their ISO extents are
zero-filled in the collection's images, so the retail file is put back first.

**4. The collection's named-file patches are no longer a mystery.** See §5.12.

**5, 6, 7. Ketchup, the sweep and the tests.** `Ketchup::ApplyBlock` now warns
when part of a record lands in a sector's 304-byte tail and is not mirrored,
instead of logging success — the silent loss that once cost `en_savemsg` 142 of
442 bytes. `ReportOverlaps` warns when two PPFs in a folder write the same byte
with **different** values, naming both files. `ApplyPPF3` validates the whole
record chain before applying any of it, so a malformed file is refused rather
than walked to a count that never reaches zero — the 306 MB log. The block check
is still skipped and now says so: the fix has no way to read the disc image
back, so it cannot be verified here. `mainsweep.py` is §5.11. `selftest.py` runs
23 tests over the pure pieces in a hundredth of a second, and each one was
checked by mutation to confirm it fails when the thing it guards is broken.

### 5.11 The one thing the main-disc sweep found — BUILT AND DEPLOYED 2026-09-08
`mainsweep.py` pairs every GCL string on both discs by the command that owns it,
so "Integral has Japanese here and USA has English" becomes a comparison rather
than a guess. Run over discs 1 and 2 it reports twelve owners; nine are inside
stages a patch family already owns, two are Japanese in identical numbers on the
USA disc as well (`cmd 4AD9`, the location titles; `chara 9302`, `rank`). One
was neither, and it is now the `en_pad2` family — `pad2.py`, two PPFs, 159 bytes
each, deployed 2026-09-08 22:02.

**What the text is.** Stage `s07b` is the Psycho Mantis room (`stage/s07b.c`
registers `CHARA_PSYCHOMANTIS`). The owner is `CHARA_2D0A_2ND` → `NewSecond` in
`game/second.c`, 45 lines long, and it settles by itself what draws the string:
the actor takes **one** string per spawn, writes it as a subtitle with
`MENU_JimakuWrite(work->message, 20000)` when `GV_PadData[1].status` first goes
true, and clears it when pad 1 comes back. So it is the line shown when the
controller moves to **port 2** for the Mantis fight, telling the player to put
it back. Integral's reads コントローラ端子1のコントローラを|使用してください。
(`rendertext.py`; three of its codes fall outside the located font bank, so read
the PNG, not the codes). USA's is `PLUG CONTROLLER INTO | CONTROLLER PORT 1.`

**This entry used to describe it wrongly, in two ways.** It said "two records"
of one owner, record 0 English on USA and record 1 Japanese, and that USA
"translated the first of the pair". Both are wrong, and reading `second.c` is
what shows it: one string per spawn means there is no record 0/record 1 to
index. What the script has is **two separate spawns, in two branches**:

| | Integral | USA |
|---|---|---|
| site A | string at script `body+0x88C`, path `elif 1903 → if 2043` | same offset, **byte-identical Japanese** |
| site B | string at script `body+0xC3A`, path `elif 2845 → if 2985` | `body+0xC0E`, **English** |

So USA translated the **later** site by offset, not the first, and left the
other — its own inconsistency, because both branches hand the same message to
the same actor.

**And there is a third site the sweep cannot see.** `s07br` carries the same
string at `body+0xC3A`. That stage is one of the 13 **Integral-only** names, and
`mainsweep.py` walks only the 82 names the two discs share, which puts every
Integral-only stage outside its universe. This is a second blind spot beside the
one §5.14 records, and it is now written down in `COVERAGE-RECORD.md` as a limit of the
tool rather than left to be rediscovered. `s07br`'s overlay source is
byte-identical to `s07b`'s (`diff` is empty), so it is the same code over
different stage data; **what that stage is for has not been established.**

Five sites in total: three per disc, two discs, and both discs hold both stages
at the same LBA with identical bytes, so the two PPFs are the same writes at the
same offsets.

**The decision, and it was the user's.** Porting only USA's one site is what
"placed where USA places it" says literally, and it would leave a player who
trips the other branch — or who is in `s07br` at all — reading Japanese. Asked
and answered 2026-09-08: **port every site.** The text is USA's own verbatim
either way; the rule exists to forbid invention, not to reproduce an oversight.

**Why nothing had to move.** A GCL STRING is length-prefixed and
`GCL_GetNextValue` advances by that length byte, never by `strlen` (the
correction §5.3 records). USA's 42 bytes fit inside Integral's 55-byte slot, so
the length byte stays at 55 and the walk steps over the same bytes it always
did. The English and its terminator go in at the front, the dead tail is spaces,
and the record's own final NUL is left alone — `menu2.py`'s convention.
`MENU_JimakuWrite` only stores the pointer and `font_print_string` reads to the
NUL (`menu/jimaku.c`), so the tail is never looked at. No container to
re-stamp, no sector to grow, no relocation, no DUMMY3M slot.

**One trap worth keeping.** The padding must go *after* the terminator, never
before it. `font_print_string` measures what it draws and jimaku centres on that
width (`field_4_x = (FRAME_WIDTH - max_width) / 2`), so trailing spaces ahead of
the NUL would silently pull the line off centre. `selftest.py` guards it, and
that guard was confirmed by mutation along with three others.

**How far it is verified.** Statically, and by effect rather than by inspection:
applying the build to the archive leaves its length unchanged, re-walks both
stages to the same 1279 and 906 commands with identical structure (so nothing
desynced), leaves all 159 changed bytes inside the three slots, and each slot
re-parses at length 55 with the English at the front. The English itself is read
out of `usa1_stage.dir` at build time rather than retyped. The two PPFs are
disjoint from all nine deployed PPFs on each disc, `ppfcheck.py --deployed` is
clean over all 31 files, and the family is registered in `rebuild.py`: the clean
run `repro17` rebuilt both PPFs **byte-identical** to the deployed ones, 27 of 27
across the set.

**Not yet seen on screen** — and it needs more than a save: the subtitle only
fires when pad 2 becomes active, so it wants the Mantis room *and* a second
controller in port 2 (or the emulator's port-2 assignment). `StageSelect = s07b`
can load the stage, but §5.7 records a direct `s14e` entry hanging without story
state, so expect to reach it in play. Which branch the game actually takes, and
whether site A is reachable at all, is worth noting when it is seen.

### 5.12 What the collection's own disc patches contain — ANSWERED 2026-09-07
Three documents said this could not be known. It can.

The collection's patch table has two kinds of entry. An *offset* patch carries
its bytes inline, so `SQHook::SetPatchWatch` logs them. A *named file* patch
carries only a name and the game reads the bytes from its own archive later, so
the watch reports "0 bytes" and the content stayed invisible — including the
five that land on the port's own text. `m2archive.py` reads them.

**How.** `windata/alldata.bin` is a flat blob and `alldata.psb.m` beside it is
its index: an MDF (`mdf\0`, a u32 size, then a zlib stream XOR-obfuscated with a
64-byte keystream from `MD5("25G/xpvTbsb+6" + the file's own lowercased name)`
seeded through MT19937 — the literal is in the game exe at 0x75ECE0). Inflated,
it is a PSB v3 whose `file_info` maps 4,926 paths to `[offset, size]`.
`099/patch/` holds 176 entries, each an MDF wrapping a *stored* deflate block,
so the payload is plain bytes behind one layer of obfuscation and its adler32
checks — which is what makes an extraction certain rather than plausible.

**What they say.** Not translation: they **blank the line telling the player to
open the disc tray** and reword the prompt. On the `title` block (`0x1822B55D`,
592 bytes, 30 differing from retail) record 1 becomes twelve spaces and record 2
is reworded; the `abst` block (`0x132F2716`, 240 bytes, 83 differing) blanks
records 1 and 3; `change` (`0x18345E07`, 174 bytes, 48) blanks record 2. One,
`disc1_18412A95_patch_PS5` on `demosel`, differs from retail in **0 of its 19
bytes** — it writes exactly what is already there. The platform suffixes
(`_NX`, `_PS`, `_PS5`, `_XBOX`, `_STEAM`) differ only in button-glyph codes.

**Three things this settles.**

* **`en_menu3`'s collision is confirmed with bytes.** Their patch overwrites 592
  bytes starting at the address of our record 0, in retail's own layout. Ours
  rewrites the same block with USA's English and different record boundaries.
  Whichever lands last decides, and if ours does not, the block is inconsistent
  — which is the `GCL:WRONG CODE` run seen three times. Raw-disc-only stands.
* **The `abst` patch our relocation orphans is harmless.** It blanked two
  Japanese strings that `en_abst` replaces with USA's English anyway.
* **The archive index confirms every image base this port found by scanning.**
  Integral discs 1/2/3 at 0, `0x2AE54800`, `0x57592000`; USA discs 1/2 at
  `0xF12F8000`, `0x11B3E5800`; USA VR at `0xD39B7000`. All six, exactly.

### 5.13 Two measurements worth keeping
**Integral's images are faithful retail dumps where it matters, and that is now
proved rather than assumed.** Put the separately supplied retail executable back
into its zero-filled ISO extent and recompute the parity the collection left
behind: `int1.exe` reproduces all **313** stored sector tails, `int2.exe` all
313, and `vrint.exe` — which is *built from the decomp*, not copied — all **308**.
A 280-byte sum over 2048 bytes, matched 934 times. So the decomp's output is the
disc's own bytes where its source is unchanged, and the raw-disc variant's
assumption about these images holds.

**`us1.exe` is not the executable the collection's USA image was built with —
IDENTIFIED 2026-09-12.** The same check reproduces only **8 of 318** tails
there. The user pointed at two real Redump dumps (`C:\Users\Tideg\Desktop\MGS1
USA`): "Metal Gear Solid (USA) (Disc 1)" (base/"Rev 0") and "... (Rev 1)",
416,363 of 651,264 bytes apart. `work/us1.exe` and `us2.exe` are both
byte-identical to the **Rev 1** dump (SHA-256 `615e1360…`, the hash
`rebuild.py` already required). Extracting the **Rev 0** dump's own
`SLUS_005.94` and running the same zero-fill parity check against the
collection's `windata/alldata.bin` reproduces **318/318** sector tails — so the
collection was pressed from Rev 0, and this port's four-executable input set
has been Rev 1 for `us1.exe`/`us2.exe` since the beginning. `items.py`,
`savemsg.py` and `rendertext.py` do read USA text out of `us1.exe` directly
(not only `usa1_stage.dir`), but every string they took from it was
independently confirmed on screen already (the 34/34 item and weapon
comparison, the memory-card captions), so the Rev 0/Rev 1 difference does not
appear to touch those text tables — whatever it changes, it hasn't produced a
visible fault. What it actually changes in play is still unconfirmed; the 64%
byte churn starting at the PS-EXE header looks like a code-size-changing
revision rather than a few patched bytes, consistent with a real bug-fix
pressing, but nobody has disassembled the difference. See README, "Audit
against the decomp" for the check and the numbers.

### 5.14 Where else does Integral's own English differ from USA's? — SWEPT 2026-09-08
Raised by the user 2026-09-07, immediately after the `SCARF` case: *"if there are
other instances of existing English in Integral that differ from USA."*

**Three are known now. Two were found by accident; the third was found by the
sweep this section asked for.** `SCARF` against USA's
`HANDKER` turned up because it happened to sit in the corner of a screenshot
taken for another purpose; the `abst` location spellings (`Tank Hanger`,
`Medi rm`, `Cmnder rm`) turned up while porting the mission log — and there is a
fourth, `Cmnd rm`, that only the sweep found. Neither of the first two was
found by looking. There is no reason to think two is the total.

**Why nothing here would have caught them.** Every sweep this project owns hunts
*Japanese*: `mainsweep.py` and `vr_sweep.py` pair a record with USA's and ask
whether Integral's is Japanese where USA's is English; `jpsweep.py` scans for
Japanese-looking pointer slots; `audit_text.py` inventories game-encoded string
candidates. A string that is already English on both discs and merely **says
something different** passes all four without a murmur. That is the blind spot,
and it is exactly the shape of both known cases.

**A method that would work, sketched.** The obstacle is pairing: the two builds
lay their data out differently, so a positional diff is meaningless. But both
known cases sit in a *sequence* whose neighbours match - `SCARF` is between
`SUPPR.` and `ROPE` on both discs - which is what a diff is for.

1. ~~**Executables.**~~ **DONE 2026-09-07, and the answer is one.** Extract the
   ordered NUL-terminated Latin strings from `int1.exe` and `us1.exe` and run a
   sequence diff over the two lists: equal runs align themselves, and a
   **replace** hunk of one string against one, with matching context either
   side, is exactly the `SCARF`/`HANDKER` shape.

   Filtering to text-like strings - printable, mostly letters - and running it
   at minimum lengths of 5, 4 and 3 characters gives the **same single result
   every time**: `SCARF` against `HANDKER` at 0x09BCC0, the one already changed.
   Nothing else in the executable differs. (Both discs' executables are
   byte-identical, so disc 2 is covered by the same pass.)

   The other hunks are all accounted for and none is portable text: the region
   and product strings (`...for Japan area` / `BISLPM-86247` against
   `...for North America area` / `BASLUS-00594`), a scatter of `.c` source
   filenames USA's build kept, USA's memory-card message pool - which shows as
   an insert only because Integral's counterparts are Japanese and so never
   enter a Latin extraction - and a few short runs of MIPS code that read as
   ASCII.
2. **Stage archives — DONE 2026-09-08.** `mainsweep.py --diff-english` sequence-
   diffs the two discs' ordered English strings per stage, so equal runs align
   themselves and a `replace` hunk is the shape being hunted. Disc 1 gives **15
   replace hunks over 82 stages, 8 holding player-readable text**, and after
   triage the residue is exactly one family: the `abst` location names
   (`chara 53C7`), where **four** pairs differ, not the three every document
   listed. The fourth is **`Cmnd rm` against USA's `Cmnd room`** — nobody had
   noticed it, and it sits in the same table as the other three. Disc 2 is
   identical. All four are live on the deployed disc: `abst_build.py` rewrites
   only the `0x9906` pages and the disc-change block, so the location list is
   copied through as Integral wrote it.

   The other seven readable hunks are all explained, and the explanation is the
   noise profile to expect: voice-clip and stage asset ids (`sound`, `selectd`,
   `chara 4EFC` — `vc319010` and the like), and hunks where one side's
   counterpart is *Japanese* and therefore never entered an English list at all
   (`abst`'s 907 recap lines, `option`'s help lines, `title`'s disc-swap block,
   and USA disc 1's `vr01`..`vr10` mission names, which Integral's disc has no
   equivalent of).

3. **The VR disc — asked, and it needs a different input.** Two things had to be
   learned. USA's VR disc carries **five languages**, so the diff must take only
   the English arm of a language branch (`lang in (None, ENGLISH)`, the test
   `vr_windows.py` already uses); without that the alignment collapses — 548
   hunks against 267 with it, and no one-against-one hunk either way. Then a
   per-owner fuzzy match (Integral's English strings absent from USA's at the
   same owner, with their nearest USA counterpart) turned up **`FAMAS` against
   USA's `FA-MAS`** across the mission titles — and it is a **non-finding**: the
   deployed `vr_en_missions.ppf` holds **142 `FA-MAS` and zero `FAMAS`**, because
   the port takes USA's window text verbatim, so those windows already read
   USA's spelling.

   **That is the lesson, and it is the opposite of the Japanese question's.**
   `mainsweep.py` reads *retail* on purpose, so a gap cannot hide behind a patch
   that is already deployed. The English-against-English question must be asked
   of the **deployed** bytes instead, because there the port's own replacements
   are precisely what has to be subtracted — otherwise the sweep rediscovers the
   port's own work and calls it a finding. Only `vr_sweep.py` has the plumbing to
   reconstruct a deployed stage (`deployed()`); `mainsweep --diff-english` does
   not, which is safe today only because the one family it finds sits in records
   no patch rewrites.

   **Half of that is fixed as of 2026-09-08, the cheap half.**
   `--diff-english` now checks each finding's stage against `PORTED` and prints
   `!! <stage> is owned by <family> and these are RETAIL bytes` beside it, with
   a closing summary naming every flagged stage. It cannot tell you what the
   patch writes - it turns a silent trap into a printed one, which is what was
   actually dangerous. Three stages flag today: `abst`, `option`, `title`. The
   `abst` four are flagged and *were* real, and are now ported, so from here the
   flag is what stops someone porting them twice.

   **And the expensive half should NOT be built.** The first plan here was a
   deployed-stage reconstruction like `vr_sweep.deployed()`, extended to follow
   `en_abst`'s and `en_brf`'s relocation into DUMMY3M. That was the wrong
   answer, and the reason is worth keeping: it re-derives by the hardest
   available route something the project already has. **The builders construct
   the ported stage in memory.** Reconstructing it from PPF records means
   parsing the patch, applying it, following a relocated directory entry and
   re-parsing the result - and needing an extension every time another family
   starts relocating.

   **The right division of authority, and it is mostly already in place:**

   | bytes | authority | mechanism |
   |---|---|---|
   | a family owns them | that family's builder | a verifier over its own built output |
   | nobody owns them | the sweep, reading retail | retail *is* deployed there, so it is already right |
   | the boundary between | the `!!` flag | "a patch owns this, go read its builder" |

   So the cheap fix is not a stopgap; it is the correct thing at the boundary.
   `abst_build.py` now verifies its location list against USA record for record,
   and `vr_windows.py` has always checked every window against USA's. **What is
   actually left is per-family verifiers where they are missing** - small, local,
   testable, and they fail at build time instead of waiting for a sweep to be
   run. Note also that the Japanese sweep and `--census` must keep reading
   **retail**, so the input is a per-question choice and never a global switch.

4. **Not textures.** Lettering drawn as art is out of scope for a text sweep and
   stays that way.

**Expect noise, and know its shape before starting.** Integral-only features
(VERY EASY, the MP5 SD, PocketStation, `rank`'s commentary), product branding and
the save-title suffix, and version or build strings will all differ legitimately.
The useful output is the small residue: the same UI element, worded differently.

**What to do with a result is already settled** - §2 amendment 4b. Each find is
the user's call, one at a time, and the answer may well be "leave it": Integral's
own English is not wrong, it is just not USA's.

### 5.15 The VR grenade delay fix - texture confirmed; briefing corrected; INI controls deployed

The user's measured fuse is approximately four seconds on both discs. Integral's
`DELAY 5.2` decal and five-second briefing are original authoring errors. The
standalone fix now covers **both assets**, as requested on 2026-09-11. It remains
outside the collection English package and the MGSM2Fix upstream PR. Since
2026-09-12 the raw English package includes it by the user's explicit request
(§5.17).

- **Texture:** USA's exact decoded `DELAY 4.0` texture, in `selectvr`'s second
  DAR, `4D80.p`. Fits Integral's existing slot losslessly. **Confirmed good
  in game, 2026-09-11 — with the English variant.** The Japanese-only
  standalone has not been checked on screen. The earlier `vab_grn` location was
  wrong: its divergent tail is a GCL script (HISTORY §29).
- **Briefing:** one digit in each of `vr_grn01`-`vr_grn05`, whose scripts duplicate
  the GRENADE LEVEL 02 window. Japanese `80 35` becomes `80 34`; every other
  byte, including all Japanese glyphs and `Targets 3`, stays unchanged. English
  gets the corresponding `5` -> `4`. **Confirmed good in game, 2026-09-11 — with
  the English variant.** The Japanese-only standalone is built and statically
  verified (§30) but **not yet checked on screen.**
- **Compatibility:** the English port changes string offsets, so the builder
  emits Japanese and English variants. `py vr_grenade.py --deploy` installs
  both language PPFs and JSON companions. The loader automatically selects the
  active layout from `IntegralVREnglishPatch`. Toggle changes need a restart,
  not a rebuild; regenerate the companions if the English PPF itself changes.
  The updated `vr_windows.py` uses the same correction helper;
  its PPF differs from the previous deployment at exactly five digits and agrees
  with the addon in either load order. When `GrenadeDelayFix` is off,
  the loader overrides those five digits to 5. No other number policy changed.

Artifacts in `WORK`: `INTEGRAL_vr_fix_grenade_delay.ppf` (Japanese standalone),
`_raw.ppf`, `_english.ppf` and `_english_raw.ppf`. Each collection variant has
709 records / 9,509 payload bytes; raw variants repair all 11 touched sectors.
`--missions-ppf <path>` builds against an explicit English base; its hash is
recorded beside the English output. README "Standalone VR grenade delay
correction" has commands, offsets, raw application order and validation details;
HISTORY section 30 records this extension. Both collection variants are now installed together with the new loader;
only the matching one is enabled. Raw variants stay outside the game folder.

The all-missions unlock was originally parked at the user's request. It is
now back in the active folder, **explicitly confirmed by the user**, and gated
by `UnlockVRMissions = false`. The parked copy is retained. All four new unlock
controls default to false; the title/extra/movie/USA mission aids are installed
behind those controls too. Previous deployed grenade/mission PPFs are backed up in
`WORK/grenade_before_briefing/`.

### 5.16 Optional patch INI controls - built, verified and deployed, 2026-09-11

Requested by the user after the patch inventory: three `[Patches]` controls
(`IntegralEnglishPatch`, `IntegralVREnglishPatch`, `GrenadeDelayFix`, default
true) and four `[Game]` controls (`UnlockVRMissions`, `UnlockVRExtras`,
`UnlockVRMovies`, `UnlockTitleBonuses`, default false). They control installed
PPFs and require a restart. `EnglishText` and `UnlockBriefing` remain separate.

The main English set is controlled together across both discs; the VR English
set is independent. The grenade control restores the original texture **and**
5-second numeral when false, including English: a companion file pins the
mission PPF and its five digit addresses, and Ketchup changes only those bytes
while loading it. Both texture language variants are installed, and selection
uses the enabled English set, not just the existence of a filename.

Native tests use the exact header Ketchup calls, covering the seven switches,
all eight VR unlock combinations, title/disc scoping, unknown mods, fingerprint
mismatch and bad digit records. `verify_patch_options.py` applies its four
English/grenade plans to the real disc stages, including all installed English
PPFs, and proves the expected textures and every briefing byte. All 49 Python
self-tests pass. Release x64 built successfully with the post-build installer
disabled (`/p:PostBuildEventUseInBuild=false`), and the ASI was copied to the
Vortex symlink target and hash-verified. The installed INI has English/VR/grenade
on, all four new unlocks off. Existing settings were preserved.

**Briefing follow-up, confirmed by the user:** preserving existing settings had
left `UnlockBriefing = true` in the active Vortex-backed INI. This is separate
from `UnlockTitleBonuses` and the VR unlock PPFs. At the user's request it was
set to false, and the user reported, "Good that fixed it." No save changes
were needed for this report. Saves started with the control enabled can still
carry its flags; disabling the control stops forcing them and does not clear
saved progress. Keep `UnlockBriefing = false` for normal play.

Backups of the previous ASI and INI are in `WORK/ini-toggle-backup/`. The native
verification report is `WORK/verified-patch-options.json`. An actual in-game
restart/test of the new INI controls remains pending; do not call the native
planner/real-disc checks an observed play test. README "Optional PPF controls"
and BUILDING "Tests" document operation and rerunning verification.

---

### 5.17 Raw package includes both grenade corrections, 2026-09-12

The user found the missing texture after making raw images. Readback of all
three BINs in `D:/mgsbuild/patched` matched all 33 PPFs in `repro34raw` exactly;
that package predated the correction. Its VR image still had Integral's
original texture and all five English fuse digits were 5. The raw packager
also omitted the standalone texture addon even with current source.

At the user's request, raw builds now include
`INTEGRAL_vr_fix_grenade_delay_english.ppf`: the USA DELAY 4.0 decal and five
agreeing briefing writes. The English mission builder already corrects those
digits. The new payload joins the package **before** `INTEGRAL_vr_zz_ecc.ppf`
is generated, so parity describes the entire English set. No standalone raw
parity addon or INI control is needed. Collection packaging remains separate.

**Built and verified:** `D:/mgsbuild/repro35raw/Integral-English-raw.zip`, 34
PPFs (24 main, 10 VR). Only the VR mission PPF, new grenade PPF and VR ECC PPF
differ from `repro34raw`; main-disc patches are identical. The new VR BIN was
built from the retail dump with English power-on retained: all 2,015 touched
sectors pass parity, decoded texture pixels match USA exactly, all five
briefings differ only at 5 -> 4, and every other stage is unchanged. The BIN
in `D:/mgsbuild/patched` was replaced and hash-verified; its previous copy is
in `repro35raw/previous-image`. Report: `repro35raw/raw-grenade-verification.json`.
Image SHA-256: `90a50dfb7ec1ed99ebae609590e119b4d3cfb8f13227f4268552b2b47380b894`.
The 49 self-tests pass. **Confirmed working in game by the user, 2026-09-12:**
"Good it works." Both grenade corrections are now verified in the raw English build.

## 6. Decisions that are the user's — ask, do not assume

**Four of these were put to the user on 2026-09-07 and deliberately left open;
one of the four - the `abst` location names - was then decided on 2026-09-08,
so THREE remain.** The three are marked **[open 2026-09-07]** below. Being held
is a decision in itself, not an
oversight: they were raised, considered alongside the `SCARF` case that was
decided the same evening, and held. Do not re-raise them as though they were
newly noticed, and do not read the passage of time as consent - each still needs
an explicit answer before anything changes.

- **The caption under READ MISSION LOG? and USA's `1/2` on single-screen pages
  — DECIDED 2026-09-11: both stay as they are** (kept Japanese by rule; one
  constant blanks it; the `1/2` reproduces USA's own behaviour) — §5.9.
- Anything under the 2026-09-03 amendment: moving English text to fit
  Integral's own art.
- The `en_savemsg` collision approach, if one is ever observed.
- Achievements on or off for a given test session (`DisableRAM` /
  `DisableCDROM`); they are **on** now. Turning them off keeps SPECIAL / PHOTO
  ALBUM reachable without earning it and has never affected the PPFs.
- **The `abst` location names — ASKED AND ANSWERED 2026-09-08: use USA's.**
  Four of them, not the three this file used to list; §5.9 has the table, the
  mechanism and the measurements. The **second** application of amendment 4b,
  after `SCARF` -> `HANDKER`, and the first that made a container grow (+12
  bytes, absorbed by `abst_build.py`'s existing re-stamping). Built, verified
  both ways and deployed the same day; not yet seen on screen.
- **The VR disc's number substitutions — DECIDED 2026-09-11: keep Integral's
  numbers (the default).** Where Integral and USA state different values, USA's
  sentence is taken with **Integral's** numbers put into it, so the text
  matches the disc it runs on. `SUBSTITUTE_NUMBERS_OFF = True` in
  `vr_windows.py` would take USA's numbers verbatim instead; not used. The grenade
  fuse is now the explicit authoring-error exception (section 5.15): after normal
  substitutions the shared grenade-fix helper corrects its five copies to 4.

  **All three claimed cases are now accounted for.** WEAPON MODE / GRENADE
  LEVEL 02 was confirmed against a real gameplay window on 2026-09-11 (README,
  "Numbers that differ between the two versions"); the differing number turned
  out to be the fuse-timer sentence, not the target count as first assumed.
  SNEAKING MODE / NO WEAPON LEVEL 10 and SNEAKING MODE / SOCOM LEVEL 03 did
  **not** match what either disc's live window showed on 2026-09-11 — on
  screen both read identically to each other, with none of the four claimed
  digits (25/35, 40/43) appearing anywhere.

  **RESOLVED 2026-09-12, by running `vr_windows.py`'s own stage-by-stage report
  instead of just its de-duplicated summary.** The summary print only shows one
  line per distinct (key, Integral-numbers, USA-numbers) triple, which hid
  *which* stage each one came from. Reproducing the loop and keeping `name`
  showed that the numbered stage each title names as its own — `vr_sud10` for
  "NO WEAPON LEVEL 10", `vr_scm03` for "SOCOM LEVEL 03" — reports **zero**
  number diffs and **zero** kept-Japanese windows for all 20 of its own
  windows. Both flagged diffs instead come from *other* stage files that
  merely carry a pooled duplicate of that exact titled window as leftover
  template content never shown under that title in play:

  | title | flagged in (pooled duplicates) | owning ("stage") copy | the live numbered stage |
  |---|---|---|---|
  | NO WEAPON LEVEL 10 | `vr_sud05,06,07,12,14` | `vr_sud08` | `vr_sud10` — 0 diffs |
  | SOCOM LEVEL 03 | `vr_sud01,02,04,05` | `vr_sud03` | `vr_scm03` — 0 diffs |

  This is exactly what "Integral's stages carry every mission a stage family
  can host, not only the reachable ones" predicted, and it independently
  matches the 2026-09-11 on-screen read word for word: no number in the live
  NO WEAPON window, `Enemies 3` identical on both discs in the live SOCOM
  window. **Nothing needs fixing** — the numbers `vr_windows.py` substitutes on
  `vr_sud01,02,04,05,06,07,08,12,14` are inert data inside stages the mission
  menu never opens under those titles, not text a player can ever see wrong.
- **The VR KEY CONFIG's `key_syukan` +11 shift**, carried over from the main
  game's 2026-09-03 approval rather than asked again.
- **The VR MOVIE EXIT box, moved up 4 px — ASKED AND APPROVED 2026-09-07.** With
  USA's two-line caption the first line's ink overlapped Integral's EXIT box by
  two pixel rows; USA makes the room with a shorter caption face *and* a higher
  box. The face stays Integral's (its own art), so the box moved instead, to
  USA's own `sp_exit` y of +66 (screen 186 against 190) — one immediate at
  overlay `+F128`, and the selection highlight follows because it anchors on the
  same object. Approved against the user's stated default, **"usually we skew
  toward the Integral visuals"**, which makes this a named exception rather than
  a precedent: §5.4a has the measurements and the reasoning.
- **The item short-name `SCARF` -> `HANDKER` — ASKED AND APPROVED 2026-09-07,
  and it set a precedent.** Integral's side-column abbreviation for item 22 was
  already English and disagreed both with USA's and with Integral's own Japanese
  description (《ハンカチ》, a handkerchief). The user's instruction created
  amendment 4b in §2: replacing Integral's **existing English** is an exception
  to the rule, allowed when asked, case by case. It is not a new default - the
  `abst` location spellings are the same shape and remain undone and unasked.
- **VR EXTRA menu record 6 — DECIDED 2026-09-11: keep Integral's PocketStation.**
  Integral's fifth item is PocketStation where USA's is a different feature,
  STAFF CREDIT, so `See the staff credits.` was never a translation of it and
  is not used. Confirmed: stays Japanese, authentic to Integral.

---

## 7. Tools (all in this directory; `py <tool>`; they find `work\` themselves)

| tool | purpose |
|---|---|
| `workdir.py` | resolves the working directory **and the build variant** (`py workdir.py` prints both); `VARIANT`/`RAW`/`pick(collection, raw)` come from `INTEGRAL_ENGLISH_VARIANT` |
| `ppfcheck.py [--deployed]` | validates PPFs exactly as Ketchup reads them — run before deploying anything |
| `optsctext.py` | builds `en_option` (sc_text texture, KEY CONFIG art, chain, relocation to DUMMY3M slot 384, doorbell stub check) |
| `verify_integral_option.py`, `verify_usa_brightness.py` | read the deployed PPFs / built-in patch back and check them |
| `shotcmp_brightness.py A.jpg [B.jpg]` | measures brightness-screen shots |
| `preope_usa.py` | Previous Operations directly from retail, USA pagination; stages PPFs unless `--deploy` is supplied |
| `brf_build.py`, `brf_widen.py` | briefing labels and quads; the build asserts zero load-delay hazards against retail |
| `overlaydiff.py [--vr] [--all] [--stage NAME] [--debug]` | **per-stage English-vs-Integral overlay comparison**: which printable strings USA's copy of each stage overlay has that Integral's lacks, net of what the deployed PPFs already write, with debug/symbol strings and the four other languages filtered. The question the 2026-09-11 memory-card finding showed nobody had asked; a byte inventory that judges per string cannot ask it. Also worth running over the executables (the same three lines of Python, in §26) |
| `hazards.py <built> <base-hex> [--retail <retail>]` | **R3000 load-delay scanner** for hand-written MIPS: a load followed at once by a read of the loaded register, a non-load write in that slot, a branch in a branch's delay slot, mfhi/mflo too close to a mult. With `--retail`, only word pairs the port changed are judged. The Master Collection's emulator does not model the delay, so no screenshot taken there can catch this class of bug - this is the check. `selftest.py` proves it catches the 2026-09-10 briefing bug at its own address |
| `savemsg.py`, `camsave.py` | the two memory-card caption ports |
| `abst_build.py [--deploy]` | the MISSION LOG port: rewrites both GCX scripts in the `abst` stage with USA's pages, swaps the bottom-bar art, packs the stage with `obj/abst.bin`, relocates to DUMMY3M slot 462, verifies, stages/deploys the PPFs |
| `abstscan.py [page N]` | mission-log scoping data, both games (retail data; the port's own checks are in `abst_build.py`) |
| `jpsweep.py` | historical disc-1 pointer-slot candidate scan; not a completeness proof |
| `gclparse.py`, `gcldec.py` | GCL container parsing / record walking — `containers_over` for resizing |
| `optscan.py` | option-stage inspection |
| `optlabel2.py` | current option captions from retail, including the restored colon; replaces the unsafe recovered experiment |
| `items.py`, `menu2.py` | recovered item/menu builders; the `title` copy lives in `menu3.py` instead |
| `menu3.py [--collection]` | the `title` stage's disc-swap copy, **raw disc only** — rewrites the five records inside `CMD 9906`'s `-v` option, re-stamps the SCRIPT/ARG/COMMAND sizes and leaves the overflowed `-v` u8; `--deploy` refuses, `--collection` rebuilds the shape that crashes so the fault can be reproduced (§5.3) |
| `audit_text.py` | main-disc and VR candidate inventory, save-title encoding; see `COVERAGE-RECORD.md` |
| `rebuild.py [--variant raw]` | the isolated build of **everything**: nine main families for both discs and the VR disc's seven, checks, manifest and ZIP. Builds Integral's VR executable from the decomp rather than copying it, and points `vr_movie` at its own output. `--variant raw` swaps the two constants and adds `en_menu3`; `--compare-deployed` checks every PPF's effective bytes against `mods/`. See `BUILD-HISTORY.md` |
| `mkimage.py` | **writes a patched disc image** from a raw build — the step that turns PPFs into something a burner or emulator can open. `--redump <disc.bin>` or `--collection --game ... --exe int1.exe` (the collection hollows the executable out, so it refuses without one). Checks four things before writing: every PPF's block check against the image, no record past the end, every touched sector's parity **before** patching, and again **after**. §22 |
| `rawdisc.py [package]` | the EDC/ECC pass `rebuild.py --variant raw` runs, and `verify_set` — apply a finished set in memory and check every touched sector against the parity the set wrote |
| `portio.py` | shared read-only disc access and deterministic PPF/stage serialisation (`stage`, `pack_stage`, `records`, `encode_records`, `changed_runs`, `ppf`, `relocation`) — the module the recovered builders and `rebuild.py` are built on |
| `iso.py` | raw-sector disc reader (`Disc(path, base)`; mode-2 24-byte headers), used to read the images inside `alldata.bin` / `dlc_japan.bin` |
| `kcplace.py`, `kcquads.py`, `kcrects.py` | KEY CONFIG port helpers: VRAM/CLUT allocation for USA's eight labels, quad extraction from an option overlay, the per-button-type label rectangles |
| `quadscan.py`, `rowargs.py` | briefing helpers `brf_widen.py` imports: quad-call arguments and a linear register simulation for the row arithmetic |
| `measure.py`, `align.py`, `rows.py` | screenshot measurement for the briefing menu (label ink, rows against the divider, right-column bands) |
| `optbright.py` | **historical** — the font-text brightness build the `sc_text` texture superseded; its wrap-width notes are still the reference for other option entries, its output is no longer an input to anything |
| `ppfgen.py`, `reloc_ppf.py` | **legacy** PPF emitter and manual DUMMY3M relocation; `rebuild.py` and the builders use `portio.ppf` / `portio.relocation` instead. Still runnable; `reloc_ppf.py` needs disc images in `discs/` |
| `preope_both.py` | **obsolete** experiment (both recaps re-wrapped, 12/19 pages); `preope_usa.py` supersedes it and needs nothing from it — a candidate for removal |
| `unlock_title.py` | builds main-game unlock PPFs, controlled by `UnlockTitleBonuses` |
| `vrlib.py` | **the VR disc's shared library**: both VR ISOs and stage dirs, the GCL parser/emitter used for VR scripts (`parse_arg`/`emit_arg`, options, expressions, the language variable), `Gcx` script/proc/font container, in-place stage repacking with sector padding, PPF records with gap merging, deploy |
| `vr_windows.py [--build] [--deploy]` | the mission-window port: pools USA's windows by title, matches by content, merges the script-local fonts, substitutes Integral's numbers, rebuilds 92 stages in place |
| `vr_exe.py [--deploy]` | the VR executable's item/weapon/capture pools and its save and load messages |
| `vr_option.py [--deploy]` | the VR option stage: help-line chain, the KEY CONFIG label transplant (per-type function, 21 call sites, `key_syukan` +11) and the re-encoded texture archive |
| `vr_menus.py [--deploy]` | the VR EXTRA menu's help lines |
| `vr_camera.py [--deploy]` | the VR camera overlay's memory-card messages |
| `vr_movie.py [--deploy]` | the MOVIE selection captions. Builds **two** PPFs into `work\`: `..._movie.ppf` (all six USA records, USA's position table and the two-line stub — **the port**, deployed) and `..._movie_e3.ppf` (the E3 caption alone, retail structure and no code change — the fallback). `--deploy` installs the first and moves the second out of `mods\`, since they overlap and only one may be present. Built on the composite, since `vr_en_missions` already owns the stage |
| `vr_kcgeom.py` | VR KEY CONFIG geometry read from an overlay: `Init_Res` quads and the per-button-type rectangles (imported by `vr_option.py`) |
| `vr_unlock.py [--deploy]` | the removable VR **mission** unlock test aid — three words, enable only with achievements off |
| `vr_unlock_extras.py [--deploy]` | the removable VR **EXTRA menu** unlock test aid — three `andi` tests in the `vrtitle` overlay's visibility-mask construction, so PHOTOGRAPHING / ALBUM / PocketStation always appear. Writes no progress; disable its INI control and restart to relock |
| `vr_unlock_movies.py [--deploy]` | the removable VR **movie** unlock test aid — one instruction in the `movie` overlay's own `count / 3` score gate, which `vr_unlock` does not touch. Writes no progress; disable its INI control and restart to relock |
| `vr_sweep.py [--samples]` | rebuilds every VR stage from the deployed PPFs and reports what is still game-encoded, beside USA's own tally — the VR equivalent of `jpsweep.py`, and the only tool here that inverts `portio.image_offset`'s 2352-byte sector geometry |
| `bridge.py` | the Squirrel-debugger client for live RAM reads/pokes (README "Toolchain and environment"); writes `sqcmd/`, `sqout/`, `bridge.log` beside itself (git-ignored) |
| `gcldump.py`, `gclprocs.py` | dump a stage script's command tree / every proc with decoded values (used to read the title script's 1P MODE path) |
| `pcx4.py` | encode/decode the 4-plane RLE PCX the texture loader expects (how `sc_text` and the KEY CONFIG art were read and written) |
| `selftest.py` | **49 tests over the pieces that need no game data** — the PPF emitter's two split boundaries, the record chain, the PCX codec, the EDC/ECC algebra, the width model, the language default, the load-delay scanner, the Japanese/English grenade numeral correction. `py selftest.py`, a tenth of a second. Ground truth lives elsewhere: `cdecc.py` against the real discs, `rebuild.py --compare-deployed` against the deployed set |
| `cdecc.py` | EDC and P/Q parity for raw Mode 2 Form 1 sectors. `py cdecc.py` is the check that proves both the sums and the retail executables: it rebuilds each zero-filled executable extent from the supplied retail file and matches the parity the collection left behind (313/313, 313/313, 308/308) |
| `rawdisc.py` | the raw-disc EDC/ECC pass. As a library `rebuild.py --variant raw` uses it to emit each disc's `*_zz_ecc.ppf`; as a command, `py rawdisc.py <package>` applies a finished raw set in memory and confirms every touched sector verifies |
| `widths.py` | how wide a ported line renders and how wide it may be: the `vrwindow` budget derived step by step from the decomp, the 255-px `max_width` ceiling, and the pool line separator. Read its docstring before adding a width assert — the per-window budget is **not** an invariant, retail exceeds it |
| `mainsweep.py` | the main discs' answer to `vr_sweep.py`: pairs every GCL string with the USA disc's by owning command, so "Integral Japanese where USA has English" is measured. `py mainsweep.py [--disc 2] [--samples]`. Its one uncovered finding is §5.11, ported 2026-09-08. Three modes were added the same day: `--integral-only` pairs each of the 13 Integral-only stages with the USA stage it is a variant of (the shared-name universe's hole — 1 string, already ported); `--diff-english` sequence-diffs the two discs' English for wording differences (§5.14 step 2 — found the fourth `abst` spelling); `--census` accounts for every Japanese GCL string and **exits non-zero unless the unaccounted bucket is 0** (§5.8) |
| `radiomap.py` | **where every `RADIO.DAT` fragment starts and where its bank-1 font sits.** Parses each candidate sector's record list (self-checking) and checks the result against the 192 fragment extents the game's own radio codes declare. Prints DISTINCT bitmaps the text lookups produce (1,200) and their blank-twelfth-row rate (100%), which is a far better measure than the share of strings attributed that the broken predecessor reported - but both are aggregates and blind to a single fragment's base slipping, so pair it with `radiotext.py --check` (§21). `py radiomap.py` |
| `radiotext.py` | **every subtitle, by walking the records the game walks** (`menu_gcl_exec_block_800478B4` and the TALK/IF/SWITCH/RANDSWITCH payloads). Replaces the byte scanner for `RADIO.DAT`, which was dropping 15% of the commentary. Also carries **the regression guard on the fragment map**: `--check` requires every fragment's bank-1 text to also appear in another fragment (worst real score 96.0%, a base slipped one glyph scores 0.0%), and `--selftest` slips bases on purpose and requires `--check` to catch them - which the first two versions of the check did not. §21 records both metrics that failed. `py radiotext.py [--check] [--selftest] [--dump out.txt]` |
| `glyphsheets.py` | renders the unidentified bank-1 glyphs to `work/glyphs-to-identify.pdf` and matching PNGs, ordered by frequency, **with a `.txt` of real decoded sentences beside each page** - that companion file is the more important half, because at 12x12 線/緑 and 間/問 are the same picture and a sentence is not. Leaves the 78 already-known shapes in unmarked, with the answers in `work/glyph-answers.tsv`, so a pass can be scored. `py glyphsheets.py` |
| `glyphfill.py` | merges a transcription into the TSV and **scores it**: against those 78 known answers, and against the invariant that bank 1 never reuses a bank-0 character. Every warning it raised in the 2026-09-10 pass was a real error. `py glyphfill.py [--score]` |
| `glyphreview.py` | prints the **full** decoded lines a glyph appears in - `--verify` does one sentence for every shape, which is the read-through that caught 黙/弄 swapped and 完 for 璧. `py glyphreview.py --verify` |
| `glyphocr.py` | identifies the game's 12x12 glyphs by matching them against a system Japanese font. **47% top-1 / 59% top-3**, measured against the 238 hand-transcribed `0x90` kanji — a shortlist tool, not an oracle; its errors are 鏡/鎌 and 線/緑, which at 12x12 are the same picture. `py glyphocr.py --validate` |
| `dumpjp.py` | **the dump**: every in-scope Japanese line, as readable text (`<disc>_<source>.txt`), as a locating index (`index.tsv`) and drawn from the game's own glyph bitmaps (`.pdf`). 68,242 lines, 3,923,944 kana/kanji, zero unresolved codes. `--scope unported` (default) keeps only Japanese USA has no counterpart for. `RADIO.DAT` comes from `radiotext.py`, not the inventory. Discs are keyed by name - `disc1`, `disc2`, `vr` - because the VR disc has a stage archive but no `RADIO.DAT`, and a loop over indices used to skip it entirely (§20). `py dumpjp.py [--discs 1,2,vr]` |
| `jptext.py` | **makes the list readable**: font codes -> Japanese (kana by arithmetic, the `0x90`/`0x91` kanji banks by transcription, bank 1 through `bank1-glyphs.tsv`'s 1,214 shapes). Bank 1 needs the block's own font blob to turn a code into a bitmap, so `jptext.py` **run on its own still reports 87.2%** - it is reading the inventory with no blobs. Hand it one (`dumpjp`, `radiotext`) and it resolves everything: the export has zero unresolved codes. Writes `work/japanese-readable.tsv`; `--hex` decodes one run, `--sample N` prints the longest. Bank 1 is a per-block table carried by the block (proven: `abst`'s blob glyph 0/1 are 記/録), so what is left is glyph recognition, not reverse engineering |
| `jplist.py` | **the list** - but see the warning in §19: its scanner **ends a run at any code it does not recognise**, so it holds only 85% of the commentary's glyphs and `RADIO.DAT` should be read through `radiotext.py` instead. Every untranslated Japanese string on all three discs, one per line, to `work/japanese-inventory.tsv` — 190,180 strings / 3,318,254 kana-kanji glyphs, 32 MB, regenerated not committed. Drops any run the USA disc also has, which is what empties `BRF.DAT` and `FACE.DAT`. `py jplist.py [--min N] [--render N]` |
| `discaudit.py` | **every file on every disc**: size, the Integral-vs-USA delta and a crude text probe. Written 2026-09-09 because every other sweep here reads only `STAGE.DIR`; the delta is the diagnostic (`RADIO.DAT` is +9.4 MB on Integral, and that is the developer commentary). `py discaudit.py` |
| `jpremain.py` | **what Japanese is still on the three DEPLOYED discs** — the only tool here that reads deployed bytes (retail + every deployed PPF, following the relocated STAGE.DIR entry for `abst`/`brf`/`option`/`preope`). 153 Japanese strings a disc, 38 on the VR disc, all itemised with reasons in `COVERAGE-RECORD.md`. `py jpremain.py` |
| `pad2.py` | `en_pad2`: USA's controller-port subtitle into all three of the sites `second.c` is spawned at, on both discs. Length-preserving — the English goes in at the front of Integral's longer slot and the length byte never changes. `py pad2.py` |
| `rendertext.py` | **reads the game's own Japanese, by drawing it.** The scripts store font indices, not Shift-JIS, so no table turns a Japanese string into characters - `game_text` can only print `<822F><8253>...`. This looks the glyphs up the way `font.c` does and renders them to a PNG: `py rendertext.py --item 22`, `--weapon N`, `--hex ...`, `--exe us1.exe`. Two things in it were settled by rendering a word whose reading was known, not by reasoning - the bit order, and a one-glyph bank offset - because either mistake produces plausible-looking Japanese that is simply the wrong Japanese |
| `m2archive.py` | reads the collection's own archive: `--roms` the disc images and bases, `--list 099/patch` what it patches, `--patches` every Integral disc-1 CD-ROM patch decoded against retail, `--extract` one member. This is what answered §5.12 |

Rescued from the session scratchpad on 2026-09-04, where they existed nowhere
durable: `bridge.py`, `gcldump.py`, `gclprocs.py` (now in this directory) and
`map_pristine.map`, the pristine executable's symbol map (now at
`D:\mgsbuild\integral-english-work\`). `items.py`, `menu2.py` and `optlabel2.py`
were likewise recovered and rewritten with explicit inputs and no implicit
deployment. Nothing the build needs lives in a scratchpad any more; the scripts
that only shaped documentation are disposable.

---

## 8. Where the details live (README section names)

Gotchas → "Gotchas" (freeze/crash triage, overlays, font limits, GCL chain,
textures, disassembly, screenshots, PPFs, toolchain) · "The three limits" ·
"Things that do not work" · "Wrap width" · KEY CONFIG → "The KEY CONFIG screen",
"The collection's KEY CONFIG interception, and how the port broke it", "How the
KEY CONFIG port was built" · PPF → "PPF3's description field is 50 bytes" ·
captions → "Memory-card messages (`en_savemsg`)", "The PHOTO ALBUM's own
memory-card messages (`en_camsave`)" · sweep → "Sweep: is any UI text still
Japanese?" · mission log → "The MISSION LOG port", "The disc-swap text: four
copies", "Why `en_menu3` is raw-disc only" (and its two sub-sections: the
diagnosis it replaces, and "The general trap: the collection may already own the
bytes you are porting") · scope → "Scope", "What stays
Japanese, and why" · brightness → "The collection shows only four of USA's six
brightness lines", "Option → SCREEN", "The sc_text texture port" · briefing →
"Briefing menu (`brf` stage)" · unlocks → "Unlocks", "Give items", "Unlock
everything", "Unlock every VR mission", "Unlocking the EXTRA movies",
"Unlocking the EXTRA menu's items",
"Achievements" · VR → "The VR disc (SLPM-86249)", "The MOVIE selection
captions", "The white caption font differs between the two VR discs",
"Sweep: is any VR text still Japanese?" · tests → "Not tested" (struck-through items are
done, with dates) · decomp → "Audit against the decomp" ·
text found late → "The controller-port subtitle (`en_pad2`, 2026-09-08)",
"The `abst` location names (Integral's own English -> USA's, 2026-09-08)" ·
what Japanese is left → `COVERAGE-RECORD.md`, "What Japanese is still there, and why"
(`jpremain.py`) · asked from outside → "Psycho Mantis's memory-card table, and
the RAM-patch question" ·
the collection's own patches → "Where the collection's own disc patches land",
"What the collection's named-file patches say" (what they contain, read
2026-09-07) · raw disc → "Raw-disc error correction, and what it proved on the
way" · widths → "Line widths: what is an invariant here and what only looks like
one" · credit and licence → `CREDITS.md`.

---

**Session history.** The dated, session-by-session account from here on — one
numbered section per work session, 2026-09-04 through 2026-09-11 (§9 "The
2026-09-04 late pass" through §27 "The 2026-09-11 cross-check against the
MGS1 Translation Toolkit") — moved to [`HISTORY.md`](HISTORY.md) on
2026-09-11, keeping the same section numbers; citations elsewhere in this
file (`§9`, `§18`, ...) point there. This file's own numbered sections stop
at §8, above.
