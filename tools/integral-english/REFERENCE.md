> Historical/reference record. For current instructions see [README.md](README.md),
> [BUILDING.md](BUILDING.md), and [NextSteps.md](NextSteps.md). Dated counts,
> machine paths, deployment state, and completed-work estimates below are not current instructions.

# Integral English text

Tooling that ports English text from **MGS1 (USA)** into **MGS Integral**
(Japanese), emitting PPF patches for the Ketchup mod loader. Nothing is
translated: every English string is copied from the USA disc, and only page
counts and line breaks change.

**Starting cold? Read [`HANDOFF-ARCHIVE.md`](HANDOFF-ARCHIVE.md) first** — where everything is, the
user's standing rules verbatim, how far each patch is verified, what remains and in
what order, and which decisions are the user's to make. This README is the technical
record; `HANDOFF-ARCHIVE.md` is the map.

**Three of the patches contain compiled code, and that code is built from
[FoxdieTeam/mgs_reversing](https://github.com/FoxdieTeam/mgs_reversing)** — the
MGS1 decompilation — pinned at `7964de7`. That project states no licence, which
is not a settled question. [`CREDITS.md`](CREDITS.md) says exactly what is used,
what is not, and what still needs asking.

## Quick start

Pick the one that matches what you're here for:

- **Just want to play it?** Read [`PACKAGE-README.txt`](PACKAGE-README.txt) — what
  the finished package contains, where it goes, and what it requires of
  MGSM2Fix. This repository does not distribute a built package (see
  [`CREDITS.md`](CREDITS.md) for why); building one is the next option.
- **Want to build it yourself?** Read [`BUILD-HISTORY.md`](BUILD-HISTORY.md) — the reproducible
  build and its five inputs (a Windows Master Collection install, four retail
  executables, a decomp checkout, a PSYQ SDK, and Python with Pillow/ninja) — then
  `py rebuild.py --output <dir>`. `py selftest.py` runs the data-free test suite
  first and takes a tenth of a second.
- **Want to understand or extend it?** [`SCRIPTS.md`](SCRIPTS.md) is a one-line-per-script
  index of everything in this directory. This README is the technical record —
  byte formats, mechanisms, every gotcha, indexed by the `##` headings below.
  [`HANDOFF-ARCHIVE.md`](HANDOFF-ARCHIVE.md) is the map: where things stand, what remains,
  and which decisions are the user's to make — **read it first if you are
  picking this up cold.**

This tooling is Windows-only (it locates the Steam install via the registry, and
its docs assume the `py` launcher and PowerShell/cmd throughout).

## What ships

| Patch | Contents |
|---|---|
| `en_items` | item and weapon descriptions, the frozen Ration/Ketchup pair, the HARD/EXTREME Mine Detector message; two code fixes (card level digit offset, SOCOM suppressor rewrite) |
| `en_menu`, `en_menu2` | menu strings |
| `en_option` | `screen brightness setup`, `key configuration setup`, `use directional buttons to test`; the brightness paragraph as USA's `sc_text` texture (four lines in the collection build); the eight KEY CONFIG label textures |
| `en_preope` | Previous Operations with USA's exact pagination: Metal Gear (13 pages, 7 lines each, last page 6), Metal Gear 2 (19 pages) — `preope_usa.py` |
| `en_brf` | briefing menu labels (20 PCX textures, quads, USA's row arithmetic and `above`) |
| `en_savemsg` | memory-card messages (`datasave.c` save/load caption tables in the executable) |
| `en_camsave` | the PHOTO ALBUM's own copy of those messages, inside the `camera` overlay |
| `en_abst` | the MISSION LOG: 122 pages in USA's two-screen layout with its page counter, arrows and EXIT, and the disc-change abstract's eight strings — `abst_build.py`, 2026-09-05 |

And on the **VR disc** (`mods/INTEGRAL/VR-DISK/`, ported 2026-09-06 and 07 from
USA's VR Missions `SLUS-00957`; see "The VR disc (SLPM-86249)"):

| Patch | Contents |
|---|---|
| `vr_en_missions` | 1808 of 1813 in-mission windows across 92 stages — titles, briefings, results, hints |
| `vr_en_items` | the VR executable's item, weapon and capture-mode pools |
| `vr_en_savemsg` | the VR executable's save and load messages |
| `vr_en_option` | the option screen's help lines and the whole KEY CONFIG screen (labels, geometry, retiled texture archive) |
| `vr_en_title` | the EXTRA menu's four help lines |
| `vr_en_camsave` | the PHOTOGRAPHING mode's memory-card messages |
| `vr_en_movie` | the MOVIE selection captions — all three clips, the two TGS ones as USA's two lines (a 16-word stub in the `movie` overlay lights both) — and the EXIT box at USA's y; built on `vr_en_missions`, which already owns the stage, and must load after it (2026-09-07) |

`en_menu3` is **raw-disc only** and must not be deployed in the collection: the
collection patches that same block itself and the two layouts do not mix, which
kills the title stage. Built and verified 2026-09-07; see "Why `en_menu3` is
raw-disc only" below.

## Standalone VR grenade delay correction

**Raw packages now include this fix (2026-09-12).** `rebuild.py --variant raw`
packages the English-layout texture/fuse PPF and computes its parity together
with the whole English set in `INTEGRAL_vr_zz_ecc.ppf`. Applying the complete
raw package therefore includes DELAY 4.0 and all five four-second briefings;
no separate addon or INI setting is needed. The standalone commands below
remain useful for Japanese images or separately assembled patch sets.
Both corrections in the raw English build were confirmed working in game by
the user on 2026-09-12 (`repro35raw`).

`py vr_grenade.py` builds the standalone grenade fix in `WORK`. The default
`INTEGRAL_vr_fix_grenade_delay.ppf` works on **original Japanese Integral**:
USA's `DELAY 4.0` texture plus the corrected briefing numeral. It is independent
of the English package and the MGSM2Fix upstream PR. Inputs are the collection's
Integral and USA VR containers, Python and these scripts; no decomp build or
extracted executable is needed.

**INI controls (2026-09-11):** deploy with `py vr_grenade.py --deploy` to
install both collection layouts and their `.ppf.json` companions. Ketchup chooses
Japanese or English at startup using `[Patches] IntegralVREnglishPatch` and the
presence of the mission PPF. `[Patches] GrenadeDelayFix = false` skips both
texture variants and restores the five English briefing digits to 5 while
loading the mission PPF. True uses the corrected decal and 4 in either language.
Changing the INI needs a game restart, **not a rebuild**. Rebuild/redeploy only
when the English mission PPF itself changes.

Companions pin the exact PPF with a 64-bit FNV-1a fingerprint (a stale-layout
check, not authentication) and the structurally located digit offsets. The
English addon also pins its mission base. Missing/stale mission metadata skips
the VR English group with an explanation in the log; a mismatched grenade
companion skips that addon. `vr_windows.py` and `rebuild.py` generate/package the
mission companion automatically. The raw PPFs remain separate, fixed disc-image
patches; an INI cannot control a patch already applied to a BIN.

The weapon-selection grenade uses `selectvr`'s second DAR, entry `4D80.p`:
200x125, 4bpp. USA's `DELAY 4.0` replaces Integral's inaccurate `DELAY 5.2`.
The complete source headers match, including VRAM (704, 372) and CLUT (992, 250).
USA's 10,568-byte PCX losslessly re-encodes to 10,453 bytes, fitting Integral's
10,512-byte slot with 59 zero bytes of padding. The rest of `selectvr` stays
byte-identical to Integral. **Confirmed good in game by the user, 2026-09-11.**

The WEAPON MODE / GRENADE LEVEL 02 briefing is duplicated in all five
`vr_grn01`-`vr_grn05` scripts. Its Japanese fuse digit is `80 35`; only `35`
changes to `34`, keeping the prefix, all Japanese glyphs, punctuation, target
count and script sizes intact. The English version likewise changes just the
`5` in `the goal! Grenades explode in 5 ` to `4`. The builder structurally
matches the window's titles and exact sentence, checks USA's corresponding
value is 4, and scans all 105 Integral stages to assert coverage.

| Stage | Japanese digit, stage offset | English digit, current stage offset |
|---|---|---|
| `vr_grn01` | `0x6CAA7` | `0x6CAC7` |
| `vr_grn02` | `0x705FB` | `0x7061B` |
| `vr_grn03` | `0x7AB1F` | `0x7AB3F` |
| `vr_grn04` | `0x7BF33` | `0x7BF53` |
| `vr_grn05` | `0x727C7` | `0x727E7` |

These offsets are evidence, not hardcoded patch addresses: the builder resolves
positions from parsed GCL each time. It verifies exactly one digit changes in
each Japanese script and that serialized PPF readback reconstructs the intended
stages. Tests also reject the wrong mission, language or unexpected numeral.

`vr_windows.py` calls the same `correct_briefing` helper after porting these
five stages. Its only effective changes from the previous deployed mission PPF
are those five `5` -> `4` bytes. This prevents it from overwriting the standalone
fix: the English addon explicitly owns the five corrected digits and both
patches agree in either order. Other numbers retain the existing substitution
policy. The loader's usual overlap audit remains active for other mods. It overrides
only the five validated mission digit writes, so the active grenade and mission
PPFs agree when enabled; when disabled, no grenade PPF is loaded.
The English package therefore also carries the corrected sentence when used
alone; the independent Japanese fix needs no English patch.

Each collection variant has 709 records / 9,509 payload bytes. Each `_raw.ppf`
variant adds EDC/ECC for all 11 touched sectors; all four PPFs carry Integral VR's
disc block check. Use the ordinary `_raw` version on a Japanese Integral VR BIN.
For English raw images, apply the matching English mission patch and its package
first, then `_english_raw` last: its sector checksums include that exact English
base. Use only collection variants in Ketchup. To remove the texture correction,
set `GrenadeDelayFix = false` and restart. This restores both the original
decal and numeral. The PPF assets can remain installed.

The briefing extension is statically verified in both languages and confirmed
in game with the English setup; Japanese visual confirmation is pending. The stage
unlock PPF is back in `mods`, gated by `UnlockVRMissions = false`. The interrupted session's `vab_grn`
cache-tail theory was incorrect: that tail is `scenerio.gcx`, not texture data.
See HISTORY sections 29-30 for the investigation and extension.

## Optional PPF controls in MGSM2Fix.ini

These seven options control installed PPF assets; they do not download or embed
game data. Restart the game after changing them. Keep the supplied filenames.

| Section | Setting | Default | Scope |
|---|---|---|---|
| Patches | `IntegralEnglishPatch` | true | All known main-game `en_*` families on both Integral discs |
| Patches | `IntegralVREnglishPatch` | true | All eight Integral VR English families |
| Patches | `GrenadeDelayFix` | true | Grenade decal and all five briefing copies; auto-selects Japanese/English |
| Game | `UnlockVRMissions` | false | Integral and USA VR mission unlock PPFs |
| Game | `UnlockVRExtras` | false | Integral VR EXTRA menu unlock PPF |
| Game | `UnlockVRMovies` | false | Integral VR movie unlock PPF |
| Game | `UnlockTitleBonuses` | false | Integral and USA title bonuses on both story discs |

`EnglishText` still selects Integral's own language bit; it is not the English
PPF master switch. `UnlockBriefing` remains a separate existing control. All
four new unlock settings are false in the installed INI. The eight unlock PPFs
(two Integral title, two USA title, two VR mission, extras and movies) are installed
but skipped until enabled; copies parked earlier remain available as backups.
Disabling an unlock does not erase progress already stored in a save.

If Integral's title-screen BRIEFING items remain unlocked with these four
controls off, check `[Game] UnlockBriefing` in the active game INI. Set it to
false and fully restart the game. On 2026-09-11 this separate setting was still
true from testing; turning it off fixed the menu, as confirmed by the user,
without editing a save. It is now false in the installed INI. A game started
while it was true can retain those flags in its save; the switch does not erase
saved progress.

`src/games/mgs1_patch_options.h` contains the exact filename/title/version/disc
mapping and the tested loading plan. Unknown patch families remain untouched.
The new config fields are read/logged by `m2config.cpp`; Ketchup applies the
selected files and any validated per-byte overrides. The two grenade language
PPFs are alternatives and may both be installed with this loader; raw grenade
variants are explicitly skipped in the collection.

## Scope: what this port changes, and what it deliberately keeps

**Verbatim USA text, placed where USA places it. Everything else stays as
Integral shipped it.** Integral is the later release and its non-text
differences may be deliberate; they are kept unless they are text, a
translation, or the positioning of text and its chrome (rules, connectors,
highlight boxes that frame the text). Decided 2026-09-02, after the unlocked
briefing comparison surfaced differences that are real but not text:

| Kept from Integral | What it is |
|---|---|
| "watched" dimming colour RGB (70, 80, 75) | USA dims flagged items, connectors and FILE boxes to (90, 105, 95); Integral's `brf_800C6634` uses (70, 80, 75), 30 immediates 1:1. Only visible once items are flagged. |
| `br_back_l` right-edge seam | 154 stencil-mask bits differ along texture x 155–159; every other briefing texture is pixel-identical (palettes differ only in unused entries). |
| vertical rule brightness, FILE-box interiors, a few collage rows | Integral's rule renders ~201 grey against USA's 166, box interiors 24–32 against 0. Not attributed; the background fade (`(frame-28)*8`, identical in both) is not the cause. Nothing the port touched. |
| 1P MODE's twenty-one explanation pages (Japanese) | An Integral-only mode: USA has no 1P MODE, and Integral ships no English for the pages, so there is nothing to port and no translation is made. See "1P MODE" under Unlocks. |
| KEY CONFIG's background contrast | Integral's `key_back_l`/`key_back_r` look higher-contrast than USA's, though the two textures are **colour-identical** — so it is not those two. Noticed by the user 2026-09-03, who decided to keep it. **Not attributed, and not confirmed by measurement**: the grey-ramp claim below looked just as solid and turned out to be an artefact of comparing 24-bit data on 5-bit hardware. Measure the rendered pixels before acting on this one. |
| KEY CONFIG's connector rules run longer at their far ends | Each label's rule extends further from the label in Integral (e.g. ~14 game px further left under `weapon`). The end that meets the *label* is in USA's place in both games — checked per label — so text placement is right and only the tail differs. Art, and stays. |

So "pixel-identical to USA" for this project means: every glyph and every
piece of chrome that positions text lands on the same pixels; brightness and
art that Integral changed on its own are not chased.

### Amendment, 2026-09-03: text may MOVE where Integral's art moved

The rule above assumes the art is common ground and only the text differs. It
is not always. The user's amendment, verbatim:

> "Where Integral's art/gui/hud/etc. is intentionally different, consider moving
> the English text to fix it, but ask me first."

So when Integral's own art is what differs, the target is **USA's relationship
between the text and that art**, not USA's absolute coordinates. Port the text,
keep Integral's art, and move the text so the relationship holds. This is a
deliberate deviation from USA's numbers, so it is **the user's call, case by
case — ask before doing it.**

| Deliberately NOT at USA's coordinates | Why, and who decided |
|---|---|
| `key_syukan` — KEY CONFIG's "first person view" (the △ button's label) — sits **+11 game px right** of USA's x | Integral's background art puts the connector curve coming off the △ button 11 px further right than USA's, because USA moved its curve left to clear the longer English label and Integral never had to. At USA's absolute x the label sat **on** Integral's curve. Measured on paired shots: the rule beneath the label starts at screen x 2343 in USA, 2442 here (99 px ÷ 8.92 px per game px = 11.1). After the shift the label is +18 screen px from its rule in **both** games — the gap is now identical, which is the point. **Asked and approved by the user, 2026-09-03**, and the case that produced the amendment above. Detail: "How the KEY CONFIG port was built". |

| the brightness paragraph's fifth and sixth lines — "Press the ○ button to return to the option screen." — are **blanked in the collection build** | The collection drops them from its own USA, because ○ is not the back button on every platform. Integral has no `sc_text` for those patches to replace, so our six-line Integral contradicted the collection's own USA two menus away. `SC_KEEP_LINES = 4` blanks the same two lines for the collection build; `= 6` is USA's own text and is what a raw PSX disc patch gets, where ○ really is the back button. **The user's call, stated 2026-09-03**: "in MC i prefer the circle message suppressed". Detail: "The collection shows only four of USA's six brightness lines". |

| the VR disc's MOVIE **EXIT box** sits **4 game px higher** than Integral shipped it (screen y 186, USA's own `sp_exit` y of +66 against Integral's +70) | USA draws each TGS caption as two lines and makes room for them two ways: a shorter caption face and a higher EXIT box. The face is Integral's own art and stays, so at USA's caption rows the first line's ink (game y 201-213) overlapped Integral's box border (201-202) by two rows. Moving the box is chrome that positions text, and it is one immediate at overlay `+F128`; the selection highlight follows on its own, because both the `cur_l` cursor and the lit state anchor on the same widget object at `work+0xF0`. **Asked and approved by the user, 2026-09-07**, who named the default it departs from in the same message: *"Usually we skew toward the Integral visuals."* Detail: "The MOVIE selection captions". |

The other three KEY CONFIG labels were checked the same way and need no shift:
each already sits USA's distance from its own rule. Only the rules' far ends run
longer in Integral, which is art, and stays.

### Amendment, 2026-09-07: Integral's own ENGLISH may be replaced, when asked

Both rules above were written about Japanese: port English where English
exists, and keep what Integral chose to do differently. Neither answers the case
where **Integral already has English and it is not USA's**. The user settled it
on the first one found, verbatim:

> "this is a case where we are replacing existing english in integral instead of
> japanese, so it's an exception to our rule"

So: an Integral string that is already English stays as it is **unless it is
asked for**, one at a time, the same way the art amendment works.

| Integral's own English replaced by USA's | Why, and who decided |
|---|---|
| the inventory side column's abbreviation for item 22: `SCARF` becomes `HANDKER` | The abbreviations are a separate string set from the descriptions - NUL-terminated names on an 8-byte stride in the executable, items 23 down to 0, already Latin on both discs - and comparing the two tables entry by entry, this was the **only** one that differed. Integral's own Japanese description calls the item 《ハンカチ》, *hankachi*, a handkerchief, so its own short label was the only thing calling it a scarf; the change makes Integral agree with itself as much as with USA. **Asked and approved by the user, 2026-09-07**, and the case that produced the amendment above. Detail: "The item short-name table" below |

Not done, and not asked: the `abst` location spellings (`Tank Hanger`,
`Medi rm`, `Cmnder rm` against USA's `Hangar`, `Medi room`, `Cmnder room`) are
the same shape of question and are still open in `HANDOFF-ARCHIVE.md` §5.9.

### Audit against the rule (2026-09-02)

Every deployed change was re-checked against the scope rule. Findings:

- **Record 6 of the option chain (the EXIT row's help line, タイトル画面に
  戻ります。) had two corrupted bytes** — an `'e'` and a space from an English
  string, written at the wrong offset by an early font-text build and carried
  forward by the pinned chain input. On screen it read タ¥トル¨面に戻ります。.
  A text bug of the port. `optsctext.py` now restores every record the port
  does not own from retail and asserts it (`PORTED_RECORDS`).
- **Record 7, the colon Integral's Japanese help lines use (字幕設定：オン,
  サウンド設定：ステレオ …), had been blanked** to avoid an overlap on the
  vibration-test row. That removed Japanese text with no English counterpart —
  against the rule. Restored; the overlap is prevented instead by entry 27 no
  longer being lit in that state, and record 27's padding absorbs the byte.
- **Record 3, the vibration-test row's own label (振動テスト, lit with the
  sentence in state 5), stays blank — decided 2026-09-02.** Integral's line was
  振動テスト：方向キー左右で振動します。, i.e. the row's name plus a sentence
  that USA's `use directional buttons to test` already covers; the label adds
  nothing the English does not say, so USA's line alone, centred, is kept.
- Everything else is text, or the geometry that positions text: labels, rows,
  rules, connectors, highlight boxes, paragraph texture, pagination. Nothing
  changed a colour, blend or background texture.

Verified in game 2026-09-02 on all eight option rows: the four Japanese help
lines read 字幕設定：オン, 字幕言語設定：英語, サウンド設定：ステレオ, 振動設定：オン
with their colons; EXIT reads タイトル画面に戻ります。 cleanly; the three English
lines are unchanged.

## Gotchas

Everything below cost at least one broken build. Sections further down have the
detail; this is the index.

### Diagnosing a freeze or crash — do these two things first

1. **Compare the rebuilt overlay's size with retail's, and check whether the
   diff touches `f924`.** Size is cheap insurance; `f924` is the mechanism.
   Growing `f924` past retail's 8 is a genuine freeze (see Overlays below).
2. **Bisect against a stock run before theorising.** Move the PPF aside and
   confirm the fault is even ours; then split it (overlay vs chain, then edit
   by edit). Every mechanism I reasoned my way to without doing this — an
   uninitialised array, the font atlas, a struct overrun — was wrong.
3. **A freeze leaves no trace in `MGSM2Fix.log`.** The last lines are always
   the periodic `check_write_mgs_savedata`, identical to a clean exit, so the
   log cannot tell you a freeze happened or where.
4. Symptoms mislead about *locality*: the failing option build froze only
   the EXIT row; overlay size and struct layout had both changed.

### Overlays

- **Keep an overlay at or under retail's byte count** — but as insurance, not
  because it is the mechanism. The `+108` / `+32` / `+0` bisection that produced
  this rule was **confounded**: the commit that reached +0 also reverted
  `f924[12]` to `[8]`, changing the retail aliasing described below. Overlays load into the BSS
  tail at `StageCharacterEntries` with ~118 KB of headroom (the largest stage
  overlay on the disc is 143,856 bytes against option's 25,842), so +32 bytes
  cannot plausibly overflow it. The padded sector slot (26,624) is not a limit
  either.
- **Do not "fix" retail's quirks — `f924` above all.** It is indexed
  `f924[work->f920]` and the cursor states run to 11, so states 8..11 read and
  write `kcb[0]`'s first bytes. Retail depends on what it finds there: enlarging
  the array to 12 redirects those reads to freshly-zeroed slots and changes which
  branch each state takes, which is the EXIT-row hang. Leave it at 8.
  The retail key-config init likewise writes `f2AFC[7..16]` past a 4-entry array;
  that one *is* inert (it stores literal 0 into already-zeroed memory, and
  entering KEY CONFIG re-stamps all 17 of `f2B0C[0..16]`), so growing `f2AFC` is
  safe where growing `f924` is not. Know which is which before you touch either.
- A struct field change shifts every later offset, so the recompiled overlay
  differs from retail in ~20 KB of bytes. That is normal, not a red flag.
- `build/*.matching.bin` is byte-identical to retail, so **the decomp is
  trustworthy** — any misbehaviour in a rebuild comes from your own edits.
- `FAIL: does not match target hash` from `build.py` is *expected* for the
  overlays we deliberately modify (`option`, `preope`).
- Overlays carry their own entry address in their header; entry points do **not**
  need pinning.
- Work areas are `GV_NewActor(EXEC_LEVEL, sizeof(Work))`, so growing `Work` is
  safe for the allocation — but not for the size limit above.

### Font and text rendering

- **`c_width = (rect.w * 4) / 12` implies 12 px per character, but the ASCII
  font is proportional.** This one formula caused two separate broken builds:
  an unnecessary font-atlas repack, and lines wrapped at the wrong width.
  Compute real widths from `font.res` (`glyph_widths()` in `optbright.py`).
- **The wrap limit is `kcb->width - 12`, not `kcb->width`** —
  `font_print_string` subtracts 12 unless `FONT_NO_KINSOKU` is set, and opt.c
  passes flag 0. So 240 px at the default 21-character budget, not 252.
- **`max_width` is one byte**, loaded `lbu`, so 255 px is a hard ceiling. The
  decomp declares it `char`; PSY-Q's plain `char` is unsigned and retail emits
  `lbu`, which is why it works.
- **Wrapping is not cosmetic — it smashes the heap.** The continuation is drawn
  18 rows down inside a 20-row band, so it lands on the CLUT row at band+20
  (multicoloured pixel noise) and keeps writing ~1.4 KB past the
  `row * height + 32` buffer. Visible result: doubled overlapping lines and
  wrecked palettes; real result: a freeze on the next screen that allocates.
- **Never size lines off USA's art — and compare like with like.** The glyph
  widths are in fact *identical* between the builds (measured ink-to-ink on
  matching Previous Operations pages: 261 vs 261, 306 vs 306). An earlier note
  here claimed Integral's were wider, from comparing a *computed advance* against
  a *measured ink extent* — different quantities. Compute advances from
  `font.res` (`glyph_widths()` in `optbright.py`) or measure ink on both screens
  in one pass, never one against the other.
- Trailing-space padding widens `max_width` too, so **pad only the shortest
  line** when balancing chain lengths.
- Zenkaku (2-byte) glyphs cost a flat 12 px. `90 1B` is ○ and `90 18` is ✕,
  mixed inline with ASCII — precedent already shipping in `int1_en.exe`
  (`Press \x90\x1b to zoom in,`).
- `font.res` is not addressable by tag id — it lives in the `init` stage and is
  found by signature (`>II 392, 2306`). Table 1 is 96 big-endian words for
  ASCII 32..127; width is bits 27:24.
- Text visibility is the font **CLUT colour**, and all 31 entries are reset to 0
  before the per-state switch — so lighting an entry in a new state is safe, and
  blanking one is just omission.
- All entries are printed once at init and drawn every frame; their positions
  are read from `dword_800C3218` at print time.
- For these option entries the ink lands at exactly the table's x and
  **table y + 16**. Calibrating that offset against *kanji* gave +15 and put
  everything 1 px low.

### The GCL text chain

- Records are `07 <len> <payload> 00`, starting at script+0x1B8.
- **Fix container sizes with `gclparse.containers_over`**, not the hardcoded
  offsets an older note lists — it finds every enclosing size field.
- **Keeping the chain delta at exactly zero avoids container arithmetic
  entirely.** Prefer it: a −8,055-byte shift crashed the script at `set map`
  with no exception, and every edit that has ever worked moved things by at
  most +780.
- Empty records (payload = a single NUL) render zero-width and are harmless —
  USA ships 24 of them.
- `GCL_GetString` returns a pointer *into* the chain; the text is read in place,
  so there is no separate string table to update.
- **A command's value list ends with a GCL_END byte, and `GCL_GetOption` stops
  only there.** A scan for a letter the command lacks steps over every option
  by its length byte — including `i`'s overflowed one — and walks into the
  text, printing `GCL:WRONG CODE` until it meets a NUL. Read only options every
  block is known to carry (the abst port does not read USA's `d`).
- **In a stage's cache section the tag sizes are offsets**, not lengths
  (`LoadCacheSection`): five files share the `c?` payload, and the last file's
  offset and the 0xFF fake tag's total must move when an earlier file grows.
  `abst_build.py` shows the arithmetic; `portio.pack_stage` fixes only the 0xFF
  size.

### Textures and quads

**EVERY DAR ENTRY'S SIZE MUST BE A MULTIPLE OF 4.** An entry header is
`{u16 id, s16 ext, u32 size}` sitting immediately after the previous payload, so
an odd size leaves the *next* header — and its `u32` — misaligned, and the
loader reads it off-alignment. Retail holds the invariant everywhere it was
checked: Integral's main `option` DAR (51 entries), USA's (57), Integral's VR
`option` (51), USA's VR (57) — **zero** unaligned sizes in all four. The VR
option port's lossless PCX re-encode left **39 sizes odd and 41 of 51 entry
starts misaligned**, and the stage crashed the emulated CPU the moment it
loaded (2026-09-06):

    > load option
    r3000: illegal instruction 0xffffffff (pc=0x10600007)

`pc` and `r2` both holding `0x10600007` — a `beq` instruction word used as an
address — is the signature: the walk derailed, a texture pointer came back as
code, and something jumped through it. **Pad every payload to 4 and put the pad
inside `size`**; `PcxInflate4`/`PcxInflate8` stop on their own byte count, so
the pad is never decoded. Add it to any DAR check: walk it the loader's way and
assert both that the walk lands on exactly 0 *and* that no size is odd.

**`pcx4`'s run cap was 62; the format allows 63.** `code = PCX_RLE_CODE + run`
with `PCX_RLE_CODE = 0xC0`, so `0xFF` is a 63-byte run and both inflaters decode
it. The encoder stopped at 62, which costs a byte on every long run. `_rle`,
`encode` and `encode8` now take `maxrun`, **defaulting to 62** so every
already-shipped patch still builds byte-for-byte identical (verified: the main
game's `en_option` rebuilds identical on both discs). The VR option builder
passes 63, which is what paid for its 82 bytes of alignment padding inside a
DAR slot with only 78 bytes spare.

**`PcxInflate4` decodes each row into a 128-byte static buffer**
(`pcx_temp_buf`, `libdg/loader.c`) with no bound check, so `stride * 4` must be
≤ 128 and a run must not overshoot the row. 8bpp goes through `PcxInflate8`
instead — one continuous stream of `w * h` into `tex->data`, no per-row buffer —
so do not apply the 4-plane rule to it. Checked against retail and USA as
controls, all four DARs are clean on both counts; this was investigated as a
crash cause and ruled out, and the controls are the reason it could be ruled
out quickly.


- **The texture is stretched to a hardcoded quad**, and `SetPacketTexture`'s UVs
  span the whole texture, so changing texture size alone does nothing. The
  selection highlight follows the same quad.
- A label's height is already in the poly: `v2 - v0`.
- `off_x + w` and `off_y + h` must both be ≤ 255 (`u_char` UVs); overflow shows
  as vertical striping.
- PCX here is 4-plane 1bpp with row-wide RLE, `PCXINFO` at offset 74 (stamp
  12345), width a multiple of 4, CLUTs on a 16-unit stride.
- Many polys have **two writers** — a settled layout block and an animated
  reveal dispatched from a jump table — and which one wins varies per submenu.
- To find version-exclusive art, **diff the two builds' DARs**: that is how
  USA's `sc_text` turned up as the only texture Integral's option stage lacks.
- **A quad constant is settled only by a like-for-like screenshot after
  deploying — not by copying USA's, not by deriving it.** USA draws `sc_text` at
  `y0 = 2`; the same constant here landed 12 rows high. The measurement-derived
  `y0 = 14` was right and an adversarial review talked me out of it. The two
  builds' draw environments differ in a way neither the decomp nor USA's binary
  shows. Deploy, shoot both, diff.
- **Pin every build input; a step that reads "whatever is deployed" will one day
  read its own output.** Moving that output aside is not a fix — it drops
  whatever the *previous* deployed file contributed. And re-run the static
  checks on the build you actually deploy, not the one before it.

### Reading disassembly

**USA's `title` overlay: the true address is the label minus 8** (memory-only
until 2026-09-04). The header's entry word, the `printf` references and every
`j` target agree on it; USA's `brf` overlay did **not** have this offset, so
check per overlay. Take file offsets from the labels and encode jump targets
from the true addresses. Found while building `unlock_title.py`.

**`stage_lookup` (`0x80022DCC`) has no bounds check** — it returns
`entry_offset + stagedir_base_lba`; the caller reads one sector and the
callback at `0x80023274` takes the sector count from `lh [header+2]`. That is
why a stage may live anywhere past STAGE.DIR with its length coming only from
its own header — the mechanism the DUMMY3M relocation of `preope`, `brf` and
`option` relies on. `DUMMY3M.DAT` (13,501 zeroed sectors) is absent from the
executable's file-name table and is never opened by the game.

- USA's brf/option overlay base is **0x800C5970**, Integral's **0x800C3208**.
  Match functions by call count, never by address.
- A `jal` takes arguments from its **delay slot** too; scanning only backwards
  mis-attributes them by one call.
- **Never write `$zero`** in a register simulator — MIPS discards it, and a
  simulator that doesn't corrupts everything downstream.
- A linear simulation cannot be trusted on a **conditionally-assigned
  register**; look for every write before believing one.
- Resource ids are `GV_StrCode` hashes computed at runtime from strings in
  `.rodata`, so they are **not** immediates in the code — grep the overlay for
  the name instead.
- **When a function indexes a global, find the same pattern in the decomp before
  naming the global.** A `lw` from `0x800A9580`, scaled by 4 into a table, was
  written down as `captions[clip]` in the VR `movie` overlay; `abst_sprt` in the
  decomp indexes the same way with **`GV_Clock`** — frame parity — and the table
  is the per-frame ordering table. That one name made a "one line per clip" rule
  look structural in the drawing code and cost two builds and two play tests
  (README "The MOVIE selection captions"). The decomp need not contain the
  *function* to name what it touches: an undecompiled actor is often the engine's
  generic module under another name, and here it was the mission log's own text
  engine, `abst.c`, linked into the overlay twice.
- **An overlay has room for a small stub without growing the stage.** Payloads
  are padded to a sector by `pack_stage`, so the last one's slack (72 bytes in
  the VR `movie` overlay's 60 sectors) takes a patch function with no payload
  moving, no sector-count change and no risk to the collection's own patches.
  Prove the RAM is overlay space the same way: every stage overlay in a game
  loads at one base, so the *largest* overlay's end is the ceiling — `init`'s
  `0x800EA1EC` on the VR disc, against the `movie` overlay's `0x800DF158`.
- **A one-word retarget beats an in-place rewrite.** Where USA needs more
  instructions than the Integral block has room for, leave the block's shape
  alone, point its `jal` at a stub, and let the stub call retail's own helpers.
  Remember `a0`-`a3` and `ra` do not survive a call, and that R3000 has no load
  interlock: a `lw` result is not readable in the next instruction, and `jr ra`
  needs a slot after `lw ra`.

### Measuring from screenshots

- Scale is **9 display px per game px** (2160/240) with a 480 px x offset.
  Verify it against something with a known texture row — the brightness screen's
  green line is `sc_back_l` row 92, i.e. game y 100.
- **Measure against the user's shots of both games, per submenu, before
  concluding anything is exact.** The briefing rows were believed within 1 px;
  against the user's USA shots they were 4 / 1 / 12 rows off per submenu, and
  the shots also showed the "highlight" function being patched was a different
  feature entirely. Use a fixed element (the FILE column) as the framing
  control, exclude the rule from the label band, and compare highlighted to
  highlighted.
- Measure **ink centroids, not band tops**: a highlighted row's glow moves a
  threshold's idea of where ink starts by 2–3 px, the same size as the effects
  being chased.

### PPFs and deployment

**Executable PPFs: never let a record cross a 2048-byte payload boundary
(memory-only until 2026-09-04).** Ketchup mirrors executable writes into RAM
per byte with `pos = k % 2352; skip if pos >= 2048`, so the part of a record
that spills into a sector's 304-byte tail is silently lost while the log still
says "loaded" — `en_savemsg` lost 142 of 442 bytes that way on 2026-09-02.
Split runs at payload boundaries and replay Ketchup's rule as an assert
(`savemsg.py` does). Check: the log's "Applied N bytes of RAM patches" must
equal `en_items`' 3,224 plus `en_savemsg`'s 531 (3,755 since 2026-09-05; it
was 3,068 + 442 while only differing runs were emitted). Image offset of an executable file offset
`fo`: `base + ((fo - 0x800) // 2048) * 2352 + (fo - 0x800) % 2048`, where
`base` is Ketchup's `ram_base` — Integral disc 1 **`0x131D2238`**, disc 2
**`0x0EB38078`**. The inverse (image → RAM) is
`0x80010000 + (img - ram_base) // 0x930 * 0x800 + (img - ram_base) % 0x930`;
an earlier attempt subtracted the exe's 0x800 header a second time and put
everything one sector low.

- **A PPF record aimed at the executable must not cross a 2048-byte payload
  boundary.** Ketchup mirrors executable writes into RAM byte by byte with
  `sector = k / 2352; pos = k % 2352; if (pos >= 2048) skip`, so a record that
  runs from one sector's payload into the next spills its remainder into the
  304-byte tail and those bytes never reach RAM — while the log still says
  "loaded". `savemsg.py` splits every run at payload boundaries and replays
  Ketchup's rule as an assert; the first build lost 142 of 442 bytes this way.
  Check any executable patch with the replay: mirrored bytes must equal written.

- Ketchup loads every PPF in `mods/INTEGRAL/INTEGRAL/{0,1}`; the log line
  `[Ketchup] base path is mods\INTEGRAL\INTEGRAL\0` confirms it picked them up.
  Keep patches in separate files so they can be disabled individually.
- Our PPFs carry **no undo data** and no Integral disc image is on disk, so
  `ppfgen.py`'s LBA lookup is unusable. Recover the mapping from a deployed PPF
  and prove it on every record (`optbright.py` does this). Disc 1 is LBA
  136654, disc 2 is 105178, both mode 2 form 1 (24-byte sector header).
- A PPF record's length is **one byte**, so split runs at 255 bytes *and* at
  2048-byte sector boundaries.
- Regenerating a PPF in place destroys the reference you were verifying
  against — save a baseline copy first; it is also your revert path.

### Toolchain and environment

**Where the real USA discs are, and why `work/us1_stage.dir` must not be used
for text** (memory-only until 2026-09-04). `us1_stage.dir` is the **European**
release despite its name — found 2026-09-01 when the shipped Previous
Operations text disagreed with the user's USA screenshots ("mercenary who was
feared" vs the file's "mercenary. He was feared"); those strings exist only in
`windata/dlc/dlc_europe.bin`. `work/us1.exe` *is* genuinely USA (SLUS ids at
0x800, "for North America area", 651,264 bytes = `SLUS_005.94`), so the exe and
the stage file were taken from different discs. The collection's
`windata/alldata.bin` holds three PSX images as raw 2352-byte sectors:

| image base | boot file | what |
|---|---|---|
| `0xD39B7000` | `SLUS_009.57` | VR Missions (US) |
| `0xF12F8000` | `SLUS_005.94` | **MGS1 USA disc 1** |
| `0x11B3E5800` | `SLUS_007.76` | **MGS1 USA disc 2** |

Find them by scanning for `\x01CD001\x01` (the PVD at sector 16; base = hit −
24 − 16 × 2352), then read the root directory at PVD+156 and `/MGS` for
`STAGE.DIR`. Extracted: `work/usa1_stage.dir` (71,892,992 bytes) and
`work/usa2_stage.dir` (71,888,896). European and Japanese releases are in
`dlc_europe.bin` / `dlc_japan.bin`. How much the mis-sourcing cost: `option`
DAR and chain byte-identical between EU and USA, `brf` DAR identical, only
`preope`'s chain differs (re-sourced); the `brf` *overlay* differs in 3,200
bytes of pointers, and the briefing layout constants were read from the EU copy
— they measured pixel-perfect against USA shots. **Rechecked 2026-09-04:**
USA's base and both sampled functions are eight bytes below the EU addresses;
all 16 row-call argument tuples and all 53 quad-call argument tuples match.
`brf_widen.py` and `brf_build.py` now use `usa1_stage.dir` and the USA addresses.

**Reading and poking live RAM through the Squirrel debugger** (memory-only
until 2026-09-04): enable `[Squirrel Debugger]` in the ini, run `bridge.py`
**before** launching (the game blocks until a client connects), then drop `.sq`
files into `sqcmd/` that call `g_emu_task.getRamValue(8|16|32, offset)` /
`setRamValue(...)`. **Offsets are RAM offsets** (`0xB3CC8`), not KSEG0
addresses (`0x800B3CC8`): memory defines log as `0xb4d9d`, Ketchup's
`PSX_ImageBase` is `0x10000`. A poke with the `0x800B…` form still logs
"Unlocked" because the read-back uses the same wrong convention — verify the
convention, not the log line. Symbol lookups: the pristine exe's map at
`D:\mgsbuild\integral-english-work\map_pristine.map` (rescued from the session
scratchpad 2026-09-04), overlay maps at `D:/mgsbuild/d/obj/asm_<stage>_lhs.map`.

**Where the working data lives — and why that changed 2026-09-03.** Every tool
here reads `work/…` relative to the current directory: the extracted
`int1_stage.dir` / `int2_stage.dir` / `usa1_stage.dir` / `usa2_stage.dir`, both
games' executables, (formerly the pinned `fonttext_disc*_option.ppf` chain input, gone since
2026-09-04), every
revert PPF, and the parked unlock PPFs. Until tonight all of it sat in the
session scratchpad under `%LOCALAPPDATA%\Temp\claude\…` — a directory that is
session-specific and lives under Windows Temp. Three gigabytes of re-extractable
data plus every backup, one Disk Cleanup away from gone, and unreachable from
any future session's scratchpad path.

It now lives at **`D:\mgsbuild\integral-english-work\`**: `work\`,
`unlocks_parked\`, `keyconfig_test\`, and the ini,
log and `opt.c` snapshots. The scratchpad copy is left in place for this
session only.

**The tools find it themselves (2026-09-04).** Every tool imports `WORK` from
`workdir.py` instead of opening `'work/…'` relative to the current directory, so
they run from anywhere. Resolution, first match wins: the
`INTEGRAL_ENGLISH_WORK` environment variable naming the root that holds
`work\`; then `D:\mgsbuild\integral-english-work`; then the current
directory, which keeps the old `cd <root>` convention working. `py workdir.py`
prints what it resolved to. Anything a tool writes goes to `WORK` as well.

**Verifiers, all of which read the deployed artefacts rather than the build:**

| tool | proves |
|---|---|
| `ppfcheck.py --deployed` | every PPF under the mods folder parses the way Ketchup parses it |
| `verify_integral_option.py` | the deployed Integral option PPFs rebuild to a stage whose `sc_text` has four lines at row 0 with the seam filler intact, on both discs |
| `verify_usa_brightness.py` | the 426 bytes in the deployed `.asi`, at the offsets in its table, splice onto the collection's own USA data into that same texture, on both discs |
| `shotcmp_brightness.py A.jpg [B.jpg]` | a screenshot's brightness paragraph position in line-heights below the green line, the notch test, and a per-band pixel diff between two shots |

- Build with `PSYQ_SDK=D:/mgsbuild/psyq` from `D:/mgsbuild/d/build`
  (`py build.py`). The SDK path default in `build.py` is wrong for this machine.
- `obj/option.bin` is the modified overlay; `build/option.matching.bin` is the
  pristine one.
- **Do not author Python through shell heredocs.** Escapes get eaten — `\x00`
  became a literal NUL byte in a source file twice, and a `str.replace` silently
  matched nothing. Write files with an editor tool. **It happened a third time
  on 2026-09-05** (`abst_build.py`): even a doubled backslash inside a quoted
  heredoc loses a level. Author source with the editor tool, and repair a NUL
  with a script that was itself written by the editor tool.
- The console is cp1252: printing a 0x90 byte raises `UnicodeEncodeError`.
  Escape non-ASCII before printing.
- `MGSM2Fix.log` contains `exceptions are enabled` for every script VM it hooks,
  so a log filter must not grep for `exception`.

## Building

**Current workflow (2026-09-04): [BUILD-HISTORY.md](BUILD-HISTORY.md).** `rebuild.py`
extracts retail inputs, exports the pinned decomp base, applies our source patch,
compiles the two overlays, and builds all eight shipped patch families into an
isolated directory. `preope_usa.py` now builds directly from retail and stages
its PPFs; `--deploy` is explicit. `optsctext.py` uses `optlabel2.py` to rebuild
its caption chain and no longer needs a pinned font-text PPF or deployed inputs.
The older experiments below are historical, not prerequisites for this build.

Needs the decompilation at `D:\mgsbuild\d` (branch `integral-english-text`,
see `decomp-overlay-changes.patch`). The `discs/` images were only ever for the
legacy `reloc_ppf.py`; `portio.relocation` reads the collection's containers.

    cd D:/mgsbuild/d/build
    py build.py --psyq_path D:/mgsbuild/psyq --variant main_exe
    ninja -f build.ninja ../obj/preope.bin ../obj/option.bin

    py preope_usa.py           # retail inputs, USA's exact 13/19 pagination;
                               # stages into WORK; --deploy is explicit

Removed 2026-09-04 as dead: `preope_ppf.py` (it packed the 12-page base stage,
so running it would have shipped the superseded layout over the live one),
`preope_mg1.py`/`preope_mg2.py` (single-recap re-wrap experiments from 08-29
that nothing reads) and `mg2_recap.inc` (referenced by nothing). `git log` has
them.

    py brf_widen.py            # VRAM placement for the briefing labels
    py brf_build.py            # quad patches + texture swap
    py reloc_ppf.py brf work/brf_en.bin 128 discs/int1.bin out/INTEGRAL_disc1_en_brf.ppf

**Check the overlay entry point after touching `preope.c`:**

    grep -m1 "  NewPreviousOperation" obj/asm_preope_lhs.map    # 800C4DA4

`preope_reserved()` in `preope.c` is padding that holds it there. **It is
probably unnecessary** — see the audit below. It is kept only because preope
took ~30 build/test cycles to get working and the change has not been tested.
Padding cost with this psyq compiler: 12 bytes overhead plus 8 per
`volatile_global = n + k;` statement, or 12 for a wide constant, giving 4-byte
granularity.

## The three limits

Each of these produces the *same* symptom — a black text area with the page
counter still drawn — which is why they took so long to separate.

1. **Both recaps' text in the GCL chain fails somewhere between 9,567 and
   10,304 bytes.** Both together need 10,304; each alone needs ~9,5xx. Proven
   by padding the chain with records past the last one the game reads, leaving
   the read text known-good: still black. So it is size, not content or record
   count. The *mechanism* is not a parser limit — see the audit below.
2. **The overlay loads at a fixed address and must stay within retail's 13
   sectors** (24,911 bytes, region ends 0x800C9358). Growing it to 31,500 to
   hold text in `.rodata` broke *both* recaps, including the one whose chain was
   untouched.
3. **The script chunk itself grows freely** — 14 sectors in use. Verified by
   declaring it 2,048 bytes longer with the chain unchanged.

Hence the design: Metal Gear in the chain, Metal Gear 2 in the script chunk past
the end of the script, located by pointer from `field_394[0].string`.

The current `MG2_RECAP_OFFSET` in `preope.c` is **22042**, measured from the
first chain record at script `+0x1B8` to the appended MG2 text at `+22482`.
`preope_usa.py` asserts it. The earlier `22029`/12-page value belonged to the
superseded wrapped-text build; current MG1 uses USA's 13-page pagination.
Changing the chain layout requires updating both builder and overlay.

## Things that do not work

- **Text in the overlay's `.rodata`** — breaks limit 2.
- **Blanking the now-unread Japanese recap records** to reclaim chain space — an
  8,055-byte negative shift crashed the script at `set map` with no exception.
  Every chain edit that has worked moved things by at most +780.
- **Stage relocation into DUMMY3M.DAT does work** — verified by relocating a
  working build byte-for-byte unchanged. Used for preope (90 sectors) and brf
  (139 sectors).
- **`-O1` on the recap files makes the overlay bigger**, not smaller.

## Wrap width

`c_width = (rect.w * 4) / 12` suggests a 42-character limit, but that assumes
12-pixel glyphs; the font is proportional. Compute real widths from `font.res`
(`glyph_widths()` in `optbright.py`) — see the Gotchas.

Previous Operations no longer wraps at all: it uses USA's own line breaks
verbatim (to 54 characters), because the recap draws each line as two sprites
totalling 384 px. The option screen is the tight one at 240 px, which is why
the brightness paragraph is re-wrapped to 36.

## The KEY CONFIG screen (ported 2026-09-03)

Was "not ported yet" until 2026-09-03. **The screen IS visible in the collection**, so porting it was never invisible
after all — before this it was the one screen still fully Japanese in a build
that is otherwise English.

**Settled 2026-09-03: interception is per version, and Integral is not one of
them.** With `DisableRAM`/`DisableCDROM` both `false` - every collection patch
active - **MGS1 (USA)'s KEY CONFIG is intercepted**: the collection draws its own
"Control Settings" panel (First Person View Mode, Controller Response Speed,
Controller Settings, Keyboard Settings) over the game's screen. Integral's was
not — **because this port broke it. Found and fixed 2026-09-03; see "The
collection's KEY CONFIG interception" below for the mechanism.** Three
attributions were wrong before the right one, so the record of what was ruled
out is kept as well.

**It is not a CD-ROM patch.** MGSM2Fix's `Ketchup` title table gives the ids:
**99** is `INTEGRAL`, **980** is `MGS1_JP`, **981** is `MGS1_US`, 982-986 the
European localisations. The collection prints its whole per-title patch
candidate list on every boot (`conv_checked_path()`), and across every saved
`MGSM2Fix.log*` generation:

| title | candidates | keyconfig patch |
|---|---|---|
| 99 `INTEGRAL` | 94-98 | none |
| 980 `MGS1_JP` | 78-84 | `test_keyconfig_disc1.bin` |
| 981 `MGS1_US` | 97-107 | none |

So the one keyconfig patch in the whole set belongs to the **Japanese** MGS1,
not USA. USA has none, and its KEY CONFIG is intercepted anyway. Stronger still:
map every USA candidate's name (the collection names each patch after its own
disc-image offset) onto the option stage's image span — sectors 27023..27103,
`0x16577868..0x165A6098` on disc 1 — and **exactly four of the 107 fall inside
it**, the four `sc_text` pieces below. Nothing patches USA's option *overlay* at
all, so no patch is redirecting that menu.

**It never was Integral, either.** The user remembered Integral intercepting it
before this port began, which would have meant our rebuilt option overlay broke
it. It did not: the logs go back to 2026-08-29, before any of this work, and
Integral is offered no keyconfig patch in any of them. The overlay rebuild
cannot have broken a patch the collection never asks for.

**Two withdrawn attributions, and the lesson.** First: "`test_keyconfig_disc1`
is why USA intercepts it" — wrong, that patch is not in USA's list. Second:
"it sits under title 980's directory, and USA loads 980/981" — also wrong, 980
is the Japanese MGS1 and USA is 981 alone. Both came from grepping patch names
across *all* logs at once and reading a title id as a version I had guessed at.
**Attribute a patch to a title by the title's own id, and get the id from
`MGS1_Ketchup` in `src/mgs1.h`, not from the order things appear in a log.**
What remains true is only what was measured: USA's KEY CONFIG is intercepted,
Integral's is not, and no CD-ROM patch explains it.

So porting this screen is invisible to a stock USA player and fully visible to a
stock Integral player - which is the version this port targets, so the work
counts. It is also what a raw PSX disc patch would need for either game.

The reference shots this was settled from - both games unintercepted, and
every label's art from both discs rendered side by side - are **not in the
repository**:
they are photographs and renderings of the games, and no game data is committed
here (`CREDITS.md`). Reproduce them from a local install: run each game with
MGSM2Fix's own patches filtered off to get the unintercepted screen, and dump
the eight textures with `kcquads.py`, which writes their quads and locations.
The mapping table below is the finding, and the finding is what ships.

The collection's button involvement elsewhere is real but separate: its Squirrel
`_update_option_button_setting` (`play_standalone_mgs.nut:844`) rewrites the
whole `GM_Configuration` word (`0xB4D9C`) every frame with its own button-type
bits — see the language-bit race under Unlocks.

**Label mapping, confirmed from the rendered art — the two `sykan` names are
swapped from what you would guess:**

| texture | Integral | USA | where it is drawn |
|---|---|---|---|
| `key_button` | ボタンタイプ | `button type` | top row selector, in a hexagon |
| `key_sykan` | シュカンモード | `first person view` | **bottom row selector**, in a hexagon |
| `key_syukan` | シュカンボタン | `first person view` | the △ button's label, top right |
| `key_buki` | ブキボタン | `weapon` | □ or ○, side depends on button type |
| `key_action` | アクションボタン | `action` | ○ or □, side depends on button type |
| `key_hohuku` | ホフクボタン | `crawl` | ✕, left |
| `key_normal` | NORMAL | `normal` | first-person mode value |
| `key_reverse` | REVERSE | `reverse` | first-person mode value |

`key_sykan` is the MODE row and `key_syukan` is the BUTTON label, despite the
names reading the other way round; both say `first person view` in USA, at
different sizes (112x13 in the hexagon, 88x10 as the button label), so swapping
them would look almost right and be wrong.

**Three things the shots settle beyond the label art:**

- **The labels change sides with the button type.** Type A draws `weapon` on the
  □ (left) and `action` on the ○ (right); type C swaps them. The same texture is
  therefore drawn at more than one position, so a port must place every quad for
  each type, not once.
- **`normal` / `reverse` are already Latin in Integral but in a bold all-caps
  face** (`NORMAL`, `REVERSE`), against USA's lowercase. Porting changes the
  style, which is a visible change with no text change — allowed under the scope
  rule, since it is USA's own art for the same words.
- **Integral has a bottom help line that USA has none of at all**, one per
  selected row: `ボタン設定 ： タイプA`, `主観モード時の操作 ： 通常操作`,
  `オプション画面に戻ります。` These are the same family as the option screen's
  Japanese help lines (the colon of record 7) and **stay Japanese** — Integral's
  own additions with no USA counterpart, exactly like 完了しました / 中です.

**How it was done.** `kcquads.py` reads USA's own quads out of its overlay,
`kcplace.py` allocates VRAM and CLUT slots, and `optsctext.py` swaps the art in
(the option stage is built by one script, so KEY CONFIG rides the same build).
See "How the KEY CONFIG port was built" below. The table is kept because it
records USA's VRAM layout, which the port does NOT reuse.

Eight `option` DAR textures need USA's art (all eight sizes below re-measured
from both discs 2026-09-03 and unchanged). All are 4bpp; sizes differ because
the Japanese and English labels differ, so this is the same job as the 20
briefing labels (swap the art, place it in VRAM, keep the quad equal to the
texture — see the briefing section):

| texture | Integral | USA | USA VRAM | USA CLUT | colours | reads |
|---------|----------|-----|----------|----------|---------|-------|
| `key_button`  | 88x12  | 88x13  | (128,492) | (1008,234) | 15 | ボタンタイプ → button type |
| `key_sykan`   | 88x12  | 112x13 | (80,480)  | (928,234)  | 16 | シュカンモード → first person view |
| `key_syukan`  | 60x7   | 88x10  | (208,480) | (960,234)  | 16 | シュカンボタン → first person view (the △ label) |
| `key_normal`  | 52x6   | 40x10  | (175,492) | (992,235)  | 11 | NORMAL → normal |
| `key_reverse` | 64x6   | 44x6   | (11,504)  | (768,236)  | 10 | REVERSE → reverse |
| `key_action`  | 64x7   | 32x8   | (175,502) | (976,235)  | 11 | アクションボタン → action |
| `key_buki`    | 44x7   | 44x7   | (0,504)   | (896,235)  | 12 | ブキボタン → weapon (same size, 287 px differ) |
| `key_hohuku`  | 52x7   | 28x8   | (120,256) | (1008,235) | 10 | ホフクボタン → crawl |

Everything else on that screen is already pixel-identical: `key_option`,
`key_symbol`, `key_a`, `key_b`, `key_c` byte-for-byte, and `key_back_l`,
`key_back_r`, `key_pad` identical once rendered (their indices differ, their
colours do not).

Note `key_normal` and `key_reverse` are already Latin in Integral, but in a
bolder all-caps face; porting them changes the style to USA's lowercase.

## The collection's KEY CONFIG interception, and how the port broke it

Settled 2026-09-03, after the user relayed a claim from the MGSM2Fix Discord
that Integral's KEY CONFIG *should* be intercepted and was not because
`DisableRAM`/`DisableCDROM` were set. The flags were not set — that session's
log records both `false` — but the claim was right that it should be
intercepted, and the cause was ours.

### What it actually is: a doorbell at `0x80200000`

The collection patches the option stage's overlay at retail `0x800C550C`,
replacing six instructions with:

    lui   v1, 0x8020        ; 0x80200000
    lui   v0, 0x0000
    addiu v0, v0, 1         ; = 1
    sw    v0, 0(v1)         ; ring
    j     0x800C5684        ; option_800C5150's epilogue, 20 bytes before the
    nop                     ;   next function

`0x80200000` is the emulator's doorbell. MGSM2Fix already knew about it without
knowing what it was for: `Ketchup::Update` carries
`if (bPatchesDisableRAM && address != 0x200000)`, i.e. that one address is
deliberately never blocked. The collection's native side watches it and puts up
its Control Settings panel.

`0x800C550C` is the first KEY CONFIG cursor call, inside `case 8`'s third branch
of `option_800C5150` — the branch that transitions to state 9. So selecting KEY
CONFIG rings the doorbell and **returns without entering the screen at all**,
which is why the panel appears over the OPTION menu with its rows still visible
behind it.

### Why the port broke it, and how the cause was found

Two bisection steps, no theorising:

1. **Move our option PPF aside and boot.** Intercepted. So it is ours.
2. **Relocate the retail stage, byte for byte, and boot.** *Not* intercepted.
   So it is not our rebuilt contents — it is the relocation.

Which then explained itself. The port parks the option stage in DUMMY3M and
repoints its `STAGE.DIR` entry; the collection still writes its patch to the
retail sectors; the game no longer reads them. The patch is orphaned. This is
the same class of bug as `menu.ppf`'s chain records, which
[the sc_text notes](#the-sc_text-texture-port) already describe — a relocated
stage stops receiving everything aimed at where it used to be.

The patch that does it is `disc1_16F8E024_patch.bin`, and it is the **only**
collection patch anywhere inside Integral disc 1's option stage. Found by
mapping every offset-named candidate the log records onto each stage's image
span:

| stage | sectors | relocated by the port? | collection patches inside |
|---|---|---|---|
| `option` | 27136..27210 | **yes** | **1** — `disc1_16F8E024_patch.bin`, stage offset 152460 |
| `preope` | 27249..27335 | yes | 0 |
| `brf` | 1..138 | yes | 0 |
| `camera` | 27211..27248 | no | 0 |
| `title` | 35391..35594 | no | 1 (`disc1_1822B55D_patch_PS5.bin`, and `title` is not relocated) |

So this was one instance, not a class. **Re-run that audit whenever a new stage
is relocated** — it is a few lines of arithmetic against the log's candidate
list, and the failure mode is silent.

### Seeing it at all needed a new tool

`disc1_16F8E024_patch.bin` is entered **with a filename and zero bytes of inline
data** — the content comes from the file, so MGSM2Fix's hook cannot read it, and
no amount of staring at patch names would have revealed what it does.

`SQHook::SetPatchWatch(start, end, label)` reports any CD-ROM patch landing in a
disc-image range — offset, length and leading bytes — and then lets it through.
Registered for both discs' option stages in `mgs1.h`. It was that watch which
caught the *second*, previously invisible patch in the same stage: 24 inline
bytes at `0x16f665ac`, which is overlay `+8964` = `0x800C550C`, the stub above.
The lesson generalises: **the collection's file-backed patches are opaque to the
hook, so watch the region, not the name.**

The watch also fired at `0x128f3e64` — the exact disc 2 address predicted by
putting stage offset 152460 through disc 2's STAGE.DIR LBA — so the same patch
targets both disc images.

### The fix: reproduce the stub, do not inherit it

`opt.c` now carries it directly, behind `OPTION_MC_CONTROL_SETTINGS`:

    else
    {
    #if OPTION_MC_CONTROL_SETTINGS
        *(volatile unsigned int *)OPTION_MC_DOORBELL = 1;
        return;
    #endif
        option_800C449C(work, -148, -70, 88, 13, 255, 1);
        work->f920 = 9;
        ...

Reproduced rather than inherited because their patch is keyed to a byte offset
inside retail's overlay, and ours is recompiled *and* relocated — so ours works
wherever the stage ends up living. It builds to the same shape at our own
addresses (the compiler picks different registers, which the emulator does not
care about):

    0x800C5434  3C028020  lui v0, 0x8020
    0x800C5438  AC430000  sw  v1, 0(v0)
    0x800C543C  08031548  j   0x800C5520     ; our epilogue
    0x800C5440  00000000  nop

Overlay came out **25,530 bytes, 312 under retail's 25,842 ceiling** — smaller
than before, because the early return elides the whole state-9 setup block.

**Set the flag to 0 for a raw PSX disc.** There is no collection to intercept
anything, `0x80200000` is past the end of a retail console's 2 MB, and the
ported KEY CONFIG screen is the entire reason its text was ported.

### What this means for the KEY CONFIG text port

In the collection, Integral's KEY CONFIG screen is now unreachable — as USA's
always was — so the eight ported label textures are **not visible there**. They
are still built, still verified, and are exactly what a raw PSX disc patch
needs. The user's call, 2026-09-03: *"that was the point of porting the text. I
want the intercept still in mc."*

## PPF3's description field is 50 bytes, and `ljust` does not truncate

Cost a 306 MB log and a crash on 2026-09-03. A hand-written PPF used a 60-byte
description in the 50-byte header field; `ljust(50, b'\x00')` pads but never
truncates, so every record offset was parsed 10 bytes late. Ketchup then wrote
255-byte blocks at addresses like `0xfa4aa0aa4aa0afe`, in a loop, at roughly a
gigabyte of log per minute.

Two guards now:

- Every tool that builds a PPF asserts its description length — `optsctext.py`,
  `preope_usa.py`, `reloc_ppf.py`. They were all padding without truncating; the
  strings happened to be short enough.
- **`ppfcheck.py`** parses a PPF exactly as Ketchup does and rejects what it
  would choke on: bad magic, records running past EOF, zero-length records,
  trailing bytes, and offsets past any plausible disc image.
  `py ppfcheck.py --deployed` checks everything under the mods folder. Run it on
  anything before it goes near the game — it caught the broken file instantly,
  and all 18 deployed PPFs pass.

## Memory-card messages (`en_savemsg`)

`menu/datasave.c` in the executable keeps two 12-entry caption tables,
`saveCaptions_8009EB4C` and `loadCaptions_8009EB7C`, indexed by the low byte of
the save/load request code (`captions[(unsigned char)dword_800ABB58]`, codes
like `0x45000003`). The request codes are identical in both games — checked by
enumerating every `lui/ori` pair in both executables — so an index means the
same state in both, and the port is index for index. USA's tables live at
`0x800A12A4` / `0x800A12D4` (found from the strings they must contain, then
confirmed against the code that indexes them at `8004EBF0` / `8004EC2C`; a
first dump that guessed the table start was off by one and made the tables look
shifted).

| idx | Integral | USA | shipped |
|---|---|---|---|
| 1 | セーブが完了しました。 / ロードが完了しました。 | "" | Integral's (USA shows nothing here) |
| 2 | セーブできませんでした。 | Save failed. / Load failed. | USA |
| 3 | エラーが発生しました。 | Error occured while saving. / … loading. | USA |
| 4 | 空きブロックがたりません。 / セーブファイルがありません。 | No empty block. / No save file. | USA |
| 5 | メモリーカードが初期化されていません。 | Memory Card is not formated. | USA |
| 6 | セーブしました。 / ロードしました。 | Data saved. / Data loaded. | USA |
| 7 | フォーマットに失敗しました。 | Formating failed. | USA |
| 8 | メモリーカードがさされていません。 | Memory Card undetected. | USA |
| 9 | セーブ中です。 / ロード中です。 | "" | Integral's |
| 10 | メモリーカードをチェックしています。 | Now checking Memory Card. | USA |
| 11 | フォーマットしています。 | Now formating Memory Card. | USA |

**Which entries are live** (from `init_file_mode_helper_helper_80049EDC`):
index 9 is requested with `0x01000009` immediately before `saveFile()` /
`loadFile()` runs, so it is on screen *during* the write or read; index 1 is
requested with `0xC1000001` on the success path (`block_75`) right after the
operation returns — that is the live completion message. USA's entries at both
are blank: it shows no caption while saving and none after a success (its
confirmation is the separate "COMPLETE" label in the file-list UI, which
Integral also has in English). So after a save Integral flashes
セーブが完了しました。 where USA shows nothing — Integral's own extra text, kept
under the rule; blanking it to match USA would be a one-pointer change.
Index 6 (セーブしました。 / "Data saved." and the load twins) is **never
requested**: no `0x…06` code exists and the only other draw is a hard-coded
`captions[4]`. Those strings are dead in both games; ported for completeness,
never visible.

USA's spelling ("formated", "occured") is kept verbatim. Everything else in the
module is already English in Integral (SAVING..., LOAD DATA, LOADING..., NO
FILE, NO SPACE, COMPLETE, YES/NO, OVERWRITE OK?, FORMAT OK?, EZ/NM/HD/EX,
MEMORY CARD 1/2) or is Integral-only Japanese drawn alongside the English (the
上書きしますか？ / フォーマットしますか？ prompts) and stays.

Mechanism: the same as the item descriptions. The 17 Japanese strings are one
contiguous pool at `0x80011F18..0x800120CB` (435 bytes); the 13 English strings
plus the 4 kept Japanese ones (338 bytes) are repacked into it and both pointer
tables rewritten — nothing outside the pool and the tables changes, and the
tool asserts that. `savemsg.py` emits a PPF per disc addressed at the
executable's sectors (Ketchup mirrors executable writes into RAM, since the
Master Collection never re-reads the executable). Both discs' executables are
identical here. The caption is drawn into the codec message-board KCB
(`radio.c`, `font_set_kcb(kcb, -1, -1, 0, 6, 2, 0)`, 252 px buffer), the same
code in both games, so placement follows.

**Verified in game 2026-09-02:** LOAD DATA with no save present draws
`No save file.` on rows 211–219, x 126–195 in both games. The header above it
differs only because `DisableRAM` (achievements off) filters the Master
Collection's own rename of MEMORY CARD → STORAGE, so Integral shows the
original wording; USA's reference shot predates that setting. The in-game save
flow (Mei Ling) has not been shot yet.

The save-slot encoding audit is recorded in [COVERAGE-RECORD.md](COVERAGE-RECORD.md).
USA also stores full-width Shift-JIS, so the old ASCII claim was incorrect.
Integral's source constructs `ＭＧＳ∫`, difficulty, time,
and the area name in Shift-JIS. Formatting and Integral's product identifier
must be distinguished from Japanese text with an English counterpart.

## The PHOTO ALBUM's own memory-card messages (`en_camsave`, 2026-09-03)

`en_savemsg` ports `datasave.c`'s two caption tables **in the executable**,
which is what the LOAD DATA and in-game save screens use. The PHOTO ALBUM has
its **own, independent copy of the same message family inside the `camera`
overlay**, and those are still Japanese. Found by shooting the unlocked SPECIAL
page: PHOTO ALBUM → SELECT MEMORY CARD reads `セーブファイルがありません。`
where the LOAD DATA screen (correctly) reads `No save file.`

Proof it is a separate copy, not a failure of the port: the Japanese
"no save file" byte sequence appears **once** in retail Integral's executable
and **zero** times in the ported executable (the pool was repacked), yet **once**
in the `camera` overlay - which the port never touches.

    Integral camera overlay   54,668 bytes   ~19 game-encoded messages at
                                             +0x0CB2C .. +0x0CD00 (~460 bytes)
    USA camera overlay        55,668 bytes   the same family in ASCII

USA's `camera` overlay carries its own English set, so this is a straight port
with a known target: `Data loaded.`, `Data saved.`, `Error occured while
loading.`, `Error occured while saving.`, `Formating failed.`, `Load failed.`,
`Memory Card is not formated.`, `Memory Card undetected.`, `No empty block.`,
`No save file.`, `Now checking Memory Card.`, `Now formating Memory Card.`,
`Now saving.`, `Save completed.`, `Save failed.` - plus the album's own
`FREE: %d BLOCK%s`, `NEW FILE [ NEED %d BLOCK%s ]` and `PHOTO %02d`.

**Ported the same day: `camsave.py`, patch `en_camsave`.**

The references turned out to be **pointer words in one region** of the overlay
payload, 0x600..0x740, and both games' overlays are the same program - so **the
pointer word's offset is the index**: slot 0x648 is the same message in both,
wherever each game's string sits. That makes the mapping exact rather than
inferred from order or content, and it is why this port was small.

    overlay base    Integral 0x800C3208, USA 0x800C5968 (the same bases as their
                    `option` overlays: stages load at a fixed address)
    Japanese        815 bytes across the paired slots
    English         509 bytes, 306 SHORTER - so the pool repacks in place, the
                    overlay stays 54,668 bytes and the stage stays 38 sectors
    result          33 pointer words rewritten, 23 distinct strings,
                    428 of the pool's 492 bytes used, 528 bytes in 44 records

Six slots point at an **empty** string in USA (0x60C, 0x62C, 0x63C, 0x65C,
0x668, 0x66C). Integral's Japanese stays there, the same decision `savemsg.py`
made for its "saving" and "save completed" captions. Slots already identical in
both (`OVERWRITE OK?`, `FORMAT OK?`, `COMPLETE`, `ERROR`, `MEMORY CARD 1/2`,
`YES`, `NO`, `SAVE DATA`) keep their address and their pointer word untouched -
only strings that actually move are re-laid.

**The trap here: that same region also holds FUNCTION pointers.** Slots 0x6E0,
0x6E4 and 0x6E8 aim at code, and `addiu sp, sp, -N` reads as bytes over 0x80, so
a "has high bytes therefore Japanese" test called them text and the first build
would have rewritten three function pointers. Every real string in these
overlays sits past 0x0C000, well clear of the code, so `read_table` requires
that. Verified afterwards that all three still hold their original values.

**Second trap: `stage()` returns the stage's sector COUNT, not its position.**
The first deployed build changed nothing on screen even though Ketchup logged
the PPF as loaded, because `overlay()` took that count (38) for the position
(27,211) and every record addressed a point ~27,000 sectors early. The position
comes from the entry table, via `ents()`. Nothing warned: the PPF was
well-formed, loaded fine, and wrote 528 bytes of correct data to the wrong
place.

**So the check that catches this class of bug is to read the DISC IMAGE back.**
For every record, seek to its offset in `discs/int1.bin` and assert the bytes
there equal retail's overlay bytes at the corresponding overlay offset. Both
discs: 44 of 44 records matched before deploying. Every future stage patch
should do this - the earlier builds' static checks all passed on a patch that
pointed nowhere.

Other checks before deploying: overlay size unchanged, every changed byte inside
the pool or the table, every rewritten pointer resolving to a NUL-terminated
string inside the pool, and the three function pointers untouched.

**Confirmed in game 2026-09-03.** PHOTO ALBUM → SELECT MEMORY CARD now reads
`No save file.` where it read `セーブファイルがありません。`, and the string is
byte-identical to USA's own for that pointer slot. Placement needs no check: the
port changes string bytes and pointer words only - every other byte is asserted
unchanged - so the caption is drawn by the same code in both games.

Still unseen, because they need those states: the other captions on that screen
(save failed, formatting, no empty block, the card-undetected line) and the six
slots where Integral's Japanese is kept.

**Fully verified 2026-09-04.** With a photo saved, the PHOTO ALBUM's load and
overwrite screens were exercised and the deployed PPF was applied to the
extracted `camera` overlay for a slot-by-slot comparison with USA: all 23
English strings present, the six USA-blank slots Japanese as designed, nothing
else. Details under "Not tested" → the PHOTO ALBUM item.

## Sweep: is any UI text still Japanese? (`jpsweep.py`, 2026-09-03)

**Result withdrawn 2026-09-04.** The sweep pairs *overlay pointer words*, so it
sees only strings the overlay addresses directly. GCL script text — read via
`GCL_GetString` from a command's string list — is invisible to it, and that is
where the MISSION LOG's Japanese lived until 2026-09-05 (see "The MISSION LOG port").
Treat the section below as "no overlay-pointed Japanese remains", not "no
Japanese remains".

The photo album's messages were found by accident, from a screenshot, which
raised the obvious question: what else is hiding in an overlay? `jpsweep.py`
answers it with the same trick that made that port small. Overlays load at a
fixed address per game, so a word equal to `base + offset` is a pointer, and the
pointer word's **offset is the same index in both games** - so a slot where
Integral's target reads as Japanese and USA's reads as an English sentence is a
port candidate, paired exactly.

**Result: across all 82 stages present in both disc 1 images, only `camera`.**
24 slots, which are the ones `en_camsave` now ports. Nothing else on disc 1 has
pointer-referenced Japanese UI text with a USA English counterpart.

Two things had to be right for that to mean anything:

- **The Japanese test must look at the LEAD byte of each pair.** The encoding is
  two-byte pairs whose first byte is 0x80-0xdf and whose second is often low
  (`82 1b`, `d0 06`). Testing every byte for the high range only reaches ~50%,
  so a 55% threshold found 2 slots instead of 24 and would have reported the
  game clean.
- **Validate the sweep against the known positive.** It is only trustworthy
  because it lights up `camera` at 24 slots. A sweep that reports "nothing" and
  has never been shown to find anything is worth nothing.

Blind spots, stated so the result is not over-read: strings reached by
`lui`+`addiu` rather than a pointer word, strings whose USA counterpart is a
single word with no space, text in the GCL scripts rather than the overlays
(which is where the ported menus live, and those are handled), and anything on
disc 2 or the VR disc.

## Three item-text faults and what they taught (2026-09-05)

The user's first shots after the MISSION LOG deployment showed two glitches in
text that had been "verified" for a week: `《Socom Pistol》ドSemi-autom / atic
pistol.` and `opens all level 17security doors.` Ketchup's audit lines in the
same log caught two of the three mechanisms in the act; the third was found by
reading the bytes. All three are fixed in `items.py` and the second lesson is
applied to `savemsg.py` too.

1. **The card level digit.** `menu_item_printDescription` (menu/item.c) does
   `itemDescription[46] = GM_CardFlag + '0'` — byte 46 is where the Japanese
   string keeps its digit. USA's string has the `1` at byte 45 and USA's exe
   stores at 45 (`sb $v0, 0x2d($a1)` at 0x8003D9E8). Integral's store hit the
   space: audit line `run 0x1102a differs at +76: wrote " securit" have
   "7securit"`. Fix: the Integral instruction at 0x8003B690 now stores at 45
   (`A082002E` → `A082002D`), in the `en_items` PPF.
2. **The SOCOM suppressor rewrite.** `menu_weapon_printDescription`
   (menu/weapon.c) writes bytes 0x70..0x72 of the SOCOM description every time
   the weapon window shows it: `d0 03 00` without the suppressor, `90 b6 91`
   with it (the Japanese text's optional last line). USA's 83-byte string ends
   long before 0x70 and USA's identical code writes into padding. In the
   repacked pool the target was the relocated Mine Detector message: audit line
   `run 0x119dd differs at +95: wrote " used in" have "<90b6><91>ed in"`. Fix:
   the six `sb` instructions are NOPs (0x8003E070..0x8003E0B0), the no-op they
   are in USA.
3. **A byte the English shared with retail kept the collection's value.** The
   stray glyph came from the line break after the title: `d0 15 80 7c`. Byte
   0x800119DC (`80`) equals retail (`80` of the grenade text's `80 23`), so no
   PPF record named it and Ketchup never wrote it; the collection's own RAM
   patch (one of the two ~2.8 KB blocks at 0x8001101C / 0x8001108C, applied
   before Ketchup's pass) had put something else there, and the engine read a
   two-byte katakana code instead of `|`. The 1-byte record for `15` at
   0x800119DB and the run starting at 0x800119DD were written and audited
   fine — the audit sees only what a PPF names, which is why "items proven
   intact" (below) was blind to this. Fix: `items.py` and `savemsg.py` now
   emit **every byte of the regions they own** (both arenas, both tables, the
   frozen-item pair inside the item table, the code words), changed or not:
   3,224 + 531 bytes, so `Applied … bytes of RAM patches` now reads **3,755**,
   Ketchup owns the whole pool after the collection's pass, and the audit
   covers all of it. The collection's block itself was not found as plain
   bytes in `alldata.bin` or the DLC containers (searched 2026-09-05), so what
   it wrote there is inferred from the glyph, not read.

Corrections this forces on older notes: "items proven intact" (the RAM-patch
collision section) held only for bytes a record named; the earlier fear about a
complete Ketchup check fighting the collection does not arise here because the
collection writes first (pass 1 only, every session); and the frozen Ration /
Ketchup descriptions were never lost — Integral's item table has 26 entries
whose last two are that pair, USA's likewise, so `N_ITEM = 26` ports them.

All three fixes were seen on screen at 12:55 the same day (SOCOM, ID Card
`level 7 security`, Mine Detector), with the audit silent. Other weapon
descriptions have not been looked at individually.

## The MISSION LOG port (`abst` stage, `en_abst`, built 2026-09-05)

**Status: ported, deployed and seen on screen 2026-09-05** (both pages of two
logs, the controls, the slide after its fix); a demo.gcx page and a count-7
page are the parts not yet looked at.
Loading a save shows `READ MISSION LOG? YES / NO` and then a page of
story-so-far text. Integral drew it in Japanese with furigana; USA has the same
122 pages in English. `abst_build.py` gives Integral USA's text, USA's layout
and USA's controls. The user's USA screenshots of 2026-09-04 (Heliport save,
`MGS1 USA/20260904233158` and `…233204`) are the reference: two screens of 7
lines, `1/2 ►` then `◄ 2/2`, EXIT at the right.

### What the stage is

Both discs carry one identical copy: STAGE.DIR sector 139, 80 sectors. It is
the whole LOAD DATA / SAVE DATA flow, not just the log: overlay `sb` (mload.c
`NewLoadData`/`NewSaveData`, abst.c `NewAbstract`, ab_demo1/2.c, ab_ch.c),
texture DAR `nd`, a cache section, two sound files (identical to USA's).

**The cache section's tag sizes are offsets** (`libfs/cdstage.c`,
`LoadCacheSection`: `GV_LoadInit(current_ptr + tag->size, id, CACHE)`). The
`c?` payload is five files: `k` @0, `l` @180, `h` @184, **scenerio.gcx @236**
(tag id 0xEA54 = `GV_StrCode("scenerio")`), **demo.gcx @57900** (0xA242 =
"demo"), and the 0xFF fake tag whose size is the section total. `portio.stage`
already skips the non-0xFF `c` tags; `pack_stage` sets the 0xFF size from the
payload, and the builder re-stamps demo.gcx's offset itself. Files are
4-aligned in the original; the builder keeps that.

**Each .gcx is `GCL_LoadScript`'s layout:** BE32 proclen, proc table
(id:BE16 offset:BE16 …, zero word), contiguous proc bodies (each a GCL ARG
`40 BE16`), BE32 script length, the script ARG, BE32 font length, the font
glyphs (`font_set_font_addr(2, …)`). scenerio.gcx: 86 procs, 80 pages (one
`0x9906` command per proc), the disc-change block, the English location list
in the script body, an 11,520-byte font. demo.gcx: 110 procs (USA 109 — the
extra 0x5FD9 is the Japanese location list's), 42 pages, the Japanese location
list, an 11,412-byte font. **The 42 demo pages are the 42 pages that carry
`d` = PROCID 0x6025.** All 122 pair 1:1 with USA's in order: every `e`/`l`/`r`
option equal (checked on every build).

**Page grammar:** `60 <BE16 size> 99 06 07` · STRID STRID · OPTION `e` ·
`l` · `r` · (`d`) · OPTION `i`: SHORT count, then count+1 STRING records
(`07 len payload`, NUL included in len), then **one `00` — the command's
GCL_END, which is what stops `GCL_GetOption`'s scan**. The `i` option's own
length byte is an overflowed u8 in both games (234 / 49) and is never read;
the builder leaves it. **Record 0 is not a header: it is the caption drawn
under READ MISSION LOG?** — `D_800C3238[0] = {88, 180, 0x6739}`, lit at init,
hidden on YES. Integral's is 作戦記録を参照しますか？ with the furigana
ミッションログ (`#{…}#` is the ruby syntax; `<9002>` separates base from
ruby); USA's is empty. 80 Integral pages carry it, the 42 demo pages have it
empty. **Kept** (`KEEP_PROMPT_CAPTION = True`): USA draws nothing there, so
under the no-translation rule the Japanese caption stays — the same case as
the memory-card captions USA leaves blank. Flipping the constant gives USA's
empty record. The user's call; not asked yet.

### What USA's overlay does (read from SLUS-00594 `sb`, base 0x800C5968)

| | Integral retail | USA / this port |
|---|---|---|
| line table `D_800C3238` | caption {88,180}, then 11 × {x 16, y 35+19k} | caption, then **14 × {x 8, y 35+22k mod 7}** (lines 8–14 reuse 1–7's rows) |
| KCB | 128×21, `+= 21`, one column at x 704 from y 256, CLUT y 276 | **128×20, `+= 20`, column at x 576 from y 256, CLUT y 275; when `font_y + 20 >= 512` the 13th KCB starts a second column at x 704** (12 per column) |
| KCB count | 12 | **24** (default count 24; the port clamps to 23 so 24 is enough) |
| sprite height | `max_height` | **`max_height − 1`** (keeps the CLUT row of each 20-row band off screen) |
| draw | one column, tpage 704/768 | **entries 0–11 with tpage x 576/640, 12–23 with 704/768**, 12 DR_TPAGEs; every sprite's x0 gets the page-slide offset |
| bottom bar | `abst_d_l` + `abst_d_r` (EXIT centred) | **`abst_d_l` (◄ at x 115–125), `abst_d_r1` (► at 193–203, first 80 px of the right half), `abst_d_r2` (EXIT, last 80 px)**; the arrow polys toggle attr 0x100 (shown) / 0 (hidden) per page |
| cursor frame | fixed around EXIT (−33..33, 81..105) | **moved at runtime** by `abst_cursor_frame(x,y,w,h,rgb,mode)`: ► = (30,86,16,14), ◄ = (−46,86,16,14), EXIT = (90,87,54,12), mode 1 = 6-px outer margin, 2-px inner overlap; USA only ever uses mode 1 |
| page counter | none | **`MENU_Printf("%d", page)` at (145,202), `"/"` at (156,202), `"%d", 2` at (167,202)**, colour (86,137,116); the total is USA's literal 2 |
| reading state | ×/○ exit, SELECT hides text | cursor on ►: RIGHT → EXIT; ○ or R1 → next page (cursor lands on EXIT). On ◄: RIGHT → EXIT; ○ or L1 → previous page (cursor lands on ►). On EXIT: LEFT → ◄ (page 2) or ► (page 1); L1 → previous page (not on page 1); R1 → next page (not on page 2; cursor lands on ◄); ○/× exit. SELECT hides/shows the current page's 7 lines; × on an arrow exits. Cursor moves play SE 31, page turns **SE 176**, exit SE 33 |
| page turn | — | **states 4/5: 8 frames sliding out at 40 px/frame, swap the lit lines at frame 9, 8 frames sliding in, then page ±1**; the counter is drawn throughout |
| fade in | cursor pieces fade in frames 0–8 | l/r images 0–64, panel constant from 65, text 80–144, **cursor pieces 136–144**; state 2 at 145 with the cursor on ► |
| fade out | 96 frames | same, plus the counter fading with the cursor over the first 8 |
| option `d` | not read | read; **`d == 1` skips the prompt**. No page sets it (80 lack it, 42 carry the PROCID), and a GetOption scan for a missing letter walks past `i`'s bad length byte into the text, so **the port does not read it** |

The `Work` layout of the port matches USA's field for field (polys1[7],
kcb[24], tpage[12], field_51C[24], cursor/page/scroll at +0x7844/48/4C,
`sizeof` 0x7854), which made the disassembly readable against the C.

**Two departures from USA's code, both invisible at rest.**

*The second sprite is 248 px wide, not 256.* `font_get_buffer_size` sizes a
128-word KCB's buffer and its VRAM upload to `kcb->width` = 504 px (42 cells of
12), so texels 504..511 of every line are never written and hold whatever the
VRAM had there. USA's second sprite (`text_sprt2`, x 264) is 256 wide and
reaches them; at rest they sit off screen at x 512, but the page slide drags
them across the panel as coloured 8-px fragments, one per line row — seen
2026-09-05 on the Comm Tower A page mid-slide (shot `20260905125444`), not on
the Heliport page, because the stale VRAM differed. `func_800C47A8` now sets
the second sprite's width to `kcb->width - 256`; the longest USA line is
~300 px, so nothing drawn changes. Retail Integral and USA have the same hole
and simply never move the sprites.

*A guard USA lacks.* USA colours lines `base+1..base+7` of a page without
checking the count; a count-7 page has 8 KCBs, and `font_set_color` on an
unallocated KCB writes through its NULL CLUT buffer. `abst_color_page` stops at
the page's last record. Nothing visible changes: those pages still show `1/2`
and can flip to an empty second screen, exactly as USA does. The 14 USA pages
with count 7 are k = 83, 85, 86, 99, 100, 101, 102, 105, 106, 113, 116, 118,
121, 122 (0-based over the 122); all other 108 have count 14.

### The build

`abst_build.py` (both discs, `--deploy` to install):

1. Parses scenerio.gcx and demo.gcx out of Integral's and USA's chunks, walks
   the procs in table order, and for every page replaces the `i` payload with
   `SHORT count_usa`, Integral's record 0, USA's records 1.. verbatim (the
   pages with 16–17 records keep their trailing empty ones), and the GCL_END.
   The COMMAND's BE16, the body's ARG length, the table offsets, proclen and the
   script length are recomputed; the fonts are untouched. Chunk 90,796 →
   104,600 bytes (+13,804).
2. The disc-change block (`a` + `e`, `ab_ch.c`) takes USA's whole `e` option
   from its letter to the block end: PROCID plus the eight strings `Insert DISC
   1.` … `The correct DISC has not been insert.` (USA's own spelling). That is
   the **fourth copy of the disc-swap text, now ported**.
3. DAR: `abst_d_l` ← USA's payload (same VRAM (0,256), same CLUT (832,233));
   `abst_d_r` → `abst_d_r1` at (0,286) with CLUT (848,233) and `abst_d_r2` at
   (20,286) with CLUT (960,233), i.e. inside the 40×30-word footprint the old
   texture freed. Everything else — including the SOLID strip and the cursor
   pieces, whose palettes differ from USA's — stays Integral's. 22 → 23
   entries.
4. Packs the stage with `obj/abst.bin` (48,087 bytes, retail 47,071), the new
   DAR and chunk: **88 sectors**, relocated into **DUMMY3M slots 462..549**
   (disc 1 LBA 292792, disc 2 303898; the STAGE.DIR `abst` entry becomes
   156138 / 198720), 760 records per disc. The builder refuses any overlap with
   the other PPFs' bytes in the mods folder.

**Verified statically:** every rebuilt body re-parses with `gclparse` (the
container sizes are self-checking); all 122 pages re-extract with USA's count
and USA's line records byte for byte, record 0 unchanged, fonts unchanged; the
PPF records rebuild the 88-sector stage byte for byte in the DUMMY3M slot and
the entry repoint lands on the `abst` STAGE.DIR entry, both discs; the compiled
overlay carries the 15-entry table, the 576/704 columns, the wrap at 512, the
cursor-frame and counter immediates and the pad masks; the collage images
(BRF.DAT, 160 px at VRAM x 128 and 256, CLUTs (768,245/246)) are clear of the
KCB columns; `ppfcheck --deployed` clean on all 20 files.

**Seen on screen 2026-09-05 (user's shots, morning):** both pages of the
Heliport and Comm Tower A logs, `1/2 ►` and `◄ 2/2`, EXIT framed, USA's rows.
The only fault was the slide fragments above, fixed the same day and confirmed
gone by the user at 13:50. **Not verified otherwise:** an unattended smoke test is not possible:
a `StageSelect = abst` launch on 2026-09-05 00:39 sat in the collection's own
launcher ("Ver.3.0.0" is the last log line), which waits for a game to be
chosen before Ketchup or any stage loads; the process was stopped by PID and
the ini restored. First things to look at when a save is
loaded: the caption under READ MISSION LOG? still Japanese (by rule); page
`1/2` with ► highlighted and seven English lines at USA's rows (ink tops at
game y 52, 74, 96, 118, 140, 162, 184; x from 8); RIGHT moves the frame to
EXIT; ○ on ► slides to `2/2` with ◄; SELECT hides the text; × leaves. Then a
count-7 page (a late-game save) shows `1/2` with an empty second screen — USA's
own behaviour, worth a decision. Measure the shots against
`MGS1 USA/20260904233158_1.jpg` the way "Measuring from screenshots" says.

**The collection's `disc1_132F2716_patch_PS5.bin` is orphaned by the
relocation, and that is harmless.** It lands at stage byte 51×2048+166 = chunk
+0xB0A6 — the BE16 size of the disc-change block. It was decoded on 2026-09-07:
240 bytes, 83 differing from retail, and what it does is blank records 1 and 3
(the "open the disc tray" lines) and reword record 4. `en_abst` replaces those
same strings with USA's English, so the relocated stage losing this patch costs
nothing. The block now holds USA's English. The watch on the original span
stays registered so the next achievements-live run shows what the collection
wanted to write.

**Left as it was:** block 80 (count 1: the caption plus one game-encoded line
in *both* games, USA's included — nothing English to port); the two location
lists (Integral's own English `Tank Hanger` / `Medi rm` / `Cmnder rm` /
`Cmnd rm` against USA's `Hangar` / `room` — Integral's text, ask before
changing; and the Japanese list, USA has none); every other texture.

**Gotchas met on the way** (also indexed under Gotchas): the heredoc ate a
backslash again and put a NUL into `abst_build.py`; `GCL_GetOption` for a
letter the command lacks walks past `i` by its bad length byte and spews
`GCL:WRONG CODE` until a NUL; `portio.read_ppf` takes a path, not bytes;
`WORK` is already the `work` directory.

## Where the collection's own disc patches land (mapped 2026-09-05)

The 2026-09-04 18:54 log ran with `DisableCDROM = true`, so it names every
CD-ROM patch the collection tried on Integral disc 1: 233 offset patches and
132 named files (`disc1_XXXXXXXX_patch`, the name is the image offset). Mapped
through STAGE.DIR (`stage_lookup` arithmetic, see "Reading disassembly"):

| target | offset patches | named files | relation to the port |
|---|---|---|---|
| RADIO.DAT | 125 | 103 | not ported |
| FACE.DAT | 25 | — | not ported |
| gameplay stages (`s03er` 11, `s18a` 9, `s10a`, `s07cr`, and 2 each in a dozen more) | ~50 | 2 | not ported |
| `ending` | 5 | 20 | not ported |
| `rank` | 5 | — | Integral-only text, not ported |
| `init` | 2 | — | the stage that holds `font.res` — unknown what they change |
| `camera` | 6 (+0x9348, +0xD614..+0xD6A8) | — | **en_camsave** patches this stage in place; none of the six falls inside or within 256 bytes before a camsave record |
| `title` | 2 (+0x165A4, +0x165CC) | 1 (+0x3B1B5 = image 0x1822B55D) | the named file starts **exactly on record 0's `07` header** in the title's disc-swap block - `en_menu3`'s target. **This is the collision that makes `en_menu3` raw-disc only**: deploying ours over theirs kills the title stage (see "Why `en_menu3` is raw-disc only"). en_menu's `RADAR OFF` record sits 130 bytes after it and is unaffected |
| `demosel` | — | 2 (+0x18FED, +0x19000) | **both start at en_menu2's disc-swap records** — one 2 bytes before our first record, one exactly on a record |
| `change` | — | 1 (+0x421F) | **2 bytes before en_menu2's first record** there |
| `option` | 1 (+0x2B04) | 1 (+0x2538C, the KEY CONFIG doorbell) | stage relocated; the doorbell is reproduced in `opt.c`. What the offset patch at `+0x2B04` changes, and whether the relocated stage loses it, is not recorded — open, `HANDOFF-ARCHIVE.md` §5.10 |
| `abst` | — | 1 (+0x198BE, the disc-change block) | stage relocated; the block now holds USA's strings |

So the collection patches **all four copies of the disc-swap text** (`change`,
`demosel`, `title`, `abst`) with named files, and `en_menu2`'s `change` and
`demosel` records begin two bytes after two of them.

**What those files contain was read on 2026-09-07 — see "What the collection's
named-file patches say" below.** The sentence that stood here for three days,
that their bytes are only visible through `SetPatchWatch`, was true about the
watch and quietly became a belief about the bytes. They are in the game's own
archive on disk, and `m2archive.py` decodes it. Watches still cover `change`,
`demosel`, `title` and `camera` on both discs (`mgs1.h`), and they remain the
only way to see *when* a patch is applied; they were never the only way to see
what is in one.

**What the new watches showed (2026-09-05 12:57 log):** the six `camera`
patches are the collection's STORAGE rename (`  STORAGE 1  `, `  STORAGE 2  `
and runs of spaces, 8–29 bytes), the two `title` offset patches are
`  STORAGE SLOT 1/2  `, and the option doorbell is its 24 bytes — none touches a
port record. The named files (`disc1_18345E07_patch` at `change`, twice;
`disc1_18412A95` / `disc1_18412BD8` at `demosel`; `disc1_1822B55D` at `title`;
`disc1_132F2716_patch` at `abst`) are registered with **0 bytes of data in the
call** — the game reads them from its archive later — so the watch proves they
apply on Windows (the `abst` one included, answering the `_PS5` question) but
cannot show their content. Only the disc-swap screens themselves can.

Which bytes win where a collection patch and a port record overlap is a
question of the patch table's order: both go through the same
`entryCdRomPatch` (Ketchup registers its PPF records after the collection's
`_set_disk_patch`, see the log order), so the port's bytes are the later entry
and most likely the ones the emulator applies last. Not proven; the disc-swap
screens have never been seen in the collection, which is also why nobody
noticed. For a raw disc none of this applies.

## The disc-swap text: four copies, and why only real play can reach the swap (2026-09-04)

The `Insert DISC 2.` / `Now Checking...` family exists in **four** stages. Their
states, so nobody has to re-derive them:

| copy | stage | state |
|---|---|---|
| 1 | `demosel` | **ported**, `en_menu2` |
| 2 | `change` - the stage that performs the disc check and the swap | **ported**, `en_menu2` |
| 3 | `title` | **not shipping in the collection** - `en_menu3` is built and verified but raw-disc only, because the collection patches the same block (see "Why `en_menu3` is raw-disc only") |
| 4 | `abst` - `ab_ch.c`, the disc-change abstract | **ported 2026-09-05** in `en_abst` ("The MISSION LOG port") |

**None of the four has been seen in the collection.** The open question is
whether the collection ever shows the game's own swap prompt at all, or swaps
silently before `change` draws it — and, since 2026-09-05, what the collection's
own named patches to all four blocks contain (previous section). A silent normal swap does not prove the
title/wrong-disc, demo-theater or abstract paths unreachable. Those need
dedicated tests; all four remain relevant to a raw disc patch.

**Disc 2 is set in exactly one place, so the developer menu can never reach
it.** `onoda/change/change.c` performs a literal CD check and is the sole
writer of the disc number:

    status = FS_ResetCdFilePosition( alloc );
    if ( status == 1 ) { printf( "THIS IS DISC 2!!\n" ); FS_DiskNum = status; }

(`FS_DiskNum` is read everywhere else - `gamed.c` builds the exe name and
`GM_Disk` from it, `radio.c` and `ab_ch.c` display it - but only `change`
assigns it.) The developer stage-select writes `mgs_loader_stage` and loads the
stage overlay directly; it never runs `change`, so **every debug load logs
`Disk ID is 0`** whatever stage is named. Two attempts on 2026-09-04 confirmed
it from both sides:

- `StageSelect = select3` -> **s11a** loaded and was fully playable (all items
  and weapons granted by the developer entry proc; a photo was taken and a
  save made) - but the save reads **DISC 1 / Comm Twr A**, and the log shows
  `Disk ID is 0` throughout. s11a is late disc 1, not disc 2 as first assumed.
- `StageSelect = s14e` -> **Cargo Elevator** loaded but the world never
  advanced: pause and the item/weapon menus worked (they are context-free), the
  stage did not (an event stage that needs prior story state). Killed by PID;
  no error in the log, just `scene "" -> "s14e"` and silence.

So a plain gameplay area (s11a) debug-loads clean, an event stage (s14e)
hangs, and neither touches the disc. **The only way onto disc 2, and the only
way to see whether the swap prompt appears, is to play across the break** -
from a late disc-1 save, then watch the transition and read the log for
`Disk ID` becoming 1. (Where the break falls in the story has not been
checked here; an earlier version of this paragraph gave a route from memory
and it was wrong.)

`StageSelect` therefore accepts three kinds of name, documented in the ini:
`true` (the top menu - TITLE / DEMO ALL / SOUND TEST only), a **menu**
(`select1`..`select4`, `selectd`; the lists the top menu never links to), or a
**stage** (`s11a`), which drops straight in with no menu. Event stages may hang
as above, and no debug load ever changes the disc.

## What stays Japanese, and why (consolidated 2026-09-03)

The rule is [no unauthorised translation](#scope-what-this-port-changes-and-what-it-deliberately-keeps):
port USA's English, never invent it. The table distinguishes Integral-only
text from unfinished ports. The old disc-1 pointer scan was not exhaustive;
see [COVERAGE-RECORD.md](COVERAGE-RECORD.md) for the expanded inventory and its limits.

| still Japanese | why | where |
|---|---|---|
| 1P MODE's twenty-one explanation pages | an Integral-only mode; USA has no 1P MODE, so there is no English to port | "1P MODE" under Unlocks |
| KEY CONFIG's per-row help lines (`ボタン設定：タイプA`, `主観モード時の操作：通常操作`, `オプション画面に戻ります。`) | USA's KEY CONFIG has no such lines | "The KEY CONFIG screen" |
| the option screen's other Japanese help lines, and record 7's colon (`字幕設定：オン`) | Integral-only rows; record 7 was wrongly blanked once and restored | "The sc_text texture port" |
| `savemsg` indices 1 and 9 (`セーブ中です`, `セーブが完了しました`) | USA draws nothing at those indices, so there is no text to port. Integral therefore flashes Japanese during and after a save | "Memory-card messages" |
| six `camsave` slots (`0x60C`, `0x62C`, `0x63C`, `0x65C`, `0x668`, `0x66C`) | USA's strings at those slots are **empty** | "The PHOTO ALBUM's own memory-card messages" |
| camera GCL caption at script `+0x1B8` | Integral has a nonempty game-encoded caption; the corresponding USA record is empty | [COVERAGE-RECORD.md](COVERAGE-RECORD.md) |
| the caption under READ MISSION LOG? (作戦記録を参照しますか？, record 0 of every mission-log page) | USA's record 0 is empty, so it is kept by the same rule as the memory-card captions; one constant (`KEEP_PROMPT_CAPTION`) blanks it — the user's call | "The MISSION LOG port" |
| mission-log block 80's one game-encoded line | USA's copy is game-encoded too, not English — nothing to port | "The MISSION LOG port" |
| the Japanese location list in demo.gcx; Integral's spellings in the English list (`Tank Hanger`, `Medi rm`, `Cmnder rm`, `Cmnd rm`) | USA has no Japanese list; the English list is Integral's own text. Now covered by the 2026-09-07 amendment - the same shape as the `SCARF` case decided that evening - and **deliberately left open** rather than swept along with it | "The MISSION LOG port", "Amendment, 2026-09-07" |
| the save-slot title (full-width Latin) | USA also uses full-width Shift-JIS; Integral has its own product suffix. No ASCII conversion is warranted | [COVERAGE-RECORD.md](COVERAGE-RECORD.md#save-slot-title-encoding) |
| record 3, the vibration-test row's label (`振動テスト`) | **kept blank, not Japanese** — the user's decision 2026-09-02: Integral's line is the row name plus a sentence USA's own line already covers | the scope table |
| `rank`'s 36 ranking-commentary sentences | Integral-only; USA's `rank` has the shared location names and none of these sentences, so there is nothing to port unless a USA counterpart turns up elsewhere | "`rank` is Integral-only text" |
| the VR disc's PocketStation help line and prompt, `vrtitle`/`vrsave` debug windows, MP5 and frozen items | Integral-only, or USA carries the identical Japanese | "The VR disc (SLPM-86249)" |
| **the MP5 SD weapon description on the MAIN discs** (`《MP 5 SD》 サブマシンガン。...`, RAM 0x80011B04) | Integral-only. USA has neither the weapon nor the VERY EASY difficulty that shows it, so there is nothing to port. **It is reachable**: on VERY EASY the FAMAS slot shows this description instead of the FAMAS one | "Descriptions that change with the game state" |

Two of these are worth revisiting only with authorisation: the 1P MODE pages
(the largest body of untranslated text in the game) and the KEY CONFIG help
lines. The save title is an encoding/branding case.

## The VR disc (SLPM-86249), ported 2026-09-06

Integral's third disc — VR training — now has USA's English from VR Missions
(`SLUS-00957`). Seven PPFs in `mods/INTEGRAL/VR-DISK/`, built by six tools that
share `vrlib.py`:

| PPF | tool | what | size |
|---|---|---|---|
| `INTEGRAL_vr_en_missions.ppf` | `vr_windows.py` | every in-mission window in 92 stages: titles, briefings, results, hints | 15 031 records, 3 370 955 bytes |
| `INTEGRAL_vr_en_items.ppf` | `vr_exe.py` | the executable's item, weapon and capture-mode name/description pools | 63 records |
| `INTEGRAL_vr_en_savemsg.ppf` | `vr_exe.py` | the executable's twelve save and twelve load messages | 36 records, 431 bytes |
| `INTEGRAL_vr_en_option.ppf` | `vr_option.py` | the option screen's help lines **and** the whole KEY CONFIG screen | 546 records, 121 471 bytes |
| `INTEGRAL_vr_en_title.ppf` | `vr_menus.py` | the EXTRA menu's four help lines | 7 records, 915 bytes |
| `INTEGRAL_vr_en_camsave.ppf` | `vr_camera.py` | the PHOTOGRAPHING mode's memory-card messages | 4 records, 617 bytes |
| `INTEGRAL_vr_en_movie.ppf` | `vr_movie.py` | the MOVIE selection captions and the overlay stub that lights two lines per clip; built on the composite with `vr_en_missions` (2026-09-07) | 69 records, 753 bytes |

`ppfcheck.py --deployed` passes. Six of the seven are disjoint; `vr_en_movie`
deliberately shares 686 of its 753 bytes with `vr_en_missions` and must load
after it, which Ketchup's name order gives (see "The MOVIE selection captions").

**Do not assume the VR disc works like the main game.** It has its own
executable, its own overlays and its own text containers; the only thing that
carries over from the disc-1/2 port is the file formats. Everything below was
established from the VR binaries themselves.

### Where the two discs live

| | Integral VR-DISC | USA VR Missions |
|---|---|---|
| disc id | `SLPM-86249` | `SLUS-00957` |
| container | `windata/dlc/dlc_japan.bin` | `windata/alldata.bin` |
| image base | `0x57592000` | `0xD39B7000` |
| Ketchup | title 99, version `VR-DISK`, disk 0 | title 101 `VR-DISK_US`, version `USA` |
| exe image offset | `0x000865F8` | `0x0000E5C8` |
| exe range | `0x99800` | `0x9C800` |
| menu overlay load address | `0x800C11A0` (`_bss_objend`) | `0x800C4350` |
| named stage entries | 105 | 105 |

Both executables are **zero-filled inside the collection** — the collection
loads them some other way — so neither can be read from the container. Integral's
is rebuilt byte-exact from the decompilation (`build.py --variant vr_exe`,
SHA-256 `c370f8e4…`) and USA's comes from a real disc image (`work/vrus.exe`).

**USA's VR executable is a five-language build.** Spanish, Italian, French,
German and English pools sit behind tables-of-tables with English first, and
GCL variable `0x11` selects the language (0 = English). `vr_exe.py` reads the
English pool; `vrlib.language_of()` recognises the same variable in the scripts,
masking with `0x00FFFFFF` because the encoded form is `0x02000011`.

### The mission windows (`en_missions`)

Every piece of in-mission text is a `chara 0xD44E` (`vrwindow`, `koba/vr/vrwindow.c`)
command inside a stage's `scenerio.gcx`. Its options are `-w` width, `-m` mode,
`-f` flags, `-i` indent, `-b` the text records, `-p`, `-s`; the overlay's
`vrwindow` draws `i_num` lines of `b_text[16]` at 18 px each and centres a
window of `i_num * 18 + 18`.

USA does not lay its stages out identically, and the shots were not taken in the
same order, so windows are matched by **content, not position**:

1. every USA window in every stage becomes a pool entry keyed by `norm_title` —
   the window's ASCII title lines reduced to uppercase alphanumerics;
2. a window is served from the **same stage** first, then from a pool-wide
   unique match, then — only for windows with no ASCII title at all — by
   position within the stage;
3. the taken bytes replace exactly `-w -m -i -b`; Integral keeps `-f`, `-p`
   and `-s`, so its own colours, flags and timing survive.

Result: **1808 of 1813 windows ported across 92 stages.** The five left alone
are `vrsave`'s one debug window and `vrtitle`'s four; USA carries the identical
Japanese there, so there is nothing to port.

### Script-local fonts, and why they had to be merged

Codes at or above `0x9A00` (after stripping the `0x6000` style flags) are not
Shift-JIS: they index a font stored **inside the script**, 36-byte 12×12 2 bpp
glyphs, at `index = a - a // 256` where `a = code - 0x9A00`. Integral's font
holds the glyphs its Japanese descriptions need; USA's holds the typographic
quotes its English uses. Blindly keeping either one produces mojibake.

`vr_windows.py` therefore merges: Integral glyphs still referenced by strings
that are *not* ported text keep the original font, USA glyphs referenced by
ported text are appended, and the ported strings' codes are rewritten to the
appended indices. Because pooled text can come from a different stage, each
ported record records which USA stage it came from so the right font is read.
26 stages end up with appended USA glyphs (5, 18 or 20 of them), 64 need no
local font at all, and `movie` keeps Integral's plus two.

### The VR KEY CONFIG on screen, and the highlight box (2026-09-07)

**First look at Integral's own VR KEY CONFIG**, with `DisableRAM = true` and
`DisableCDROM = true` so the collection's interception is out of the way. The
port is right: Integral's own screen draws (not the collection's Control
Settings panel), and all eight labels read English — `button type`,
`first person view` on both the bottom-row selector and the △ button, `weapon`,
`action`, `crawl`, `normal`, `reverse` — through all three button types, whose
rectangles the per-type function rewrites every frame. `key_syukan`'s +11 px
shift does its job: Integral's connector corner sits further right than USA's,
and the label keeps USA's gap from it instead of landing on the curve.

**One fault, spotted by the user: the selection highlight was the wrong size.**
On the `first person view` row the lit box stopped short and the hexagon's right
chevron hung outside the glow. Measured off paired shots (9 display px per game
px, x offset 480), reading the band inside the box but above the glyphs:

| | glow span | width |
|---|---|---|
| Integral, before | game x 10..101 | 91 |
| Integral, after | game x 12..125 | 113 |
| USA | game x 11..125 | 114 |

24 px short, which is exactly `112 - 88`: USA's `key_sykan` art is 112 wide
where Integral's Japanese is 88.

**Why the port missed it.** The eight labels are placed by `Init_Res` quads, and
those were all transplanted. The highlight is **not** an `Init_Res` quad —
Integral draws it with hardcoded arguments, `glow(work, x, y, w, h, 255, ...)`
at `0x800C23E4`, and the `key_sykan` row has **two** such call sites, one per
selection state, both carrying retail's `x = -149, w = 88`. Nothing in
`vr_kcgeom`'s quad model can see them, because they are not quads. They were
found by scanning the overlay for the `addiu a1/a2/a3/v0` argument shape and
diffing every hit against USA's overlay, which is how the pair at `+2234` and
`+2424` turned up alongside the untouched `key_button` pair at y −70.

The fix is four immediates in `vr_option.CALL_SITE_PATCHES`: `x` −149 → −148 and
`w` 88 → 112 at both sites, so the box spans −148..−36, the label quad's own
extent. The height stays Integral's 12 against USA's 13-tall art — that is
Integral's own and was not what looked wrong. The `key_button` row's pair is
left alone, since that label is 88 wide on both discs.

Rule 3 covers this without an amendment: it names "highlight boxes" among the
chrome that positions text.

**The general lesson, and it is the same one as the MOVIE captions:** a port
that transplants geometry has to account for every *kind* of primitive that
draws the thing, not just the kind it already models. The quads were complete
and correct, and the screen was still wrong, because a second mechanism drew
part of the same widget. Both times the missing piece was hardcoded arguments to
a helper rather than data in a table.

**Still Japanese on that screen, and correctly so:** the help line under the
controller (`主観モード時の操作 ： 通常操作`, `ボタン設定 ： タイプB`, and the
rest of records 17..25). USA leaves those records **empty** — its own KEY CONFIG
draws nothing there — so under the no-translation rule they stay Integral's.
Same call as the READ MISSION LOG? caption.


### The MOVIE selection captions (`vr_en_movie`, 2026-09-06, solved 2026-09-07)

The one line under the thumbnail on EXTRA -> MOVIE. The clip *descriptions* were
already English (they come from `vr_en_missions`); these short captions were not.
They live in `OPTION 't'` of the `movie` script's `CMD 9906` whose chara is
`0xFAA8` - **not a top-level command**: it sits inside an enclosing command's
`-e` option wrapped in an ARG, so it is found with `vrlib.walk_commands`, not by
scanning `parse_arg`'s result. `vr_movie.py` builds it.

| | Integral, one record each | USA |
|---|---|---|
| TGS clip A | 36 B, `東京ゲームショウ'98春 出展映像A` | **two** records: 25 B `Exhibition clip {"}A{"}for` + 33 B `the Tokyo Game Show, Spring '98.` |
| TGS clip B | 36 B, the same with `B` | two records, likewise |
| E3 clip | 23 B | 26 B `Video clip from E3 (6/97)` - **one** record |
| | 4 records, `-t` = 105 bytes | 6 records, `-t` = 156 bytes |

**The arithmetic is the verification.** Replacing each Integral record with USA's
counterpart(s) changes the record bytes by +24, +24 and +3 = **+51**, and
105 + 51 = 156 exactly, so a correct full edit makes Integral's `-t` payload
*byte-identical to USA's* - which `verify()` asserts, along with the script
round-tripping and the stage keeping its 251,904 bytes in place.

#### The actor is the engine's generic numbered-text module

The decomp names the chara but does not implement it -
`source/stagevr/movie.c` has `{ 0xfaa8, (NEWCHARA *)0x800d15b8 }` and
`include/charalst.h` points at `chara/movie/movie.c`, **a file that does not
exist** - so it was disassembled. Integral's `movie` overlay loads at
**`0x800C11A0`** (the chara table's `0x800D15B8` lands on a function prologue at
overlay `+0x10418`, which fixes it), and the actor's name string at `+1CDD8`
reads `'movie.c'`.

What it turned out to be is the same numbered-text module `abst.c` implements in
the decomp - the mission log's own text engine, down to
`work->field_51C[index].string` - linked into this overlay twice, and the
captions use the first copy. That is the key to the whole thing, because
**reading `abst.c` gives the C for code with no source**:

| overlay | address | what |
|---|---|---|
| `+CFFC` | `0x800CE19C` | **helper1**: allocate line `i`'s KCB - a 64-word x 21-row rect at (832, 256 + 21i) with the CLUT on the band's last row, wrapping to x 896 at y+21 >= 512 - then `font_set_kcb(kcb, -1, -1, 0, 6, 2, 0)`, a buffer, and `font_set_color(kcb, 0, table[i].color, 0)` |
| `+D170` | `0x800CE310` | **helper2** (`abst.c`'s `func_800C47A8`): `if (string) { font_print_string; font_update; font_clut_update; }`, store the width and `max_height - 1`, place from the position table (`num 1` = centre on x, y), then `string = NULL` and **`num = 1`** - the slot is now live |
| `+D2B4` | `0x800CE454` | **the draw** (`abst_line_sprt`): walk up to 12 slots and emit four `SPRT`s for **every** slot whose `num == 1`, plus two tpage prims. Its only difference from USA's is the immediate `0x340` (832) against `0x300` (768) |
| `+D5CC` | `0x800CE76C` | `font_set_color(&kcb[i], 0, colour, 0)` + `font_clut_update` |
| `+D628` | `0x800CE7C8` | per frame, over all `count` slots: `+D5CC(t, i, 0)` - **every caption starts each frame invisible** |
| `+D684` | `0x800CE824` | **`highlight(work, i)`**: `+D5CC(work + 0x2c, i, 0x6739)` - light caption `i`. One call site |
| `+FBA8` | `0x800D0D48` | `Act` - per frame; calls `+D628`, four update functions, then the draw |
| `+FE44` | `0x800D0FE4` | **the builder**: `GCL_GetOption('t')`, then `GCL_NextStr`/`GCL_GetString` **until NULL, at most 24**; per record it stores the string, clears `num`, calls helper1, helper2 and `+D5CC(t, i, 0)`; finally stores the record count at `work+0x45C` |
| `+FF58` | `0x800D10F8` | the unlock gate (`count / 3` vs 45 and 75) - reads `-m`, nothing to do with captions |
| `+10418` | `0x800D15B8` | `NewChara` - `GV_SetNamedActor(work, Act, kill, "movie.c")` |

So **the number of lines on screen is simply how many slots are lit**, and the
draw imposes no limit at all.

#### The line count: one word, and USA's own arithmetic

The two discs differ in exactly one place, in the clip-selection code:

    Integral +F448   lhu  v0, 0x1e(s7)      ; the per-clip unlock bitmask
                     srav v0, v0, s6        ; s6 = the clip index
                     andi v0, v0, 1
                     beqz v0, done
                     lw   a0, 0x48(sp)      ; work
                     jal  highlight         ; highlight(work, clip)
                     move a1, s6
                     j    done

    USA      +F4F0   (the same bitmask test, then an extra
                      `lw v0, 0x2c(s7); bnez v0, done` - USA hides the caption
                      while the carousel is animating)
                     sll  s0, s6, 1
                     lw   a0, 0x48(sp)
                     jal  highlight         ; highlight(work, clip*2)
                     move a1, s0
                     lw   a0, 0x48(sp)
                     jal  highlight         ; highlight(work, clip*2 + 1)
                     ori  a1, s0, 1

USA lights a clip's two lines; Integral lights its one. USA's sequence is 14
words and Integral's block has 11, so it cannot be written in place. Instead the
call site keeps its shape and **only its target changes** - one word - and a
16-word stub does the doubling by calling retail's own `highlight` twice:

    two_lines(work, clip):  highlight(work, clip*2); highlight(work, clip*2+1)

`a0`/`a1` do not survive a call, so `work` goes to the frame's argument-save area
and `clip` to `0x10(sp)`; `ra` is restored only after the second `jal`, which
writes it; two `nop`s cover R3000 load-delay slots (`lw` then use, and `lw ra`
before `jr ra`). `vr_movie.two_line_stub()` emits it from named encoders and
`verify_overlay` re-reads it out of the built stage.

**Where the stub lives, and why that is safe.** It is appended to the overlay
payload at `+1DFB8` (RAM `0x800DF158`). Three facts make that a non-event:

- **the stage does not grow.** The payload is 122,808 bytes and `pack_stage`
  pads each payload to a sector, so its 60 sectors already hold 122,880: the 64
  bytes land in padding the stage carries either way. No payload moves, the
  stage keeps its 123 sectors and its LBA, and no collection patch is orphaned
  (see "No stage is ever relocated").
- **the RAM is overlay space by construction.** Every VR stage overlay loads at
  `0x800C11A0`; USA's own `movie` overlay is 132,384 bytes there (9,576 past
  Integral's end) and Integral's `init` runs to `0x800EA1EC`. Nothing else can
  own `0x800DF158`.
- **six lines is what retail dimensioned the module for.** The slot array ends
  at `work+0x2c+0x434+24*224 = work+0x1960`, exactly where `Act`'s two tpage
  primitives sit, and the builder's own cap is 24 records; the draw walks 12.
  Their KCBs go to VRAM x 832..848, y 256..382 for six lines, inside the arena
  the allocator itself treats as its own (x 832, then 896, y 256..511), and no
  texture in the stage's DAR - **identical in both discs** - is anywhere near
  x 832. Retail already puts four boxes there.

#### The position table is the other half

helper2 places each line from a 12-byte table - `{int num; short x; short y;
int color}`, exactly `opt.c`'s model - at overlay `+82B4` (`0x800C9454`;
USA's is `+82BC`, `0x800CC60C`). The two differ in **y alone**:

    entry        0    1    2    3    4    5
    Integral   208  208  208  196  196  196
    USA        196  208  196  208  196  208

USA alternates because each caption is a pair of lines; Integral repeats one row
because each is one line. `num` (1 = centre on x), `x` (160) and the colour
(`0x6739`) are already identical, so the port writes four halfwords. Note what
this does to the E3 caption: USA puts its single line on the **upper** row
(entry 4, y 196) where Integral's sits at 208, so it moves up 12 px to USA's own
placement - text placed as USA places it, which is the rule.

#### The widths are measured, because wrapping here is not cosmetic

The caption KCB is a 64-word rect, so `c_width = rect.w * 4 / 12` is 21 cells =
252 px and `font_print_string` wraps at `kcb->width - 12` = **240 px** - the same
budget as the main game's option entries, where a wrap lands the continuation on
the CLUT row and writes past the buffer ("Wrap width"). Measured against
Integral VR's own `font.res` (found by the same `>II 392, 2306` signature, in its
`init` stage), USA's lines are 132, 133, 144 and **201 px** - the longest,
`the Tokyo Game Show, Spring '98.`, clearing the limit by 39 px and the one-byte
`max_width` ceiling by 54. `check_widths()` asserts it at build time.

**Hence two builds, and the full one now ships.**

- **`INTEGRAL_vr_en_movie.ppf` - the whole port, DEPLOYED 2026-09-07.** USA's six
  records, USA's position table, and the retargeted `jal` plus the stub. All
  three captions in USA's English, the two TGS ones as USA's two lines.
- **`INTEGRAL_vr_en_movie_e3.ppf` - the E3 caption alone, the fallback.** One
  record for one record: retail's record count, order and overlay untouched, so
  it is correct under any mapping and needs no code change. It shipped from
  2026-09-06 (verified on screen) until the full build replaced it, and it is
  what to fall back to if the stub ever has to come out. It leaves both TGS
  captions Japanese.

The two **overlap**, so exactly one may sit in `mods/INTEGRAL/VR-DISK/`:
Ketchup applies a folder in name order, so `..._movie_e3.ppf` would land last
and overwrite part of the full build. `vr_movie.py --deploy` installs the full
one and moves the other to `work/` with a `.was-deployed` suffix.

**THE LOCAL-FONT TRAP, caught on screen.** The first deployment of the full
build rendered `Exhibition clip` followed by two kanji instead of the quotes.
USA's records reference script-local font glyphs `{q01}`/`{q02}` - its
typographic quotes - and at those indices **Integral's** font holds different
glyphs, exactly the mojibake this README already warned about under
"Script-local fonts". Copying a USA record verbatim is never safe.
`vr_movie.font_remap` fixes it by matching glyph **bitmaps** between the two
fonts: USA's two quotes turn out to be already present in Integral's merged font
at indices 14 and 15 (codes `9A0E`/`9A0F`, appended by `vr_en_missions` - the
"movie keeps Integral's plus two" note), so no font surgery is needed, only a
code rewrite. Matching bitmaps rather than hardcoding those indices keeps it
correct if the merge ever changes. After the remap the `-t` payload matches
USA's length and differs in exactly 4 bytes: the two codes in each TGS first
line. `verify()` compares against the remapped expectation, not raw USA bytes.

**THE COMPOSITE TRAP, for the third time.** `vr_en_missions` already rebuilds the
`movie` stage - 2,219 bytes of it, including the clips' English descriptions and
the merged script-local font - so the first `vr_movie` build, made from retail,
**overlapped it across all 704 of its bytes** and would have reverted those
descriptions. It was caught by checking byte overlap between the deployed PPFs
before trusting the build, and never reached a running game. `composite()` now
builds on retail *plus every deployed VR PPF's writes to this stage*, excluding
this patch's own two outputs so the builder can never consume itself. Same trap
as `menu.ppf`'s chain records and `optsctext`'s pinned input: **when another
patch already owns a stage, build from what the game will actually see.**
`inplace_records(..., merge_gap=0)` then keeps each record to the bytes that
really change; with the default 64 they span unchanged bytes and the outcome
would depend on which PPF Ketchup applied last. Residual overlap is inherent -
both patches write the same script, 686 of this patch's 753 bytes - and this one
must win, which it does because Ketchup loads a folder by name and
`..._missions.ppf` sorts before `..._movie.ppf`.

**Resolved 2026-09-07: the stage now has one owner and the two patches share no
bytes at all.** `vr_windows.py` still ports the `movie` stage like any other -
its clip descriptions are mission windows - but writes none of its records,
handing the finished stage to `vr_movie.py` as `work/vr_movie_base.bin`
(`vr_windows.HANDOVER`). `vr_movie` emits against **retail** and owns the stage
outright. Overlap went from 686 bytes to **0**, the dependency on a deployed
folder (`INTEGRAL_ENGLISH_VR_PPF_DIR`) is gone, and `rebuild.py` now refuses any
VR overlap instead of allowing this one pair.

It was safe to do because it is checkable by effect rather than by faith:
applying the new pair leaves the disc byte for byte as the old pair did. The two
sets differ at 136 positions **as files**, and every one is a byte equal to
retail either way, written redundantly by the old set inside a merged run.
Positions where the disc would actually differ: **0**. Both files must be
replaced together; neither works beside the other's old copy.

#### Three corrections, and the one that mattered

This section was wrong twice on the mechanism and once on an address, and each
error cost a build and a play test. They are kept because the shape of the
mistake is the lesson.

1. **The record count alone** (2026-09-06). Six records with retail's single
   highlight draws one line per clip, so clip A got a sentence fragment and the
   other two clips someone else's line - exactly what the shots showed. Right
   data, wrong count of lit slots.
2. **The position table alone** (2026-09-07). Writing USA's y values moved the
   rows on screen exactly as predicted - TGS B a row lower than the others - and
   still gave one line each. That was read as *"the layout is not data-driven,
   so nothing here is data"*, and the table was nearly reverted. Both halves
   were needed: the table places the lines, the code decides how many there are.
   "Identical code plus USA's data must give USA's behaviour" was refuted by
   experiment, and the right conclusion was not "so it is all code" but "so
   something outside the functions I diffed decides the count".
3. **The address underneath both.** `Act`'s tail reads a global at `0x800A9580`,
   scales it by 4 and loads `+0x1EC` from a table, and that was read as
   `captions[clip]` - which made `record = clip` look structural in the draw and
   sent the search into the draw and the "five per-record helpers". The decomp
   names it: `abst_sprt` indexes the same way with **`GV_Clock`**, the frame
   parity, and the table is the ordering table per frame. The draw is handed an
   **OT, not a string**, and it draws every lit slot. Nothing there ever limited
   a clip to one line.

The lesson that generalises: **when a disassembled function indexes a global,
find the same pattern in the decomp before naming the global.** One `lw` read as
a clip index sent two sessions looking for a line count in the drawing code,
when the module was the mission log's own text engine all along and the count
was two `jal`s in the input handler. The `f`/`m` options were never the lead
either - `-m` is read by the unlock *gate* at `+FF58` - and they stay Integral's,
being GCL variable references that would point Integral at slots its own scripts
never write:

    OPTION 'f'   Integral  VAR 11 00 04 80      USA  VAR 11 00 04 82
    OPTION 'm'   Integral  VAR 12 00 04 82      USA  VAR 12 00 04 81

The `-p` option's two procs (`A878`, `0EF7`) are the same ids in both and are
the stage transition procs (they carry the `"movie"`/`"vrtitle"` names).

**One structural note worth keeping:** the two `movie` stages are not parallel -
USA's carries an extra `cr` tag and ~100 KB more cache data than Integral's - so
nothing here could have been done by wholesale stage substitution.

**VERIFIED ON SCREEN 2026-09-07**, all three clips: TGS ROLL A reading
`Exhibition clip “A” for` / `the Tokyo Game Show, Spring '98.` on two rows,
TGS ROLL B the same with `B`, and E3 `Video clip from E3 (6/97)` on one - right
text, right clip, and the typographic quotes rendering as quotes, so the
local-font remap is right too.

#### The EXIT box had to move, and that was a decision, not a detail

Measured off those shots (9 display px per game px, x offset 480): the EXIT box's
bottom border sits at game y 201-202 and the caption's **first line's ink runs
201-213**, so with USA's rows they overlapped by two rows; line 2 (215-225) and
E3's single line (203-213) were clear.

USA avoids the collision **twice over**, and the user confirmed both halves from
their own disc: USA VR's caption face is **shorter**, and its **EXIT box sits
higher**. The face stays Integral's — a font face is its own art, not text (see
"The white caption font differs between the two VR discs"), and line 1's ink is
12 rows only because it is topped by the two 12x12 script-local quote glyphs,
where the same font draws the option screen's `Sound setting.` in 8. So the room
comes from the box, which is **chrome that positions text** under the user's
rule 3.

**Asked, and approved 2026-09-07** — and recorded as an exception, because the
user named the default in the same breath: *"Usually we skew toward the Integral
visuals."* Nothing else about the box changes; its art, colour and size are
Integral's.

**Where the position lives.** Every widget in this stage is built by
`Init_Res(slot, 0, GV_StrCode(name), y)`, which centres the texture horizontally
from its own header and offsets it by `y` from screen centre 120. Listing all 28
such calls in both overlays gives y values **identical between the discs except
one**:

| resource | Integral | USA |
|---|---|---|
| `d3_sp_movie_head` | −82 (screen 38) | −82 |
| `d3_mv_base` | −5 (115) | −5 |
| `d3_mv_ya_l` / `_r` | −60 / +55 | −60 / +55 |
| **`sp_exit`** | **+70 (screen 190)** | **+66 (screen 186)** |

The measured retail top border is exactly 190, which confirms the model. One
immediate carries it — a scan of every `addiu`/`ori`/`slti` in either overlay
finds precisely one instruction holding 70 (Integral) or 66 (USA) — so the port
writes `addiu a3, zero, 66` at overlay `+F128`, USA's own value, and
`exit_patch` asserts USA's overlay really says it.

**And the selection highlight moves with it.** The widget is a single object at
`work+0xF0`, and both things that light it anchor on that object rather than on
a coordinate of their own: `+10198` attaches the `cur_l` cursor as
`f(work+0xF0, strcode, 1, 1)`, and `+F38C` sets its lit state as
`f(work+0xF0, 0xFF, 1)`. Those two call sites, and all ten references to
`work+0xF0`, are byte-identical between the discs. So USA's entire widget — box,
label and highlight — sits at 186 because of that one immediate; nothing else
could position the highlight, or USA's own would be misplaced. That is the
argument for why moving one immediate is sufficient, and it is evidence rather
than assumption because it is USA's own working screen that depends on it.

Deployed 2026-09-07 with 3 rows of clearance under the box instead of a 2-row
overlap. **The box and its highlight are the one thing here not yet seen on
screen.**

Checks that passed on the deployed result: the merged local font is **identical**
(540 bytes / 15 glyphs) and so is the proc table; the `-t` payload equals USA's
in length and differs in the 4 remapped glyph bytes; the overlay differs from
retail in exactly five words (the `jal` and four position-table halfwords) plus
the appended stub; the stage stays 251,904 bytes and 123 sectors;
`ppfcheck --deployed` is clean over 27 files and no two of them overlap at all
since the ownership split of 2026-09-08 (this patch shared 686 bytes with
`vr_en_missions` until then, and depended on Ketchup's name ordering); and
applying every deployed PPF in
Ketchup's own order and disassembling the result gives `+F464 jal 0x800DF158`,
the stub verbatim, `addiu a3, zero, 66` at `+F128` for the EXIT box, and the six
records at y 196/208/196/208/196/208 - clip A lines 0 and 1, clip B lines 0 and
1, the E3 caption and its empty second line.

### What `vr_unlock` does and does not open (2026-09-06)

The test aid patches three predicate instructions in `selectvr` and that is all
it governs: **the mission menu**. Verified against the deployed PPF — the two
cleared-predicates' `and v0, …` become `addiu v0, zero, 1` (stage `+1B400`,
`+1B488`) and the score's `subu v1, a2, a0` becomes `addiu v1, zero, 0x100`
(`+1BB88`), exactly as the module documents; by emulation that is 46 → 361 of
373 menu items.

**It does not open the EXTRA movies** - those are gated in the `movie` overlay
instead, which is why the mission aid never touched them and why USA's
unpatched VR disc showed `???` there too. **Found and solved 2026-09-06**, see
"Unlocking the EXTRA movies" below.

**Saving with the aid deployed is safe** — corrected 2026-09-06, having first
told the user otherwise. Nothing in it writes progress: the save is built from
the VRAM bitmap, which only a genuinely cleared mission ever touches, so a save
written while it is deployed still records real progress only. Deleting the PPF
relocks exactly as before.

### Unlocking the EXTRA menu's items (`vr_unlock_extras`, 2026-09-07)

A third gate, found when the user reported only MOVIE, ALBUM and EXIT on the
menu - leaving three of `vr_en_title`'s four ported help lines unreachable,
PHOTOGRAPHING not even present to select. **Three separate gates on one disc,
one per aid**: the mission menu in `selectvr`, the clips in `movie`, the menu
items here in `vrtitle`.

The menu keeps a visibility bitmask at `work+0x1e` and builds it from progress
flags in the word at `+0x1a1c`, one item at a time, each test also bumping the
item count in `s2`:

    +0639C  lhu   v1, 0x1e(s6)      ; the mask so far
    +063A4  ori   v0, v1, 1         ; MOVIE - unconditional
    +063AC  lw    v0, 0x1a1c(s1)    ; the progress word
    +063B4  andi  v0, v0, 3         ; PHOTOGRAPHING
    +063D4  andi  v0, v0, 0x10      ; ALBUM
    +063F8  andi  v0, v0, 0x40      ; PocketStation

Each item's block is `lw` / `andi` / `beqz` / set the bit / bump the count. The
aid forces the three **tests** to pass (`andi` -> `addiu v0, zero, 1`) rather
than writing the mask, and the reason is the count: the same blocks that set the
bits increment it, so a mask forced from outside would leave the menu's layout
and navigation disagreeing with what is drawn. It writes no progress - the mask
is rebuilt from the progress word on every entry - and it patches the `vrtitle`
overlay, which `vr_en_title` does not touch (that one rewrites the script
chunk's records), so the two cannot interfere.

With it, all five items appear. PocketStation's help line is Japanese on
purpose: Integral's fifth item is PocketStation where USA's is STAFF CREDIT, so
USA's `See the staff credits.` is not its translation.

### Unlocking the EXTRA movies (`vr_unlock_movies`, 2026-09-06)

The MOVIE clips are gated by the `movie` overlay's own progress score, not by
the mission bitmap `vr_unlock` patches - which is exactly why that aid left them
`???`, and why USA's unpatched VR disc did too. Found by disassembling around
the caption command's `-m` read, at Integral movie overlay `+FFA0`:

    lw    s1, 6608(s1)       ; a progress count
    ori   v1, v1, 0x5556     ; v1 = 0x55555556
    mult  s1, v1             ; the standard signed divide-by-3 ...
    sra   v0, s1, 31
    mfhi  t0
    subu  v1, t0, v0         ; ... so v1 = count / 3      <-- the score
    slti  v0, v1, 45         ; gate 1
    slti  v0, v1, 75         ; gate 2   -> bits in work+30

`count / 3` against thresholds is the same scoring pattern `vr_unlock.py`
documents for the mission menu, so the fix is the same single instruction:
`vr_unlock_movies.py` replaces the `subu` with `addiu v1, zero, 0x100`, above
both thresholds. **Verified in game: all three thumbnails appear.** It writes no
progress - the score is recomputed from the count every run - so deleting the
PPF relocks. It patches the `movie` **overlay**, which `vr_en_missions` does not
touch (that patch only rewrites the script chunk), and the builder asserts both
that and that no deployed PPF writes those 4 bytes.

### The white caption font differs between the two VR discs — and it is not ours

Noticed by the user 2026-09-06. **No VR patch writes a single byte to the
`init` stage** (sectors 1..288, image `0x001373A0..0x001DC9A0`), which is where
the global font lives; mapping every deployed VR PPF's records onto the disc's
named stages gives `vr_en_missions` → the 92 mission stages, `vr_en_option` →
`option`, `vr_en_title` → `vrtitle`, `vr_en_camsave` → `camera`,
`vr_unlock` → `selectvr`, and `vr_en_items`/`vr_en_savemsg` → the executable.
Our only font work is the *script-local* 12×12 sets at codes ≥ `0x9A00`
(previous section), which hold typographic quotes and the like, never the Latin
alphabet. So the difference is between the retail discs.

**Why is UNMEASURED, deliberately.** The obvious guess — Integral being a
Japanese release, its half-width Latin glyphs are drawn in a Japanese font's
style — is exactly the claim that was made and *withdrawn* on the main game:
"Integral's Latin glyphs are wider than USA's" drove a margin redesign that was
unnecessary, because measuring gave identical widths (261 vs 261, 306 vs 306)
and the error was comparing a computed *advance* against a measured *ink
extent*. An attempt to locate USA VR's width table by structure returned 120
false candidates, so nothing here is asserted. To answer it: render the same
string from both discs' fonts and diff the glyph bitmaps. Scope note if they do
differ — a font *face* is Integral's own art, not text, so the rule keeps it and
porting USA's would be an art port under the 2026-09-03 amendment: ask first.

### No stage is ever relocated

Relocating a stage orphans the collection's own patches to it — the KEY CONFIG
"doorbell" lesson from the main game. Every VR stage is therefore rebuilt **in
place**: `repack_stage` 4-aligns the chunk, and where the new script is shorter
the stage is padded back to its original sector count (`fit_in_place`). Ten
stages shrank by one sector and were padded; **none grew**. PPF records are
merged when they are within 64 bytes of each other (`merge_gap`).

### The executable's pools (`en_items`, `en_savemsg`)

`vr_exe.py` owns every byte of five arenas and rewrites them whole, so no
string can overrun its neighbour:

| pool | Integral table | arena |
|---|---|---|
| items | `0x8009C11C` | `0x80010EA0`–`0x80011668` |
| weapons | `0x8009C304` | `0x800116DC`–`0x80011B60` |
| capture modes | — | `0x80011F0C`–`0x800120C0` (indices 1 and 9 kept) |
| save messages | `0x8009C884` | rewritten in place |
| load messages | `0x8009C8B4` | rewritten in place |

USA's tables are at `0x8009F0DC`, `0x8009F300`, `0x8009FA0C` and `0x8009FA3C`.
Two traps: the weapon arena runs on into debug strings (`HERE %d` and friends),
so it ends at `0x80011B60`; and a USA item header is `\xB0\x14`, not `\x90\x14`,
because of a style flag — `titled()` masks with `0x9F`. `frozen` items and the
MP5 pointer (`0x8009C17C`, `0x800A9220`) have no USA counterpart and are left.

Save and load indices 1 and 9 stay Japanese: USA draws nothing there, the same
rule as the main game's `savemsg`.

### The option screen and KEY CONFIG (`en_option`)

One PPF for one stage, because both live in it.

*Help lines.* `chara 0x976C` option `-e`, 31 records, index for index with USA.
Records 1, 2, 3, 5, 6, 12 and 26 take USA's English (`Sound setting.`,
`Vibration setting.`, `Use directional buttons to test.`, `Key configuration
setting.`, `Return to the title screen.`). Everything USA leaves empty — the
colon at 7, `オン`/`オフ`/`ステレオ`/`モノラル` at 8–11, the unused brightness
paragraph at 13–16, the KEY CONFIG rows' lines 17–25, the language rows 28–30 —
stays Integral's. The chain grows 49 bytes inside its existing sector, so the
stage keeps its 73 sectors.

**Seen on screen 2026-09-06, and it took three faults to get right.** The first
build crashed the stage outright (the DAR alignment invariant above). With that
fixed, the help lines showed but were wrong in two more ways, and both were the
main game's problems recurring:

- **A doubled sentence.** USA holds `Use directional buttons to test.` at
  records **3, 12 and 26**, but Integral draws record 3 at the row-*label*
  position and 12/26 at the sentence position, so porting 3 drew a second full
  sentence over the first. This is the main game's 2026-09-02 decision exactly:
  record 3 (`振動テスト`, the row's own name plus a sentence USA's line already
  covers) **stays blank**. `optlabel2.py` writes `b' '` for the main game;
  `vr_option.py` now has `BLANK = {3: b' '}` for the same reason.
- **Integral's colon and value drawn beside the English.** Records 7/27 (a
  colon) and 8–11 (`オン`/`オフ`/`ステレオ`/`モノラル`) are Integral's own and USA
  leaves them empty, so they stay Japanese — but Integral *lit* them next to the
  ported line, giving `Sound setting.：ステレオ`. The lever is the one the main
  game used for entry 27: the state switch calls a colour helper per entry and a
  reset loop sets **every** entry to colour 0 first, so an entry the switch skips
  is invisible. No text changes. `LIT_PATCHES` NOPs the helper `jal` for the
  colon and retargets each conditional value's `j` past the shared call, on the
  three rows that now carry USA's English — and leaves it alone on the
  Integral-only row and the two KEY CONFIG sub-rows, whose lines are still
  Japanese. The switch is at overlay `+EE0..+1240` (reset loop `+F44`, helper
  `+934`, dispatch `jr v0` at `+F94`); `vr_option.py`'s docstring maps every
  state.
- **Off-centre text.** `opt.c`'s placement: `num == 0` draws at `(x, y)`;
  `num == 1` centres (`x - max_width/2`, `y - max_height/2`). Retail Integral VR
  is `num 0` with an x hand-set per **Japanese** string (76, 100, 42, 88, 94,
  122 …) at y 190; **USA VR is `{1, 160, 196}` for all 31 entries.** An English
  line of a different width in a Japanese slot therefore sits off-centre.
  `POS_PATCHES` gives each ported entry USA's own `num`/`x`/`y` — the same
  substitution the main game's table already carries (`{1, 160, 200}`, and
  `opt.c` comments it). Table at overlay `+0x10`, 31 × `{int num; short x;
  short y; int color}`. Measured on the five shots afterwards: every caption's
  ink centre is within **0.3 game px** of screen centre.

**What this says about the verification.** All three faults shipped past static
checks that could not see them: the quads and per-type rectangles were confirmed
by *re-executing the transplanted function with a simulator that reads the same
offsets the code writes* — which proves the arithmetic, not that the result is
right for Integral's screen — and the DAR was checked for **fit** but not for
alignment. Screen-visible properties need a screenshot or a retail control, and
the retail control is what made the alignment fault obvious the moment it was
measured.

*KEY CONFIG.* Same program as the main game's `opt.c`, so the same two authorities
apply and `vr_kcgeom.py` reads both from the binaries rather than measuring
screenshots:

- `Init_Res(work, strcode, poly, x0, sp+16 y0, sp+20 x1, sp+24 y1, sp+28 abe,
  sp+32 orient)` sets every label's first quad. The delay-slot `sw` to the stack
  is an argument too, and missing it loses `orient`.
- A per-button-type function — entered through `lw rX, 9760(a0)` — rewrites the
  four movable labels' rectangles every frame, and *that* is the authority for
  `key_action`, `key_buki`, `key_hohuku` and `key_syukan`. It ends where the
  next function's `lw rX, 9764(a0)` begins; stopping at the first `jr ra` cuts
  it short, because it has three early returns.
- POLY base is `work + 1612`, 40 bytes per poly (`x0 +8`, `y0 +10`, `x1 +16`,
  `y1 +18`, `x2 +24`, `y2 +26`, `x3 +32`, `y3 +34`); the labels are polys 10–13.

The port transplants USA's per-type function (`+0x15B4`–`+0x181C`) over
Integral's (`+0x1528`), retargeting its two `j` instructions and nopping the
slack, and rewrites 21 call-site words. `key_syukan` carries the same **+11**
horizontal shift the user approved for the main game on 2026-09-03, applied at
both the type-1 block and the common tail (the constants `49` and `137` each
appear twice; an immediate search masked `0xFFE0FFFF` finds both, and
`type_rects` verifies the result).

*The texture archive.* USA's eight labels are larger than Integral's and did not
fit: +824 bytes against 484 bytes of slack. `pcx4.py` gained a lossless
8-bit codec (`decode8`/`encode8` around `PcxInflate8`: one RLE stream, tail
`0x0C` plus palette) and every texture in the archive is re-encoded, which
recovers 263 bytes from the 4 bpp images and 163 from the 8 bpp ones. The DAR
goes 120 348 → 120 754 bytes inside its 120 832-byte slot, and `kcplace.py`
allocates the new VRAM and CLUT positions.

### The EXTRA menu (`en_title`) and the camera (`en_camsave`)

`vrtitle`'s `chara 0x5667` option `-t` holds 11 records. Records 2–5 are the
help lines for EXIT, MOVIE, PHOTOGRAPHING and ALBUM and take USA's English.
**Record 6 is not USA's `See the staff credits.`** — Integral's fifth item is
PocketStation (its texture `0x29A8` reads "PocketStation" where USA's reads
"STAFF CREDIT"), so USA's line is not its translation and record 6, the
PocketStation prompt and its はい/いいえ (7–9) all stay Japanese.

The camera overlay's message table sits at overlay `+0x608` (save), `+0x638`
(load), `+0x668` (prompts) and `+0x708` (photo) on Integral, and `+0x7A8`,
`+0x7D8`, `+0x808`, `+0x8E4` on USA. Strings are packed into the pool
`0x0CB1C`–`0x0CD08`; 429 of its 492 bytes are used. The empty string must be
placed at the **start** of the pool — writing it later lets a following string
overwrite it.

### Numbers that differ between the two versions

Three windows were measured (2026-09-07, static analysis of the stage data)
to state different values in the two versions. Where the count of numbers
matches, the USA sentence is taken and **Integral's numbers are substituted
into it**, so the text never contradicts the disc it runs on:

| window | Integral | USA | on-screen, 2026-09-11 |
|---|---|---|---|
| SNEAKING MODE / NO WEAPON LEVEL 10 | 25 | 35 | **does not match** — the live window carries no number at all; the two games' text is identical |
| SNEAKING MODE / SOCOM LEVEL 03 | 40 | 43 | **does not match** — the live window reads `Enemies 3` on both discs |
| WEAPON MODE / GRENADE LEVEL 02 | 5 | 4 | **confirmed** — `Targets 3` is unchanged (identical on both); the substituted number is the fuse-timer sentence, `Grenades explode in 5 seconds` (Integral) against USA's own `4 seconds` |

**Grenade exception, 2026-09-11:** that third row is now corrected to 4 by
`vr_grenade.py`, including the original Japanese briefing. `vr_windows.py`
applies the same correction after the normal number substitutions, changing
only the five duplicate fuse digits. The other two cases retain their policy.
See "Standalone VR grenade delay correction" above.

**Only the third row is verified against what a player actually sees.** The
first two were checked against a real gameplay window on both discs
(SwanStation, both games, 2026-09-11) and neither shows the claimed numbers —
GRENADE LEVEL 02's own count (`Targets`/`Enemies`) is unchanged in all three
rows, which is what misled the original read: the differing digit for
GRENADE LEVEL 02 sits inside the sentence, not the count line, and the other
two rows' claimed digits do not appear on screen anywhere. Two explanations
are open and neither is confirmed: the 2026-09-07 measurement may have read a
non-live template copy of that key (Integral's stages carry every mission a
stage family can host, not only the ones a real playthrough reaches — see
"Integral's scripts are templates" above), or the figures may simply be
wrong. **Treat the first two rows as unconfirmed until `vr_windows.py`'s own
per-stage report is checked against which copy is actually reachable in
play.**

Three more windows differ in *count* and are left as USA wrote them, because a
count mismatch means the digits are not the same quantity. Number matching
ignores glyph codes (`<XXXX>`), skips the two title lines, and requires the
digit to have no letter beside it — otherwise `C4` and `E3` are read as
numbers.

### Deferred, with reasons

| item | why |
|---|---|
| ~~the `movie` stage's `-t` titles~~ | **ported 2026-09-07.** USA has 2 records where Integral has 1, and the count was two calls to `highlight` in the clip-selection code, not data — a 16-word stub in the overlay's padding; see "The MOVIE selection captions" |
| the camera's EXORCISE textures | image work, not text |
| the staff-credit roll | a USA-only feature; Integral has PocketStation there |
| the PocketStation help line and prompt | Integral-only, no USA counterpart |
| `vrtitle`'s four debug windows, `vrsave`'s one | USA carries the identical Japanese |
| `vrtitle`'s memory-card captions (the CLEAR DATA screen: `セーブファイルがありません。` and its 23 siblings) | USA's `vrtitle` table carries the identical Japanese (2026-09-11); the same module in `vrsave` and `selectvr` IS English in USA and is ported by `vr_en_memcard` |
| MP5, frozen items, the mine-detector line | no USA text exists |

### Sweep: is any VR text still Japanese? (`vr_sweep.py`, 2026-09-06)

`vr_sweep.py` rebuilds every VR stage as the game will see it — retail bytes
with all six deployed PPFs applied — and groups every remaining game-encoded
GCL STRING record by the command that owns it, beside the same tally for USA's
disc. Over all 105 stages:

| | game-encoded records | plain English |
|---|---:|---:|
| Integral, patched | 222 | 10 809 |
| USA VR Missions | 940 | 10 344 |

The ported disc carries **fewer** undecoded records than USA's own, because
USA's 940 include the four other languages' accented text. Integral's 222 break
down as:

| owner | count | what |
|---|---:|---|
| `chara D44E` (`vrwindow`) | 192 | **181 are English with local-font glyphs** — typographic quotes around `lock-on`, `THE TRUTH IS OVER THERE`, `Tokyo Game Show, Spring '98`. Codes at or above 0x9A00 index the script's own font, which the decoder cannot read, so they only look Japanese. The other 11 are `vrsave`'s one debug window and `vrtitle`'s four, identical in USA |
| `chara 976C` (option) | 22 | the Integral-only option rows USA leaves empty; the 9 English beside them are the 7 ported lines and the colon |
| `chara 5667` (`vrtitle`) | 4 | PocketStation's help line and prompt and its はい/いいえ, beside the 4 ported EXTRA lines |
| `chara FAA8` (movie) | 3 | the `movie` stage's titles — deferred, USA has two records where Integral has one |
| `chara 81C7` (camera) | 1 | the camera caption at script `+0x1B8`; USA's record is empty |

So every VR string that has a USA English counterpart now has it, and what
remains is exactly the list under "Deferred, with reasons".

**The trap this tool had to survive: the disc image has raw 2352-byte sectors.**
A PPF offset is `(lba + off // 2048) * 2352 + 24 + off % 2048`
(`portio.image_offset`), so reconstructing a patched stage means inverting that,
not dividing by 2048. A first version of the sweep used `lba * 2048`, applied
every record at a nonsense offset, and reported a finished port as almost
entirely unported — including "0 records applied" for two stages whose PPFs were
demonstrably correct. Anything that reads a deployed PPF back has to use
`image_offset`'s geometry.

### The memory-card modules in `vrsave`, `selectvr` and `vrtitle` (`vr_en_memcard`, 2026-09-11)

Found from a CLEAR DATA screenshot on the collection: `セーブファイルがありません。`
under `NO FILE`, on a disc whose executable table for that caption had read
`No save file.` since 2026-09-06. The VR disc's memory-card module
(`savemngr.c`: SAVE DATA / LOAD DATA / SAVE REPLAY DATA / LOAD REPLAY DATA /
CLEAR DATA) is compiled into **three stage overlays** as well as the
executable, each with its own 12 save and 12 load captions and the two
prompts, exactly the shape `vr_en_camsave` ported in `camera`:

| stage | Integral tables (overlay offset) | USA tables | USA text |
|---|---|---|---|
| `vrsave` | save `+0x8B8`, load `+0x8E8`, prompts `+0x918`; pool `+0xFF4C..+0x10138` | `+0xA58`, `+0xA88`, `+0xAB8` | English |
| `selectvr` | save `+0xD6B8`, load `+0xD6E8`, prompts `+0xD718`; pool `+0x1F264..+0x1F450` | `+0xD978`, `+0xD9A8`, `+0xD9D8` | English |
| `vrtitle` | save `+0x948`, load `+0x978`, prompts `+0x9A8` | `+0x8D0` | **Japanese** |

`vr_memcard.py` ports the first two index for index, USA's tables identified by
their strings (`Save failed.` at 2, `No save file.` at load 4, `Now checking
Memory Card.` at 10), indices 0/1/9 and both prompts empty in USA and so kept
Integral's, as `en_savemsg` and `vr_camera` decided; each pool takes the
English plus the kept Japanese in 390 of its 492 bytes, in place, sector count
unchanged. One PPF, `INTEGRAL_vr_en_memcard.ppf`, 7 records across the two
stages. `vrtitle` is the CLEAR DATA screen, and **USA's own `vrtitle` table
holds the identical Japanese** - USA VR Missions draws the same
`セーブファイルがありません。` there - so it stays by the standing rule; it is the
same category as its four debug windows.

**How it was missed, exactly.** `vr_sweep.py` and `mainsweep.py` read GCL
records, and these are overlay `.rodata` pools. `jplist.py`, the byte
inventory, did see them, but it classified by *string*: the caption also
lives in `vrtitle`, where USA carries the identical Japanese, so every copy
inherited "USA has the same Japanese" and the two stages where USA has
English were never compared as stages. The per-stage question - what
English does USA's copy of *this* overlay have that Integral's lacks? - is
what `overlaydiff.py` now asks, over every stage on both discs and over the
executables, net of what the deployed PPFs already write. Run on 2026-09-11
it found this family and nothing else of its kind: the 881 other VR
candidates are USA's debug symbol tables and printf strings, disc 1's 22 are
the same, and the executables' residue is a boot string and debug prints.

One string is left to look at rather than read: USA's `selectvr` has a menu
record at `+0xCAB4` whose text is `SAVE?`, where Integral's counterpart record
carries an empty string. Probably an unused window; not text the player is
known to see.

### The collection's own patches to the VR disc

Five `SetPatchWatch` ranges are registered in `src/mgs1.h` so the log names any
collection patch that lands in a VR stage we rewrite: the option stage
(`0x5E7EB0`–`0x611D60`), camera (`0x611D60`–`0x627A80`), `vrtitle`
(`0x315B1C0`–`0x31B5430`), `movie` (`0x4C26BA0`–`0x4C6D5B0`) and `vrsave`
(`0xCFA6E0`–`0xD5D320`).

## How the KEY CONFIG port was built (2026-09-03)

**USA's quads, read from USA's binary, not measured.** `kcquads.py` derives the
option overlay's load address (it is not in the header) as the one base that
gives every `key_*` string exactly one adjacent `lui`+`addiu` reference — that
is **0x800C5968** for USA, and all sixteen strings resolve, in ascending code
order, which is the check. It then simulates the overlay linearly and reads each
`Init_Res(work, strcode, poly, x0, sp+16=y0, sp+20=x1, sp+24=y1, abe, orient)`
call. Two traps cost real time here:

- **`jal` is opcode 3, not 0x0C.** With the wrong opcode the simulation never
  sees a call boundary and every argument comes back empty.
- **`r0` must be read-only in the model.** A `nop` is `sll zero,zero,0`, so a
  model that writes `rd` unconditionally poisons the zero register and then
  every `addiu rX, zero, imm` yields nothing.

**CORRECTION: the `Init_Res` quads are not where these rectangles live.** A
per-button-type function (`opt.c` ~line 778) rewrites `poly[13..16]` every frame
and therefore overrides them. `kcrects.py` reads that function instead, and it
is the authority: **every one of USA's rectangles is exactly its art's size** —
`key_action` 32x8, `key_buki` 44x7, `key_hohuku` 28x8, `key_syukan` 88x10 — so
USA never scales these labels. The three of them rotate between a left-middle, a
right-middle and a lower-left slot as the button type changes, each keeping its
own size in every slot; `key_syukan` never moves. All twelve rectangles (three
types x four labels) now carry USA's values, and the overlay came out **25,670**,
172 bytes under retail's ceiling — the rewrite is smaller than what it replaced.

Two traps cost a build each, and both are recorded because neither is guessable:

- **UVs are 8-bit.** `SetPacketTexture` computes `u1 = off_x + w` and
  `v1 = off_y + h` from `DG_SetTexture`'s `off_x = (px % 64) * 4` and
  `off_y = py % 256`. Either reaching **256** wraps to 0, and the quad then
  samples the whole texture page — which renders as pixel noise, not as a
  misplaced label. Two labels sat at VRAM y 504 with 8-row art: 248 + 8 = 256.
  `kcplace.py` and the builder now assert `<= 255`.
- **`abe` must be 1 on all eight.** USA's label palettes have **no (0,0,0)
  entry** — index 0 is a visible grey — and `LoadPalette` (`libdg/loader.c`)
  maps only pure black to the transparent 0x0000. USA therefore draws every one
  of these labels semi-transparent, which blends the background away. Integral
  passed a literal 0 for four of them, which its own art could afford because
  that art did have a transparent index 0; with USA's art those four rendered an
  opaque box behind the text. USA's textures also carry PCXINFO flag 0x18, i.e.
  blend rate 1 (`(flag & 0x30) >> 4`), which comes along with the art.

### Two more things the paired shots caught (2026-09-03)

**The selection cursor has its own rectangle, and it was Integral's.** The green
fill behind a highlighted hexagon row is the `cur_*` group, positioned by
`option_800C449C(work, x, y, w, h, shade, type)` - not by the label. Integral
passed `(-149, -70, 88, 12)` and `(-149, 38, 88, 12)`; USA passes
`(-148, -70, 88, 13)` and `(-148, 38, **112**, 13)`. The 88 is why the fill
stopped short of the wider English "first person view". Both call sites per row
now carry USA's values, read out of USA's overlay. Note USA's cursor is
deliberately 1 px left of its label and 1 px shorter - an inset, not flush.
Verified: the fill spans screen x 579..1604 in both games.

**`key_syukan` is shifted +11 px right of USA's own x, on purpose.** Integral's
background art puts the connector curve coming off the triangle button 11 game
px further right than USA's, because USA moved its curve to clear the longer
English label and Integral never had to. USA's absolute x therefore left the
label sitting on Integral's curve. Measured on paired shots: the rule beneath
the label starts at screen x **2343** in USA and **2442** here (99 px, at 8.92
px per game px). After the shift the label sits **+18 screen px** from its rule
in both games - identical. This is the user's rule of 2026-09-03: where
Integral's own art differs on purpose, move the English text to keep USA's
relationship to that art, and **ask first** — asked and approved for this label
on 2026-09-03. It is also listed in the Scope section's table of deliberate
deviations from USA's coordinates, which is the place to look for "why does this
not match USA?".

The other three labels were checked the same way and need no shift: each one's
distance to its own rule already matches USA. Only the rules' far ends differ,
which is Integral's art and stays.

**Verified 2026-09-03 against paired USA shots** (the same six states shot in
both games, so they pair one to one): every label band matches at **dy 0, dx 0**
on both edges (`key_syukan` excepted, by the deliberate +11 above). **Outstanding:** a thin horizontal rule under two labels runs
about 14 game pixels longer in Integral. Ruled out so far: the label art (USA's
own), the background art (`key_back_l`/`key_back_r` are **colour**-identical
between the games — only their palette indices are permuted), the per-type
geometry (those four polys only, now USA's), and palette collisions (none, with
real entry counts: widths run 1..128, so 16-alignment alone is not enough).
Neither overlay contains any 1-2 row tall rectangle written from literal
coordinates, so that rule's geometry is computed somewhere still to be found.

**For the record, USA's `Init_Res` quads** (first frame only, since the per-type
function overwrites them). Four differ from Integral's, four are identical:

| texture | Integral quad | USA quad | USA art | note |
|---|---|---|---|---|
| `key_button`  | (-149,-70,-61,-58) | (-148,-70,-60,-57) | 88x13  | quad changed |
| `key_sykan`   | (-149,38,-61,50)   | (-148,38,-36,51)   | 112x13 | quad changed |
| `key_normal`  | (-35,41,17,47)     | (-18,39,22,49)     | 40x10  | quad changed |
| `key_reverse` | (29,41,93,47)      | (40,42,84,48)      | 44x6   | quad changed |
| `key_buki`    | (-136,-18,-92,-11) | same               | 44x7   | already USA's |
| `key_syukan`  | (78,-39,138,-32)   | same               | 88x10  | **USA stretches** 88x10 into 60x7 |
| `key_action`  | (74,-18,138,-11)   | same               | 32x8   | **USA stretches** 32x8 into 64x7 |
| `key_hohuku`  | (-136,2,-84,9)     | same               | 28x8   | **USA stretches** 28x8 into 52x7 |

Those last three are not a mistake in the extraction: the USA localisation
replaced the art with narrower English words and **left the Japanese-sized quad
alone**, so USA's own screen scales them. Since the engine stretches a texture
to its quad, keeping the quad reproduces USA's scaling exactly — which is what
matching USA means. This is the opposite of the briefing labels, where USA's
quads did match its art and the canvas had to be padded instead.

**VRAM and CLUT.** USA's own slots are unusable here: Integral carries five
Japanese text textures USA lacks, and four of USA's eight slots clash. So
`kcplace.py` frees Integral's eight, then places USA's largest-first with the
constraints one tpage imposes — `(px % 64) * 4 + w <= 256` and
`py % 256 + h <= 256` — preferring the band the option stage already keeps these
labels in, and staying clear of the option screen's font KCBs (x 768..960,
y 256..344; `opt.c` sets `rect.x` 768/832/896, `w` 64, `h` 21 stacked, CLUTs at
y 276). Result: four labels keep Integral's slot, four move to y 460, and all
eight CLUTs go to y 233, x 320..432.

| label | VRAM | was | CLUT |
|---|---|---|---|
| `key_button`  | (464,460) | (128,492) | (336,233) |
| `key_sykan`   | (336,460) | (80,480)  | (320,233) |
| `key_syukan`  | (486,460) | (208,480) | (352,233) |
| `key_normal`  | (364,460) | (175,492) | (400,233) |
| `key_reverse` | (16,504)  | (11,504)  | (384,233) |
| `key_action`  | (0,504)   | (175,502) | (416,233) |
| `key_buki`    | (108,504) | (0,504)   | (368,233) |
| `key_hohuku`  | (64,504)  | (120,256) | (432,233) |

**Checks that ran before deploying**, on the built stage rather than the plan:
all 57 textures decoded, **zero** VRAM rectangle overlaps, 57 distinct CLUT
slots for 57 textures, no CLUT row inside any texture (both directions), every
texture within its tpage, and the DAR walk landing on exactly 0. Overlay 25834
of retail's 25842 bytes; DAR 121,680 -> 128,332; stage 75 -> 78 sectors, still
DUMMY3M 384 (384..461), disjoint from `brf` at 128..266 and `preope` at 0..89.
Revert with `work/backup_before_keyconfig_disc{1,2}.ppf`.

**Not changed:** `abe`/`orient` keep Integral's values. USA's are not reliably
readable (they come from saved registers the simulation cannot always resolve),
and they affect blending, not layout.

**Still Japanese on that screen, by the rule:** Integral's per-row bottom help
line, which USA has no counterpart for.

## Why `en_menu3` is raw-disc only (diagnosed 2026-08-28, corrected 2026-09-07)

`menu3.py` ports the same five disc-swap messages ("Insert DISC 1.", "Press the
Start Button", "after inserting DISC 1.", "Now Checking...", "The correct DISC
was not inserted.") into the `title` stage - the fourth and last copy. It is
built and verified, and it **must not be deployed in the collection**. The
reason is not in our file.

**The collection patches this exact block itself.** Its CD-ROM patch
`disc1_1822B55D_patch` lands at image offset `0x1822B55D`, which is not merely
inside the block - it is the address of record 0's `07` header. Its bytes come
from `099/patch/disc1_1822B55D_patch_PS5.bin`, so the patch watch reports it as
"0 bytes" and its contents stay invisible. It is **not** filtered in normal play:
the 164 "filtering CD-ROM patch" lines in a boot log are the ghost restores, and
the one Sept-3 log that shows this patch being filtered was a `DisableCDROM =
true` session.

Deploy ours on top and the title stage dies on entry with a run of
`GCL:WRONG CODE` whose bytes are the English letters, then the process exits -
the log ends mid-run. Seen 2026-08-28, again 08-29, and reproduced exactly on
2026-09-07, the same seventeen bytes every time:

    82 73 68 65 74 61 72 74 75 74 74 6f 6e 22 22 22 22

which read out of `Press the Start Button` and then off the end of the `.gcx`.

**The bytes the interpreter walked are not ours.** `GCL_GetNextValue` was
transcribed from `libgcl/parse.c` and run over the builder's output from every
start offset in the chunk: no start reproduces that sequence, and the trailing
`22 22 22 22` does not occur anywhere in the stage. Two patches are writing the
same five strings over the same bytes with different layouts, and the walk
desynchronises in the mixture.

Decisive detail: the shape that crashed is the **length-preserving** one, which
leaves every record header, every length byte and every container size exactly
as retail. If retail's layout survives intact and the stage still dies, the
fault cannot be the record shape.

**So it ships for a raw PSX disc only** (NextSteps 5.4), where no collection
patch exists. `py menu3.py` writes `INTEGRAL_disc{1,2}_en_menu3_raw.ppf` into
`work/`; `--deploy` refuses and prints why; `--collection` rebuilds the crashing
shape for anyone who wants to reproduce the fault. Decided with the user
2026-09-07, who chose raw-only over the alternative below.

**If the collection's wording ever has to be ours**, the lever exists:
`SQHook::SetPatchFileBlacklist("disc1_1822B55D_patch")` behind an ini flag, the
same mechanism `BrightnessText` uses to take the brightness texture back. Then
the port owns the block and there is no mixture. Not done, because it is an ASI
change buying a screen the collection cannot reach - the same conclusion KEY
CONFIG reached from the other direction.

### The diagnosis this replaces, and why it survived so long

This section previously said the interpreter "resumes parsing at the early NUL"
because `menu2.py` writes `English + NUL + spaces`, and prescribed shortening
the STRING length bytes and shrinking four enclosing containers. Both halves are
wrong, and both checks are cheap:

- **`GCL_GetNextValue` advances a STRING by its length byte** - `p += *p + 1`,
  `libgcl/parse.c` - never by `strlen`. An early NUL inside a payload cannot
  desynchronise it.
- **The disabled artefact had no container fault.** The PPFs that sat in
  `mods/_disabled/` change payload bytes only - 150 of them - leaving every
  record header, length byte and container size as retail. They re-parse cleanly
  with `gclparse`: 25 records at retail's offsets, same as the untouched stage.

The prescription was written on 09-03 from the 08-28/29 logs and attached to an
artefact it had never been tested against; nobody re-ran it, so the file sat
disabled for four days behind a fix that addressed nothing. **The pattern worth
keeping: a diagnosis carried forward onto a different artefact.** When a build is
shelved as broken, record which build - the next person will assume the
diagnosis and the file match.

### The general trap: the collection may already own the bytes you are porting

This is the third time the collection's own patches have shaped a port, and the
first time they have blocked one outright:

| where | what the collection does | what the port did about it |
|---|---|---|
| KEY CONFIG (`option`) | intercepts the screen with its own Control Settings panel | reproduced the doorbell in `opt.c`; the user prefers the interception |
| the brightness paragraph | replaces USA's `sc_text` with its own four-line version | `BrightnessText` blacklists their files and ranges and writes ours |
| the `title` disc-swap block | patches the same five strings with its own layout | **cannot coexist** - raw disc only |

The check to run *before* building anything that writes a block: map the
collection's patches to it (README "Where the collection's own disc patches
land") and see whether one starts inside the region. If it does, the choices are
to blacklist theirs, to match their layout, or to ship raw-only - and that is a
decision to take before the build, not after a crash.

## The collection shows only four of USA's six brightness lines (found 2026-09-03)

Comparing the collection against SwanStation (a much more accurate emulator)
running the same discs:

| | brightness paragraph |
|---|---|
| SwanStation, USA disc | **six** lines, ending "Press the ○ button to return to the option screen." |
| the collection, USA | **four** lines, ending "game." |
| SwanStation, Integral disc | four Japanese lines, a blank line, then the ○ line |

USA's `sc_text` texture is 232x70 and **contains all six lines** (decoded with
`pcx4.py`; the whole 70 rows are inked). The collection is therefore dropping
two lines of USA's own help text.

It is not a data or revision difference: the USA `option` stage extracted from
the collection's `alldata.bin` (base `0xF12F8000`, STAGE.DIR lba 132344, sector
27023, 81 sectors) is **byte-identical** to the same span of the real USA disc's
STAGE.DIR — all 165,888 bytes.

It is not caused by disabling achievements: the collection's USA shot showing
four lines is from 2026-08-31, and that session's log records
`bPatchesDisableRAM: false` and `bPatchesDisableCDROM: false`, i.e. every
collection patch active.

Position, measured with the text's own line pitch as the ruler so aspect ratio
and the collection's border art cancel out (anchor: the green line):

    SwanStation USA   first line 1.88 line-heights below the green line
    collection USA    first line 2.88 line-heights below the green line

So the collection draws the block **exactly one line lower** and 24 rows
shorter. That is the same +12-row offset already met when placing the ported
quad (`y0 = 2` in USA's code renders where `y0 = 14` puts it), and a symmetric
12-row inset top and bottom accounts for both numbers. What inside the
collection's renderer does it is not identified.

### It is four of the collection's own CD-ROM patches (settled 2026-09-03)

The test ran: with `DisableRAM`/`DisableCDROM` **true**, the collection's USA
draws **all six lines**, at 2.05 line-heights below the green line against
SwanStation's 2.01 — the same place. So the collection was truncating it, and
the truncation is four CD-ROM patches, found by mapping the log's
`filtering CD-ROM patch file disc1_<offset>_patch` names onto stage bytes:

    disc1_165A34CC  disc1_165A3BD8  disc1_165A4508  disc1_165A4E38
    -> USA option stage, tag 1 (DAR), all four inside the ONE 5852-byte
       `sc_text` entry, at payload offsets 0, 1500, 3548 and 5596

i.e. the collection replaces that whole texture, in 2048-byte pieces. Its
replacement keeps the 232x70 canvas and centres four lines in it, which is why
the block also sat 12 rows lower: 70 − 46 = 24, split evenly. The dropped
sentence is "Press the ○ button to return to the option screen." — the ○ name is
platform-specific, and the collection substitutes its own button UI everywhere
else too (it rewrites the button bits of
`GM_Configuration` every frame).

**This corrected the port.** `SC_ROWS` is back to **70** and the quad back to
USA's own `Init_Res(work, "sc_text", po, -121, 2, 111, 72, 0, 0)`. The earlier
"measured, not copied" `y0 = 14` with a 46-row crop had been fitted to the
collection's replacement artwork, not to USA — so the adversarial review that
argued for copying USA's constant was right, and the screenshot that appeared to
refute it was of patched art. It also means the shipped build was **dropping a
line of text both originals have**, which the no-abridgement rule forbids.
Rebuilt and deployed 2026-09-03: DAR 121680 → 127508 (+5828), stage 75 → 78
sectors, still DUMMY3M 384 (brf at 128..266 untouched); revert PPFs in
`work/backup_sctext46_disc{1,2}.ppf`.

**Lesson.** The collection's patch set is part of the baseline. A constant fitted
to a collection screenshot is fitted to whatever the collection patched, and
changing the patch flags changes the target. Before deriving any constant from a
shot, check the session's `bPatchesDisable*` lines, and check whether the asset
appears in the log's filtered patch list.

### The dark bar over the grey ramp: one re-centring causes both symptoms

The user, looking at the collection's four-line text: *"at the top of the MC's
text there's a gray bar that looks like it's a chunk of the darkest of the gray
brightness bar. Like the text was in the right place with the gray bar partially
behind it, then the whole text including that part of the bar moved down."*

That is exactly it, and USA's own texture proves it. Decoding `sc_text` and
counting palette indices per row:

    rows 0-1   194 of 232 px are palette index 0 = (8, 8, 8)
    row  2      85 px of it
    bg         index 12 = (0, 0, 0), 67% of the canvas

**The whole canvas is opaque** — `Init_Res`'s `abe` argument is `0`, and the
rendered pixels agree: below the paragraph, inside the canvas's x span, both
games read luminance 0.00 where the strip beside it reads 1.13. So the 232x70
canvas is a solid black backdrop for the text, spanning game rows 122..191, and
index 12 `(0,0,0)` is **not** see-through here. (An earlier note in this file
claimed it was; that is corrected, and it matters for any quad drawn with
`abe = 0`.)

Which makes the `(8,8,8)` rows a **seam filler**, and a precise one. The ramp's
own bands, measured left of the canvas: value ~8.9 down to game row 125.3, then
~1.1 below. USA's canvas rows 0..3 land on game rows 122..125 — exactly the
ramp's `8` band — and are painted one step off black so the backdrop's top edge
disappears into it. Below row 126 the ramp is already ~0, so plain black hides
there by itself. The art is fitted to `y0 = 2` to the row.

The collection re-centres the four remaining lines in that canvas and the seam
filler travels down with them, leaving pure black over the ramp's `8` band.
Measured on its USA: rows 122.3..125.3 read 0.00 inside the canvas against 8.89
beside it — a four-row notch, which is the bar the user spotted, and the text
one full line pitch lower. One re-centring, both symptoms. Nothing in the
renderer, nothing to do with MGSM2Fix, nothing to do with this port.

### So the collection build blanks the lines instead of re-centring (2026-09-03)

`SC_KEEP_LINES = 4` (`optsctext.py`) drops the same two lines the collection
drops, and changes **nothing else**: same 232x70 canvas, same
`Init_Res(work, "sc_text", po, -121, 2, 111, 72, 0, 0)` quad, line 1 still at
row 0, bar still hidden at the top. `drop_lines()` finds the six line starts
from an ink profile (rows `[0, 12, 24, 38, 47, 63]`), takes the blank gap above
the first dropped line with it (cut at row 46), and blanks x 2..231 to the
transparent index — preserving the vertical teal rule at x=1, and *restoring*
the one at x=227 across the six pixels where the dropped line's ink had been
covering it. Because the canvas and quad are untouched there is no overlay
change at all, so the 25,842-byte ceiling is not back in play.

Not cropping is the point. A 46-row texture would need the quad shortened to
match — UVs come from the texture's own size via `SetPacketTexture`, so a
70-row quad over 46 rows of art stretches it — and every one of those numbers
is a chance to reintroduce the offset. Blanking touches one thing.

Verified end to end from the **deployed** PPFs (not the build's intent):
reconstruct the 78 DUMMY3M sectors from the PPF's own 682 records, walk the tag
table and DAR, decode `sc_text`. Both discs: 232x70 at vram(512,256)
clut(1008,237), inked lines starting at rows `[0, 12, 24, 38]`, 194 px of index
0 in each of rows 0 and 1, no ink below. Six-line revert PPFs are in
`work/backup_sctext6_disc{1,2}.ppf`.

**Measured in game 2026-09-03, paired collection shots at 3840x2160** (ours:
`Integral Mod …/MC/20260903174615_1.jpg`, theirs: `MGS1 USA/MC/20260903174739_1.jpg`),
anchored on the green line and scaled by the text's own 108 px line pitch:

| | first line below the green line | notch over the ramp's `8` band |
|---|---|---|
| SwanStation USA (README reference) | 1.88 line-heights | none |
| **ours, Integral** | **1.86** | **none** — no row inside the canvas reads darker than the strip beside it |
| the collection, USA | 2.86 | rows 122.3..125.3 read 0.00 vs 8.89 |

Our text is exactly 108 px = one line pitch = 12 game rows higher than the
collection's, landing 0.02 line-heights from SwanStation — inside the noise of a
JPEG measurement. Horizontally all four lines are pixel-identical to the
collection's: lines 1-3 agree on both edges to the pixel, and line 4's 9 px
right-edge disagreement is a threshold artefact (columns 1138..1144 sit at
115-122 luminance in both shots, either side of the 120 cutoff; both go dark at
1146). Same glyphs, same x, one line pitch higher, no notch.

### Fixing it for USA too, in MGSM2Fix rather than on the disc (2026-09-03)

Integral's copy of this texture is ours, so `SC_KEEP_LINES` settles it there.
USA's is the collection's, and the user asked for that fixed too — as an
MGSM2Fix feature, since it is a collection bug with no connection to this port.
It is, and `UPSTREAM.md` carries the row.

**Why MGSM2Fix can do it at all.** `Ketchup::ApplyBlock` already wraps
`SQEmuTask::EntryCdRomPatch`, i.e. "write these bytes at this disc-image
offset" — exactly a PPF record. So no new mechanism was needed, only a table the
fix carries itself: `Ketchup_DiskPatch` (title, version, disk, offset, bytes),
returned by a new `M2Game::SQKetchupPatches()` and applied by
`Ketchup::ProcessBuiltins()` at the top of `ProcessDisk`, before the early
return that skips a missing mods folder.

**Why not a PPF in `mods/MGS1_US`.** A built-in leaves the reference game
byte-stock whenever the setting is off, so pixel comparisons against USA need no
file shuffling. A PPF would have to be moved aside each time.

**426 bytes.** Do not re-encode the texture: `pcx4.encode` makes different
run-length choices than whatever produced USA's art (5818 bytes against 5852 for
the *same pixels*), so a whole-payload diff is 5288 bytes across 335 runs. But
this PCX is four bit planes, run-length encoded **one row at a time**, so rows
are independent in the byte stream. Keep USA's own bytes for rows 0..45 and
replace only rows 46..69: **426 bytes at payload offset `0x1064`**, against the
1653 bytes they replace. The payload's declared size never changes — the new
encoding is shorter, and the decoder stops after 70 rows without reading the
1230 bytes left over.

**The offsets, and how they were checked.** `option` is STAGE.DIR sector 27023
on both USA disks, the `sc_text` payload is 5852 bytes at file offset 55499300,
and row 46 is at 55503496. Through
`(lba + fo / 2048) * 2352 + 24 + fo % 2048`, with STAGE.DIR at LBA 132344 and
100801:

    disk 0   payload 0x165A34CC..0x165A4F38   row 46 at 0x165A4790
    disk 1   payload 0x11EE2B7C..0x11EE45E8   row 46 at 0x11EE3E40

Three independent confirmations of disk 0. The bytes at
`alldata.bin + 0xF12F8000 + 0x165A4790` equal the bytes STAGE.DIR holds at file
offset 55503496. The write lands inside one sector, so it needs no splitting.
And the collection's own first patch for this entry is named `disc1_165A34CC` —
which is exactly `0x165A4790` minus 4196 payload bytes minus two 304-byte sector
strides, i.e. the collection names each piece after its image offset in the very
same space, and its piece 1 begins at the payload's first byte. Its four pieces
therefore cover payload `[0, 1500)`, `[1500, 3548)`, `[3548, 5596)`,
`[5596, 5852)`, and our 426 bytes sit inside piece 3 — so piece 3 would overwrite
them, and all four have to be filtered whichever mode is chosen.

**Filtered two ways, on purpose.** The four filenames are what was observed on
disk 0 and are verified in game. `SQHook::SetPatchRangeBlacklist` adds a
half-open disc-image range, matched against the offset the patch is entered
with, which covers the same archive entry on **either** disk without having to
know what the collection called each piece there — the disk 2 names have never
appeared in a log, because USA disc 2 has never been booted here. The range is
tight enough to be safe: of the 107 patches the collection offers title 981,
exactly four fall anywhere inside the 81-sector option stage, and they are those
four pieces.

**Why not just move the quad.** `SetPacketTexture` takes the UVs from the
texture's own size, so the quad cannot crop — and shifting it up 11 rows to meet
the collection's re-centred text would drag the opaque canvas's top edge onto
the ramp's brighter 16 band, a worse notch than the one being fixed.

**Verified statically end to end, on both disks, against the shipped binary.**
`verify_usa_brightness.py` (the scratchpad's `verify_shipped.py`, rescued 2026-09-04) trusts nothing the build tools claim: it pulls
the 426 bytes out of the deployed `MGSM2Fix64.asi` (they sit at file offset
`0x259BA0`, exactly once), reads the two offsets out of the shipped table,
checks each against the image offset recomputed from that disk's STAGE.DIR,
confirms the collection's own `alldata.bin` still holds the expected pre-patch
bytes there, then applies the binary's bytes at the binary's offset over the
game's own payload and decodes the result: 232x70, four inked lines starting at
rows `[0, 12, 24, 38]`, 194 px of the `(8,8,8)` filler in each of rows 0 and 1,
rows 0..45 byte-identical to the game's own, no ink below.

One caveat worth keeping: the data has to be a plain `.rdata` array, not a
`std::vector` initializer_list. As an initializer_list the optimiser synthesised
it — the 426 bytes appeared nowhere in the object file *or* the binary — which
would have worked at runtime but left nothing to verify. `static inline const
unsigned char MGS1_BrightnessTextData[426]` emits real bytes, and the check
above only exists because of that.

**Verified on screen 2026-09-03, and the two games now agree exactly.** Paired
collection shots at 3840x2160 with every collection patch active and
achievements live (`bPatchesDisableRAM: false`, `bPatchesDisableCDROM: false` in
that session's log): `Integral Mod/MC/20260903202117_1.jpg` and
`MGS1 USA/MC/20260903201944_1.jpg`. Anchored on the green line and scaled by the
text's own 108 px line pitch, **both** now read

    green line y=906, four lines at rows 1107 / 1215 / 1323 / 1449
    first line 1.86 line-heights below the green line  (SwanStation USA: 1.88)
    no row inside the canvas darker than the strip beside it

The collection's USA was 2.86 before this. Better than matching numbers, the two
shots are *identical pixels* over the whole screen below the header — different
files, different hashes, and a straight image difference gives **max 0** across
every band from y=433 to y=1800: the ramp above the green line, the green line,
the gap, all four lines of text, and below them. Only the header area differs at
all (max 45, mean 0.034 over 119,040 sampled pixels), which is the option
screen's own chrome and JPEG noise, not this texture.

So Integral-via-`SC_KEEP_LINES` and USA-via-`BrightnessText` land on exactly the
same pixels, from two completely different mechanisms — a rebuilt DAR entry on a
relocated stage in one, and 426 bytes spliced into the collection's own archive
entry in the other.

Still derived rather than observed: the range filter's own contribution on disk 0
(the filename filter would have caught those four anyway) and everything about
disk 2, which has never been booted here.

## WITHDRAWN: the brightness grey ramp is not actually different

This section used to say `sc_back_r`'s greys were uniformly **7 levels darker**
than USA's (72/64/56/48/40 against 79/71/63/55/47) across 40% of that half, and
flagged it as arguably the content of a calibration screen. **That was wrong,
and the user caught it by simply noticing the two screenshots look identical.**

They look identical because they **are** identical. `LoadPalette`
(`libdg/loader.c`) builds the 15-bit CLUT the hardware uses with `>> 3` per
channel, and every one of those differing pairs collapses to the same 5-bit
value:

    Integral 72 -> 9    USA 79 -> 9        Integral 48 -> 6    USA 55 -> 6
    Integral 64 -> 8    USA 71 -> 8        Integral 40 -> 5    USA 47 -> 5
    Integral 56 -> 7    USA 63 -> 7

Measured: of `sc_back_r`'s 14,553 pixels that differ as 24-bit RGB, **529
survive the conversion** - and none of those are the ramp. On screen, 47 sampled
bands across both halves of the rendered screen differ by **0.0** levels.
`sc_back_l`'s 550 differing pixels do survive, but they are edge and element
pixels, not the ramp.

**The lesson, which cost this a second time:** a difference in 24-bit texture
data is not a difference on screen. This hardware holds 5 bits per channel, so
compare palettes *after* the `>> 3`, or compare the rendered pixels. The same
mistake produced the withdrawn claim about the KEY CONFIG background's contrast
- see that row in the Scope table, which is now the only one of these left
standing and is itself unattributed.

## Not tested

- ~~The three item-text fixes on screen~~ **Done 2026-09-05.**
- ~~The MISSION LOG slide after the sprite-width fix~~ **Done 2026-09-05.**
- ~~The MISSION LOG on screen~~ **Done 2026-09-05.** Still unseen: a demo.gcx
  page, which needs a disc-2 save, and a count-7 page, which also shows
  whether USA's `1/2` on a single-screen page is wanted.
- **Disc 2 in game.** Still never reached — and now it is known why the
  developer menu cannot get there: disc 2 is set only by `change.c`'s CD check,
  which only the real story swap runs, so every debug load stays `Disk ID 0`
  (two attempts 2026-09-04: s11a playable but DISC 1; s14e hung). See "The
  disc-swap text: four copies". The route is a late disc-1 save played
  across the break. Neither has any *other* title's disc 2 been launched, which
  is why no disc-2 patch candidate has ever appeared in a log. What the port
  touches is measured identical across the two discs (2026-09-03):

  | what the port patches | disc 1 vs disc 2 |
  |---|---|
  | the executable (`en_savemsg`, `en_items`) | **byte-identical**, all 641,024 bytes, same hash. USA's two are identical too. |
  | `option` (75 sectors) | byte-identical |
  | `preope` (87 sectors) | byte-identical |
  | `brf` (138 sectors) | byte-identical |
  | `camera` (38 sectors) | byte-identical |
  | `demosel`, `change` | byte-identical |
  | `title` (204 sectors) | **6 bytes differ**, and they are three per-disc LBAs (`0x00016CCB`/`0x00013DE0`, `0x0001721D`/`0x00014369`) in tag 6 — stream addresses, not text. `unlock_title.py` reads each disc's own source, so this is handled. |

  Both discs' STAGE.DIR are the same size with the same 95 entries at the same
  sectors. So a single build is correct for both wherever the source is
  identical, which is everywhere except `title`.

  Disc 2's *geometry* is verified against the real image, not just assumed:
  `emit()` asserts DUMMY3M is blank at the target slot and that the option entry
  still reads 27136, both read from disc 2's own image; and the readback checks
  reconstruct disc 2's 78 DUMMY3M sectors from its own PPF records and find
  `sc_text` correct and the doorbell stub at the right overlay offset. What
  remains untested is only that it *runs*.

  Nothing in scope is disc-2-specific. Integral is an (En,Ja) release, so the
  game's own dialogue and codec text are already English by the language
  setting; this port is menus and UI only, and every menu stage lives on both
  discs identically.
- ~~`en_savemsg` with achievements live~~ **Answered 2026-09-04 — the struck
  item further down; this one is kept for the method.** The six collection RAM patches that
  fall inside its pool land mid-run, where Ketchup's first-byte check cannot see
  them. The screen test turned out not to work: LOAD DATA in the collection
  goes straight to `No save file.` (slot 4, untouched) and never shows `Now
  checking Memory Card.` — its storage layer is instant. So `Ketchup::Audit`
  (2026-09-04) compares every byte of every RAM run read-only every ~5 s and
  warns on any mid-run mismatch: a `differs from what was written` line naming
  a pool address means the writes stick, silence means they do not. **The first
  audited session (2026-09-04 17:28) was 40 seconds of title → option → title
  and logged nothing — which is not yet evidence,** because the collection's
  memory-card rename family is plausibly applied only when a memory-card screen
  is visited. The session that answers it needs `DisableRAM = false`, a visit
  to LOAD DATA, and ideally a save; **run it before any achievements-off
  testing**, since with `DisableRAM = true` there is nothing to observe. See
  "The collection's RAM patches collide with `en_savemsg`".
- ~~SCREEN and EXIT after the doorbell build~~ **Done 2026-09-04:** all three
  branches of `case 8` behave correctly, and the doorbell fires only where it
  should.
- ~~The save side of the memory-card messages~~ **Done 2026-09-04:** save and
  load both showed the correct English/Japanese mix, nothing garbled.
- ~~`en_savemsg` with achievements live~~ **No collision observed, 2026-09-04**
  (`Ketchup::Audit` ran every ~5 s across the whole save-and-load session above
  with the rename active and logged nothing). Residual caveat: a mid-run write
  followed within 30 frames by an unrelated re-apply would be invisible to both
  checks; nothing suggests that happens.
- **Whether the disc-swap prompt is reachable in the collection at all** (the
  three copies that ship). The collection swaps discs by itself, so the game's
  own prompt may never draw. Decided by the same disc-2 run above. See "The
  disc-swap text: four copies". The fourth copy, `en_menu3`, is settled a
  different way: the collection patches that block itself, so in the collection
  the text there is the collection's, not ours - see "Why `en_menu3` is raw-disc
  only".
- ~~The other ~22 PHOTO ALBUM strings~~ **Done 2026-09-04:** all 23 English
  strings present and identical to USA; the only Japanese left is the six
  slots USA itself leaves blank. Verification gotcha, still live: `int1_stage.dir`
  is the *unpatched* extraction, so reading it shows Japanese everywhere —
  apply the deployed PPF first, or the comparison is meaningless; and the
  three `addiu sp` function pointers at 0x6E0–0x6E8 decode as "text" and look
  like misses — they are code, the documented camsave trap.
- **`[Patches] PreserveConfiguration` catching a real stale write.** Three clean
  runs; the race it guards is intermittent and has not been caught in the act.
  See `UPSTREAM.md`.

## Briefing menu (`brf` stage)

The briefing's videos, subtitles and character bios are **already English in
Integral** — only the menu chrome is Japanese. Those labels are PCX textures
drawn as textured quads, which is why no ASCII label exists in STAGE.DIR, the
executable or BRF.DAT.

`GV_StrCode` maps resource names to archive ids. All 20 labels exist in both
archives under the same id, one to one:

| id | Integral | USA |
|---|---|---|
| `br_s00` | 潜入方法 | infiltration method |
| `br_s01` | タイムリミット | time limit |
| `br_s02` | 作戦責任者 | person in charge of the operation |
| `br_s03` | サポート要員 | support crew |
| `br_s04` | ロイ キャンベル | Roy Campbell |
| `br_s05` | メリル | Meryl |
| `br_s06` | Dr. ナオミ | Dr. Naomi |
| `br_s07` | 人質 | hostages |
| `br_s08` | 核兵器 | nuclear weapons |
| `br_s09` | テロリストの要求 | the terrorists' demand |
| `br_s10` | 遺伝子強化 | genetic strengthening |
| `br_s11` | テロリストの兵装 | the terrorists' armament |
| `br_s12` | 次世代特殊部隊 | next-generation special force unit |
| `br_s13` | 全員賛同の理由 | the reason for unanimous approval |
| `br_s14` | FOX HOUND部隊 | unit FOX-HOUND |
| `br_s15` | リキッド スネーク | Liquid Snake |
| `br_f00`–`br_f03` | FILE 00–03 | mission description / operation outline / operation member / detailed information |

### The texture is stretched to the quad

`source/overlays/brf/onoda/brf/b_select.c` is decompiled and settles the model
that cost the most time here:

    void brf_800C983C(int prim, int tex_id, POLY_FT4 *poly, int xl, int yt, int xr, int yb, ...)
    {
        brf_800C97CC(prim, poly, xl, yt, xr, yb, abe);   /* setXY4(poly, xl,yt, xr,yb) */
        tex = DG_GetTexture(tex_id);
        poly->u0 = tex->off_x;  poly->u1 = tex->off_x + tex->w + 1;   /* whole texture */

The screen rect is a **hardcoded quad** and the UVs span the **whole texture**,
so **rendered size is the quad's, and texture size does not affect it at all**.
A label draws at its true size only when its canvas equals its quad. That is
why an earlier attempt — dropping USA's native-size textures into free VRAM —
changed nothing on screen, and it is the reason to prefer padding over scaling:
padding a canvas out to the quad keeps the art 1:1, scaling distorts it.

It also dissolves the shared-register problem *for the art*. Several labels
share one `xr` immediate, so they share a quad — but they need not be the same
width: size the shared quad to the group's largest member and pad the smaller
ones with the palette's black entry, and each still renders at its own true
size.

That is not enough once the **selection highlight** is considered, because the
highlight follows the quad. `br_s06` (Dr. Naomi, 52 px) shares `xr` with
`br_s02` (112 px), so its highlight ran 60 px past its text; `br_s09` shares
with `br_s11` and ran 8 px over. USA gives each its own `xr`.

Un-sharing needs one spare instruction slot per argument block and there is
none — **except** that the rewritten positioner overwrites all four poly `y`
values every frame, which makes the GetResources `yt`/`yb` dead for every
`br_sNN`. Their loads are the spare slots. All four rows are unconditional, so
the positioner always runs and the `y` really is always replaced:

    br_s02  800C9EF8  addiu t0, zero, 138   ; its own xr
                      addiu s5, zero, 78    ; and leaves br_s06's xr in s5
                      sw t0,16 / sw t0,20 / sw t0,24 / sw s1,28 / nop
    br_s09  800CA134  addiu s4, zero, 146
    br_s11  800CA1C8  addiu t0, zero, 154   ; no longer reads s4
                      sw t0,16 / sw t0,20 / sw t0,24 / sw s1,28 / nop

All 15 label quad widths then equal USA's exactly, and every canvas equals its
own art with no group padding.

### The three quad families

Found by disassembling the 38 `brf_800C983C` call sites in `GetResources`
(`asm/overlays/brf/brf_800C99C0.s`) and reading the resource-name string loaded
before each one. Args are `a3=xl, 16(sp)=yt, 20(sp)=xr, 24(sp)=yb, 28(sp)=abe,
32(sp)=orient`.

**A — full quad** (`br_s02 s04 s06 s07 s08 s09 s11 s12 s14`): `xl=26` plus real
`yt/xr/yb` immediates. Width and height both patchable. Shared immediates:

    800C9EFC = 86    br_s02.xr, br_s06.xr
    800CA038 = -7    br_s06.yt, br_s09.yt
    800CA040 = 6     br_s06.yb, br_s09.yb
    800CA134 = 122   br_s09.xr, br_s11.xr

**B — indented rows** (`br_s01 s03 s05 s10 s13 s15`): `xl=46`, `yt=yb=0`, one
unique `xr`. `xr - xl` equals the Integral texture width exactly for all six.
Width patchable, height not — see below.

### Height: take it from the texture's own V coordinates

`brf_800C69B4` drew every `br_sNN` into a quad of one hardcoded height
(`addiu v1, a2, 13`). **The selection highlight follows that quad**, so a fixed
height gives a fixed highlight — measured by diffing screenshots that differ
only in which row is selected:

    fixed quad   mine 24, 24, 24 ...                          (uniform)
    USA          5, 6, 7, 9, 10, 11, 12, 13, 14, 23, 24       (per row)

USA varies because it sizes each box to its own label: `above + below + 5`
equals that label's texture height, exactly, for all 16.

The exact per-label height is already in the poly, and the positioner never
touches it. `brf_800C983C` sets `v0 = tex->off_y` and
`v2 = tex->off_y + tex->h + 1`, and `tex->h` is height−1, so **`v2 - v0` is the
texture height**. Unlike the poly's `y`, the UVs are stable across frames, so
reading them cannot accumulate.

Reading it costs four instructions, and the function has them: its x
normalisation is entirely redundant, because `setXY4` already leaves `x2 == x0`
and `x1 == x3`. The two `lh` and the two `sh` that copy x are dead:

    lbu  a0, 29(v0)      v2
    lbu  a1, 13(v0)      v0
    nop                  the R3000 load delay: a1 is not readable yet
    subu v1, a0, a1      v1 = texture height
    addu v1, a2, v1      v1 = y + height
    sh a2,10 / a2,18 / v1,26 / v1,34

(The `nop` was missing until 2026-09-10 and the Master Collection never
showed it - see "The briefing row box on a real disc" below.)

The quad is now the texture height per label — the same rule USA uses — so the
highlight matches without approximation, and every canvas is USA's art at USA's
size with no padding, scaling or squashing.

Two earlier approximations are gone with it: a fixed 20-row quad (which made
the highlight uniform) and `advance - 6` (which left it 0–3 rows tall). Measured
before the fix: highlight 14.0 game px against USA's 11.0, both with their top
2.0 px above the ink — the tops already agreed, only the height was over.

### The `above` offset: read back from the texture's VRAM row

USA's box is `[y - above, y + below + 5]` with a per-label `above` of 2, 3 or 4
(its positioner's fifth argument; `rowargs.py` extracts all 16). Ours was
`[y, y + h]`, so every label sat `above` rows low relative to its row anchor —
and, worse, the row anchors themselves were not USA's (next section), which is
why an earlier pass measured "9 of 16 exact" and concluded the shift was not
worth fixing. Measured against the user's USA shots, no label was exact.

`above` is not derivable from the poly, and the positioner has no slot for a
table lookup — but it already reads the poly's `v0`, which is the texture's
VRAM row (`off_y = py % 256`), and **we choose `py`**. So the builder places
each `br_sNN` at `py % 8 == above` and the positioner reads it back:

    lbu  a0, 29(v0)      v2
    lbu  a1, 13(v0)      v0 = py % 256
    subu v1, a0, a1      h = texture height
    andi a1, a1, 7       above
    subu a0, a2, a1      top    = y - above
    addu v1, a0, v1      bottom = top + h        (== y + below + 5)
    sh a0,10 / a0,18 / v1,26 / v1,34

Ten of the eleven slots, `a2` intact for the `y + advance` return. Box height
is unchanged, so the selection glow (which follows the quad) is unchanged. The
placement constraint moved 15 of the 16 labels in VRAM; `brf_build.py` asserts
`py % 8 == USA_ABOVE[tid]` for every label after the rebuild.

### Where each list starts: USA's arithmetic, not Integral's

Both builds centre a submenu's list on the total height of its items, from
different formulas. Measured from the user's shots (left FILE column identical
in every pair, so the framing is the same), the right column sat **4 rows high
in operation outline, 1 row low in operation member and 12 rows high in
detailed information** once `above` was accounted for. The item count `n` is a
runtime variable (briefing flags at `work+128/136/144/164/176/184`), so the
constants had to become USA's formulas, not USA's values:

    outline    s0 = -41 - (20n - 15) / 2        Integral: (20n - 7)     800C6EEC  -7 -> -15
    member     s0 = -21 - (20n -  5) / 2        Integral: (20n - 7)     800C6FE4  -7 -> -5
    detailed   s0 =   9 - (16n + 10d - 11) / 2  Integral: ~((17n-4)/2)  800C7100  22-word rewrite

`d` is the number of two-line labels (1, or 2 once `br_s13` unlocks); the
rewrite fits in the block's own three `nop`s and drops the redundant signed
halving (the value is always positive). For the user's save (n = 1 / 3 / 6)
all three now compute exactly USA's `-43 / -48 / -38`.

USA's member block also has a second branch for four or five items; see "The
member submenu with four or five items" below — it is now replicated.

### The rule constants are USA's own

With each submenu's half-height `v` now USA's, the rule needs no compensation:
top `s0 - 4` (Integral drew `s0 - 2`), bottoms `v - 36`, `v - 16`, `v + 14` —
USA's immediates verbatim. The earlier `-43` / `-12` were corrections for the
different `v`; the lengths already matched (12.8 / 62.8 / 102.8 game px) and
only the tops move with the rows.

### `brf_800C6930` is not the selection highlight: it draws the tree connectors

It was patched as the highlight once (box `[y, y+5]`, bar `[y+5, y+6]`), on the
belief that USA's `above` was the label's. It is not. The function positions,
per **flag-gated, indented item** — (27,28) `br_s01`, (29,30) `br_s03`, (31,32)
`br_s05`, (33,34) `br_s10`, (35,36) `br_s13`, (37,38) `br_s15` — the two textured
`br_line2` quads of an **L-shaped connector**: a *drop* from the parent row down
to the item and a short *bar* into its label. Both games draw them only while
the item's flag is 1, which is why they were invisible until the unlock.

Geometry, from USA's `800C910C` (its fifth argument is `K`):

    bar   [y+3, y+4]        x 14..22   (Integral had [y+10, y+11], x 30..44)
    drop  [y-K, y+3]        x 13..17   (Integral had [y-4,  y+10], x 29..33; the
                                        texture's line sits one texel in: draws at 14 / 30)
    K = s4, except br_s03's site (s4+8) and br_s13's (14)
    s4 = 10 in the outline block, 7 if the member block takes its 17-row branch,
         and 6 in the detailed block (800C99AC)

Integral's function takes no `K`, so it now reads it from `t8` (seeded 10 in the
outline block, set to `t9-10` by the member block, 6 by the detailed block),
adds 8 when `a2 == 30` and uses 14 when `a2 == 36`. The x's live in
`GetResources` (twelve `br_line2` calls, six per quad type) and are patched to
USA's; the first attempt moved only the bars and changed nothing visible, since
the drop is what shows. The last pixel found was the detailed drops being one
row taller: USA's `s4 = 6` there, not the member's 7.

### The member submenu with four or five items: USA's second branch

USA's member block has two branches. Three items: advance 20 (`br_s02` 30),
start `-21 - (20n-5)/2`, connector `K` 10. Four or five (Meryl and/or the
support crew unlocked): advance **17** (`br_s02` 27 = `17|10`), start
`-21 - (17n-2)/2`, `K` 7. Integral's advances were fixed immediates, so the
block is rewritten (same 44 words, two spare): the advance lives in `t9`, the
K base in `t8`, and the start is `(t9*(n-1) + 15) / 2`, which is `20n-5` or
`17n-2` in one expression (`mult`/`mflo`, no branch). The four `br_s03..s06`
positioner calls take `addu a3, t9, zero`; `br_s02` takes `ori a3, t9, 10`.
`t9` itself is computed by a five-word helper — `slti v1,a0,4; sll; addu; jr
ra; addiu t9,v0,17` — parked in the frame function's dead tail at `800C69A0`,
because the block had no room for it. `t8`/`t9` survive the block: its only
calls are our positioner (a0,a1,v0,v1) and frame function (t0,t1,a1,a3,v0,v1).
Simulated for n = 3, 4, 5 against USA's formulas before building: identical.

### `br_s01` (time limit) has its own reveal animation

Like `br_s00`, its x is animated every frame (`x0 = xl`, `x1 = xl + w*step/6`),
so the `GetResources` `xl` patch alone left it at Integral's 46 and 84 wide.
Integral's `w` is 84, which divides by 6, so the compiler folded the chain to
`14*step` and hardcoded 46 twice; USA's 52 does not, so it multiplies by the ÷6
reciprocal that `a0` still holds from the `br_s00` block (loaded at `800C7648`,
untouched in between). Rebuilt in place as `52*step/6 + 29`: the block's four
y-normalising stores are dead (the positioner writes all four y's each frame),
and the signed-quotient fixup is unnecessary for a non-negative numerator.
Settled widths 0, 8, 17, 26, 34, 43, 52 — USA's.

**Verified in game 2026-09-02, unlocked state (all sixteen items), sixteen shot
pairs against the user's USA set:** outline (2), member (5) and detailed (9),
after the `K = 6` fix every pair differs in 18–43 screenshot pixels (of 2.3 M),
all at game x≈155, y≈162–167 — left of the rule, grey 41 vs 97, identical in
every pair since the very first comparison, so a background detail rather than
anything the menu draws; no game pixel of the menu differs. Before the `K = 6`
fix each detailed shot had exactly two extra pixels, the tops of two drops. The
selection glow itself never differed: it follows the label quad.

**Verified in game 2026-09-02** against the user's USA set, ten shot pairs
(outline; member with each of three rows highlighted; detailed with each of six):
label row bands, rule extents and highlight boxes identical, and a pixel diff of
the right-column region (game x 150-320, y 55-190, tolerance 60/255) is 0.00%
on every pair. The FILE column was already identical.

### The horizontal connector is dim on the collection, and bright on hardware (2026-09-10)

Poly 25, the connector from the selected FILE button to the submenu's rule,
draws at full brightness on SwanStation and as a faint line on the Master
Collection - for USA, retail Integral and the port alike, so it is not the
port. Its texture `br_line1` is 4x2 with two **different rows**: row 0 grey
135, row 1 grey 23. The layout makes the quad one pixel tall (`y` -41 to -40)
while its V coordinates span both rows, so the one screen row has to pick a
texel and the renderer's interpolation decides which. Hardware and
SwanStation's software renderer take row 0; the collection's renderer samples
at the pixel centre, half-way down a two-row span, and takes row 1. The
2026-09-02 measurement of the connector's overrun past the rule as "+24
brighter" is that row showing through - grey 23 - and was recorded as "faint"
without asking why. `br_line2` (the rules and the L-connector drops) keeps its
bright texel in a column drawn 1:1 across the quad's 4-px width, so both
renderers agree on it. The discs are right and the collection was the odd one
out - and since 2026-09-11 MGSM2Fix corrects it at the source: `[Patches]
ThinTexturedQuads` hooks M2's GP0 polygon dispatch and gives a one-pixel
textured polygon the texel of its leading edge, for USA and Integral alike
(NextSteps §26, UPSTREAM.md). With it the connectors and the L-connectors'
bars draw on the collection as they do on hardware.

### The vertical rule

The rule left of the submenu items is **poly 26** (the first `br_line2`), found
by scanning every poly-x store for the rule's x: `x0/x2 = 19`, `x1/x3 = 23`.
Not `br_line1` (poly 25) — that one is a *horizontal* rule, and the code that
writes it only sets x, reading and writing y back unchanged.

Integral sizes it centred, from a hardcoded 20 per item — its **original** row
advance:

    v1   = (20n - 7) / 2
    s0   = -41 - v1            the list's starting y
    rule = [s0 - 2, v1 - 38]   length = 2*v1 + 5 = 20n - 2

Verified: that predicts 17 game px for the one-item operation-outline submenu
and 57 for the three-item operation-member one; measured 16.8 and 56.8.

USA's rule is structurally different — its code is a top-anchored animated
reveal (`y0/y1 = -55`, `y2/y3 = a3 - 55`, x ramped by a timer) rather than a
centred bar — and measures 12.6 and 62.8 for the same two submenus, i.e. it
grows ~25 per item against Integral's 20, in the opposite direction on each.

All three rules now use USA's constants; see "The rule constants are USA's
own" above. (An earlier version compensated for Integral's different `v` with
`-43` / `-12`; those are gone.)

### Horizontal: move the whole group, not just the labels

Integral's submenu sits **20 game px right of USA's**. The FILE column already
measures pixel-identical to USA, so both games share the actor origin — which
means this is genuinely different chrome, and USA's `xl` values are the target
rather than an offset from Integral's.

Moving the labels alone never worked: it either broke the label-to-rule gap or
pushed `the terrorists' armament` (128 px) off the 160 px edge. The rule has to
move with them, and every x involved is an immediate:

    800C6F28   s6  19 -> -1     vertical rule left    (polys 26 / 40 / 42)
    800C6F38   s5  23 ->  3     vertical rule right
    800C6F18   s4  addu s4,a3,zero -> addiu s4,zero,0
                               connector right end   (polys 25 / 39 / 41)
    label xl        26 -> 10, 46 -> 29   (USA's values)
    800C7674/76A4   26 -> 10             br_s00's animated x

**The layout block is not the last word on x.** Every rule and connector is
rewritten by an animated reveal (`x = 11p - K`) dispatched from the jump table
at `800C74C0`, and for the *outline* submenu that block is still active once
settled — so it won, and that rule stayed at game x 20, sitting inside
`infiltration method`, while the others followed `s6`/`s5` to 0. The animated
constants need the same shift:

    800C73A0  -46 -> -66   p25 connector right end
    800C7424  -24 -> -44   p39 connector right end
    800C745C  -46 -> -66   p41 connector right end
    800C74FC  -47 -> -67   p26 outline  rule x0/x2
    800C7508  -43 -> -63   p26 outline  rule x1/x3
    800C7580  -25 -> -45   p40 member   rule x0/x2
    800C7584  -21 -> -41   p40 member   rule x1/x3
    800C75B8  -47 -> -67   p42 detailed rule x0/x2
    800C75C4  -43 -> -63   p42 detailed rule x1/x3

The connectors' **left** ends are a separate matter and were wrong until
2026-09-10 - see "The connectors' left ends" below. Lesson: after moving
anything in this menu, check for a second writer — the settled frame and the
reveal animation set the same fields from different code, and which one wins
varies by submenu.

`s4` is `addu s4, a3, zero` — it reuses `br_s00`'s **advance** of 20 as an x
coordinate, so it needs a real load rather than an edited immediate, and `a3`
must keep its 20.

### The connectors' left ends (found by the user 2026-09-10)

The horizontal connector from the selected FILE button to the submenu's rule
(polys 25 / 39 / 41) starts at a hardcoded x, and this file used to say those
left ends "anchor to the FILE column and stay put". They anchor to the FILE
**boxes**, and the port gave the FILE column USA's boxes (`br_f00..03` at USA's
widths) while leaving the left ends at Integral's. Measured on SwanStation, the
outline connector began 7 game px inside the `operation outline` box (display
704 against a box edge at 750, where retail USA's begins at 741); in the
member submenu it stopped 8 px short of the box instead, because Integral's
value there is *right* of USA's. Both are the same bug seen from either side.

| connector | poly | Integral | USA | writers |
|---|---|---|---|---|
| outline | 25 | -46 | **-39** | `800C6F14` (layout, `s7`) and `800C7390` (reveal) |
| member | 39 | -24 | **-32** | `MEMBER_NEW[6]` (layout, `t0`) and `800C73F0` (reveal) |
| detailed | 41 | -46 | **-27** | layout stored `s7`, shared with the outline; now `t1`, loaded with -27 in `DETAIL_NEW`'s load-delay slot, stored at `800C717C`/`800C7184`; reveal `800C744C` |

`CONNECTOR_LEFT` / `CONNECTOR_LEFT_P41` in `brf_widen.py`. Why it survived the
26 shot pairs: those compared game x 150-320, and the left ends sit at 114-136.
The right ends (`ANIM_X`, moved by `GROUP_DX`) were already USA's.

At USA's `xl` nothing overflows (family A 10 + 128 = 138, family B 29 + 120 =
149), so the earlier right-edge clamp is gone and both families sit at USA's
real indent. An assertion keeps it that way.

### Constraints that must all hold

`pcx4.py` decodes/encodes the format: standard PCX, 1bpp × 4 planes, 128-byte
header, RLE where `code > 0xC0` is a run of `code - 0xC0`. The RLE stream is
**row-wide** — `PcxInflate4` decodes `stride * nplanes` bytes as one stream, so
runs cross plane boundaries.

1. **Width must be a multiple of 4.** `DG_LoadInitPcx` does `w /= 2` then
   `tex->dim.w = w / 2`, and `PcxInflate4`'s inner loop ends early when the
   remaining byte count is not a multiple of 4.
2. **Keep Integral's `px/py/cx/cy`.** USA's differ completely; using them
   uploads labels over the FILE menu and over the video timestamp.
3. **4-byte alignment.** Every archive entry size must be a multiple of 4. An
   unaligned entry misaligns everything after it and the loader takes a wild
   jump (seen as `pc: 30824000`).
4. **The U coordinate must not overflow.** `DG_SetTexture` keeps
   `off_x = (px % 64) * (16 / bpp)` in **texels**, and `brf_800C983C` sets
   `poly->u1 = off_x + w + 1` into a `u_char`. So the real limit is
   `(px % 64) * (16 / bpp) + w + 1 <= 255`, which is stricter than the
   unit-wise page test and supersedes it. Over 255 the U wraps and the quad
   samples across the whole page as vertical stripes.

   The V axis has the same limit: `off_y = py % 256`, `poly->v2 = off_y + h`,
   which also keeps a texture inside one 256-row tpage half. `br_s14` broke on
   it when the row height went to 20 (`239 + 20 = 259`).

   The U form was the navigation-order garble. `br_s09`/`br_s11` at `(928, …)` 128
   wide gave `128 + 128 + 1 = 257`; the unit test `(928 % 64) + 32 <= 64`
   passed because 32 + 32 = 64 exactly. They were the only two labels over the
   limit and the only two that garbled. Moving them to `px = 896` (`off_x = 0`)
   fixes it. Max across the build is now 249.
5. **Occupancy needs real colour depth.** The archive is 47 textures at 4bpp and
   4 at 8bpp; VRAM units are `ceil(w * bpp / 16)`, not `ceil(w / 4)`.
6. **CLUTs are on a 16-unit stride** and every label's is exclusive, so raising
   `n_colors` from Integral's 5 to USA's 16 fits exactly. Palette and
   `n_colors` must come from the USA artwork.

`brf_build.py` asserts every quad equals its texture, plus 1–5 above. That
assert caught a real bug: `brf_quads_all.json` had recorded `br_s12`'s `yb` at
`800CA214`, which is actually its `xl` (`addiu a3,zero,26`) — the patch had been
moving br_s12's left edge, and both immediates being 26 hid it.

## Option → SCREEN (brightness) screen

Ported by `optbright.py`. The two builds draw this paragraph by **completely
different mechanisms**, which is why no amount of searching USA for the English
text worked at first.

**USA draws it as one texture.** `option`'s DAR carries `sc_text`
(`GV_StrCode` 0x2FBD), 232x70, 4bpp, VRAM (512,256) — a single image holding
six pre-rendered lines:

    Adjust the monitor brightness so the gray
    scale below the green line cannot be seen,
    for the appropriate brightness to play this
    game.
    Press the O button to return to the option
    screen.

Integral's `option` DAR has **no** `sc_text` and Integral's option overlay does
not contain the string `sc_text` at all, so there is no code to draw it. The
resource-name strings in the two overlays differ exactly here: USA has
`sc_text`, Integral instead has `int_op_language1/1_w/2/3/3_w`.

**Integral draws it as font text from the GCL chain.** `opt.c:2750` binds every
KCB entry from the chain in order:

    for (i = 0; i < 31; i++)
        work->fEC4[i].string = GCL_GetString(GCL_NextStr());

`GCL_GetString` returns a pointer *into the chain* (`libgcl/parse.c:193`), so
the text is inline in the option stage's 0xFF chunk, chain at +0x1B8. The
brightness lines are:

| record | glyphs | `dword_800C3218` x,y | content |
|--------|--------|----------------------|---------|
| 13 | 20 | 36, 110 | line 1 |
| 14 | 20 | 36, 125 | line 2 |
| 15 | 20 | 36, 140 | line 3 |
| 16 | 19 | 36, 155 | line 4 |
| 24 | 18 | 36, 175 | `○ボタンでオプション画面に戻ります。` |
| 27 |  1 | 110, 300 | the parked colon — USA shows none |

The 15 px row pitch and the 20 px gap before record 24 match the screenshot
exactly (measured 65 px from line 1 to the ○ line; the table gives 175-110=65),
which is what identifies the records without a glyph table. Record 24's first
glyph is `901B` (○); record 25 is the same sentence without `○ボタンで`, used
by another submenu.

### Why the fifth line is not simply "no English counterpart"

Integral shows five lines; USA shows four. It is tempting to treat
`○ボタンでオプション画面に戻ります。` as Japanese-only and leave it, but it is
**not** — lines 5-6 of `sc_text` are its English counterpart, authored by
Konami. So this line can be ported without translating anything; it is only
USA's *option* screen that does not display those two rows of the texture.

### What the port needed — and the VRAM analysis that was wrong

**No VRAM work at all.** That is worth stating plainly because the first plan
here was a font-atlas repack, and it was built on a false premise. The formula
in `option_800C339C` / `font_get_buffer_size`,
`c_width = (rect.w * 4) / (c_skip + 12)`, reads like 12 px per character, which
makes a 42-character line need a 128-unit lane, which collides with the next
column, which forces a repack. Every step of that follows from the 12.

The 12 is only the *cell* size used to size the buffer. `put_hankaku_4bpp`
returns each glyph's own advance, so ASCII is **proportional**, and the wrap
test in `font_print_string` compares pixels:

    if (next_width > 0 && (x + dx + next_width + kcb->c_skip - 1) >= buf_width)

against `buf_width`, which is `kcb->width` = `(c_skip + 12) * c_width - c_skip`
= 252 px at the default 21-character budget, **less 12** — `font_print_string`
does `if (!(kcb->flag & FONT_NO_KINSOKU)) buf_width -= 12;` and opt.c passes
flag 0. So the real limit is **240 px**, and `option_char_width` can stay at its
default for all six lines: no lane, column or band moves.

The same bound shows up independently: `rect.w = kcb->max_width` and
`max_width` is a single byte, loaded `lbu` at 0x800C3598 in both retail's
overlay and ours, so 255 px is the engine's own hard ceiling for an option line
and the 240 px wrap sits inside it.

**USA's line breaks cannot be used verbatim, and this cost a broken build.**
USA's lines are 41-43 characters, and its `sc_text` renders them in 222-227 px —
so they look like they fit. They do not: Integral's half-width Latin glyphs are
wider than the font that texture was authored with. Measured on screen from our
own build, `"Adjust the monitor brightness so the gray"` — 41 characters —
renders **239 px**, one pixel inside the limit, and the 42- and 43-character
lines wrapped.

What a wrap does here is worth knowing, because it is not a cosmetic failure.
The continuation is drawn 18 rows down (`l_skip + 12`) inside a band that is
only 20 rows tall, so it lands on the CLUT row that sits at band + 20 — the
multicoloured pixel noise — and it keeps writing past `row * height + 32`
= 2,592 bytes of `GV_AllocMemory`, about 1.4 KB into the heap. The visible
result was overlapping doubled lines and corrupted palettes; the real result was
a heap smash that **froze the game** when a later screen allocated.

So the paragraphs are rejoined and re-wrapped to 36 characters, words untouched,
keeping USA's 4 + 2 line shape. Longest line measured 209 px, ~31 px of margin.
This is the same conclusion the preope recaps reached for the same reason, and
`integral-preope-three-limits` had already recorded the half of it that matters
most — that the formula's 12 px per character is not the font's real metric.
It does *not* say English fits USA's wrapping; measure our renderer, not USA's
art.

What the port actually changes:

1. `dword_800C3218` entries 13-16, 24 and 27 → x 43, y 118, 130, 142, 154, 166,
   178. USA's paragraph has a **12-row** line pitch with its first ink row at
   screen y 134 and ink left at x 43; measured on our own build, these entries
   put their ink at exactly the table's x and 16 rows below its y, so the table
   values are USA's positions less that offset. (The first attempt used +15 and
   x 42, from a calibration against *kanji*; the screenshot then measured ink at
   x 42 / y 135, which is where the 1 px corrections come from.)
2. Entry 27 — a colon USA never shows, previously parked at y=300 — takes the
   second line of USA's own wrapping. Its colour call moves from `case 5` (the
   vibration screen) to `case 7` (this screen). Colours are reset for all 31
   entries before that switch, so dropping it from case 5 blanks the colon
   there, which is what USA shows.
3. Records 13-16, 24 and 27 get USA's words on the 36-character wrap. The ○ is
   the font's own glyph `90 1B` mixed with ASCII, exactly as `int1_en.exe`
   already ships (`Press \x90\x1b to zoom in,`), and it costs 12 px rather than
   ~6, since `font_get_glyph_width` returns 12 for anything non-hankaku.
4. The chain delta is held at **exactly zero** by padding record 27 with 15
   trailing spaces, so no container size field moves and nothing after it
   shifts. A large negative shift has crashed this script before. The padding
   goes on the *shortest* line, because trailing spaces widen `max_width` too —
   padding a near-limit line would push it into a wrap.
5. The font atlas is retail's again — `option_column_x` gone, entry 12 back to a
   64-word rect, `option_char_width` deleted. Against `font.res` the three
   ported strings measure 174, 142 and 137 px in a 240 px lane, so none of that
   widening was ever needed, and removing it is most of what got the overlay
   back to retail's size.

### The overlay must not exceed retail's byte count

This is the hardest-won constraint in the whole stage, and it is not about
sectors. `option`'s overlay loads at a fixed address, so **one byte past
retail's 25,842 corrupts whatever follows it** — and the symptom is a freeze on
an option row that has nothing to do with the edit. Measured, by bisecting one
build at a time against a stock run:

| overlay | vs retail | KEY CONFIG row | EXIT row |
|---------|-----------|----------------|----------|
| 25,950  | +108      | freeze         | freeze   |
| 25,874  | +32       | ok             | freeze   |
| 25,842  | **+0**    | ok             | ok       |

The 26,624-byte padded slot is *not* the limit and is far too generous to
protect you; `optbright.py` asserts against retail's actual size, read from the
base image. Note how badly this misleads: at +32 only EXIT broke, which looks
exactly like a bug in the EXIT row. The earlier option work grew `f924[8]` to
`[12]` chasing that freeze, and that change — plus `option_char_width`, entry
12's 128-word rect and the column move — is *what pushed the overlay over*.
Growing a struct or adding a helper to fix an option-screen freeze is very
likely to cause one.

So before theorising about any option-screen freeze: **compare the overlay's
size with retail's.** Then bisect against a stock run. Both were skipped here,
at the cost of a dozen relaunches.

`f924` is back at retail's `[8]` for the same reason. Retail reads *and writes*
past its end into `kcb[0]` because the cursor states run to 11; retail tolerates
that, and correcting it costs 16 bytes of struct plus every offset after it.
Leave retail's quirks alone unless there is evidence they hurt.

**Superseded: the texture route shipped, and the paragraph is now pixel-exact.**
Everything above about the 240 px wrap and the 36-character re-wrap describes the
*font* path, which is still what the option screen's other entries use — but the
brightness paragraph is no longer one of them. See "The sc_text texture port"
below. The rest of this section is kept because the font limits it documents are
real and still bound every other entry on the screen.

For the record, the reasons this route was first rejected were both wrong. "No
spare poly" — the screen submenu's `POLY_FT4 field_5D4[4]` / `int f2AFC[4]` grow
to 5 harmlessly (see below), and the real bound was `GM_MakePrim`'s pack count,
not the arrays. "USA's runtime moves the quad" — it does not; USA's `sc_text`
string is referenced exactly once in its overlay and there is no second writer to
that poly. The apparent movement was my own coordinate-system error.

## The sc_text texture port

USA draws the brightness paragraph as **one 232x70 4bpp texture**, `sc_text`, on
a quad - never with the font. Integral had no such texture and no code naming it.
Porting it makes the paragraph pixel-exact, because it is USA's own artwork on
USA's own quad: USA's line breaks, USA's glyph rendering, all six lines, none of
the font path's limits. Built by `optsctext.py`.

| | |
|---|---|
| overlay | 25,830 bytes |
| DAR | 121,680 -> 125,856, 56 -> 57 entries |
| stage | 75 -> 77 sectors, relocated to **DUMMY3M idx 384** |
| sc_text | VRAM (512,256), CLUT (1008,237) |
| key_pad | moved (512,256) -> (512,326), which is what USA does |
| quad | `Init_Res(work, GV_StrCode("sc_text"), po, -121, 14, 111, 60, 0, 0)` — texture cropped to rows 0..45 |

### The three things that would have shipped as bugs

**`brf` is also relocated into DUMMY3M**, at idx 128..266, next to `preope`'s
0..89. The obvious slot - 90, just past preope - would have overwritten 40
sectors of the shipped briefing stage. Worse, a blankness check would have
*passed*, because PPFs are applied by the loader at runtime and never written
back, so the image on disk shows all 13,501 DUMMY3M sectors as zero. Occupancy
must include other patches. `optsctext.py --deploy` checks deployed occupancy;
`rebuild.py` checks overlapping writes across the complete packaged set. Slot 384 leaves brf 256 sectors of growth room.

**`menu.ppf` writes chain records 4 and 5** ("screen brightness setup", "key
configuration setup") into the option stage. After relocation those writes land
on a stage the game no longer reads, so both labels would have silently reverted
to Japanese. The relocated image is now built directly from retail with
`optlabel2.build` reconstructing every owned caption. Unowned records, including
record 7's colon, match retail. The old pinned font-text PPF is no longer an
input; `verify()` checks the final rebuilt chain before emitting.

**Never let the builder consume its own previous output** — this shipped as a
bug on 2026-09-02. Once the sc_text PPF was deployed, `composite()`'s "every
deployed PPF" included it, and its entry repoint would send the walk out of the
file; my workaround was to move the deployed option PPF aside before rebuilding.
That silently dropped the font-text PPF's 1,519 bytes of chain edits, so records
3/7/12/13-16/24/26 reverted to retail Japanese: "use directional buttons to
test" came back as `振動テスト...` with a garbled glyph (the user caught it). The
static record check had been run on the *previous* build, not the one deployed.
The current retail-based builder removes that feedback path entirely, and the
record assertions run on every build.

**The quad's y: the two engines do not place the same constant in the same
place.** USA's own call is `(-121, 2, 111, 72)`. Deployed here, that rendered the
paragraph exactly **12 rows above** USA's on-screen position — measured `dy -12`
on all four lines with `dx 0`, i.e. horizontally pixel-identical, vertically
shifted. The measurement-derived `y0 = 14` (which an adversarial review had
talked me out of in favour of "just copy USA") turned out to be correct. Lesson:
neither copying USA's constant nor deriving one from a screenshot settles a quad
position — **only a like-for-like screenshot comparison after deploying does**,
because the two builds' draw environments evidently differ. One shot, measured,
fixed it.

**USA shows four lines; the texture has six.** Rows 47–69 (the ○-button
sentence) never appear on USA's screen, so the texture is cropped to rows 0..45
in the DAR (line 4 ends at row 45, row 46 is blank). Data-only, keeps the quad
1:1, and the crop is asserted to decode round-trip.

### Why the fifth quad rides the existing loop

Two ways to add it. Appending a `POLY_FT4` to the end of `Work` and copying it
explicitly leaves every existing field offset alone - but it needs a per-frame
40-byte struct copy, and measured **+156 bytes against 92 reclaimed**. Growing
`field_5D4[4]`/`f2AFC[4]` to 5 lets the existing copy loop carry it for nothing:
**-12**. The real allocation bound was neither array but `GM_MakePrim`'s pack
count for `field_2C`, 4 -> 5, which is heap.

Growing `f2AFC` shifts which `f2B0C` slots retail's key-config init overrun
zeroes, from `[3..12]` to `[2..11]`. That is inert, twice over: those writes
store literal `0` into memory `GV_NewActor` has already zeroed, and entering KEY
CONFIG sets **all 17** of `f2B0C[0..16]` before the screen draws. The
`f924` -> `kcb[0]` overrun relationship also survives, because `field_5D4` shifts
`f924` and `kcb` together.

### A correction on the overlay size limit

The Gotchas section says the option overlay must not exceed retail's 25,842
bytes, from a `+108` / `+32` / `+0` bisection. **That bisection was confounded**:
the commit that took it to +0 also reverted `f924[12]` to `[8]`, and *that* is a
real freeze mechanism - retail indexes `f924[work->f920]` with cursor states up
to 11, so states 8..11 read and write `kcb[0]`'s first bytes, and enlarging the
array redirects those reads to zeroed slots and changes which branch each state
takes. Meanwhile overlays load into the BSS tail at `StageCharacterEntries` with
roughly 118 KB of headroom - the largest stage overlay on the disc is 143,856
bytes against option's 25,842 - so +32 bytes cannot plausibly overflow anything.
Treat "stay at or under retail" as cheap insurance rather than the mechanism, and
treat **any** change to `f924` as the thing to be afraid of.

### Verification before deploying

`optsctext.py` asserts, and these all passed: a loader-style DAR walk consuming
exactly 57 entries with `remaining` hitting 0 and every entry 4-aligned; UV fit
(`off_x` 0 + 232 <= 255, `off_y` 0 + 70 <= 255, 58 units from a 64-aligned x so a
single tpage); DUMMY3M disjointness against the composited PPF map plus a
blankness read of all 78 target sectors on both discs; the STAGE.DIR entry
reading retail's 27,136 before being repointed. Then, end to end on **both**
discs: applying the emitted PPF to the disc image in `dlc_japan.bin` resolves
`option` to DUMMY3M idx 384 and reads the 78-sector block back byte-identical,
with exactly one record outside the slot (the entry repoint).

Verified in game 2026-09-02: the four paragraph lines occupy screenshot rows
1215-1286, 1323-1385, 1431-1502 and 1557-1601 in both the USA shot and ours —
pixel-identical placement — and the vibration-test help line is English again.

### Reverting

`work/backup_before_sctext/` holds all 12 PPFs from before this change, and
`fonttext_disc{1,2}_option.ppf` in that folder is specifically the previous
font-text option build. Copy those two back over
`mods/INTEGRAL/INTEGRAL/{0,1}/INTEGRAL_disc{1,2}_en_option.ppf` and the
brightness screen returns to the re-wrapped font text, which is positionally
exact but not pixel-exact.

Re-running the builder after deployment needs nothing moved aside: the caption
chain is reconstructed from retail by `optlabel2.py`, without any prior PPF.

### Building and deploying

**Historical — the font-text build.** `optbright.py` needed `work/int1_stage.dir`
(retail), `work/int1_stage_opt11.dir` (whatever the deployed option PPF
contained) and a rebuilt `D:/mgsbuild/d/obj/option.bin`, and rewrote both
discs' option PPFs in place. Since 2026-09-04 `optsctext.py` builds from retail
and stages (see [BUILD-HISTORY.md](BUILD-HISTORY.md)); nothing below is a build step.

No Integral disc image is on disk, so the STAGE.DIR LBA cannot be looked up the
way `ppfgen.py` does it. Instead the script **recovers the geometry from the
deployed PPF and proves it**: it solves `lba` from the first record, checks that
mapping against all 2,610 shipped records, and then re-encodes that same diff
and requires it to reproduce the shipped file byte for byte before writing
anything. Disc 1 is LBA 136654, disc 2 is 105178, both mode 2 form 1
(24-byte sector header).

### Dead ends, recorded so they are not repeated

`sc_back_l` and `sc_back_r` (160x224 4bpp, VRAM (80,256) and (208,256)) are the
full-screen background halves and are **textless in both builds** — they hold
the grey ramp, the green line, the 57/49/41/33/24/16/8/0 numbers and the grid.
Decoding and rendering both confirmed it.

The English paragraph is not ASCII anywhere: not in `us1.exe` (a PS-X EXE whose
SDK strings *are* greppable, so ASCII would have been found), `us1_stage.dir`,
`us1_brf.dat`, `us1_radio.dat`, `windata/alldata.bin` or either
`windata/dlc/dlc_*.bin`. `grep` on `alldata.bin` does find "screen brightness
setup" and "use directional buttons to test" twice each, which proves the search
was working. A delta-pattern search (invariant under any uniform additive
offset) and UTF-16/high-bit/XOR/0x81-padded variants all found nothing either.
The one `Adjust` hit in `us1_stage.dir` at 0x29915A2 is a false positive inside
a symbol map (`GM_ConfigMotionAdjust`); the two `monitor` hits are the source
filename `monitor1.c` in `s08b`/`s08br`.

USA's option chain really does have the paragraph slots empty. Its whole 680-byte
0xFF chunk holds 28 records of which only four are non-empty — `[4] screen
brightness setup`, `[5] key configuration setup`, `[12]` and `[26] use
directional buttons to test` — with `07 01 00` (a bare terminator) for the
rest, including 13-16.

## Audit against the decomp

Findings from re-checking the shipped work against the decompiled source.

**The overlay entry point does not need pinning.** Every overlay carries its
entry address in its own header: `obj/preope.bin` word[1] is `800C4DA4`,
`obj/option.bin` word[1] is `800C93A8`, and the build writes it. Nothing else
references either address — an exhaustive search of `int1.exe`, `STAGE.DIR`,
`BRF.DAT`, `DEMO.DAT` and `RADIO.DAT` for the literal word and for genuine
`lui`+`addiu` pairs finds only the overlay headers themselves. The one apparent
hit in `int1.exe` forms `800B4DA4`, a different address.

`opt.c` proves it empirically: our changes moved `NewOption` +100 bytes from
retail's `800C9344` to `800C93A8`, the header followed, and the option screen
works in game. So the documented reason for `preope_reserved()` is wrong, and
the −152-byte shift that once killed the host process had some other cause. The
padding is retained untested; removing it would free ~100 bytes in an overlay
that has a hard size limit.

**"GCL chain size ceiling" names a symptom, not a mechanism.** `libgcl/parse.c`
imposes no limit on the number or total size of chain records — records are read
in place by pointer, `argbuffer[32]` holds command arguments only, and the
1-byte length field caps a single record at 255 bytes, which nothing here
approaches. The stage buffer is `GV_GetMaxFreeMemory(GV_NORMAL_MEMORY)`, so a
memory interaction is the likely mechanism; our `Work` also grew 14,336 bytes
(184 → 248 entries × 224). The 9,567–10,304 threshold is real and reproducible,
but its cause is still unidentified.

**Leaving the GCL cursor short is safe.** Replacing MG2's 152
`GCL_GetString(GCL_NextStr())` calls with a blob walk leaves `next_str_ptr` 152
records behind. `preope.c` reads the chain in only three places (lines 810, 819,
829), all before that point, and the interpreter tracks its own `commandline_p`,
so nothing downstream is affected.

**`option_char_width()` is dead code.** It returns 24 for entries 4 and 5, but
`font_set_kcb` clamps: `w2 = (rect.w * 4) / (c_skip + 12)` = 21 for a 64-word
column, and `if (width > 0 && width <= w2)` fails, so `c_width` stays 21. Those
two lines render correctly anyway — `font_get_buffer_size` gives a 252-pixel
buffer and the proportional font fits 23–24 English characters in far less.
Harmless, but the comment claims an effect it does not have.

**Correction (2026-09-04): retain `f924[8]`.** The earlier claim that growing
it to `[12]` was a fix is withdrawn. Retail reads/writes beyond the array into
`kcb[0]`; preserving that relationship is necessary for the working overlay.
Growing it shifts the subsequent fields and freezes EXIT. The checked-in
decomp patch retains eight entries. See "Overlays" and "A correction on the
overlay size limit" for the measured evidence.

**Verified correct.** `dword_800C3218` entry 12's `num = 1` really does centre
(`rect.x = x - max_width / 2`). Both `opt.c` and `preope.c` are fully decompiled
with no `INCLUDE_ASM`, so growing their `Work` structs is safe. The
45-character wrap is consistent with the formula, not just with testing.

**Stale comment.** `option_column_x`'s comment says entry 12's texture sits "at
832"; the code assigns `option_column_x[1]` = 896. The layout is sound — entry
12 occupies x 896–1021 at y 256–276 and column 3 starts at y 277 — but the
number in the comment is wrong.

**Fixed.** `preope.c` compared the string terminator against a literal 0x00 byte
embedded in the source rather than the `'\0'` escape, which made the whole file
binary to `grep` and `diff`. psyq compiled it identically (`obj/preope.bin` is
byte-for-byte the same before and after, entry still at `800C4DA4`), but any
tool that re-saved the file would have silently dropped the byte.

## Unlocks: what gates the title-screen extras, and how the briefing is forced

Needed because the briefing-menu port can only be checked in the unlocked
states (Meryl / support crew / `br_s10` / `br_s13` / `br_s15`) and the user's
save has none of them. Read from the decomp (`onoda/brf/b_select.c`,
`onoda/open/open.c`, `libgcl/variable.c`) and the stage scripts.

**Briefing items are GCL `$f:` flags.** `GetResources` (`brf_800C99C0`) reads
the menu's `-f` option — sixteen values — with `GCL_GetOption('f')` /
`GCL_NextStr` / `GCL_StrToInt`, and the brf stage script (tag `c?`, offset
0x374, byte-identical in Integral and USA) supplies them as sixteen variable
references `14 bb 00 4c/4d/4e`: `GCL_GetVarTypeCode` 4 = bool, offset 0x4C–0x4E
into `var_buf`, bit `bb` — i.e. bits 0x4C.1–7, 0x4D.0–7, 0x4E.0. An item is
counted and drawn only while its flag == 1. `var_buf` is `libgcl/variable.c`'s
static 1024-short GCL variable memory: zeroed by `GCL_InitVar` on boot (a stage
init with `-v`), set by the stage scripts as the story advances, saved and
restored with the game. So a fresh boot shows 1 / 3 / 6 items in both games,
and the earlier idea that the flags meant "watched" was wrong — after a movie
ends the overlay stores `field_80[idx] = 1` too (800C9240), which is the same
state.

    var_buf     Integral SLPM-86247/86248  0x800B3CC8    USA disc SLUS-00594/00776  0x800B6448
    linkvarbuf  Integral                   0x800B4D98    USA disc                   0x800B7518

**The Master Collection's USA executable is not the retail disc's.** Read live
through the sqdbg bridge, its `GCL_GetVar` says `addiu a3, a3, 0x6440`: var_buf
at `0x800B6440`, 8 bytes below the `us1.bin` value, and MC's `scene_name`
define is `0xB7500` where the disc layout predicts `0xB7508`. Writes at the
disc-derived address landed on var_buf+0x54 and the menu never saw them — while
the log still said "Unlocked", because the read-back was of the same wrong
bytes. So `UnlockBriefing` does not hardcode: `variable.c` declares `var_buf`,
`sv_linkvarbuf`, `sv_var_buf`, `stage_name`, `linkvarbuf` in that order, which
puts var_buf 0x10D0 below linkvarbuf and MC's `scene_name` define (it is
`stage_name`) 0x10 below it, so `var_buf = scene_name - 0x10C0`. Checked against
live RAM in both games: Integral `0xB4D88 - 0x10C0 = 0xB3CC8`, USA
`0xB7500 - 0x10C0 = 0xB6440`, both matching the running code's constants.

MGSM2Fix's `GetRamValue`/`SetRamValue` take **RAM offsets**, not KSEG0 addresses
(the memory defines log as `0xb4d9d`; Ketchup's `PSX_ImageBase` is `0x10000`).

**`[Game] UnlockBriefing = true`** (MGSM2Fix, `src/mgs1.cpp`) holds all sixteen
flags set while the scene name is `title` or `brf`, and never otherwise —
var_buf is the live game's flag memory once a stage runs. Relies on the brf
stage's init not carrying `-v` (which would zero var_buf on entry, ahead of any
per-frame poke); the title script's `-v` sites are on other commands. Confirmed in game on Integral (all sixteen items, 2026-09-02). Setting the flags also makes the flag-gated frame polys
(27–38, see the brf section) draw, which is what USA does in the same state.

**Everything else on the title screen comes from the memory-card scan**
(`open.c` ~7600–7800), which reads only the *names* of the saves on the card:

    name[12] == 'G'   a game save; `(name[17] - '@') & 7` is the clear rank ->
                      fB2C[0..3] -> demo_rank 0..6 (EXTREME needs demo_rank != 0)
                      and, non-zero, has_clear_data = 1 (title BSS 0x800D92D0)
    name[12] == 'C'   photo data  -> photo_flag
    name[12] == 'V'   VR data     -> vr_flag

`spe_rank = photo_flag + 2*(demo_rank != 0) + 4*has_clear_data` picks the
SPECIAL menu page, and `has_clear_data` also runs the title script's `-k`
proc (0x0A1E). Not forced here: `demo_rank`/`photo_flag`/`vr_flag` live in the
title actor's heap Work, and the Master Collection keeps its saves in
`userdata/<id>/2131630/remote/data_008_0000.bin`, not as a raw card image. If
those states are ever needed, the honest lever is a cleared save (rank 6) plus
photo and VR saves; the cheap one is a PPF on the `title` overlay forcing the
scan's results.

## Give items (test aid): `[Game] GiveItems`

Added 2026-09-04 so disc 2, a Mei Ling save and the PHOTO ALBUM could all be
tested from one stage-select start instead of a playthrough: the Camera is
otherwise behind the Nuclear Building B2 armoury's level-6 door.

`GiveItems = 12` (the Camera; the ini lists every id) writes `GM_Items[id] = 1`
and `GM_ItemsMax[id] = 1` once per gameplay stage, only where the count is still
zero. Addresses come from `include/linkvar.h`: `GM_Items` is
`&linkvarbuf[37]` (byte `+0x4A`, "0x4a Items"), `GM_ItemsMax` is `GM_Items + 24`
(`+0x7A`), both shorts, and `linkvarbuf` is `scene_name + 0x10` — the relation
`UnlockBriefing` already uses and that was read back on both Integral and USA.
Gameplay stages are the `sNNx` / `dNNx` names; title, menus and the developer
`select` stage are left alone. The developer stage select itself
(`[Game] StageSelect`) sets no inventory — `stage/select.c` only spawns
`CHARA_STAGESELECT` and the vibration editor — so this is what fills the gap.

`linkvarbuf` is saved with the game, so a save made after the grant keeps the
item. Use it on a test save with achievements off.

## Unlock everything (test aid): `unlock_title.py`

Both games gate their title-screen extras on the memory-card scan in the
`title` overlay (`open.c`, `title_open_800D1CB4`): a cleared game save gives
`demo_rank` 1–6 (EXTREME difficulty, DEMO THEATER) and `has_clear_data` (1P
MODE, Integral only), a photo save gives `photo_flag` (PHOTO ALBUM), a VR save
`vr_flag`; `spe_rank = photo + 2*(demo≠0) + 4*clear` selects the SPECIAL page.
`unlock_title.py` patches the *derivation* of those results in the overlay, so a
fresh save sees everything, and emits one PPF per disc addressed at the `title`
stage's sectors (not relocated): `INTEGRAL_disc{1,2}_unlock_title.ppf` and, for
the USA reference game, `mods/MGS1_US/{0,1}/MGS1_disc{1,2}_unlock_title.ppf`.
Named `_unlock_`, not `en_`: delete them to restore normal gating.

| site | Integral | USA |
|---|---|---|
| `if (photo == 1)` / `if (vr == 1)` guards on the work stores | `bne` → `nop` ×2 | same |
| "no clear save" branch (`demo_rank = 0`) | store 6 instead, jump past the rank chain | same |
| `if (has_clear_data == 1) spe_rank += 4` | store 1 into `has_clear_data` where it was read, drop the test | absent in USA (no 1P MODE, no `has_clear_data`) |

Result: Integral `spe_rank` 7 (page with PHOTO ALBUM, DEMO THEATER, 1P MODE),
USA `spe_rank` 3 (its maximum: PHOTO ALBUM, DEMO THEATER); EXTREME selectable
in both. Every patched word is asserted against retail first.

**USA title overlay addresses are label −8.** The USA `title` payload's code
sits 8 bytes later than a base-plus-offset label predicts — its header entry
(`800D5578` vs the labelled `NewOpen` at `800D5580`), its printf references and
every `j` target agree — while USA's `brf` overlay does not have this offset.
File offsets are label-based, so patches land; encoded jump targets must use
the true (label −8) address. This is also why the USA release build's
`demo_rank` chain read as nonsense on first sight ("default 4"): the labels
were off, not the code.

### The briefing row box on a real disc: a load-delay hazard (found and fixed 2026-09-10)

`en_brf`'s `ROW_H` - eleven instructions at `0x800C69C8`, inside
`brf_800C69B4(work, idx, y, advance)` - drew the briefing's right column as
vertical stripes of sampled VRAM on SwanStation's software renderer, while
the Master Collection rendered the same bytes cleanly. The bisect (NextSteps
§24) narrowed it to those eleven words and then spent six formula guesses on
them. The arithmetic was right. **The scheduling was wrong.**

The R3000 delivers a load one instruction late: the instruction in the slot
after `lbu a1, 13(v0)` reads the *old* `a1`, which here is the caller's poly
index (9..24). The port's `subu v1, a0, a1` sat in that slot, so the height
came out as `v2 - idx` - a hundred rows and more - and every label was
stretched down the column. Retail's words never read a register in the slot
after loading it, because the compiler schedules for the delay; the port's
were written by hand and did not. The fix is one `nop` between the second
load and the subtraction, eleven words in place, no stub, no stage growth.

**Why the collection hid it.** M2's emulator does not model the load delay,
so the arithmetic ran as written there, and the 26 shot pairs that signed
`en_brf` off were drawn by it. The collection is not only a different
renderer; it is a **lenient CPU**, and no screenshot taken there can catch
this class of bug. That makes the check static: `hazards.py` scans every
word the port writes against retail's for a load followed at once by a read
of the loaded register (and three rarer patterns), `brf_build.py` asserts
zero before it writes the stage, and `selftest.py` proves the scanner
catches the 2026-09-10 pattern at its own address. The same scan over every
other hand-patched object - both executables' pools, the language default,
the VR MOVIE stub, the VR option call sites - found nothing else.

**What the failed attempts had in common**, for anyone who meets the
symptom again: every variant that broke had a load-use pair (`lbu` then
`andi`, `lbu` then `subu`, `lh a1` then `sh a1`), and every variant that was
clean had none. A hand-written MIPS sequence that "should work" and does
not, on hardware or an accurate emulator but not on the collection, is this
until proved otherwise.

### 1P MODE (Integral only): its Japanese pages, and the language it starts in

Selecting 1P MODE on the SPECIAL page runs the title script's `-s` proc
(0x1137). That proc does not start the game: it opens Integral's VR-style text
window (`koba/vr/vrwindow.c`; actor 0xD44E with `-w 32 53 256 118`,
`-m 16 17 240 101`) on **twenty-one pages of Japanese**, five procs each
chaining to the next through the window's `-p` option — 0x28CC (3 pages),
0x19B7 (5), 0x19B8 (5), 0x19B9 (3), 0x19BA (5) — and the last one ends on proc
0x963D. The pages exist only as the Japanese `-b` strings: USA has no 1P MODE
and Integral has no English for them, so under the no-translation rule they stay
exactly as shipped. They are a kept Integral difference, not an unported one.
(`gclprocs.py` / `gcldump.py`, in this directory since 2026-09-04, decode a stage script's procs;
the title's proc table sits at chunk offset 0xEC: BE32 length, then
`id:BE16 offset:BE16` entries ending in a zero word, bodies after it, script
body at 0x10DE.)

Proc 0x963D is the 1P game start and mirrors NEW GAME's proc 0x13EF: `start -v`
(GCL_InitVar: zeroes var_buf and linkvarbuf but restores GM_GameLevel and
GM_Configuration), then `$w:118000be` = GM_FirstPerson = 1, GM_GameLevel = 1
(NORMAL — the last page says so), GM_Disk = 1, the shared item/weapon init proc
0xC8CF, and after 24 frames proc 0xB7ED: `load d00a -m … -s 1`, the same stage
NEW GAME loads. The log agrees: `scene_name "title" -> "d00a"`.

**Language.** The 1P game ran in Japanese although English was selected. The
language is GM_CONFIG_ENGLISH (0x0100) in GM_Configuration (linkvarbuf[2],
0x800B4D9C; MGSM2Fix's `language_setting` define is that word's high byte,
0xB4D9D, mask 1. Exactly four places act on the bit: `radiomes.c:526` picks
the English half of a codec fragment, `radio.c:1320` picks `NO RESPONSE` over
its Japanese twin, `movie.c:54` `MovieType` and `jimctrl.c:390` pick the
stream. `opt.c` sets it and reflects it into the option row; nothing else in
the decomp reads it. **`font_draw_string` does not** - an earlier version of
this line said it did, and the correction matters: no text draws differently
because of this bit, so nothing this port wrote in place depends on it).
Everything that could clear
the bit was checked against the decomp and the binaries, and nothing on this
path does:

- the 1P procs never reference `$w:11800004`. Across every stage script on both
  discs (exact GCL encoding: type nibble via `(v<<1)>>25`, link flag
  `v & 0xF00000 == 0x800000`, offset `v & 0xFFFF`) the only writes to it are
  `abst`/`rank` toggling the tuxedo bit 0x20; `roll`, `s10a`, `s18a` only read
  it (English credits and text branches).
- the executable has no direct store to 0x800B4D9C. Stores through the
  `linkvarbuf` base are GCL_InitVar (restores it), GCL_SetVar (scripts, above),
  DrawReadError (VIBRATION_OFF) and five `title` sites — all `andi 0xF7FF`,
  the RADAR_OFF clears on the DEMO THEATER items.
- `datasave.c` rewrites the 0xE100 option bits only while loading a save; no
  save is loaded. GCL_RestoreVar (continue) and `load -r 0` hard restarts are
  not on the path either.

So the writer is outside the game: the Master Collection's own layer — and a
`setRamValue` trace added to MGSM2Fix (2026-09-03) names it. **The collection's
script `_update_option_button_setting` (`system/script/play_standalone_mgs.nut`,
line 844, called from `poll` every frame) writes the entire 16-bit
GM_Configuration word at 0xB4D9C about sixty times a second**, with values such
as 0x100 (English, button type A) and 0x110 (the same plus the game's
"options changed" bit 0x10 once the option screen has been visited). It is how
the collection imposes its own button-type setting on the game — and the reason
the KEY CONFIG screen was once thought to be "intercepted" by the collection -
that attribution is withdrawn, see the KEY CONFIG section. It is a
read-modify-write of the live word — confirmed by the read trace: the same
function does `getRamValue(16, 0xB4D9C)` at line 837 and `setRamValue` at line
844, every frame — so any game-side write to GM_Configuration
that lands between the collection's read and its write is lost, and the
language bit is one of those. The run in which 1P MODE played in Japanese
happened without these diagnostics; the next run, with them, kept English
throughout (2,700 writes traced, none changing the word), so the loss is
intermittent and was not reproduced. The collection's earlier access hook on
GCL_SetVar (`ACC HOOK: L80021778 R5=100`) and the `MEDAL_MISSION_START` unlock
turned out to be unrelated achievement plumbing.

MGSM2Fix now: logs every change of the language byte with its scene; traces the
collection's writes to the word (all of them at first, then only the ones that
change it) and its first reads, each with the Squirrel call stack; and
**restores English whenever the bit is cleared in any scene but `option`** —
the option screen being the only place the player can change it, a change seen
there is followed instead. The intro pages are untouched by any of it.

The general fix is **`[Patches] PreserveConfiguration`** (default on, every MGS1
version): at the collection's `getRamValue` of the word MGSM2Fix notes what it
is about to read; at its `setRamValue` it re-reads the word as the game has it
by then and rewrites the collection's argument in place to carry only the bits
the collection actually changed (`(now & ~changed) | (value & changed)`), so
the button-type enforcement survives and every other setting the game wrote in
between survives too. It logs `Preserved GM_Configuration` when it had to
intervene. The word's address is `scene_name + 0x14` (linkvarbuf is 0x10 above
variable.c's `stage_name`, GM_Configuration is linkvarbuf[2]); a version where
that did not hold would never see a collection write at that address and would
be left alone. The EnglishText guard stays as a second layer for the language
bit. This fix is not Integral-specific and is tracked for a separate upstream
pull request in `UPSTREAM.md`.

Also visible in that log: `Set the sixteen briefing flags (… write 3, scene
"title")` immediately after the 1P `start -v`. UnlockBriefing re-sets the flags
GCL_InitVar has just zeroed while the scene is still `title`, so a game started
from the title (NEW GAME or 1P MODE) begins with all sixteen briefing flags set
in its var_buf. That is the option doing its job, but it is state a stock game
would not carry into a new save; keep UnlockBriefing off when saves matter.

## Unlock every VR mission (test aid): `vr_unlock.py`

The VR discs gate their missions on something the main game never uses: a
**bitmap held in VRAM**. `selectvr`'s window manager (`nobu/vr_slct/winmngr.c`,
chara `0xAE06`) owns a 384-byte buffer — 16 rows of 6 words, 192 bits per row —
and copies it to and from the video RAM rectangle **(160, 224) 12×16** with
`LoadImage` / `StoreImage`. Every one of the ninety mission stages carries the
same rectangle in its own overlay, so a mission sets its bit in VRAM as it is
cleared and the bit is still there when `selectvr` reloads: **VRAM is what
survives a stage load.** The memory-card save is built from that buffer (plus
0x226 record shorts and one flag word), so the bitmap, not the menu's item
flags, is the progress.

Two static byte tables turn (group, item) into a bit:

    row = A[group] - 1     A at INT 0x800CE944 / USA 0x800D1DF0
                           A == 0 means the group has no missions
    bit = B[group] + item  B at INT 0x800CE98C / USA 0x800D1E38

and the menu itself is 70 groups of 28 bytes (INT `0x800CDB54`, USA
`0x800D0E08`; count at +4, item array at +8, items 60 bytes each). An item's
first word is its flags:

| bit | meaning |
|---|---|
| `0x80000000` | drawn |
| `0x40000000` | **unlocked** — selectable, and drawn bright instead of grey |
| `0x20000000` | gated: consult the clear bitmap for this item |
| `0x10000000` | a menu entry rather than a mission |
| `0x00080000` | cleared (set every frame, never stored) |

Once per frame the manager walks all 70 groups. Each item with `0x20000000`
asks the bitmap; a set bit marks it cleared and **unlocks the next item in the
group** (INT `0x800DBE2C`). A separate pass derives, per group, whether *every*
item is cleared into a 70-bit array (INT `0x800C12AC`), and a long cascade then
hands `0x40000000` to items of other groups — the ladder that opens the harder
modes. A third gate scores the bitmap (`count / 3` against 80 / 70 / 60 / 50)
and opens the last four categories' first items.

All three questions go through one instruction each, so the test aid is three
words:

| | Integral | USA |
|---|---|---|
| group fully cleared? | `0x800DBDA0` | `0x800DF32C` |
| mission cleared? | `0x800DBE28` | `0x800DF3B4` |
| progress score | `0x800DC528` | `0x800DFA30` |

The first two are the `and` in a predicate's `jr ra` delay slot, replaced by
`addiu v0, zero, 1`; the third is the `subu` that computes the score, replaced
by `addiu v1, zero, 0x100`. The mission-cleared predicate keeps its
`A[group] == 0` early return, so groups with no missions are not falsely marked.

Emulating the real overlay image through all five unlock passes to a fixpoint
(`scratchpad/vr/emu.py`) gives:

| | items unlocked, stock | patched |
|---|---|---|
| Integral VR-DISC | 46 of 373 | **361** |
| USA VR Missions | 45 of 372 | **357** (cascade pass only) |

The twelve that never open are group 67, whose menu id is `0xFFFF` and whose
item count table reads 0 — a placeholder, not a reachable menu.

**Nothing here writes progress.** The save file is made from the VRAM bitmap,
which only a genuinely cleared mission ever touches, so a save written while the
patch is deployed still records real progress only. Deleting the PPF restores
the original gating exactly — the "relock, not permanently" the test rule wants.

`vr_unlock.py` writes `INTEGRAL_vr_unlock_missions.ppf` (3 records, 99 bytes)
and `VRUS_unlock_missions.ppf`, and with `--deploy` puts them in
`mods/INTEGRAL/VR-DISK/` and `mods/VR-DISK_US/`. Ketchup's `RootPath` adds a
version folder only when a title has more than one version and a disk folder
only when a version has more than one disk, which is why the USA VR disc's
folder has no subdirectory. Named `_unlock_`, not `en_`.

**Test procedure** — the standing rule is never to unlock with achievements live:

1. `MGSM2Fix.ini` `[Patches]`: `DisableRAM = true`, `DisableCDROM = true`.
   This is also what makes Integral's own KEY CONFIG visible, because the
   collection stops intercepting the screen.
2. `py vr_unlock.py --deploy`
3. test
4. delete the deployed `*_unlock_missions.ppf`
5. `DisableRAM = false`, `DisableCDROM = false`

## Achievements

Per upstream (nuggs), the `DisableRAM` + `DisableCDROM` combination in
`MGSM2Fix.ini` suppresses achievements. There is no dedicated option.

**In effect since 2026-09-02** (to progress the save for the briefing unlocks
without earning achievements). Both flags filter the Master Collection's own
Squirrel patch tables (`SQNative_setRamValue` / `SQNative_entryCdRomPatch` in
`src/sqhook.cpp`). Verified from `MGSM2Fix.log`: the session filtered 348
CD-ROM offsets, 848 CD-ROM files and 88 RAM patches, against a pre-flag
baseline of 48 / 30 / 0 (the blacklist paths log the same message).

**Ketchup is unaffected.** All 5,542 `[Ketchup] CD-ROM write` lines in that
session have no `filtering` line immediately before them — the pre-hook runs
synchronously inside the call and Ketchup logs right after, so a dropped write
would always show one — and the executable-side RAM patches applied (3,068
bytes, 54 blocks). Likely because the filter resolves its data argument as an
`SQBinary` instance while Ketchup passes a plain array, but that is inferred,
not traced. The English text should be checked visually after any MGSM2Fix
update, since a change to that hook would silently drop every PPF.

**Side effect on comparisons:** `DisableRAM` also filters the collection's
cosmetic RAM patches — the memory-card screens revert from STORAGE 1 / 2 to
MEMORY CARD 1 / 2 — so a shot taken with the flags on differs from one taken
before in those strings. Not a port difference.

### The collection's RAM patches collide with `en_savemsg` (found 2026-09-03)

**Superseded 2026-09-05 — read "Three item-text faults" first.** The
"items proven intact" conclusion below held only for bytes a PPF record named:
a byte the English shared with retail was never written by Ketchup and kept
whatever the collection's block put there (the SOCOM line break). Since
2026-09-05 `en_items` and `en_savemsg` own every byte of their pools and
tables, so neither the first-byte blind spot nor the mid-run one applies to
them any more; the byte counts quoted below (3,068 / 442 / 3,510) are the
old differing-run figures. The analysis is kept as the record of how the
collision was measured.

Read the filtered list out of the log
(`filtering RAM patch offset 0x… with size 0x…`) and six of the collection's
patches land **inside the caption pool `en_savemsg` repacks**
(`0x80011F18..0x800120CB`):

| patch address | size | retail string there | caption slots |
|---|---|---|---|
| `0x80011F34` | 14 | メモリーカードにエラー… | save 10, load 10 |
| `0x80011F6C` | 14 | メモリーカードが初期化… | save 8, load 8 |
| `0x80011F90` | 14 | フォーマットに失敗… | save 7, load 7 |
| `0x80011FC4` | 14 | メモリーカードをチェック… | save 5, load 5 |
| `0x80011FC8` | 14 | (into the same string) | — |
| `0x80012020` | 14 | セーブしました。 | save 2 |

Those are fixed addresses. `en_savemsg` **repacks** the pool, so after the port
those addresses sit in the middle of different strings — `0x80011F34` now holds
`failed.\0Error `, `0x80011FC4` holds `…\0Now chec`. With achievements enabled
(`DisableRAM = false`, the collection's default) the collection would write its
own 14 bytes over those offsets and corrupt the ported English mid-string. The
port has only ever been tested with `DisableRAM = true`, which is why this has
not been seen.

**Re-examined 2026-09-03 with achievements live (`DisableRAM = false` since
17:00), and the picture is sharper — and wider — than the paragraph above.**

`en_items` collides too, and far more heavily. Mapped with Ketchup's own rule
(`ram = 0x10000 + (img - ram_base) / 0x930 * 0x800 + (img - ram_base) % 0x930`),
`en_items` mirrors into `0x80010EAE..0x80011B02`, and the collection's two large
RAM patches — `0x1101c` +2895 and `0x1108c` +2838 — cover
`0x8001101C..0x80011BA2`: essentially the whole item-text pool. Yet English item
text has been read for days with `DisableRAM = false` and never looked wrong.

The reason is `Ketchup::Update`, and it also decides what can be concluded about
`savemsg`. Every 30 frames Ketchup checks **the first byte of each RAM run** and,
if any differs, rewrites every run and logs `Applied … (pass N)`. Every
`en_items` run inside the collection's range *starts* inside it, so a collection
write there would flip a first byte and force a pass 2. Across every saved log
with `DisableRAM = false` — including the 08-29 sessions spent reading item
text — the highest pass is **1**. So the collection's two big writes never
landed after Ketchup's first pass. Items are proven intact.

**`savemsg` is not.** The six 14-byte writes fall **mid-run**: the pool's runs
start at `0x80011F19`, `0x80011F27`, `0x80011F29`, `0x80011F2B`, `0x80012078`,
`0x80012098`, `0x800120B4`, and none of `0x11F34`, `0x11F6C`, `0x11F90`,
`0x11FC4`, `0x11FC8`, `0x12020` is one of them. Ketchup's check cannot see those,
so the pass-1 evidence says nothing about the pool. The `STORAGE 1 / 2` rename
*does* show on the memory-card screens with the flag off, so that patch family
is being applied at some point; whether the six pool members land before or
after Ketchup's pass, and whether they stick, is unknown.

**The test that settles it:** enter LOAD DATA with `DisableRAM = false` and
watch the `Now checking Memory Card.` caption (load slot 5 — one of the six).
`No save file.` proves nothing; it is slot 4 and untouched. Garbled →
live.

**Two honest fixes, and a decision between them:**

1. Make Ketchup's check complete — every byte, still every 30 frames (3,510
   bytes; trivial). Then the port's bytes are re-asserted whichever way the
   race goes, and the six messages read as USA wrote them (`Memory Card …`).
   Verbatim USA text, which is the port's rule. **Caveat:** if the collection
   *re-applies* its patches (say, each time the memory-card screen opens), a
   complete check would fight it and the text would flicker between the two
   for up to 30 frames, then — after Ketchup's back-off at eight applies — for
   up to 480. Measure whether it re-applies before choosing this.
2. Let the collection's wording win for that family, the way the ○-button line
   was let go: exclude the six addresses from the port, or filter nothing and
   pin them. But the collection's replacement is *Japanese* storage wording,
   so on an English pool it would read as mixed garbage — this option only
   works together with adopting the collection's own English (its USA
   rename strings), which has not been looked at.

`DisableRAM = true` is no longer an option worth listing: it kills achievements.

The collection also patches, outside the pool: the memory-card UI strings
(`MEMORY CARD 1/2`, `SELECT MEMORY CARD`, `PRESS * TO SELECT MEMORY CARD`,
`PRESS SELECT TO EXIT`) and two overlapping ~2.8 KB blocks of game-encoded text
at `0x8001101C` (+2895) and `0x8001108C` (+2838) covering the same region —
almost certainly one per language. All of that is the memory-card module's
`.rodata`; none of it is the brightness screen.

The deployed ini is a symlink into Vortex's mod folder
(`%APPDATA%\Vortex\metalgearsolidmc\mods\MGSM2Fix-*\MGSM2Fix.ini`); edit the
target, not the link (`sed -i` on the link replaces it with a plain file).
Pre-change copy: `MGSM2Fix.ini.before_achievements` at the working-data root (NextSteps §1). To restore
achievements, set both back to `false`.

## What the collection's named-file patches say (read 2026-09-07)

`m2archive.py` decodes the collection's own archive, so the five CD-ROM patches
that land on this port's text can be read instead of inferred. Its docstring has
the format; this is the result.

**They do not translate anything. They blank the line that tells the player to
open the disc tray, and reword the prompt** - the same kind of change, for the
same reason, as dropping the brightness screen's ○-button line. There is no tray.

| patch | image | bytes | differ | what it does |
|---|---|---:|---:|---|
| `disc1_1822B55D_patch_PS5` | `0x1822B55D` (`title`) | 592 | 30 | record 1 → twelve spaces, record 2 reworded |
| `disc1_132F2716_patch_PS5` | `0x132F2716` (`abst`) | 240 | 83 | records 1 and 3 blanked, record 4 reworded |
| `disc1_18345E07_patch_PS5` | `0x18345E07` (`change`) | 174 | 48 | record 2 blanked, record 1 reworded |
| `disc1_18412BD8_patch_PS5` | `0x18412BD8` (`demosel`) | 223 | 30 | the same shape |
| `disc1_18412A95_patch_PS5` | `0x18412A95` (`demosel`) | 19 | **0** | writes exactly what is already there |

The platform suffixes (`_NX`, `_PS`, `_PS5`, `_PS_SWAP`, `_XBOX`, `_STEAM`)
differ only in button-glyph codes inside the same strings.

**There are 101 named-file patches for Integral disc 1, not five.** `py
m2archive.py --patches` decodes every one of them against retail. Most are
small - 52 to 140 bytes, a handful of differing bytes each - and 77 do not begin
on an `07` record header, so they are not text blocks at all. The five above are
simply the ones that land where this port writes. Anyone wondering what the
collection changes elsewhere now has the tool rather than a guess.

**This confirms why `en_menu3` is raw-disc only, with bytes rather than
inference.** Their `title` patch overwrites 592 bytes starting at the address of
our record 0, in retail's own record layout. Ours rewrites the same block with
USA's English and different record boundaries. Whichever lands last decides the
block, and if ours does not, the walk desynchronises - which is the
`GCL:WRONG CODE` run seen on 2026-08-28, 08-29 and 09-07.

**And it independently confirms every image base this port found by scanning for
volume descriptors.** The archive index names the disc images and their offsets:
Integral discs 1, 2 and VR at `0`, `0x2AE54800` and `0x57592000`; USA discs 1
and 2 at `0xF12F8000` and `0x11B3E5800`; USA VR at `0xD39B7000`. All six match.

## Raw-disc error correction, and what it proved on the way (2026-09-07)

A PPF for the collection only ever writes a sector's 2048-byte payload, because
the emulator reads the payload and ignores the error-correction tail behind it.
A patch for a real PlayStation disc image is not finished there: changing a
payload byte invalidates that sector's EDC and its P/Q parity. `cdecc.py`
computes both and `rawdisc.py` recomputes the tail of every sector a raw set
touches - 413 per main disc, 2003 on the VR disc - shipped as one more PPF per
disc. Each raw PPF also gains a PPF3 block check so a tool can refuse the wrong
image, and `py rawdisc.py <package>` applies a finished set in memory and
confirms every touched sector verifies.

**The invariant that makes it trustworthy** is checked per sector: before
anything is applied, the sector as we believe retail has it must verify against
its own stored parity. That is a 280-byte sum over 2048 bytes, so it cannot pass
by accident, and it refuses to emit a tail for any sector whose true content is
unknown.

Two things fell out of it that were not the point:

* **The retail executables are the discs' own, and the decomp's build is
  byte-faithful.** Every executable's ISO extent is zero-filled in the
  collection's images (313 of 313 sectors on Integral disc 1, 308 of 308 on the
  VR disc) while keeping the parity of the sector it used to be. Put the retail
  file back and the parity has to come out equal to what is stored - and it does,
  313 times, 313 times, and **308 times for `vrint.exe`, which is built from the
  decomp rather than copied**. Where its source is unchanged, the decomp's output
  *is* the disc.
* **`us1.exe` is not the executable the collection's USA image was built with —
  IDENTIFIED 2026-09-12.** The same check reproduces only 8 of 318 tails.
  Redump catalogues two USA MGS1 pressings — "Metal Gear Solid (USA) (Disc 1)"
  and "Metal Gear Solid (USA) (Disc 1) (Rev 1)" — and the two disagree over
  **416,363 of 651,264 bytes** (64%), starting at offset 0x10, both built with
  the same PSYQ library RCS ids (`bios.c` 1.86, `sys.c` 1.140), so this is a
  real Konami source-level revision, not a different SDK. Extracting each
  pressing's own `SLUS_005.94` and running the same zero-fill/parity check
  against `windata/alldata.bin` gives a clean answer: the **base/"Rev 0"**
  dump reproduces the collection's stored sector parity **318/318**, and the
  **"Rev 1"** dump — which is byte-for-byte what `work/us1.exe` (and `us2.exe`)
  already is, SHA-256 `615e1360…` — reproduces only **8/318**, the same figure
  this section used to report as unexplained. So the collection's own USA
  image was pressed from the base/Rev 0 disc, and this port's `us1.exe`/
  `us2.exe` inputs have been the Rev 1 disc all along.

  **This does not appear to have affected anything shipped.** `items.py` and
  `savemsg.py` read USA text out of `us1.exe` directly (`rendertext.py`'s
  `ITEM_TAB`/`WEAP_TAB` offsets), and every string those builders took from it
  was independently confirmed on screen against real USA gameplay (the
  34-of-34 item/weapon comparison, the memory-card captions) — so whatever the
  Rev 0/Rev 1 difference actually changes in the executable, it does not touch
  the item, weapon or save-message text tables those extractions depend on.
  `usa1_stage.dir`/`usa2_stage.dir` (real disc dumps, not an executable) remain
  the source for everything else, which is why this was never more than a
  measurement worth keeping. What the Rev 0/Rev 1 difference actually *is* in
  gameplay terms has not been investigated — the 64% byte churn starting at the
  PS-EXE header suggests a code-size-changing edit rather than a handful of
  patched bytes, consistent with a real bug-fix revision, but that is
  unconfirmed.

## Line widths: what is an invariant here and what only looks like one

`widths.py` derives a `vrwindow`'s real budget from the decomp, one call at a
time: `align4(w) - 16` pixels of margin, to VRAM words, to whole 12-px character
cells (`font_set_kcb`), to `kcb->width` (`font_get_buffer_size`), less the 12 px
`font_draw_string` reserves unless `FONT_NO_KINSOKU`. A 256-wide window gives
228 px of room.

**Then measuring said not to assert it.** Retail Integral has **107** window
lines over that budget and retail USA has **19**; the port has 49, all of them
lines USA itself ships. So the budget is a good estimate and a bad invariant,
and a build-time assert on it would fail on the game's own data.

What is enforced instead has a witness for every case:

* `substitute_numbers` may never make a line render wider than the USA line it
  came from. USA drew that line in that window, so its width is known safe. The
  rule that keeps a right-aligned number in its column pads or eats whole
  *spaces*, and a space is not the width of the digit it replaces, so this is a
  real thing to guard even though nothing currently trips it.
* `kcb->max_width` is a `u8` read with `lbu`, so **255 px** is a ceiling nothing
  in the engine can lift. Checked over every record the port writes.

`vr_exe.py` has no width check on purpose, and says so: its pools hold multi-line
descriptions, the English ones break on `0x807C`, and retail Integral's own
Japanese entries in the same arena measure 540 px unsplit - so they break on
something not yet established, and asserting there would fail on shipped bytes.

## The item short-name table (`SCARF` -> `HANDKER`, 2026-09-07)

The inventory shows two different strings for an item: the **description** in
the panel, which `en_items` has always ported, and the **abbreviation** in the
side column, which it had not - because that set is already Latin on both discs
and was assumed, correctly for 21 of its 22 entries, to be identical.

It is a block of NUL-terminated names on an 8-byte stride, items 23 down to 0:

    Integral  0x09BCB8  SUPPR. SCARF   ROPE DISC MINE.D ... SCOPE CIGS
    USA       0x09E440  SUPPR. HANDKER ROPE DISC MINE.D ... SCOPE CIGS

Comparing the two entry by entry, **one differs**. Item 22 is `SCARF` on
Integral and `HANDKER` on USA, and Integral's own Japanese description for that
item reads 《ハンカチ》 - *hankachi*, a handkerchief - so its abbreviation
disagreed with its own text. Changed on the user's instruction; see the
amendment under "Scope". `en_items` now owns those 8 bytes outright
(`items.py`, `IN_SHORTNAME`), so Ketchup writes and audits all of them.

**Why `HANDKER` and not `Handkerchief`.** The slot is 8 bytes, so seven
characters plus a terminator is the ceiling. USA's string is exactly seven, so
it lands where `SCARF` sat and nothing moves - the patch grew by 8 bytes per
disc and changed nothing else. Any other abbreviation would have been text this
project invented rather than ported, which the standing rule forbids.

**Two things about the block that are easy to trip over.** Cold Medicine and
Diazepam are **not** in it: both names are 8 characters, one too many for a slot
that must also hold a terminator, so both games keep those two names elsewhere -
a fixed-stride walk of the region silently mis-pairs everything after them.

**And the `MP 5 SD` nearby is not a table entry at all** - corrected the same
day, after a fixed-stride walk of the region had suggested Integral's table was
one entry longer than USA's. It is a **string literal** in
`menu_weapon_init_helper` (`menu/weapon.c`), emitted into rodata beside the
table because the code names the slot inline:

    if (weaponIdxCopy == WP_Famas && GM_GameLevel == GM_LEVEL_VERYEASY)
        str = "MP 5 SD";

USA's build has no such literal because it has no VERY EASY. Walk this region as
NUL-terminated strings and compare by content; a stride walk runs off the end of
the table into literals and mis-pairs everything after it, which is exactly the
mistake that produced the wrong claim.

**Reading the Japanese.** None of this could be settled from the encoded bytes,
which is what `rendertext.py` exists for: the strings are font indices, not
Shift-JIS, so `《ハンカチ》` reaches `game_text` as `<9014><822F><8253><820B><8221><9015>`
and can only be read by drawing it with the game's own font.


## `RADIO.DAT`, the codec, and Integral's developer commentary (2026-09-09)

The codec script is not in `STAGE.DIR`. `menu/radiomes.c` loads it out of
`RADIO.DAT` by sector, with the fragment size packed into the top byte of the
radio code (`size = (pRadioCode / 0x1000000) * 2048`), so every GCL sweep this
project owns was structurally incapable of seeing a word of it. That is how
6.5 MB of Japanese went unnoticed while four documents said the port was
complete. `discaudit.py` exists so the next such file cannot hide the same way.

### Two halves

`RADIO.DAT` is 11,198,464 bytes on Integral and 1,776,851 on USA - 6.3x - and
the difference is not a translation:

| region | bytes | content |
|---|---:|---|
| `0x0000000`–`0x042C54C` | 4,375,884 | the story codec, English and Japanese together |
| `0x042C54C`–`0x0AAC050` | 6,814,468 | Japanese only; not one dialogue-length English run in 6.5 MB |

The boundary is found by taking the largest gap between consecutive
dialogue-length ASCII runs - 6,814,468 bytes, which is the region itself.

**The English half is USA's script, whole.** 35,273 dialogue lines / 1,145,926
bytes against USA's 35,193 / 1,143,269. Integral therefore already *has* English
codec text and picks it with the runtime language setting - the same setting
`[Game] EnglishText` holds through title setup and the collection's own writes
race against. There is nothing to port here, and that is worth stating plainly
because the file's size invites the opposite conclusion.

### Reading the other half

None of it is Shift-JIS - it is font indices, like every other string in this
game - so it was read by drawing it with the game's own font (`rendertext.py`):

* 「ニンジャにつづきスネークも　装衣えを用意すること」
* 「、デモはゲーム中とは別モデルでやる予定だったので」 — the cutscenes were
  planned to use a different model from the in-game one
* 「さらにこのインテグラル　では」 — *furthermore, in this Integral…*, which is
  the line that settles what this content is
* 「られたメモリをどうやりくりするか」 — juggling the memory they were given

Structural corroboration: `d0 03`, a Japanese text control code, occurs **64,087**
times in Integral's file and **4** times in USA's; 95.1% of the region's 2 KB
blocks are distinct, so it is not a repeated pattern; and that memory line's
32-byte run recurs 328 times, so the conversations share boilerplate.

### Two false starts worth keeping

**A byte-range regex cannot classify binary.** Counting runs of lead bytes
`0x81`–`0x9F` said 31.7% of the file was Japanese and put "Japanese" in windows
whose hex contains almost no kana at all. Restricting to the kana banks and then
corroborating with the record structure (`d0 03` density) is what held up.

**Volume ratios nearly hid it.** 964 KB of Japanese against 1,146 KB of English
is 42% - precisely what a faithful translation of the same script gives, since
Japanese needs fewer characters. Read as a ratio it says "paired subtitle
track, nothing extra". It was mapping *where* each language sits, window by
window, that exposed a 6.5 MB block with no English in it. **A global ratio can
be exactly right and still describe the wrong thing.**

### It is translation, not porting

USA never shipped any of this, so there is no English to copy, and the standing
rule leaves it alone: porting means moving USA's own words, and there are none
here. Anyone who wants this commentary in English is starting a translation
project, which is a different undertaking with a different rule set - and the
first thing it needs is not in this repository.

The same applies to the two small pockets `discaudit.py` turned up, both
Integral-only and both confirmed by rendering: ~1.8 KB in `DEMO.DAT`
(「そしてテロリストの」, story narration) and ~1.0 KB in `VOX.DAT`
(「エンジンやプロペラのノイズ」, sound-design commentary beside the audio it
describes).

## Psycho Mantis's memory-card table, and the RAM-patch question (2026-09-08)

Raised from outside the project: someone asked whether the collection's *vanilla
RAM patches* are documented anywhere, mentioning a known USA MGS1 bug where "a
byte is off by 0x20 and it breaks one of Mantis's memory card game voicelines",
and guessing that a RAM patch would be lighter than a PPF for it. Two separate
questions, and this port can answer one of them outright.

### Where the table is

Mantis reads the memory card and names other Konami games he finds. The list he
matches against is a flat table of product codes in the `s07b` stage archive -
`s07b` being the Mantis room - stored as **12-byte records: a 10-character code
then two NULs**, no count, terminated by the next string in the section.

| disc | offset in `<x>_stage.dir` | entries | codes |
|---|---|---:|---|
| USA (`usa1`, `usa2`) and the collection's `us1` | `0x018A5FB8` | 24 | `SLUS-00707`, `00600`, `00674`, `00678`, `00614`, `00591`, `00445`, `00067`, `00296`, `00486`, `00293`, `00289`, `00295`, `00288`, `00447`, `00235`, `00292`, `00294`, `00045`, `00064`, `00238`, `00049`, `00055`, `00048` |
| PAL (`pal1`) | `0x01898670` | 27 | `SLES-01535` … `SLES-00036` |
| Integral | — | 221 | 185 `SLPM` + 36 `SLPS` |

The `A` sitting immediately before the first USA code (`…02 e1 41` then
`SLUS-00707`) is **not** part of it: the 12-byte stride puts entry 0 at
`0x018A5FB8` on the `S`, and that byte belongs to the data before the table.
A first reading of the hex called it a stray prefix and a broken first entry;
the stride disproved it. Worth stating because it is the obvious wrong answer.

### On the reported bug: not in this table

All 24 USA codes are well-formed, uppercase, and **byte-identical across
`usa1_stage.dir`, `usa2_stage.dir` and the collection's own `us1_stage.dir`** -
so whatever the off-by-`0x20` byte is, it is not a case error in the code list,
and the Master Collection has not introduced or inherited a difference here.
That narrows it rather than finding it: the remaining candidates are the mapping
from a matched code to a voice line, the card-directory read itself, or a
comparison length - all of which live in the `psyco` overlay, which the decomp
has not covered (`charalst.h` lists `CHARA_PSYCHOMANTIS` with `?`). Anyone
chasing it should start from the overlay in `s07b`, not from the executable:
there are no product codes in `SLPM_862.47` or `SLUS_005.94` at all.

### On the documentation question: yes, and it is new

The collection's patches for a title are **not** undocumented any more, and they
are not RAM patches in the sense the question assumed - they are CD-ROM patches
applied to the disc image, in two kinds:

* **offset patches**, which carry their bytes inline; `SQHook::SetPatchWatch`
  logs those, including their contents.
* **named-file patches**, which carry only a name, the payload living in the
  game's own archive. Those are what three documents here called unknowable, and
  `m2archive.py` extracts them: `windata/alldata.bin` is a flat blob and
  `alldata.psb.m` beside it is its index (an MDF-wrapped, XOR-obfuscated zlib
  stream keyed by `MD5("25G/xpvTbsb+6" + the lowercased file name)` through
  MT19937), inflating to a PSB v3 whose `file_info` maps 4,926 paths to
  `[offset, size]`. `099/patch/` holds **176** of them. See "What the
  collection's named-file patches say".

So a full inventory of what the collection patches per title is a tool run away.
And the instinct in the question is right on the other point: MGSM2Fix already
has the lighter-weight mechanism, and it does not need a PPF. `Ketchup_DiskPatch`
/ `SQKetchupPatches()` let a title ship disc-image bytes of its own from inside
the ASI, `SQHook::SetPatchRangeBlacklist` drops the collection's own patches by
offset range, and Ketchup defers executable writes to the RAM mirror. The
brightness-text fix for USA MGS1 (title 981) is already built exactly that way
and is the worked example - a handful of bytes in `.rdata`, no patch file. A
Mantis fix would be the same shape, once the byte is located.

## The `abst` location names (Integral's own English -> USA's, 2026-09-08)

The second application of amendment 4b, after `SCARF` -> `HANDKER` above, and
the first one that made a container grow. Both games spell 30 location names in
the MISSION LOG, they align 1:1, and **four** differ:

| Integral | USA |
|---|---|
| `Tank Hanger` | `Tank Hangar` |
| `Medi rm` | `Medi room` |
| `Cmnder rm` | `Cmnder room` |
| `Cmnd rm` | `Cmnd room` |

Three of those were known - both found by accident while porting the mission
log. The fourth, `Cmnd rm`, was found by `mainsweep.py --diff-english` on
2026-09-08 and had been in no document. Asked and approved the same day.

### Where they live, and why no search had found them

One `0x9906` command in `scenerio.gcx`'s **script body**, which `mainsweep.py`
calls `chara 53C7` after the actor its first STRID spawns. Its shape is unlike a
mission-log page: it carries **no option list at all**, just 31 records directly
in the value list, 30 of them the English names. That is why `page_of` returns
None for it and why `rebuild_body` skipped it for as long as the builder existed.

    60 <BE16 size> 99 06 <u8 ofs> 06 53 c7 06 c8 bb 08 5f d9 <31 records> 00

And the names are **two-byte font codes, not ASCII** - `0x80xx` for Latin
letters, `0x9001` for a space - so `Tank Hanger` is stored as
`8054 8061 806e 806b 9001 8048 8061 806e 8067 8065 8072 00`. That is why
searching the PPFs for the ASCII string found nothing and briefly suggested the
port had already changed them. Both games use the same encoding, so USA's bytes
transplant directly and no glyph work is needed.

### Take the whole command, not the four records

`USA_LOCATION_NAMES` in `abst_build.py`, beside `KEEP_PROMPT_CAPTION`, and it
substitutes **USA's entire block**. That is deliberate, because the block carries
*two* derived length fields:

* the COMMAND's own BE16 size, which `resize_block` would have handled; and
* a **u8 at `start+5`** that `option_starts` uses to reach the option list -
  `0xAD` on Integral's disc against `0xB9` on USA's, exactly 12 apart.

Patching the four records and re-stamping only the BE16 would leave a block whose
size says one thing and whose offset byte says another. Taking the block whole
keeps both in step by construction, and it is also the most literal reading of
the rule: the bytes are USA's own. Everything above the command -
the proc body's ARG length, the proc table offsets, the proclen, the script
length - `abst_build.py` already recomputes for the mission-log pages, so
nothing else had to change.

`demo.gcx`'s location list is Integral's own **Japanese** one, 31 records USA has
no counterpart for. It is not offered to the substitution and stays as it is.

### What it cost, measured both ways

One name is length-neutral (`e` -> `a`); three grow by two characters, which is
4 bytes each at 2 bytes a glyph. **+12 bytes**, and the constant makes that
checkable rather than asserted: the rebuilt chunk is **104,600** bytes with
`USA_LOCATION_NAMES = False` and **104,612** with it true. The stage is still
**88 sectors** and still lands in DUMMY3M slots 462..549, so no budget moved and
no other patch's bytes were touched (`no overlap with 614384 other PPF bytes`).

### The verifier that now guards it

`verify_chunk` re-parses the location list **out of the rebuilt script** and
asserts it equals its source record for record, that there are 31 of them, and
that the constant was honoured exactly once:

    verified: the 31-record location list equals USA's exactly; 4 record(s)
    differ from retail Integral

Its u8 offset byte is checked implicitly and that is the neat part: the verifier
finds the block with `location_block`, which calls `option_starts` - which reads
that u8 - and requires the empty option list it points at. A wrong offset byte
therefore fails to find the block at all rather than passing quietly. Running
with the constant off reports Integral's own list and 0 differing records, so
both directions are exercised.

**Not yet seen on screen.** The names appear in the MISSION LOG's own location
column.

## Descriptions that change with the game state (mapped 2026-09-07)

An item or weapon description is not always the string its table points at. Two
functions decide what actually gets drawn - `menu_item_printDescription` and
`menu_weapon_printDescription` (`menu/item.c`, `menu/weapon.c`) - and they are
the only two callers of `menu_printDescription`, so this list is complete.

It is worth having in one place because three of the port's own fixes are cases
of it, and because the last row is a Japanese string a player can still reach.

| what | when | how | the port |
|---|---|---|---|
| **Ration** shows the frozen-Ration text | `GM_FrozenItemsState == 1` | the table pointer is swapped | **ported.** `N_ITEM = 26`: the item table's last two entries are this pair, so USA's frozen texts come across with the rest |
| **Ketchup** shows the frozen-Ketchup text | `GM_FrozenItemsState == 1` | swapped | **ported**, same way |
| **ID Card** gets its security level | always, for that slot | one byte of the string is **overwritten in place** with the digit | **fixed.** Integral stored at byte 46, where USA stores at 45; against USA's string that wrote over the space and produced `level 17security`. `items.py` retargets the `sb` |
| **SOCOM** gains or loses a suppressor line | `GM_SilencerFlag` | three bytes at 0x70..0x72 **overwritten in place** | **removed, to match USA.** USA's description is 83 bytes, so its identical code writes into padding and nothing shows; in Integral's repacked pool those offsets landed in the next string. `items.py` NOPs the six stores |
| **Mine Detector** says it cannot be used | `GM_GameLevel >= GM_LEVEL_HARD`, so HARD **and** EXTREME | swapped for a separate string | **ported.** USA's is 59 bytes against Integral's 53, so `items.py` relocates it and repoints the `lui`/`addiu` pair that reaches it |
| weapon 1 **is the MP5 SD, not the FA-MAS** | `GM_GameLevel == GM_LEVEL_VERYEASY` | not a label swap - Integral **replaces the weapon** and reuses `WP_Famas` as its id. Three places: `check_type` in `game/item.c` returns 0 for that id, so the **FA-MAS pickup never spawns**; `menu/weapon.c` names the slot from an inline `"MP 5 SD"` literal; and the description pointer becomes `wpn_mp5_description` (0x800AB5CC -> 0x80011B04) | **not ported, and it stays Japanese.** USA has neither the weapon nor the VERY EASY difficulty, so there is no counterpart to take |

**The last row is the one to know about, and it is confirmed on screen**
(2026-09-07 23:07, on VERY EASY): the slot reads `MP 5 SD` and the panel draws
Integral's Japanese, wrapped to three lines. It is a 103-byte string in the main
game that a player reaches simply by starting on VERY EASY and opening the
weapon menu:

    《MP 5 SD》 サブマシンガン。□ボタンを押すと発砲。
              押しつづけると、フルオート連射。サプレッサー装備。

It sits at file 0x2304, immediately past `ARENA_B`'s exclusive end, so the
repack never touches it. Neither sweep would have found it either: it is an
executable string, so `mainsweep.py` (GCL only) cannot see it, and
`audit_text.py` scans the executables only for the save-title probes. It was
found by reading the two printer functions, which is the lesson - **a string
table does not tell you which string is drawn.**

**All six are now confirmed on screen.** The four ordinary ones were read
against USA in the 34-pair comparison; the two conditional ones were photographed
on 2026-09-07 once `[Game] GiveItems` made them reachable regardless of
difficulty (`HANDOFF-ARCHIVE.md` §5.7):

* the Mine Detector on HARD, drawing USA's three lines - `《Mine Detector》` /
  `Cannot be used in` / `HARD or EXTREME mode.` - which is also the first
  on-screen proof that the relocation and the repointed `lui`/`addiu` pair work;
* the FA-MAS slot on VERY EASY, drawing Integral's Japanese MP5 SD text under an
  `MP 5 SD` label, which is correct and not a fault.

## Sweep: the main discs, and the three questions it answers (`mainsweep.py`)

`jpsweep.py` above scans for Japanese-looking pointer slots and `audit_text.py`
inventories string candidates by framing; both say what they cannot conclude.
`mainsweep.py` (2026-09-07, extended 2026-09-08) does for discs 1 and 2 what
`vr_sweep.py` does for the VR disc: it pairs every GCL string with the USA
disc's **by the command that owns it**, so a judgement about bytes becomes a
comparison between two discs.

    py mainsweep.py [--disc 2] [--samples]     is anything still Japanese?
    py mainsweep.py --integral-only            the 13 stages the pairing misses
    py mainsweep.py --diff-english             both English, worded differently
    py mainsweep.py --census                   account for every Japanese string

It reads **retail** on both sides, deliberately: the point of the default mode
is to enumerate everything USA has in English and then subtract what the port
covers, so a gap cannot hide behind a patch that is already deployed. That
choice is right for three of the four modes and wrong for the fourth - see
"the input is part of the result" below.

### The default mode: tallying by owner

    owner X:  Integral 40 Japanese, 0 English | USA 0 Japanese, 41 English   <- port it
    owner Y:  Integral 12 Japanese, 0 English | USA 12 Japanese, 0 English   <- USA never did either
    owner Z:  Integral  0 Japanese, 30 English | USA 0 Japanese, 30 English  <- done

Disc 1 reports twelve owners holding Japanese where USA's counterpart has
English. Nine are inside stages a patch family already owns; two are Japanese in
identical numbers on the USA disc (`cmd 4AD9`, the location titles; `chara 9302`
in `rank`). The twelfth was `chara 2D0A` in `s07b` and is now `en_pad2`.

### `--integral-only`: a pairing's universe is part of its answer

The default mode compares the **82 stage names the two discs share**, which puts
all 13 Integral-only stages outside it - and that is how the third copy of the
controller-port string, in `s07br`, went unnoticed while the section describing
it claimed to have found everything. Each Integral-only name pairs onto the USA
stage it is a variant of (the base name without its trailing `r`, or `init` for
`init_ve`), and across all 13 on both discs there is **exactly one** Japanese
string whose base-stage owner has English: that `s07br` copy. The hole was worth
closing and it was empty.

### `--census`: buckets that must sum

`COVERAGE-RECORD.md` used to say "about 160 remain across gameplay stages and need
verification against their callers", which is what a framing heuristic can
honestly say. This asks a question with a complete answer instead: for every
Japanese string Integral has, what is at the same owner on the USA disc? Both
discs give the same figures.

| bucket | disc 1 | disc 2 |
|---|---:|---:|
| inside a stage a patch family owns | 1,260 | 1,260 |
| USA is Japanese there too | 17 | 17 |
| the owner does not exist on the USA disc | 83 | 83 |
| **unaccounted - a porting target** | **0** | **0** |
| total | 1,360 | 1,360 |

The buckets are asserted to sum to the total, so the tool cannot report a clean
result by losing a string, and it **exits non-zero** if the last bucket is ever
not zero: a regression guard, not a report. The first bucket is "accounted for
there", not "ported" - it includes the strings kept Japanese by rule.

### `--diff-english`: the blind spot every sweep shared

All four of this project's sweeps hunt *Japanese*, so a string that is already
English on both discs and merely **says something different** passed all of them
unremarked. Both cases known before 2026-09-08 were found by accident. A
positional diff is meaningless because the two builds lay their data out
differently, but both known cases sat in a *sequence* whose neighbours matched,
which is what a diff is for: the two discs' ordered English strings per stage,
through `difflib`, so equal runs align themselves and a `replace` hunk is the
shape being hunted.

Disc 1: **15 replace hunks over 82 stages, 8 holding player-readable text**, and
after triage one family - the `abst` location names, where four pairs differ and
not the three three documents listed. Know the noise before starting: voice-clip
and stage asset ids (`vc319010`, `vr01`), and hunks where one side's counterpart
is *Japanese* and so never entered an English list at all.

### The input is part of the result

Run against the VR disc, a per-owner fuzzy match reported `FAMAS` against USA's
`FA-MAS` across the mission titles. It is a **non-finding**: the port takes USA's
window text verbatim, and the deployed `vr_en_missions.ppf` holds 142 `FA-MAS`
and no `FAMAS`. The sweep had rediscovered the port's own work.

Retail input is right for the Japanese question and backwards for this one,
because here the port's own replacements are exactly what must be subtracted. So
`--diff-english` now prints `!! <stage> is owned by <family> and these are
RETAIL bytes` beside any finding in a `PORTED` stage, and names them again in
its summary. Three stages flag today: `abst`, `option`, `title`.

**The fix is not a deployed-stage reconstruction**, which was the first plan and
would re-derive by the hardest available route what the builders already build in
memory - parse the PPF, apply it, follow `en_abst`'s and `en_brf`'s relocation
into `DUMMY3M`, re-parse - and would need extending for every family that starts
relocating. Authority belongs where the bytes do: a family's own builder verifies
its own output (`abst_build.py` checks its location list against USA record for
record; `vr_windows.py` checks every window), the sweep's authority stops at the
boundary, and the flag marks where it stops. Two of USA's five VR languages will
also confuse any diff there: take only the English arm of a language branch
(`lang in (None, ENGLISH)`) or the alignment collapses.

## The controller-port subtitle (`en_pad2`, 2026-09-08)

The last string on the main discs that had a USA counterpart and no patch
family. `mainsweep.py` found it; `pad2.py` ports it; it is the smallest family
here — 159 bytes a disc, three slots, nothing moved.

### What draws it

Stage `s07b` is the Psycho Mantis room (`stage/s07b.c` registers
`CHARA_PSYCHOMANTIS`, `CHARA_PSYCHOMERYL`, `CHARA_MEMPSY`). The owner
`mainsweep.py` names `chara 2D0A` is `CHARAID_2D0A_2ND` → `NewSecond`, and the
whole of `game/second.c` is 45 lines:

```c
work->message = GCL_GetString(GCL_NextStr());   /* ONE string per spawn */
...
if (GV_PadData[1].status && work->using_pad2 == FALSE) {
    work->using_pad2 = TRUE;
    MENU_JimakuWrite(work->message, 20000);     /* pad 2 went live */
} else if (work->using_pad2 == TRUE && GV_PadData[0].status) {
    work->using_pad2 = FALSE;
    MENU_JimakuClear();                         /* pad 1 came back */
}
```

So it is the subtitle shown when the controller is moved to **port 2** for the
Mantis fight, telling the player to put it back, and cleared when they do.
`20000` frames is over the `> 10000` test in `MENU_JimakuWrite`, which selects
subtitle type 2 — the lower of the two rows (`menu/jimaku.c`).

Integral's string is コントローラ端子1のコントローラを｜使用してください。, read with
`rendertext.py`; three of its codes fall outside the located font bank, so read
the PNG rather than the code list. USA's is
`PLUG CONTROLLER INTO | CONTROLLER PORT 1.`, and both carry the same `|`
separator — `0x807C` in the Japanese, plain `0x7C` in USA's.

### Five sites, and why two documents said one

`second.c` takes one string per spawn, so a claim that this owner has "two
records, record 0 and record 1" cannot be true — and both `HANDOFF-ARCHIVE.md` and
`COVERAGE-RECORD.md` said exactly that until 2026-09-08. What `s07b` actually holds is
two separate **spawns**, in two branches of its script:

| | Integral | USA |
|---|---|---|
| site A | string at script `body+0x88C`, path `elif 1903 → if 2043` | same offset, byte-identical Japanese |
| site B | string at script `body+0xC3A`, path `elif 2845 → if 2985` | `body+0xC0E`, English |

USA translated the **later** site, not the first. A third site is in `s07br` at
`body+0xC3A` — an Integral-only stage, therefore outside `mainsweep.py`'s
universe, which is the 82 stage names the two discs share. Three sites a disc,
two discs, and both discs hold both stages at the same LBA with identical bytes.

USA's own build is inconsistent here: both branches hand the same message to the
same actor and only one was translated. Copying that placement literally would
leave a player who trips the other branch reading Japanese, so the user was
asked, and the instruction (2026-09-08) was to port every site. The text is
USA's own verbatim either way.

### Length-preserving, and where the padding must go

A GCL STRING is length-prefixed and `GCL_GetNextValue` advances by that length
byte, never by `strlen` (see "Why `en_menu3` is raw-disc only" for the
correction that established this). USA's 42 bytes fit inside Integral's 55-byte
slot, so the length byte stays at 55, the walk steps over the same bytes it
always did, and nothing after it moves. `MENU_JimakuWrite` only stores the
pointer; `font_print_string` reads to the NUL. The result per slot is

    'PLUG CONTROLLER INTO | CONTROLLER PORT 1.' 00 <12 spaces> 00

**The terminator must come immediately after the text.** Padding placed *before*
it would be drawn, and `font_print_string` measures what it draws while jimaku
centres on that width (`field_4_x = (FRAME_WIDTH - pFont->max_width) / 2`), so
trailing spaces would pull the line off centre with nothing in the bytes to
explain why. The record's own final NUL is left in place, which is `menu2.py`'s
convention and what `portio.records` asserts. `pad2.fill` is the one function
that does this and `selftest.py` guards all four properties of it; each guard
was confirmed by mutation.

No width check is needed or wanted: this is the very line USA draws at the same
call site, which is the only width warrant this project accepts (see "Line
widths: what is an invariant here and what only looks like one").

### Verification

Static, and by effect rather than by inspection. Applying the build to
`int{1,2}_stage.dir` leaves the archive's length unchanged; re-walking `s07b`
and `s07br` gives the same 1279 and 906 commands with identical structure, so
nothing desynced; all 159 changed bytes fall inside the three slots; and each
slot re-parses at length 55 with the English at the front and its length byte
untouched. The English is read out of `usa1_stage.dir` at build time rather than
retyped. Both PPFs are disjoint from all nine other deployed PPFs on their disc,
`ppfcheck.py --deployed` is clean over 31 files, and `pad2.py` is in
`rebuild.py`'s family list so the reproducible build covers it.

**Never seen on screen.** The subtitle only fires when pad 2 becomes active, so
it needs the Mantis room *and* a controller in port 2 (or the emulator's port-2
assignment) — no save or stage jump reaches it on its own.
