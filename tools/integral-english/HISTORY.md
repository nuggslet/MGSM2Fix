# Session history — MGS Integral English text port

The dated, session-by-session account of how this port was built, split out of
[`HANDOFF-ARCHIVE.md`](HANDOFF-ARCHIVE.md) on 2026-09-11. That file's §1–8 is the
current-state reference — where things are, the user's rules, what remains,
decisions to make; this file is the timeline: a short overview below, then the
full numbered sections §9–27 it summarizes, moved here verbatim (same section
numbers, so every `§9`/`§18`/etc. citation elsewhere still resolves to the
right place — just here instead of there). Later sessions append sections 28
onward; historical deployment notes describe their session, not the current INI.

---

Written 2026-09-04 (evening), updated the same night after the
reproducible-build pass (§9), through 2026-09-05 as the MISSION LOG port,
the item-text fixes and their on-screen checks landed (§10), on 2026-09-06
when the **VR disc** was ported (§11), and through 2026-09-07, the day the VR
disc was tested on screen and the three items that were still open all closed:
the MOVIE captions, `en_menu3` and the VR KEY CONFIG (§12), and through
2026-09-09, when the untranslated Japanese was dumped and the glyph
identification was set up as the one open task (§17), and on 2026-09-10,
when the raw disc booted for the first time and the briefing turned out to
be broken on it (§24 - `ROW_H`; **fixed late the same day**: an R3000
load-delay hazard, one `nop`, and a scanner so it cannot recur), and when that task was
finished - all 1,200 bank-1 glyphs named, 100% of the
Japanese readable as text, and the fragment map §17 rested on found to be
wrong for 93% of strings and rebuilt (§18), and then the export itself
finished: the byte scanner retired for a walk of the game's own records, a
bank-1 index bug found that had been naming every `0x97xx` glyph one position
too far along, and 25 more characters identified - the last of them, 蒼, named
from outside the disc because it occurs exactly once on it - leaving the export
complete with **zero** unresolved glyph codes (§19), and finally the VR disc
added - it had been dropped by a loop over two disc indices, so 97% of the
export looked like all of it - and the last game data taken back out of the
repository, the glyph table now shipping digests of the font bitmaps rather
than the bitmaps (§20), and a regression guard put on the fragment map after two
plausible-looking metrics were measured and thrown away (§21) - and on
2026-09-08, when the housekeeping was cleared, the sweep's one uncovered
finding became the `en_pad2` family, the three sweeps §5 had filed under "to
investigate" were all run, and the four `abst` location names were decided
(§5.11, §5.9, §5.14, §15), and finally later on 2026-09-10, when the Redump
dumps of all three discs were measured against the collection's embedded
copies - identical outside the hollowed-out executables, which those dumps also
supply at the exact hash the builder demands - and `mkimage.py` closed the gap
between a raw build and an actual patched disc image, from either source
(§22) - and late that night and into 2026-09-11, when the briefing's
connector left ends were found to have been Integral's under USA's boxes and
were moved (§26), and the collection's faint connector lines were traced to
M2's renderer sampling the wrong texel of a one-pixel quad and fixed at the
source in MGSM2Fix as `[Patches] ThinTexturedQuads`, then confirmed with every
briefing unlocked, twenty pairs at 0.00% against USA (§26).

Everything that used to live only in the assistant's private memory files
(`~/.claude/projects/.../memory/*.md`) was merged into `REFERENCE.md` and
`HANDOFF-ARCHIVE.md` on 2026-09-04. Those files may still exist, but **the repo
documents are authoritative**; if they disagree with a memory file, the memory
file is stale.

On 2026-09-11, `tools/integral-english`'s hardcoded references to the author's
own drive letters and username were removed from every script that had them
(`workdir.py`'s `WORK`/`GAME`/`DECOMP` resolution, already portable, was
extended to the handful of standalone helpers that had not been wired to it —
see `HANDOFF-ARCHIVE.md` §1), a CI job was added that runs `selftest.py` and
compile-checks every script on a clean runner, `SCRIPTS.md` was generated as a
one-line-per-script index, and this file was created holding the run-on intro
paragraph split out of `HANDOFF-ARCHIVE.md`. Later the same day `HANDOFF-ARCHIVE.md`'s
own §9–27 — the session-by-session log making up 58% of that file's length —
moved into this file too, and `REFERENCE.md`'s "Not tested" section had its
long-resolved, strikethrough-marked entries trimmed to one line each, so a
newcomer's first read is the map, not the changelog.

---

## 9. The 2026-09-04 late pass: what changed and what it settled

Commits `d988e1d` … `5b8d280`. In one evening the project went from "the
deployed PPFs work but some were built by scripts that no longer exist" to a
build anyone can reproduce and check:

- **`rebuild.py` + `BUILD-HISTORY.md`**: isolated, retail-input build of all eight
  families for both discs; PPF framing and sector-boundary validation; a
  cross-set overlap check; a ZIP with `SHA256SUMS.txt` and `build-report.json`
  (environment, SDK file hashes, every input and output). The clean run matched
  the deployed set (see §4). It never installs.
- **Recovered builders** (`items.py`, `menu2.py`, `optlabel2.py`) with explicit
  inputs; `optsctext.py` builds its caption chain from retail via
  `optlabel2.py`, so the pinned font-text PPF and the "builder consumes its own
  output" hazard are gone; `preope_usa.py` builds both recaps from retail with
  an explicit `--deploy`; `brf_*` read the real USA stage and the row/quad
  constants were re-verified against it (all 16 row and 53 quad tuples match).
- **`audit_text.py` + `COVERAGE-RECORD.md`**: three-disc candidate inventory with its
  limits stated; the save-slot title is full-width in USA too, so it is
  branding/encoding, not a port target; one more retained camera caption noted.
- **Stale guidance corrected**: `f924` stays `[8]` (growing it *causes* the
  EXIT freeze); `MG2_RECAP_OFFSET` is 22042 for the 13-page build; a silent
  normal disc swap does not prove the other three disc-text copies unreachable;
  `UPSTREAM.md` has real hashes and current status; `BrightnessText` is
  `[Patches]` and USA-only.
- Verified independently afterwards: the ZIP hash, all 16 `reference_effect_equal`
  flags, the decomp patch equal to the live decomp diff, deployed mods untouched
  (`ppfcheck --deployed` clean on all 18 files).

None of this closed the gameplay items, the disc-text family, the raw-disc
variant, VR or the census. It made them buildable and checkable when they are
done — which the Mission Log then was (§10).

## 10. The 2026-09-05 pass: the MISSION LOG, then three item faults

The user's go-ahead came with the USA screenshots ("the English one takes 2
screens") and "I want everything ported over perfectly when I return. Don't
forget to use the decomp files for reference where it helps." In order:

- **Night — the port.** `abst_build.py` rewrites the two GCX scripts
  (scenerio.gcx and demo.gcx — the cache section's tag sizes are offsets, and
  the 42 `d`-PROCID pages are demo.gcx's) with USA's counts and line records
  verbatim, keeps Integral's record 0 (the caption) and both fonts, and
  re-stamps every container; the disc-change abstract gets USA's eight strings
  (+13,804 bytes). `abst.c` grown to USA's model (128×20 KCBs in two VRAM
  columns, the 15-entry line table, the counter, the cursor frame, the slide,
  USA's input model), read from USA's overlay instruction by instruction, with
  one guard against colouring KCBs a count-7 page never allocates. USA's three
  bottom-bar textures in the footprint of Integral's two; every other texture
  and palette stays Integral's. 88 sectors, DUMMY3M 462..549, both discs.
  `rebuild.py` builds nine families and three overlays. An unattended smoke
  test was tried and cannot work: the collection's launcher waits for a game.
- **Night — three item faults**, found from the user's first shots after the
  deployment and pinned down by Ketchup's audit lines (README "Three item-text
  faults"): the card level digit offset (code, 46 → 45), the SOCOM suppressor
  rewrite into the Mine Detector text (code, six stores NOPed), and a
  retail-equal byte the collection's RAM patch owned (both exe PPFs now write
  every byte of their regions; `Applied … RAM patches` reads 3,755).
- **Night — the collection's disc-patch map** (README "Where the collection's
  own disc patches land"): it patches all four disc-swap text copies with named
  files two bytes before `en_menu2`'s `change`/`demosel` records; watches added
  for `change`, `demosel`, `title`, `camera` on both discs. The ASI build hung
  overnight (six idle `cl.exe`, stopped by PID); rebuilt and deployed 12:49.
- **Morning — on screen.** The user's shots: both mission-log pages of two
  logs right; the item fixes right; the page slide showed coloured fragments
  (stale VRAM in texels 504..511 of each line — the KCB buffer is 504 px wide
  and USA's second sprite 256 — fixed by drawing 248 px, decomp `26d27f1`,
  deployed 13:05, **confirmed clean 13:50**). The watches showed the
  collection's `camera` and `title` offset patches are its STORAGE rename
  strings and that its named disc-swap patches register with no inline data.
- **Clean run `repro7`** (13:06) reproduces all 18 deployed PPFs after every
  fix; `ppfcheck --deployed` clean on 20 files.
- Doc corrections found on the way and folded in: the `_disabled` path, the
  PatchWatch-blind-under-DisableCDROM note, the upstream re-port sizing, eight
  stale README passages (verify_shipped.py, scratchpad tools, the optbright
  build paragraph, `discs/`, the pinned chain input, the What ships row, the
  mission-log cross-reference, the ini snapshot path), and the "items proven
  intact" conclusion, which held only for bytes a record named.

---

## 11. The 2026-09-06 pass: the VR disc

The whole of §5.5 as it used to read ("not started") is done. What the pass
established, beyond the patches themselves:

- **The VR disc is its own game.** Its own executable, overlays and containers;
  the only thing the main-game port supplied was the file formats and the
  discipline (own every byte of a pool; never relocate a stage; run
  `ppfcheck.py`). Every address in the README's VR section was read from the VR
  binaries, not assumed from disc 1.
- **USA's VR executable ships five languages.** English is first in the
  tables-of-tables and GCL variable `0x11` selects; the port reads the English
  pool and `vrlib.language_of()` recognises the same variable in scripts.
- **Windows are matched by content, not position.** The user's warning that the
  shots were not taken in the same order was right about the data too: the two
  discs do not lay their stages out alike. Titles reduced to uppercase
  alphanumerics key a pool; same-stage matches win, then pool-unique matches,
  then position for the few windows with no ASCII title.
- **Script-local fonts had to be merged, not chosen.** Codes ≥ `0x9A00` index a
  font inside each script. Integral's holds the Japanese glyphs, USA's the
  typographic quotes; keeping either alone produces mojibake, so USA's glyphs
  are appended and the ported strings' codes rewritten.
- **Nothing grew.** Ten stages shrank by a sector and were padded back, so no
  stage moved and no collection patch was orphaned.
- **Progress on the VR disc lives in VRAM.** The clear bitmap is a 12×16
  rectangle at (160, 224) that every mission stage's overlay knows; the save
  file is built from it. That is why `vr_unlock.py` can unlock everything
  without ever writing progress, and why deleting it restores the real state.
- **The KEY CONFIG textures did not fit** until every texture in the option
  stage's archive was re-encoded losslessly, which is why `pcx4.py` gained an
  8-bit codec.
- **The disc image has raw 2352-byte sectors.** A PPF offset is
  `(lba + off // 2048) * 2352 + 24 + off % 2048`. The first version of
  `vr_sweep.py` divided by 2048 instead, applied every record at a nonsense
  offset, and made a finished port look unported — for about twenty minutes it
  looked like a serious gap. Anything that reads a deployed PPF back must use
  `portio.image_offset`'s geometry. The port itself was always correct; only
  the checker was wrong.

Left where it was: the collection still intercepts VR's KEY CONFIG, so seeing
Integral's own needs `DisableRAM` and `DisableCDROM`. The ASI was rebuilt with
five VR patch watches and deployed at 00:25.

## 12. The 2026-09-07 pass: the VR disc on screen, and the last three items

A long day. The VR port's first play test fixed three faults; then the three
items that had been open longest all closed - the two-line MOVIE captions,
`en_menu3`, and the VR KEY CONFIG behind the collection's interception. Two
earlier conclusions recorded here were corrected in the process, and one
recorded diagnosis turned out to describe an artefact it had never been tested
against.

- **The option screen crashed the stage, and the cause is a general invariant.**
  `load option` died with `r3000: illegal instruction`. A DAR entry header is
  `{u16 id, s16 ext, u32 size}` written immediately after the previous payload,
  so an **odd payload size misaligns the next header's `u32`**. Our rebuilt
  archive had 39 odd sizes and 41 of 51 entry starts misaligned; all four retail
  DARs checked (Integral and USA, main and VR) have zero. Padding every payload
  to 4 fixed it. It cost run length: `pcx4._rle` had capped runs at 62 when
  `PCX_RLE_CODE(0xC0) + run` allows **63**, so `maxrun` became a parameter (it
  still defaults to 62, and the main game's `en_option` rebuilds byte-identical).
- **Then the option rows overlapped.** Record 3 held the vibration-test sentence
  that Integral draws at the row-label position, so the sentence appeared twice
  — blanked, the same decision the main game took on 2026-09-02. Integral's
  colon and values were still lit beside the English — unlit through the
  per-state colour switch, which sets every entry it skips to colour 0. And the
  lines sat off-centre — each ported entry got USA's `{num 1, x 160, y 196}`,
  measured afterwards within 0.3 game px of centre.
- **The EXTRA movies were gated separately from the missions.** `vr_unlock`
  patches `selectvr`; the clips are gated in the `movie` overlay's own
  `count / 3` score against 45 and 75, which is why they stayed `???` with the
  mission aid in place — and why USA's unpatched VR disc showed `???` too.
  `vr_unlock_movies.py` replaces one instruction. All three thumbnails appear.
- **The EXIT box moved 4 px up, and that was the user's call.** With USA's
  two-line caption the first line's ink overlapped Integral's EXIT box by two
  pixel rows. USA makes the room with a shorter caption face *and* a higher box;
  the face is Integral's own art and stays, so the box moved to USA's own
  `sp_exit` y — one immediate, and the selection highlight follows because it
  anchors on the same widget object. Asked and approved, and recorded as an
  exception to the user's stated default, "usually we skew toward the Integral
  visuals" (§5.4a, §6, and the README's amendment table).
- **The MOVIE captions took three attempts, and the third is deployed.** USA
  draws each TGS caption as two lines where Integral draws one per clip. Adding
  USA's records gave `record = clip` (clip A a fragment, the other two clips
  someone else's line); writing USA's caption **position table** moved the rows
  exactly as predicted and *still* gave one line each, which was read as "so it
  is all code". Both were half-right. The actor is the engine's generic
  numbered-text module — the same one `abst.c` implements — and it draws *every*
  slot that is lit, so the line count is only ever how many slots get lit. That
  is the one place the discs differ: USA calls the actor's own
  `highlight(work, i)` **twice**, for `clip*2` and `clip*2+1`, where Integral
  calls it once. The port retargets that single `jal` at a 16-word stub in the
  overlay's own sector padding, and keeps USA's records and position table. All
  three captions are English, deployed, and were seen on screen the same day
  (the user's shots at 01:03: both TGS captions on two rows, E3 on one); the
  EXIT box's 4 px move that followed is the part not yet seen. §5.4a and README
  "The MOVIE selection captions".

- **`en_menu3` is done, and the answer was "not here".** The last Japanese text
  in the main game with a USA counterpart now has a builder (`menu3.py`) and two
  verified PPFs - and they ship for a raw PSX disc only. The collection patches
  that same block itself, at the exact address of our first record, and the two
  layouts do not mix: the title stage dies on entry. Both the August crash and a
  fresh one on 09-07 are the same seventeen bytes. The recorded diagnosis
  (container sizes) turned out to describe nothing that was wrong with the file,
  and the shape that crashed is the one that leaves retail's layout untouched.
  The user chose raw-only over blacklisting the collection's patch. §5.3.

- **The VR KEY CONFIG was seen for the first time, and the highlight box was
  wrong.** With `DisableRAM`/`DisableCDROM` on, Integral's own screen draws with
  all eight labels English in all three button types and `key_syukan`'s +11
  clearing the curve. The user spotted what the measurements had not: the
  selection highlight on the `first person view` row stopped 24 px short, which
  is exactly USA's 112-wide art against Integral's 88. It is drawn by hardcoded
  `glow(work, x, y, w, h, ...)` calls, not by an `Init_Res` quad, so the geometry
  transplant never saw it - four immediates fixed it, re-measured at 113 px
  against USA's 114. §5.5, and README "The VR KEY CONFIG on screen".
- **Upstream was measured and deliberately not merged.** A fast-forward is
  impossible (133 commits of our own), every file the port touches has moved,
  and the ten upstream commits change no MGS1 behaviour at all - their `mgs1.cpp`
  diff is 0 insertions and 130 deletions. Deferred to the pull request, which
  needs the same work anyway. §5.6.

Three process notes worth keeping:

- **Ship what can be seen, not just what is safe.** The E3-only caption build
  was deployed first because it was provably safe under any mapping — but it is
  the clip behind `???`, so the user's next look showed Japanese captions and
  nothing else. Visibility should be checked before safety.
- **"The code is identical, so it must be data" is worth one experiment.** Every
  function in the caption *actor* was diffed against USA's and found logically
  identical, which made a data explanation feel forced rather than chosen. The
  position table was data, the write demonstrably landed, and the behaviour did
  not change. Reasoning of that shape earns a test, not a build. The error the
  failed test then invited was the mirror image — "so it is all code, in the
  functions I already read" — when the answer was seven words away in a function
  nobody had opened: the *input handler*, not the actor.
- **When a disassembled function indexes a global, find the same pattern in the
  decomp before naming the global.** `Act`'s tail loads from a table with a
  global at `0x800A9580` and that was written down as `captions[clip]`, which
  made `record = clip` look structural in the drawing code and sent two sessions
  hunting for a line count there. The decomp's `abst_sprt` indexes the same way
  with **`GV_Clock`** — frame parity — and the table is the per-frame ordering
  table. One misnamed global cost two builds and two play tests. The general
  lever: the decomp does not have to contain the *function* to name what it
  touches, and here it contained the whole module under another name.

## 13. The 2026-09-07 evening pass: the seven-item review, closed

A review of the port asked what could be improved in the patches themselves
rather than in the text, listed seven things, and then did all seven. Details
are in §5.10 to §5.13; what the day is worth remembering for is smaller than
that list.

- **A budget you derived is still a guess until you check it against retail.**
  The `vrwindow` width budget was read out of the decomp one call at a time and
  is, as far as anyone can tell, correct — and retail Integral has 107 lines
  over it. Asserting it would have failed on the game's own shipped data. The
  invariant that survived is the one with a witness: never render wider than the
  USA line this came from, because USA drew it.
- **The thing three documents called unknowable took one afternoon.** "A named
  patch's bytes are only visible through `SetPatchWatch`, and named-file patches
  carry no inline data" was true and complete about the *watch*, and it quietly
  became a belief about the *bytes*. They were in a file on disk the whole time.
  When a document says something cannot be known, check whether it means cannot
  be known or merely was not tried by the route in front of you.
- **A checksum makes a good oracle for questions that are not about checksums.**
  Recomputing sector parity was meant to make raw-disc patches correct. It also
  proved the retail executables are the disc's, proved the decomp's build is
  byte-faithful, found the collection's zero-filling in a second measurement,
  and showed that its USA image was not pressed with the executable this port
  uses. None of that was the reason for writing it.
- **Test the tests.** All 23 passed on their first run, which is not evidence.
  Mutating three modules one at a time and confirming each mutation is caught is
  evidence — and it immediately turned up a real trap: a same-length edit
  restored within the same second leaves Python running the **cached bytecode**,
  so a test can appear to fail against source that is already correct. Clear
  `__pycache__` before believing a result like that.

## 14. The 2026-09-07 late pass: the item text read against USA, end to end

The evening's seven-item review (§13) was the port looking at itself. What
followed was the opposite - the game being read on screen against its donor,
which turned up two faults in the fix, one gap in the port, and two corrections
to things this file had asserted.

- **`[Game] GiveItems` had never worked, and nobody knew** because nobody had
  pointed it at an empty inventory in five weeks. An item Snake does not have
  is stored as **-1**, not 0, so its "only where the count is zero" guard could
  never fire; it logged twenty-four grants and made none. Fixed, along with two
  neighbouring faults, and `[Game] GiveWeapons` added beside it once items alone
  turned out not to be the ask. §5.7.
- **All 24 items and all 10 weapons were then photographed in both games and
  compared** - text, line breaks, and glyphs including `《》`, the button glyphs
  and the apostrophes. **34 of 34 identical.** The one difference in any of the
  shots was in a table the port had never touched: the side column's
  abbreviation, `SCARF` against USA's `HANDKER`.
- **That produced a new rule.** Replacing Integral's *own English* is a case
  neither standing rule covered; the user's answer is amendment 4b in §2, and
  `SCARF` -> `HANDKER` is its first and so far only application. Four other
  questions of the same family were deliberately left open.
- **A description is not always the string its table points at.** Reading the
  only two functions that print one turned up six slots that change with the
  game state - and one of them, the MP5 SD that replaces the FA-MAS on VERY
  EASY, is Japanese, reachable, and correct. Both conditional forms were then
  seen on screen for the first time. §5.1a.

Three process notes, and the first two are the same note twice:

- **"Built" is not "exercised."** `GiveItems` was written, compiled, reviewed and
  documented as working, and its guard had never once been in a state where it
  could fire. A guard that never fires looks exactly like a guard that is never
  needed.
- **A stride is not a structure.** Walking the short-name region on a fixed
  8-byte stride said Integral's table had an extra entry, `MP 5 SD`. It does
  not: the table ends earlier, and that string is a literal the compiler put
  nearby. Walking the same region as NUL-terminated strings gave the right
  answer immediately. The first version of that claim reached this file before
  the second version corrected it.
- **The bytes could not be read without drawing them.** None of the Japanese
  here is Shift-JIS - it is font indices, so `game_text` can only ever print
  `<822F><8253>`. `rendertext.py` draws a string with the game's own font, and
  two of its own details (bit order, and a one-glyph bank offset) were settled
  by rendering a word whose reading was already known. Both mistakes produce
  plausible-looking Japanese that is simply the wrong Japanese, which is the
  only reason the check was worth making.

## 15. The 2026-09-08 pass: the sweep's last finding, and what the sweep could not see

`en_pad2` was built, verified, deployed and registered in one short pass (§5.11
has the record). It is the smallest family in the port — 159 bytes a disc, three
slots, no container touched — and the interesting part is not the patch.

- **Reading the caller settled in five minutes what the notes had guessed at
  for a day.** `game/second.c` is 45 lines. It takes one string per spawn and
  draws it with `MENU_JimakuWrite` when pad 2 goes live. That single fact showed
  the "two records, record 0 and record 1" this file described could not exist:
  what `s07b` has is two *spawns*, in two script branches, and USA translated
  the later one. The general form of this is already in §13 — when a document
  describes data, check whether anyone has read the code that consumes it.
- **A sweep's universe is part of its result.** `mainsweep.py` reports "one
  finding" and that is true of the 82 stage names both discs share, which is
  what it compares. The same string sits in `s07br`, one of the 13
  Integral-only names, where nothing was ever going to look for it. Neither the
  tool nor the three documents quoting it said so. A tool that pairs two things
  can only see their intersection, and that boundary belongs in the write-up
  beside the count.
- **The user's decision is the one worth recording.** USA is inconsistent here:
  the same message reaches the same actor down two branches and only one was
  translated. "Verbatim USA text, placed where USA places it" read literally
  means shipping that inconsistency. Asked, and the answer was to port all five
  sites — the rule forbids inventing text, not fixing a donor's oversight with
  the donor's own words. Worth remembering the next time the rule and the
  outcome point in different directions: ask, and say which way each points.
- **The trap in a length-preserving edit is where the padding goes.** After the
  terminator it is dead bytes; before it, it is drawn — and jimaku centres on
  the width it measures, so the line would drift off centre with nothing in the
  bytes to suggest why. `selftest.py` guards it, and the guard was proved by
  mutation, as §13 requires: four mutations, four failures.

### The same day: the three sweeps that were owed, and one input that is wrong

The housekeeping was done first (the four unlock aids deleted, the flags back to
`false`, the disjoint VR pair finally deployed, the branch committed), and then
the three items §5 filed under "to investigate" were all run. Two closed
cleanly; the third found something and then taught the method a lesson.

- **The Integral-only stages (`--integral-only`).** All 13 pair onto the USA
  stage they are a variant of, and across all 13 on both discs there is
  **exactly one** Japanese string whose base-stage owner has English:
  the `s07br` copy `en_pad2` had just ported. So the hole in the sweep's
  universe was worth closing and was empty. That is the good outcome, and it is
  only knowable by looking.
- **The census (`--census`).** All **1,360** Japanese GCL strings on each disc
  now fall in one of three explained buckets with **0 unaccounted**, and the
  buckets are asserted to sum to the total, so the tool cannot report a clean
  result by dropping a string. It exits non-zero if the last bucket is ever not
  0. This replaces "about 160 remain and need verification" with a number:
  none.
- **English against English (`--diff-english`), and the fourth spelling.** The
  sweep §5.14 asked for found the `abst` location table has **four** differences
  from USA's, where three documents listed three. `Cmnd rm` against USA's
  `Cmnd room` had never been written down. The residue really is that small: 15
  replace hunks over 82 stages, 8 with readable text, and after triage one
  family.
- **And the input is wrong for that question.** Run against the VR disc, a
  per-owner fuzzy match reported `FAMAS` against USA's `FA-MAS` in the mission
  titles - which the port had already fixed, because it takes USA's window text
  verbatim: the deployed PPF holds 142 `FA-MAS` and no `FAMAS`. `mainsweep.py`
  reads **retail** on purpose, so a Japanese gap cannot hide behind a deployed
  patch. For English-against-English that discipline is exactly backwards: the
  port's own replacements are what must be subtracted, so the question belongs
  on the **deployed** bytes. It is safe today only because the one family it
  finds sits in records no patch rewrites - which is a fact that was checked,
  not a property of the tool. **The rule: match the input to the question, and
  say which one the tool reads.**

USA's VR disc also carries five languages, so any diff there must take only the
English arm of a language branch (`lang in (None, ENGLISH)`); without it the
alignment collapses and the output is 548 hunks of nothing.

### And then the location names were decided

The four `abst` spellings the sweep turned up were put to the user the same
evening and the answer was **use USA's**. That makes two applications of
amendment 4b in two days, and this one had teeth the `SCARF` case did not: the
text grows, so a container had to move.

- **The block carries two derived lengths, not one.** A COMMAND's BE16 size is
  the obvious one; the u8 at `start+5` that `option_starts` reads to reach the
  option list is the one that would have been missed - `0xAD` against USA's
  `0xB9`, exactly the 12 bytes of growth. Patching the four records and
  re-stamping only the BE16 would have produced a block that disagreed with
  itself. Taking USA's **whole command** keeps both in step by construction, and
  is also the most literal reading of the rule.
- **The names are font codes, not ASCII.** `0x80xx` Latin, `0x9001` space. An
  ASCII search of the PPFs for `Tank Hangar` finds nothing, which briefly looked
  like evidence the port already owned them. `rendertext.py` exists for exactly
  this reason and the same trap is recorded in §14.
- **The constant was run both ways.** Off: 104,600-byte chunk, list equals
  Integral's own, 0 records differing. On: 104,612, equals USA's, 4 differing.
  A measurement of the cost rather than a claim about it - and the 12-byte delta
  is the arithmetic checking itself.
- **The verifier catches the offset byte without testing it.** `location_block`
  finds the command through `option_starts`, which reads that u8 and requires the
  empty option list it points at, so a wrong value fails to find the block at all
  instead of passing quietly. The best checks are the ones a wrong answer cannot
  route around.

And the input question the user pushed back on - whether the English-against-
English sweep needed a deployed-bytes reconstruction - resolved the other way
once it was thought through. The builders already construct the ported stage;
reconstructing it from PPF records would re-derive that by the hardest route and
need extending for every relocation. The authority for owned bytes is the
family's own verifier, the sweep's authority stops at the boundary, and the `!!`
flag marks where. §5.14 step 3 carries the table.

## 16. The 2026-09-09 pass: the file nobody had opened

A question from outside the project - is there an Integral-exclusive Japanese
developer-commentary codec channel? - turned out to have an answer no tool here
could reach. There is, it is **6.5 MB**, and the reason it was invisible is
structural rather than careless.

- **Every sweep read one file.** `mainsweep.py`, `vr_sweep.py`, `jpsweep.py`,
  `audit_text.py` and the day-old `jpremain.py` all walk `STAGE.DIR` and the
  executables. A disc has nine files. Codec dialogue lives in `RADIO.DAT`,
  loaded by sector out of `menu/radiomes.c`, so no GCL walker could ever have
  seen a word of it - and four documents said the port was complete without
  naming the file their claim was about. `discaudit.py` now audits all nine.
- **The diagnostic was a size delta, not a sweep.** Integral's `RADIO.DAT` is
  6.3x USA's. That single number is what located the commentary; the same
  measure clears `BRF.DAT` (381 KB of English on both) and `FACE.DAT`
  (identical), which had been assumed rather than checked.
- **A correct ratio described the wrong thing.** 964 KB of Japanese against
  1,146 KB of English is 42%, exactly what a faithful translation of the same
  script gives - so the totals said "paired subtitle track, nothing extra".
  Mapping *where* each language sat, window by window, is what exposed a 6.5 MB
  block containing no English at all. Aggregate ratios can be right and still
  answer a different question.
- **The port's scope did not change.** USA never shipped the commentary, so
  there is no English to copy and the standing rule leaves it alone. What
  changed is what the documents may claim: "nothing with a USA counterpart is
  still Japanese" is true of the text this port covers, and was being read as
  true of the disc. `COVERAGE-RECORD.md` now states both sentences.
- **The general lesson, and it is the same one as §13's.** When a claim of
  completeness is made, say what it ranges over. Every sweep here answered its
  question correctly inside a universe none of them named - shared stage names
  for `mainsweep`, one archive for all of them - and each time the gap was found
  by someone asking about a thing outside it rather than by the tools.

## 17. The 2026-09-09 pass: the untranslated Japanese, dumped — and what to do next

### DONE - see §18

The glyph identification is finished: **all 1,200 shapes are named and 100.00%
of the Japanese decodes**. Two things this section said are wrong and are
corrected in §18, because they matter more than the fact that the job is done:

* **"1,813 glyph shapes"** was an artefact. The real count is **1,200**; the
  extra 613 came from a fragment map that was wrong for 93% of strings.
* **"Two rules give 100.000% (94,246 of 94,246 strings)"** measured that every
  string got *an* answer, which the rule guarantees. It was not a check. The
  check is how many distinct 12x12 bitmaps the text lookups produce: 91,834
  then, 1,200 now. `radiomap.py` prints it.

The image budget this section warns about also turned out to be the wrong
worry. The whole pass cost eleven images, because the glyphs were read against
decoded sentences (`glyphsheets.py` writes them beside each page) rather than
by squinting harder at 144 pixels.

### The scope, which is the user's decision and narrows this a lot

**Only Japanese that MGS1 USA has no counterpart for.** Asked and answered
2026-09-09. `RADIO.DAT` holds two halves: a story-codec region where the
Japanese is a *subtitle track* for conversations USA ships in English, and a
commentary region with no English anywhere in it. The first is covered by USA
and out of scope; only the second needs anything.

| | rows (disc 1) | kana/kanji | |
|---|---:|---:|---|
| story codec `0x0`-`0x042C54C` | 16,869 | 282,280 | out of scope |
| **commentary `0x042C54C`-`0x0AAC050`** | **77,361** | **1,369,719** | in scope |

`jplist.py` already applied that test to `DEMO.DAT`, `VOX.DAT`, `BRF.DAT` and
`FACE.DAT` by subtracting anything that also appears in USA's copy of the same
file - which is what empties the last two entirely. `RADIO.DAT` slipped through
it, because USA's codec text is plain ASCII and there were no font-code runs to
subtract against. `dumpjp.py --scope unported` is where the region split lives.

### What was produced

`py dumpjp.py` writes **156,379 lines over 5,589 pages**, every line rendered
from the game's own glyph bitmaps, so the images are exact by construction:

    work/jpdump/disc1_RADIO_DAT.pdf   77,361 lines  2,763 pages   the commentary
    work/jpdump/disc1_DEMO_DAT.pdf       576 lines     21 pages
    work/jpdump/disc1_VOX_DAT.pdf        338 lines     13 pages
    work/jpdump/disc1_STAGE_DIR.pdf       98 lines      4 pages
    work/jpdump/disc2_*.pdf                            (RADIO is a duplicate)
    work/jpdump/index.tsv            156,379 rows

`index.tsv` locates every line: disc, source, fragment base, byte offset, page
and row, the text decode so far, and the raw codes.

Verified without eyes, because the image budget was gone by then: 4,000 sampled
lines render with none blank or sparse, and every glyph cell of a line matches
the font byte for byte.

### Bank 1's table, and reading the parser instead of guessing

`0x9A01 + i` indexes a `.gcx` script's own font blob; `RADIO.DAT` carries one
per fragment, and `menu/radiomes.c`'s
`menu_radio_codec_task_proc_80047AA0()` gives the address outright:

    radioDatIter   = fragment + 8
    fontAddrOffset = BE16(radioDatIter + 1) + 1
    font_set_font_addr(1, radioDatIter + fontAddrOffset)

so `base = fragment + 9 + BE16(fragment + 9)`. Fragment 0 yields `0x1B1`, the
value proved independently by finding the single place in 11 MB where 本 is
immediately followed by 出 - they sit at adjacent indices in that conversation.

**Fragments are sector-aligned but MULTI-sector** (`size = (radioCode /
0x1000000) * 2048`), which is the fact that made attribution hard: a
mid-fragment sector can produce a sane-looking base by accident, and a string's
true fragment can be tens of KB behind it. Two rules give **100.000%** (94,246
of 94,246 strings, 77,361 of 77,361 in the commentary): a candidate must contain
a string in its text region `[frag+8, base)`, and a string belongs to the
nearest candidate behind it that covers it, searching **128 KB** back.

Five attempts preceded that and are worth knowing so they are not repeated:
first-plausible-run-after-the-text (99.4%), coordinate ascent on
repeated-sentence agreement (61.5%), greedy non-overlapping fit (49.9%), a
sequential fragment walk (78.1%), the same walk with a stricter accept test
(16.3%). The lesson is the ordinary one: the first method was the best and was
abandoned over a 0.6% residue instead of being repaired, and reading the
parser - which took one grep - beat all five.

### Dead ends, measured, so nobody spends a day on them

Template matching cannot identify these glyphs. Two references were tried
against the 238 hand-transcribed kanji, which is a real labelled test set from
the same font:

| reference | top-1 |
|---|---:|
| MS Gothic / Meiryo, 96px rasterised then area-averaged to 12x12, ZNCC + ink gate | 47% |
| **Shinonome 12-dot** (MIT, native JIS X 0208 at exactly 12 dots, 6,879 glyphs) | **0% exact, 5.9% fuzzy** |

Shinonome was the right idea - a native 12-dot bitmap font is the
apples-to-apples comparison - and it failed because **Konami drew their own
12x12 design**. There is no font to look these up in. An OCR engine is also the
wrong shape: `manga-ocr` and Tesseract recognise text *lines* at real
resolution, and the input here is 144 pixels, where the glyph *is* a specific
bitmap design rather than a picture of a character.

## 18. The 2026-09-10 pass: the commentary is readable, and the map it rested on was wrong

**All 1,200 bank-1 glyph shapes are identified: 100.00% of the glyph codes in
`japanese-inventory.tsv` now decode to text.** Read that qualifier - it is
load-bearing, and "Where the number stops" below says exactly what it excludes.
The developer commentary §16 found is no longer a stack of pictures; it is
156,379 lines of readable Japanese in `work/jpdump/disc1_RADIO_DAT.txt` (plain
text, conversations separated by a blank line) and `work/jpdump/index.tsv`
(one row per line, with offsets and raw codes). The channel says what it is in
its own words:

> この周波数では「メタルギアソリッド」制作スタッフによる制作過程での裏話などをお伝えします。
> なお、この周波数のみ字幕言語設定が英語の場合でも日本語で表示されます。

That second sentence is the game telling you §16's conclusion directly: this
frequency stays Japanese even with the language set to English. There is no
English to port and the standing no-translation rule leaves it alone.

### The thing to learn from this pass

§17 set up the glyph work on a fragment map that was **wrong for 93% of
strings**, and said so in the language of certainty: "Two rules give 100.000%
(94,246 of 94,246 strings)". That number counted strings that got *an* answer.
It could not have counted anything else — the rule always terminates.

One cheap measure exposes it. Bank-1 codes index a font, so resolving them
yields 12x12 bitmaps; **count the distinct ones.** A Japanese font has a couple
of thousand. The old map produced **91,834**, and only 9.8% of them had the
blank twelfth row that every real glyph in this font has. The current map
produces **1,200**, all of them with it.

So: when a walk over data reports a percentage, ask what the percentage would
look like if the walk were wrong. If the answer is "the same", it is not a
check. `radiomap.py` prints the distinct-bitmap count for exactly this reason —
that is the number to look at, not the share of strings attributed.

### What the map is now (`radiomap.py`)

Two things fixed it, both from reading the game rather than the bytes.

* **Parse the script.** A `RADIO.DAT` fragment's script is not GCL; it is a
  record list of its own (`menu_gcl_exec_block_800478B4`, `menu/radiomes.c`):
  `FF <code> <BE16 size> <payload>`, next record at `+size+2`, a 0 byte ends it,
  and the font blob sits at `script + totalSize + 1`. Walking those records is
  self-checking — a sector that is not a fragment desynchronises within a record
  or two. 597 of 5,468 sectors survive, and the first ends at exactly `0x1B1`,
  the value §17 had proved independently.
* **Get an answer key.** Fragments are named by "radio codes", unpacked by
  `sub_80047D70`: `startSector = code & 0xFFFF`, Japanese `(code >> 24)` sectors
  there, English `((code >> 16) & 0xFF)` sectors immediately after. The codes are
  arguments to the GCL `radio` command (id `0x24E1`, `GV_StrCode("radio")`,
  `game/script.c`) in the stage scripts; 192 are recoverable. **All 192 Japanese
  starts and all 192 English starts are among the 597 the parse found, and no
  declared extent is overrun.** That is what makes the parse trustworthy instead
  of merely plausible.

A nested `IF`/`SWITCH` body has the same header as a fragment, so one that lands
8 bytes after a sector boundary parses like one. Dropping every candidate inside
another candidate's script region removes all 28 such cases the radio codes prove
false and none of the 384 they prove true, leaving **553 fragments**: 26 of
94,246 strings unattributed, zero glyph over-runs.

Note the layout this exposes: **Integral's `RADIO.DAT` stores each conversation
twice, Japanese then English, adjacent**, and one runtime flag picks the half.

### How the 1,200 were named, and how good it is

Not by looking harder at 144 pixels. `glyphsheets.py` now writes, beside each
page of glyphs, a `.txt` of **real lines from the game with the glyph marked and
everything already readable spelled out**. At 12x12 線/緑, 鏡/鎌 and 間/問 are
the same picture; in a sentence they are not. The bitmap says which characters
are possible and the sentence says which one it is. Every case where the two
disagreed, the sentence was right: 望 was 量 (大量に分泌), 屈 was 肩 (肩もみ),
惜 was 情 (情報), 問 was 聞 (直接聞くさ), 昔 was 替 (すり替えた).

Then `glyphreview.py --verify` prints **one full decoded sentence per glyph, all
1,200**, and they get read. That is what catches the rest: 黙 and 弄 had been
swapped ("なぜ今まで黙っていた" / "…のように弄ばれつづけた"), and two more fell
to comparing a bitmap against its near-twin — 完 was 璧 (identical to 壁 in its
top three rows, with 玉 below where 壁 has 土) and 朴 was 林.

**Measured, not asserted:** 78 shapes had already been identified from the stage
archives, and `glyphsheets.py` puts them on the sheets unmarked with the answers
in `work/glyph-answers.tsv`. **78 of 78 correct.** Two of those 78 were scored
wrong at first and turned out to be errors in the *key* (below), which is the
only reason to trust the other 76.

The whole pass cost **11 images**: eight sheet pages, two magnified sheets for
the name kanji that no sentence can check, and one single glyph.

### Five characters in the existing tables were wrong

The complete decode makes bank-0 mistakes visible, because a wrong bank-0
character now sits in an otherwise readable sentence.

| where | was | is | the sentence that settles it |
|---|---|---|---|
| `GLYPH_90[0x9078]` | 句 | **匂** | 硝煙の匂いがなつかしいぜ |
| `GLYPH_90[0x90A5]` | 端 | **奪** | 力を奪う事ができる / メリルに服を奪われて |
| `GLYPH_90[0x90D0]` | 継 | **繊** | 筋繊維を刺激してみたの / 大胆にして繊細 |
| `BANK1[('roll',0x9A05)]` | 五 | **六** | レイブンは六人もの人間を運んだ (against 四人運び) / 第六感 |
| `BANK1[('rank',0x9A0D)]` | 液 | **清** | ウイルス兵器だ。必ず血清がある |

All five are fixed in `jptext.py`. The 五/六 one is worth dwelling on: it was
"proved" by 二万五千 in the staff roll, and the roll gives no way to tell 五 from
六 at 12 pixels. `RADIO.DAT` does, twice.

### An invariant worth keeping

**Bank 0 and bank 1 never share a bitmap, and — once those five are corrected —
never share a character either.** All 1,200 bank-1 shapes were compared against
every glyph in `font.res`: not one matches. So a character bank 0 already has
(the 255 kanji of `GLYPH_90`, the kana, the punctuation) cannot be the answer to
a bank-1 glyph, and `glyphfill.py` warns when an assignment breaks that. Every
warning it raised in this pass was a real error — three in `GLYPH_90`, none in
the new work.

### Every conversation already has an English slot (asked 2026-09-10)

Asked whether the process could be reversed - English written where the
Japanese is. The engine already has the mechanism, and the commentary already
has the space. `sub_80047D70` picks a fragment half by the language flag:

    startSector = code & 0xFFFF
    Japanese    = (code >> 24) sectors there
    English     = ((code >> 16) & 0xFF) sectors immediately after

Measured on disc 1, over the fragments the radio codes name:

| | Japanese half | English half |
|---|---|---|
| story codec | 28.7% glyph codes | **1.6% glyph codes, 73.9% ASCII** - `"Be careful, Snake. That air lock is set..."` |
| commentary | 35.6% glyph codes | **35.6% glyph codes** - a copy of the Japanese |

So the story codec's English half is plain ASCII, and **the commentary's
English half is a duplicate of the Japanese** - which is exactly what the
channel says about itself: 「なお、この周波数のみ字幕言語設定が英語の場合でも日本語で表示されます。」
None of the 192 recovered codes declares an empty English extent.

Writing ASCII into the English half therefore shows in English mode and leaves
the Japanese untouched in Japanese mode - no relocation, no new allocation.
The budget is comfortable: ASCII is one byte per character against two for
glyph codes, and the English half needs no font blob (1.49 MB of the
commentary's 6.81 MB is font).

What would still have to be built: a writer that keeps the record's BE16 size,
the script's BE16 total and the fragment's declared sector count in agreement,
and that respects the 240px line width (README, "font render limits"). Then
the usual on-screen check.

**None of this is authorised.** Filling those slots means writing English that
Konami never shipped, which is translation - rule 1, verbatim: *"I'm not
authorizing you to translate (yet) anything only in english with no port should
be left in Japanese."* The mechanism is recorded here so the answer exists; the
decision is the user's.

### Where the number stops, measured the day it was claimed

The 100% is over `japanese-inventory.tsv`, and **the inventory is not the
file**. `jplist`'s scanner walks for runs of glyph codes and **ends a run at
any code it does not recognise**, so text after such a code starts a new row
and the code itself is dropped. Two ranges it does not recognise carry real
text:

* **`0x91xx`** - bank 0's second kanji page. `GLYPH_90` covers `0x90xx` and
  only four characters of `0x91xx` were ever identified, so the scanner treats
  the rest as "not Japanese".
* **`0x97xx`** - bank 1 above index 255. The README says bank-1 codes "never
  leave `0x9601`-`0x96FF`"; that is wrong. The commentary's font blobs hold up
  to **441** glyphs and the codes run straight on into `0x97xx`.

The symptom is visible in the dump: 「無限バンダナは制作チーム内では昆」 - the
布 of 昆布 is `0x9106`, the run stops on it, and the next row begins after it.

Measured over the commentary region, counting only codes that resolve to a
glyph passing the blank-twelfth-row test:

| | glyph instances |
|---|---:|
| captured by the inventory (all decode) | 1,729,477 |
| **skipped by the inventory** | **301,473** (14.8%) |
|  of those, decodable with today's tables | 287,601 (95.4%) |
|  needing new identifications | 13,872 (0.68% of the commentary) |

So the shape table holds up well on the text it never saw - 640 of the 653
distinct bank-1 shapes in the skipped bytes are already named. What is missing
is small and specific: **13 bank-1 shapes** and **23 bank-0 codes**, the
commonest being `0x9101` (2,538 uses), `0x8F65` (2,198), `0x9110` (1,754) and
`0x910C` (1,712).

**The next job, in order:** extract text from the parsed `TALK` records instead
of scanning bytes - `radiomap.walk_block` already gives the records, and
recursing into `IF`/`SWITCH` bodies is the missing piece - then name those 36
characters. That takes the commentary from 85.2% of its glyphs to all of them.

And note what this is an instance of. §16 said: *when a claim of completeness
is made, say what it ranges over.* The claim above ranged over the inventory
and was written as if it ranged over the disc, one section after that lesson
was recorded. The habit does not install itself.

### What is left

* **One glyph, two uses.** `('title', 0x9A27)` in 「⟪9A27⟫のラブソング」, a
  demo-theater label in the stage archive. Its bitmap is mostly mid-tone rather
  than stroke-and-background, so it may not be a kanji at all; it was
  unidentified before this pass too. 2 uses out of 4.2 million.
* ~~**The VR disc's stage text is not in the dump.**~~ **Fixed §20.**
  `dumpjp` looped over disc indices 0 and 1, so the VR archive's 31 `vr`
  rows were skipped entirely. The three discs are keyed by name now and the
  export is 68,242 lines. It was *not* small: it was a whole disc, and the
  note above talked itself out of checking.
* **`dumpjp.stage_blobs` assumes one font blob per stage.** `roll` and `abst`
  have two. It takes the first, which is right for everything checked so far.
* **Nothing here changes the port's scope.** USA never shipped the commentary,
  so there is no English to copy. What changed is that the Japanese can now be
  read, by anyone, as text.

## 19. The 2026-09-10 pass: the export finished, and the scanner retired

**The unportable Japanese is exported in full.** 68,242 lines, 3,923,944
kana/kanji, **zero** unresolved glyph codes. (The figures here were
68,211 / 3,923,661 until §20 added the VR disc.)

    work/jpdump/disc1_RADIO_DAT.txt   the developer commentary, plain text
    work/jpdump/disc<n>_<source>.txt  DEMO.DAT, VOX.DAT, STAGE.DIR likewise
    work/jpdump/index.tsv             one row per line, with offsets and codes
    work/jpdump/*.pdf                 the same lines drawn from the game's font

### What was still wrong when §18 was written

§18 said 100% and meant "100% of `japanese-inventory.tsv`". Chasing that
qualifier turned up two real defects, and the second one had been quietly
corrupting text since the beginning.

**1. The inventory is not the file.** `jplist`'s scanner ends a run at any code
it does not recognise, dropping the code and starting a new row after it. It
did not recognise `0x91xx` (bank 0's second kanji page) or `0x97xx` (bank 1
above index 255), and both carry real text - so the inventory held **85.2%** of
the commentary's glyph instances. The fix is not a better scanner:
`radiotext.py` walks the records the game walks (`menu_gcl_exec_block_800478B4`
and the `TALK`/`IF`/`SWITCH`/`RANDSWITCH` payloads), which is where the text
provably is. It finds **33,277 subtitles and 2,257,918 glyph instances** in the
commentary against the inventory's 1,729,477, reaches all **125** commentary
fragments where the inventory reached 124, and has more text than the inventory
in **every single fragment** - it never trades one gap for another.

**2. The bank-1 index was off by one from `0x97xx` on.** Every tool here used
`code - 0x9601`. The game uses `zen_index` (`font/font.c`, via
`rendertext.zen_index`), which is `((code - 0x956B) - code/256) | 0x1000` - the
index drops one per 256-page, because each page's `00` entry is not a glyph. So
`code - 0x9601` is exactly right inside `0x96xx` and one too high from `0x97xx`
onward, naming every glyph one position too far along.

The inventory only ever contained `0x96xx` codes, so **the error was invisible
until the record walk started reading `0x97xx`** - 58,704 codes in the
commentary, every one of them a wrong kanji. It showed up as one sentence
rendering differently in different fragments:
「プログラムの記述が楽になる効果」 in one, 「述□が楽になる効果」 in another. `radiomap.bank1_index`
is now the single copy of that rule and every tool calls it.

The lesson is the cheap one again: the two renderings could not both be right,
and that was visible in the output long before it was explained. Read the
output.

### The 24 characters that were hiding behind the scanner

The text the inventory dropped needed 11 more bank-0 codes and 13 more bank-1
shapes. Every one is settled by a sentence, not by a second look at 144 pixels:

| | | |
|---|---|---|
| `0x9101` 気 無邪気 | `0x9102` 絶 気絶している | `0x9103` 安 安田有希子 |
| `0x9104` 属 付属機関 | `0x9106` 布 昆布 | `0x9107` 完 完成した |
| `0x910C` 自 建物自体 | `0x910D` 拳 中国拳法 | `0x910E` 銃 銃口 / 銃身 / 銃弾 |
| `0x9110` 初 業界初 / 当初 | `0x900D` 『 pairs with `0x900E` 』 | 貢 + 献 作品総体に対し貢献する |
| 師 ドット絵師 | 泣 怒ったり泣いたり | 委 想像力に委ねた |
| 梯 梯子昇降 | 即 即座に当たる | 伴 それに伴うカメラ |
| 痢 下痢モーション | 又 又、後半の | 涙 （涙） |
| ＊ and ＄ - ○＃％＆＊＄! , a censored expletive | | |

`bank1-glyphs.tsv` now holds **1,213** shapes and `GLYPH_91` fourteen codes.

### What is left, exactly

* **Nothing.** The last glyph, `('title', 0x9A27)`, is **蒼** - see below.
  1,214 bank-1 shapes, zero unresolved codes in the whole export.
* **Two `IF` records fail to parse**, both in the story-codec region, which is
  out of scope (USA ships that dialogue in English). The commentary walk is
  clean.
* **The story codec is not exported.** By design - §17's scope decision - but
  note the record walk only reaches 43 of its 425 fragments, so anyone who
  wants it should expect to work on the walk first.

### The last glyph: 蒼, and why the disc could not name it

`('title', 0x9A27)` resisted every method here, and the reason is instructive:
**there was nothing on the disc to check it against.** The bitmap occurs
exactly once per disc - index 38, the last glyph of the `title` blob - and
appears in no other stage on disc 1, disc 2 or the VR disc, and in none of
`RADIO.DAT`'s 1.4 MB of font blobs. Its one string, 「⟪9A27⟫のラブソング」,
sits orphaned among the memory-card prompts. Context is what settled the other
1,213; here there was none to have.

It came from outside: TCRF's *Metal Gear Solid (PlayStation)* page documents
the Japanese DEMO THEATER, whose four rolls are titled 蒼のラブソング,
蒼色の青春, 紅のラブソング and 紅色の青春. Integral replaced all four with
メリル / オタコン / （赤忍者）メリル / （赤忍者）オタコン - and the disc agrees
exactly: searching both main archives for the byte sequence のラブソング finds
**one** hit each, the leftover ROLL A title. The other three went with their
strings, which is why 蒼's glyph survives with no companion characters.

The bitmap decomposes as 艹 over 倉 (`g647`), which is 蒼. Note the honest
weight of that last check on its own: at 12x12 a shifted 倉 agrees with it on
85% of pixels and an unshifted one on 84.7%, so the pixel test decides nothing.
What decides it is the external source naming the exact string plus the disc
holding exactly that string, once, orphaned.

*A method note.* This is the one character out of 1,214 that no amount of
reading the disc could settle, and the lesson is not that the method failed -
it is that a cross-reference is the whole method, and when a glyph has exactly
one use anywhere, there is nothing to cross-reference. Recognising that early
is worth more than another pass over the pixels.

### What is now believed with what evidence

| claim | how it is checked |
|---|---|
| the fragment map is right | all 192 JP + 192 EN radio-code starts parse; no declared extent overrun; 1,200 distinct bitmaps from the inventory's codes, 100% with the blank twelfth row. **Those are aggregates and cannot see one fragment slip** - `radiotext.py --check` is the per-fragment guard, and `--selftest` proves it catches a slip (§21) |
| the text extraction is complete | more text than the inventory in *every* commentary fragment, all 125 reached, 0 unresolved codes |
| the glyph table is right | 78/78 on the labelled holdout; every one of 1,200 read back against a full sentence; the 13 new ones each settled by a sentence |
| the bank-1 index is right | it is `zen_index`, from the game; and the sentence that used to render two ways now renders one way everywhere |

## 20. The VR disc, and no game data in the repository

Two small things, both from the same question: is what is committed actually
what it says it is?

### The export was missing a disc

`disc1_RADIO_DAT.txt` is 97% of the export, which is close enough to all of it
to stop looking - and stopping there would have been wrong. `dumpjp.main`
looped `for disc_ix in (0, 1)`, so the **VR disc was never dumped at all**: 31
lines of it, sitting in `japanese-inventory.tsv` the whole time under a `vr`
key nothing consumed. §19 noted the VR disc had no `RADIO.DAT` and moved on
without noticing that it does have a stage archive.

The fix is that the three discs are now three names, not two indices:
`STAGE_DIR = {disc1, disc2, vr}` maps each to its archive, `collect(disc)`
takes the same key the inventory rows carry, and the `RADIO.DAT` half is
skipped for `vr`, which genuinely has none. Output files are named by that key
too, so the third is `vr_STAGE_DIR.txt` rather than a `disc3_` that would be
wrong. The export is 68,242 lines now; it was 68,211.

*Worth naming the shape of this one.* Every check in §18 and §19 was a check
on **content** - do the glyphs resolve, does the text match the records, does
one sentence render the same way twice - and all of them passed on a set that
was silently missing a member. A completeness check has to enumerate the
inputs, not audit the outputs. The inventory had a `vr` row all along; nothing
compared the set of discs in it against the set the dumper walked.

### No game data in the repository

`CREDITS.md` says "No game data is in this repository." That had stopped being
true, in three places, and all three were added by this work:

| what | why it counted | what now |
|---|---|---|
| `reference/keyconfig_*.jpg`, 6 files, 1.46 MB | photographs of both games running | deleted; `REFERENCE.md` says how to reproduce them, and the label mapping they settled stays |
| `keyconfig-textures.png`, 37 KB | eight of the game's own textures, rendered side by side | deleted, same |
| `bank1-glyphs.tsv`, 104 KB | its `shape_hex` column was 1,214 raw 12x12 font bitmaps - the Japanese font itself, in hex | `shape_id`: a 64-bit digest of each bitmap (`jptext.shape_key`) |

The digest is the interesting one, because it costs almost nothing. Lookup
hashes the bitmap the game hands it and reads the table, so every decode works
exactly as before - the round-trip is byte-identical and the 78-answer holdout
still scores 78/78. What it can no longer do is *render* a glyph from the
committed copy, which was a real debugging aid (it is how the 完/璧 and
朴/林 misreads were caught). That capability stays available where the data
legitimately is: `work/glyphs-to-identify.tsv` keeps `shape_hex`, and
`glyphreview.py` reads that one. `jptext.load_shape_table` accepts either
spelling, so the two are interchangeable for lookup.

`glyphfill.py --publish` is the step that strips the bitmaps out. It exists so
the next transcription pass cannot put them back by accident, and it refuses to
write if two shapes ever hash alike. `.gitignore` blocks images under
`tools/integral-english/`.

**What is deliberately kept:** short quotations of game text in the
documentation - the sentence a glyph was read against, the subtitle that
exposed the `0x97xx` index bug. Those are the evidence; a finding nobody can
check is not a finding. They come to a few thousand characters against the 3.9
million the export holds, and the export is written to `work/`, outside the
repository, where it stays.

### "Is that all of it?" - the two other ways the inventory under-reports

Asking whether `disc1_RADIO_DAT.txt` was the whole export turned up the VR
disc above. Chasing the same question through the remaining containers turned
up two more limits, neither of which costs the export anything, and both worth
knowing before anyone trusts `japanese-inventory.tsv` for something new.

**`BRF.DAT` and `FACE.DAT` are scanned and correctly produce nothing.** They
are image data. 56 runs in `BRF.DAT` match the font-code range by coincidence
and `looks_like_prose` rejects every one.

**`--min 6` drops short strings in raw files.** `jplist.runs_in` keeps a run
only if `core_count(run) >= minimum`, and `core_count` counts kana and kanji
only - the long-vowel mark, `。`, `、` and full-width symbols are excluded,
because those are exactly what makes binary noise look like prose. The
threshold is not optional: in a raw file any two bytes in `0x8140`-`0x9AFF`
decode as a font code, so without it the scan drowns. But it is blunt, and
short genuine strings fall under it. `セーブ中です。` is seven characters on
screen and **five** by `core_count` (`セ ブ 中 で す` - the `ー` and the `。` do not
count), so a min-6 scan does not see it.

This is a *second*, independent way the inventory under-reports, on top of the
scanner bug in §19 that ended runs at unrecognised codes. Another reason the
export stopped sourcing `RADIO.DAT` from it.

**Where that led, and why it does not change the export.** Rescanning the
executables at `--min 3` finds four strings that a min-6 scan misses -
セーブが / セーブ中です。 / ロードが / ロード中です。 at `0x2718`, `0x27BB`,
`0x27FF`, `0x285B`. They are **not on the retail discs**: those offsets are
zero in retail bytes on both discs. They appear only in the *deployed*
executable, and the patch that owns `0x2600`-`0x2A00` is our own
`INTEGRAL_disc<n>_en_savemsg.ppf` (531 EXE bytes). So the export, which reads
retail bytes, is not missing them.

**What it does raise is a port question, and it is open.** `en_savemsg`
relocates the save-message table into that free space, and Japanese strings
are sitting in the relocated copy. Either those slots have no USA counterpart,
in which case leaving them Japanese is correct under rule 1, or they were
missed. Deciding it means reading the relocated table against USA's, not
guessing from these four - and the `en_savemsg` area already has an
UNDETERMINED note against it (README, the MC RAM-patch collision). Filed here
so it is not lost; it is not part of the export and does not block it.

## 21. A regression guard on the fragment map, and two metrics that failed

`radiomap.py` prints two aggregates - distinct bitmaps produced and their
blank-twelfth-row rate - and they catch a map that is grossly wrong. They
caught the one that was (§18): 91,834 distinct bitmaps at a 9.8% blank rate.

**They cannot catch one fragment's base going bad.** A base wrong by a whole
number of glyphs still slices on glyph boundaries, so every bitmap it reads is
a real glyph with a real blank twelfth row. Valid bitmaps, wrong characters,
both aggregates unmoved against 125 fragments, and the output is fluent-looking
nonsense. Nothing committed would have said a word.

`py radiotext.py --check` closes that, and `--selftest` proves it does.

### Two metrics that looked right and were worthless

Recording these because both are the obvious thing to reach for, and the
second one cost real time.

**"What share of this fragment's bank-1 codes hit a *named* shape?"** This is
the check I proposed, and it does not work now the table is complete. Every
shape in every blob is named, so slipping a base one glyph just reads a
*different named shape*: the score moves from 100.00% to **99.89%**. It was
diagnostic during the identification pass, when the table was half empty. It
is saturated now, and a saturated metric is worse than none because it reads
like a pass.

**"What share of its characters are in the corpus top 100?"** Real text
over-samples common characters heavily, so a permuted mapping should flatten.
It does flatten - but there is no margin. **Bank 1 holds the rare kanji**;
bank 0 has the common ones. So bank-1 frequency is inherently flat and
fragment-specific. Legitimate fragments run down to **34.1%** and a slipped
base sits at about **35%**. Chasing the four lowest-scoring fragments to see
whether they were broken found them decoding as clean, ordinary commentary.

The general lesson: before trusting a metric, **slip a base on purpose and see
what it reads.** Both of these looked convincing until they were made to fail.

### What works: cross-fragment agreement

The commentary is duplicated across fragments, and every copy carries its own
font blob with its own codes - so the same sentence is encoded differently in
each and must still decode identically. That is the invariant that exposed the
`0x97xx` index bug in §19, and it does not care how rare a fragment's
vocabulary is.

**Only lines that contain a bank-1 code are scored**, because those are the
only lines a wrong base can change. This matters more than it sounds:
fragment `0x03F6800` has 23 lines and **two** that use bank 1, so scoring all
of its text dilutes a slip to 8.7% and it walks through any sane floor. On
bank-1 lines alone the same fragment is correctly reported as *too thin to
check* rather than passed.

Measured on both discs:

| | |
|---|---|
| fragments checked | 126 |
| no bank-1 line - nothing a base can break | 40 |
| too thin (<4 bank-1 lines) | 5 |
| worst legitimate agreement | **96.0%** |
| median | 100.0% |
| one base slipped by a single glyph | **0.0%** |

The default floor of 50% sits in the middle of that gap. A slip large enough
also makes the record walk itself throw, which `check` reports rather than
raising.

### The selftest, and why it is not optional

    py radiotext.py --selftest

It slips three fragments' bases by +1, -1 and +8 glyphs and requires `--check`
to name each one. **The first two versions of the check passed their baseline
and failed this**, which is the only reason they were caught: a guard nobody
has watched fail is not a guard.

Its victims are drawn from exactly the set `--check` claims to cover, so it
cannot pass by testing something the check never promised. Fragments with no
bank-1 line are reported separately rather than counted as passes - one of
them holds nothing but four copies of a leftover English developer warning,
and counting it would inflate the number that means something.

**What this does not check** is whether a shape is named *correctly* - that is
the 78-answer holdout in `glyphfill.py --score`. This checks the base.

## 22. The 2026-09-10 pass: the Redump images, and patched discs that build

The question was whether the Redump dumps of Integral work with these patches,
and whether a patched image can be built from **either** them or the
collection's embedded copies. Both answers are yes, and both were measured
rather than argued.

### The two sources are the same disc

Streaming all three Redump `.bin` files against the same discs inside
`windata/dlc/dlc_japan.bin`, sector by sector:

| | disc 1 | disc 2 | VR |
|---|---|---|---|
| Redump size | 719,667,312 | 745,788,624 | 481,969,488 |
| collection's copy | identical length | identical length | identical length |
| sectors that differ | 312 | 312 | 307 |
| where | `SLPM_862.47` | `SLPM_862.48` | `SLPM_862.49` |
| anywhere else | **none** | **none** | **none** |

1.9 GB across three discs, and the **only** bytes that differ are the
executable extents the collection zero-fills — exactly what `rawdisc.py` says
and nothing more. (312 of 313 sectors, not 313: one sector of the executable is
genuinely all zeros, so the hollow copy happens to equal it.) The 1024 bytes at
`0x9320` that a PPF3 block check carries are identical on all three.

**So the Redump set closes the last of §5.4's three prerequisites** — the one
that could not be closed from inside, because it is a question about a file
this project does not ship.

### It also supplies the executables the build asks for

`BUILD-HISTORY.md` requires `int1.exe` and `int2.exe` as separately supplied inputs,
because extracting them from the collection yields zeros. Extracted from the
Redump images instead, both hash to
`4b8252b65953a02021486406cfcdca1c7670d1d1a8f3cf6e750ef6e360dc3a2f` — **the
exact hash the builder demands**. The VR disc's `SLPM_862.49` hashes to
`c370f8e41ec8fb78238bfe2ddbfc25a6d37ec8f0972c86ebfde075ecd4ee8dca`, which is
the hash `rebuild.py` already checks its **decomp-built** VR executable
against. So the decomp reproduces that executable byte for byte, and a real
retail dump says so independently. With the Redump set present the build has
no unsourced input left.

### `mkimage.py`

Everything else here emits PPFs and stops, because the collection applies them
itself. `mkimage.py` is the missing step. It takes a raw build's PPF folder and
writes a patched `MODE2/2352` image plus its `.cue`, from either source, and it
checks four things before it writes anything:

1. every PPF's block check equals the image's own bytes at `0x9320`;
2. no record reaches past the end of the image;
3. every touched sector verifies against **its own stored parity before
   patching** — a 280-byte sum over 2048 bytes, which is what actually proves
   the dump is the pressing the patches were computed against;
4. every touched sector verifies **again after patching**, against the parity
   the set's own `zz_ecc` PPF wrote.

Check 4 is why the set is all-or-nothing: a tail is computed from the final
payload of the whole set, so dropping one family leaves right data behind wrong
parity. `--allow-partial` skips it for bisecting and says so in the output.

**From the collection it requires `--exe` and refuses without it.** That is the
one asymmetry between the sources and it is not a formality: an image built
from the collection's copy without the retail executable put back is 641,024
bytes of zeros where the game's code belongs. Neither the collection nor this
repository can supply those bytes.

### Built, on the current patch set

`repro21raw` (32 PPFs — 12 per main disc, 8 VR, `en_pad2` included) applied to
all three Redump dumps:

| disc | PPFs | touched sectors | patch bytes | parity before | after |
|---|---:|---:|---:|---|---|
| 1 | 12 | 417 | 907,240 | all pass | all pass |
| 2 | 12 | 417 | 907,240 | all pass | all pass |
| VR | 8 | 2,003 | 4,058,372 | all pass | all pass |

Content-checked on disc 1 rather than trusting the counts: the executable now
holds `Cannot be used in`, `Anti-anxiety`, `Sniper rifle` and `HANDKER` (and no
longer `SCARF`), and `preope`, `brf`, `option` and `abst` point into DUMMY3M at
slots 0, 128, 384 and 462 — the documented four.

**And the two sources produce the same disc.** Building disc 1 from the Redump
dump and from the collection's copy with `int1.exe` supplied gives two files
with the same SHA-256,
`a51415b91c03465274795b5b010049918c1b630820823514947f491616ea85e1`
on `repro21raw` (and the same agreement on the older `repro14raw` set, at a
different hash). Which source you start from does not matter; only whether the
executable is accounted for. Getting there needed one length rule: the
container pads to a 2048-byte boundary between images, which is not a sector
boundary, so flooring the span to whole 2352-byte sectors is what reproduces
the Redump length exactly on all three discs.

### What testing the failure paths found

Both were found by running them, not by reading the code, which is the only
reason they are not still there:

- **A failed post-patch check used to leave the bad image on disk** - 719 MB
  that fails its own parity, under the name the good one would have had. Both
  parity checks now run in memory over the touched sectors alone (about a
  megabyte on a main disc, 4.7 MB on VR) and **nothing is written until both
  pass**; any failure mid-write unlinks the partial file.
- **A collection build pointed at a disc image is the realistic mistake**, and
  it is caught: its PPFs carry no block check (warned) and no `zz_ecc`, so all
  415 touched sectors fail the after-check and the run stops with the reason.
- **The block check discriminates all three discs** - disc 1, disc 2 and VR
  have different bytes at `0x9320`. Handing disc 2's image the disc-1 folder
  stops on the first PPF, before anything is read past the header.

### What this does and does not establish

It does **not** boot a disc. Every check here is static — parity, framing,
block checks, string content — and the raw variant's first run on real
hardware or a strict emulator is still the open item in §5.4. What it removes
is everything that stood *in front of* that test: there is now a command that
produces the image to test.

    py mkimage.py --redump "<disc 1>.bin" --ppfs <pkg>/mods/INTEGRAL/INTEGRAL/0 \
        --output "MGS Integral English (Disc 1).bin" --cue

### The method note, and it is §16's again

Two of §5.4's three blockers had been closed in code on the same evening the
paragraph naming them was written, and it sat there for three days reading like
open work. The section listing what remains is not evidence; `rebuild.py` is.
Before planning work off a paragraph in this file, read the code it describes.

## 23. The 2026-09-10 pass: the language default, offered rather than assumed

A raw disc has no ASI, so the one runtime behaviour a player would miss is
`[Game] EnglishText`. `mkimage.py` now asks whether to bake it in, and applies
it or not according to the answer.

### What the bit is, and what it is not

`GM_CONFIG_ENGLISH` (0x0100) in `GM_Configuration` (`linkvarbuf[2]`). Grepping
the decomp gives **exactly four** places that act on it:

| | |
|---|---|
| `radiomes.c:526` | picks the **English half** of a codec fragment - §18's mechanism |
| `radio.c:1320` | `NO RESPONSE` over its Japanese twin |
| `movie.c:54`, `jimctrl.c:390` | pick the movie / cutscene subtitle stream |

`opt.c` sets it and reflects it into the option row; `datasave.c` restores it
from the memory card. **Nothing else reads it.** The README used to list
`font_draw_string` as a fifth reader and that was wrong (`cfbc635`) - the
correction is load-bearing, because if font drawing tested the bit then the
menus this port wrote in place would depend on it. They do not: **every string
this port wrote is English whether the bit is set or clear.** What the bit
gates is *Integral's own* English.

So the honest description of the default is: English menus, Japanese story,
until the player visits Integral's OPTION screen - where the setting is
Integral's own and saves to the memory card.

### Where it is set, and why the patch fits in 72 bytes

`GCL_StartDaemon` runs once, from `Main()`, and its second call is
`GCL_InitVar` - which reads `GM_Configuration`, zeroes all of `linkvarbuf`,
and writes the value back. A store in **that call's delay slot** therefore
lands before `GCL_InitVar`'s body and is carried through it by the game's own
code. Once per boot, before anything reads it, and the option screen and
memory card still override it afterwards.

Three instructions have to be found room for in an 18-instruction function,
and they are paid for exactly:

| word | paid by |
|---|---|
| `sh $v1, off($v0)` | the `nop` in `jal GCL_InitVar`'s delay slot |
| `addiu $sp, $sp, 0x18` | the `nop` after `lw $ra` (a load-delay slot) |
| the third | turning the last call into a **tail call** - `GCL_ChangeSenerioCode` is a leaf ending in `jr $ra`, so jumping to it with `$ra` restored returns straight to `Main()` and the `jr $ra` word is freed |

18 instructions in, 18 out, same 72 bytes, same five calls in the same order.
Nothing relocates.

### There is no free space in that executable, and that was checked

The first plan was a stub in a zero run. Every zero run in the image turns out
to be live: the 4,203-byte one at `0x800AA095` holds `Hcount`, the 1,771-byte
tail holds `GM_StageName` - they are `.sdata`/`.sbss` inside the loaded image,
not padding. `__bss_obj` is at `0x800ABBB0`. And `GCL_ResetSystem`, the one
`/* do nothing */` function next door, is **called** from `0x8002AA68` - a scan
for every `jal`/`j`/pointer to it found one, against a control scan that found
two for `GCL_InitVar`. Fitting in place was not elegance; it was the only
option, and finding that out cost less than assuming it.

### Nothing is hardcoded

`langdefault.py` matches `GCL_StartDaemon` by its exact 18-word shape and
requires **exactly one** match; reads `linkvarbuf` out of `GCL_InitVar`'s own
`lui`/`addiu` pair, confirmed by the two `lh` at +2 and +4 that the C names
`GM_GameLevel` and `GM_Configuration`; and checks the tail-call target really
is a leaf before jumping to it. That is why it works unchanged on the VR
executable, where the function is at `0x8001FE10` and `linkvarbuf` is 0x23A0
lower - both derived, neither typed in:

    int1.exe   GCL_StartDaemon at 0x8001FCDC, GM_Configuration at 0x800B4D9C
    int2.exe   GCL_StartDaemon at 0x8001FCDC, GM_Configuration at 0x800B4D9C
    vrint.exe  GCL_StartDaemon at 0x8001FE10, GM_Configuration at 0x800B29FC

(`int1.exe` and `int2.exe` are byte-identical, so the two main discs take the
same patch at the same offset.)

### How it asks

`--english-default ask` is the default. On a terminal it explains what the bit
does - including that it does *not* affect this port's text - and takes y/n.
**Off a terminal it refuses** rather than choosing: a build script has to pass
`yes` or `no` explicitly. Declining is byte-exact: disc 1 built with
`--english-default no` reproduces `a51415b9…`, the same image as before this
feature existed.

### The one thing that needed care

The set's `zz_ecc` PPF computes each tail from the final payload of the whole
set - so adding 72 bytes to a sector invalidates the tail the set wrote for
it. `mkimage.py` recomputes the tail for the sectors the language patch
touches, and **only** those; everywhere else the set's own tail still governs,
and the post-patch parity check over every touched sector is what says so.
Measured: disc 1 goes from 417 touched sectors to 418, disc 2 the same, VR
2,003 to 2,004, and all of them verify.

### Tested

Seven tests in `selftest.py` (35 total now), over a synthetic executable so
they need no game data: same length, all five calls preserved in order, the
store lands on `GM_Configuration`, it follows `linkvarbuf` rather than
assuming it, the store is in the delay slot, and it refuses an unrecognised
function, a non-leaf tail-call target and two matches. Each was confirmed to
fail when the module is mutated - changing the bit value or turning the tail
call back into a `jal` both break it.

And the patch was read back out of a finished image: the disassembly of
`SLPM_862.47` inside `MGS Integral English (Disc 1).bin` shows
`addiu $v1, $zero, 0x100` / `sh $v1, 0x4d9c($v0)` / `j 0x8001fcb0` with
`--english-default yes`, and the retail `jr $ra` without it.

**Still not booted.** Same caveat as §22: this is static verification.

## 24. The 2026-09-10 pass: the raw disc booted, and the briefing is broken

The raw variant ran for the first time (§5.4). Everything checked so far is
right except the **briefing**, whose right column renders as vertical stripes
of sampled VRAM. This section is written while the fault is still open,
because the eliminations are worth more than the conclusion will be.

### FIXED, late 2026-09-10: one `nop`. Read this before the rest of §24

The fault was never the arithmetic. It was the **R3000 load delay**: the
instruction after a load still sees the register's *old* value, and the
port's eleven words read `a1` in the very slot after `lbu a1, 13(v0)`:

    lbu  a0, 29(v0)      v2
    lbu  a1, 13(v0)      v0
    subu v1, a0, a1      <- a1 is still the caller's poly index (9..24)

So `height` came out as `v2 - idx` - a hundred rows and more - and every
label was stretched down the column: vertical stripes of sampled VRAM.
Retail's eleven words never touch a register in the slot after loading it
(the compiler schedules for this), and neither does any other block the port
rewrote in this overlay. `hazards.py` now proves that on every build.

Every observation below falls out of it, with nothing left over:

| observation | why |
|---|---|
| the same bytes render cleanly on the Master Collection | its emulator does not model the load delay, so the arithmetic ran as written - the 26 shot pairs could never have shown it |
| SwanStation and hardware break | both model the delay |
| `height` alone: smears | `subu a0, a0, a1` right after `lbu a1`: height = v2 - idx |
| `above` alone: rows shifted | `andi a1, a1, 7` right after `lbu a1`: above = idx & 7, 0..7 by row |
| the POLY_FT4 code-byte guard made a diagonal fan | `lbu` then `andi` on the code byte - one more hazard |
| the stub running retail's own words: clean | retail's words have no hazard |
| the stub's restored X normalisation did not help | `lh a1, 8(v0)` then `sh a1, 0x18(v0)`: x2 took the row's top y |

The premise this section built on - that the routine "also draws rows that
are not textured labels" - is **false**. `b_select.c` decompiles
`brf_800C69B4`, and the register simulation over `brf_800C6E88` (its only
caller) lists sixteen call sites, every one with a poly index 9..24: the
sixteen `br_sNN` labels, all textured, all with valid UVs. The reverse
engineering §24 asked for was done and found nothing to fix.

The fix is in place, eleven words, no stub and no stage growth: `lbu a0`,
`lbu a1`, **`nop`**, then the same arithmetic and the four stores. The
overlay is retail's 127,702 bytes again. `brf_widen.py` `ROW_H_NEW` carries
the words and the explanation; `brf_build.py` asserts zero load-delay
hazards against retail before it writes the stage; `hazards.py` is the
scanner and `selftest.py` proves it catches exactly this pattern at exactly
this address. Scanned the same way the same evening: `en_items`,
`en_savemsg`, `vr_en_items`, `vr_en_savemsg`, the language default on both
executables, the VR MOVIE stub and the VR option call sites - **no other
hazard anywhere in the port**. (The scanner reports four "branch-slot" hits
inside the relocated string pools at `0x80011E00`-`0x80012200`; those are
text bytes, not code.)

**SEEN ON SCREEN 22:38 the same night.** Seven SwanStation shots of the
`repro32raw` disc 1 image (outline, member and detailed submenus, with
`infiltration method`, `person in charge of the operation` and `hostages`
highlighted, and EXIT) read against the 2026-09-02 MC set: label art at true
size, single- and two-line highlight boxes, the 16/26 and 20 row advances,
rules and horizontal connectors all match. The save on that disc has none of
the six flag-gated indented items earned (the user confirmed it), so the
L-connectors and the member block's 17-row branch (`FRAME_NEW`, `MEMBER_NEW`,
`DETAIL_NEW`) are still unseen on an accurate renderer; `hazards.py` clears
them and MC drew them right, but seeing them on the raw disc needs a save with
those briefings earned - `UnlockBriefing` is an ASI feature and does not
exist there.

**The lesson is new for this project.** The Master Collection is not only a
different renderer; it is a **lenient CPU**. Anything written by hand in
MIPS and verified only there can be wrong in exactly this way, which is why
the scan was run on every family and not only `brf`. And §16's rule applies
to the theory in the rest of this section: six formula guesses were made
against a routine whose arithmetic was right all along, and the one
sentence that would have ended it - "what does the instruction after the
load see?" - was never asked.

*The rest of §24 is the record of the bisect as it stood before the cause was
found. The eliminations were sound; the conclusions drawn from them ("both
halves are wrong", "reverse engineer the callers") were not.*

### What is established

| | |
|---|---|
| unpatched Integral and USA, SwanStation | **clean** |
| patched, `en_brf` removed (parity regenerated) | **clean**, Japanese |
| patched, `en_brf` present | **broken** |
| the same `en_brf` on the Master Collection | **clean** |
| `en_brf`, raw build vs collection build | **byte-identical**, 276,482 bytes, 0 differing offsets |

So `en_brf` is the fault; it is not raw-specific; and the collection build
carries the same bug and always has. The emulator is not at fault - the
settings are the accurate end of the scale (`GPU_Renderer = Software`,
`ResolutionScale 1`, no PGXP, no filtering, no widescreen hack) and retail
discs render correctly under them.

### The verification that could not have caught it

§4 records `en_brf` as verified by "26 shot pairs, 0.00% right-column diff".
That compared **Integral-on-MC against USA-on-MC**. Both sides were drawn by
the same emulator, so anything MC does differently from a PlayStation cancels
out of the difference exactly and is invisible to the method. The check
establishes that the port reproduces USA's layout *as MC executes it*; it
cannot distinguish that from reproducing it as hardware executes it.

That figure was quoted twice on 2026-09-10 as though it settled the question.
It does not, and the general form is worth keeping: **a differential test
against a reference rendered by the same suspect component proves only
agreement, never correctness.** §16's lesson again - say what the claim
ranges over.

### Three hypotheses tested and rejected

1. **The language default (§23).** Rejected: the briefing overlay reads
   `GM_Configuration` zero times, and an image built without the language
   patch is equally broken.
2. **`ufits` is wrong for 8bpp** (a page is 128 texels wide at 8bpp, not 256,
   and the guard uses a flat 255). Real bug in principle, not this one:
   every one of the 20 widened labels is 4bpp and every one fits, the
   tightest at 248 of 256.
3. **Move the relocated labels to VRAM rows 256..511**, which the whole stage
   leaves empty. **Strictly worse**: the relocated labels themselves render
   as garbage while the three that stay in place are fine. This code path
   cannot address the lower half of VRAM - the tpage field selects one
   256-row half and the briefing assumes the top one. `vfits` permits
   `py >= 256` and that permission is wrong here. Reverted; the comment in
   `brf_widen.py` now says so.

### What the current placement does, and what it does not explain

`busy()` models the `nd` payload's 51 textures and nothing else. USA's labels
are wider than Integral's - `br_s00` goes 13 units to 25 - so 17 of 20 cannot
stay put, and the search relocates them to **x 896..958, y 1..156**, a band
Integral's stage uses only at y 226-228 (CLUTs, which are avoided).

But the relocated labels *draw correctly* there. It is the right column that
does not. So this is not simply "the labels landed on something": their bytes
are intact and correctly addressed. Something the port changed is making the
**column** sample wrongly.

A scan for the runtime uploads that `brf_800CAC7C()` performs found nothing
in the stage - it pages content in from `BRF.DAT` by sector, so the data is
outside every payload this toolchain parses.

### RESOLVED TO ONE ROUTINE - read this part first

The bisect finished. **`ROW_H` is the fault**: 11 instructions at `0x800C69C8`,
inside `set_row_box(work, i, y, advance)` at `0x800C69B4`. Everything else in
`en_brf` - the texture swap, the VRAM placement, the quad widths, the xl
moves, the five block rewrites - is innocent.

| build (all disc 1, SwanStation software renderer) | briefing |
|---|---|
| retail, unpatched | clean |
| `en_brf` removed entirely | clean, Japanese |
| textures + VRAM placement only | clean English, badly squashed |
| + quad/xl/rule/anim/start-y/connectors (`INTEGRAL_BRF_NO_COUNTS=1`) | **clean English, correct sizes, row spacing wrong** |
| + counts, - the five block rewrites | striped |
| `ROW_H` alone | striped |
| full set | striped |

**`TEST - Disc 1 no counts.bin` is a good disc.** Correct English everywhere;
the only visible flaw is that `next-generation / special force unit` sits
cramped, because `ROW_H` - the thing that fixes row spacing - is off in it.
That build is `D:/mgsbuild/repro25nocounts`.

### What ROW_H does, and why both halves of it are wrong

Retail's eleven words set the row's box to `[y, y+13]` and normalise the
quad's X corners (left pair take x0, right pair take x3). The port replaced
them with:

    height = v2 - v0        (bytes at poly+0x1D and poly+0x0D)
    above  = v0 & 7
    box    = [y - above, y - above + height]

and, needing six words for that arithmetic in an eleven-word hole, **deleted
the four X-normalisation stores**.

Tested on screen, each term alone, through the stub described below:

| box | on screen |
|---|---|
| `[y, y+13]` - retail, via the stub | **clean** |
| `[y - above, +13]` - only the shift | bad: rows shifted and overlapping |
| `[y, y + (v2-v0)]` - only the height | **really bad**: smears |
| both | bad |

Two different failures with one root cause: **the routine asks the polygon
for per-label information the polygon does not reliably carry.**

* `above` reads `v0 & 7`, but only the sixteen `br_s*` labels ever had their
  VRAM row chosen to encode anything (`row_ok`). Every other row this routine
  draws - and it draws plain decoration too - gets a meaningless 0-7 shift.
  A 7px shift cannot smear, which is why this failure is layout, not garbage.
* `height` reads `v2 - v0`, which is the texture height only when the poly is
  a textured label. On anything else it is two arbitrary bytes.

Guarding on the GPU code byte (`poly+7 & 4`, set for POLY_FT4) so only
textured quads take the UV path was tried and **did not fix it** - it changed
the failure from vertical smears to a diagonal fan.

### The delivery mechanism works, and is proven

There is no room at `0x800C69C8` and no free space inside the overlay - not
one 32-byte zero run in 127,702 bytes. There is room **after** it: every stage
overlay loads at `0x800C3208`, and the game itself loads `init_ve`
(169,568 bytes) there against brf's 127,702, so ~41 KB past brf's end is
scratch. So `ROW_H` becomes `j <stub>` + `nop`, and the stub is appended to the
overlay, ending `jr $ra` / `addu $v0,$a2,$a3` (the original return value).

**This was verified with a control**: a stub containing retail's exact eleven
instructions rendered exactly like retail. The jump, the appended memory, the
return - all sound. `brf_build.py` grows the overlay, `nsect` is recomputed
from the payload sizes, and the stage goes from 138 to 139 sectors, which
DUMMY3M slot 128 absorbs (next stage at 384).

So whatever the right formula turns out to be, there is unlimited room to
write it. That constraint is gone.

### What to do next, and why it is not another formula

The information the routine needs - this row's height and shift - is
per-label, and the **caller** knows which label it is drawing. The port
smuggled it through the texture's VRAM position only because it had no spare
instructions. That constraint no longer applies.

The next step is reverse engineering rather than iteration: disassemble every
`jal 0x800C69B4` site in `brf_800C62B0` and `brf_800C6E88`, work out what each
one draws and what poly index it uses, and pass the height in properly. A
static table indexed by poly slot will **not** work - the same slot draws
different labels on different pages.

Six formula guesses were tried on screen before this was accepted. Do not try
a seventh.

### The diagnostic switches, all in `brf_build.py`

    INTEGRAL_BRF_NO_CODE=1        textures + placement only, no geometry
    INTEGRAL_BRF_NO_REWRITES=1    skip the five block rewrites
    INTEGRAL_BRF_NO_COUNTS=1      skip all six count/index groups
    INTEGRAL_BRF_SKIP=a,b,c       skip named groups: rowh s00 s00x
                                  unshare memadv advances

(`INTEGRAL_BRF_ROWH_MODE` and `INTEGRAL_BRF_ROWH_PASSTHROUGH` went with the
stub when the cause was found; the row box is eleven words in place again.)

They are diagnostics, not features. `INTEGRAL_BRF_NO_CODE` also relaxes the
quad==texture assertion, because with the geometry discarded they legitimately
disagree.

### Four groups were never tested alone (moot since the fix)

`INTEGRAL_BRF_NO_COUNTS` turns off six groups. Only `rowh` was isolated and
shown guilty. **`s00`, `s00x`, `unshare`, `memadv` and `advances` may be
perfectly fine** - they were switched off as collateral and never individually
retested. Before shipping anything with `rowh` disabled, turn those four back
on and check, or the disc is missing layout work it did not need to lose.

*With the cause found (top of §24) nothing is disabled: `hazards.py` finds no
hazard in any of them, and `repro32raw` ships them all. The one check still
owed is the full set on screen.*

### A measurement that lied, worth keeping

Scoring the screenshots programmatically - lit pixels and tall runs in the
right column - reported the `above` build as **cleaner than the control**. It
was not; the user's eyes said bad. Corrupted rows overlapping each other light
*fewer* pixels than correct text does, so the metric was anti-correlated with
correctness in exactly the case it was built to judge. §21 already records two
metrics that failed this way. Three now.

### The old plan, kept for its reasoning

`en_brf` does two separable things, and only one of them was tuned by eye:

| half | how it was derived |
|---|---|
| texture swap + VRAM placement | fit constraints - `ufits`, `vfits`, `row_ok`, `busy` |
| quad immediates + row arithmetic (`FRAME`, `MEMBER`, `S01`, `DETAIL`, `ROW_H`, xl, connectors) | **matched against Master Collection screenshots** |

`INTEGRAL_BRF_NO_CODE=1` builds the first half alone: every assert still
runs, the geometry changes are discarded, and the labels draw stretched to
Integral's original quads. Clean-but-stretched implicates the tuned half;
still-broken implicates the placement.

The suspicion is the tuned half, and it came from the user: the geometry was
reverse-engineered to make Integral's output *look like* USA's on MC, so a
polygon whose UVs are wrong in a way MC tolerates would have been accepted as
correct. `FRAME_NEW` rewrites "frame polys 27-38" - and a polygon with wrong
UVs samples VRAM as vertical stripes, which is the symptom.

### Two things this does not change

The collection build is not at risk of regressing: M2 shipped a final patch
months ago and will not change the renderer underneath it. And the fix, when
it comes, belongs in `en_brf` and not in MGSM2Fix - the two builds share this
patch byte for byte, and fixing it in the ASI would repair one environment,
leave every other one broken, and split a file that is currently identical.

## 25. The 2026-09-10 working state, for whoever opens this next

Written at the end of the session that booted the raw disc, and updated late
the same night when the briefing was fixed. **Read the top of §24 first** - the
cause and the fix are there. This is only where things are.

### Discs and images

| what | where |
|---|---|
| Redump dumps (zipped, MODE2/2352) | `C:\Users\Tideg\Desktop\MGS1 Integral` |
| extracted | `D:\mgsbuild\redump` |
| built images | `D:\mgsbuild\patched` |
| RetroArch screenshots | `C:\Users\Tideg\My Drive\RetroArch\Screenshots` |

The three images in `patched\` (`MGS Integral English (Disc 1/2/3)`) were
rebuilt at 23:24 from **`repro33raw`** (`repro32raw`'s briefing fix plus the
connector left ends of §26; the 22:31 images were removed first) - and disc 3
again at 11:37 on 2026-09-11 from **`repro34raw`**, which adds
`vr_en_memcard` (9 VR PPFs, 2,009 sectors verified); all three passed `mkimage.py`'s before-and-after
parity over every touched sector (418 / 418 / 2,004), and the fixed eleven
words were read back out of both main-disc images, once each, at the
relocated `brf` stage, with the old sequence absent. `TEST - Disc 1 no
counts.bin` beside them is the 17:05 diagnostic build (`repro25nocounts`) and
can go. **The briefing on disc 1 was looked at 22:38**: seven shots, every
reachable state matching the MC set (§24, top). Unseen: the flag-gated
indented items, which that disc's save has not earned.

### The builds that matter

| directory | what it is |
|---|---|
| **`repro33raw`** / **`repro33`** | `repro32raw` plus the connectors' left ends (§26). The raw one is what the images in `patched\` are built from; the collection one's two `en_brf` PPFs are **deployed** in `mods\` since 23:24 (the pair they replaced is `workrf_deployed_before_connector_disc{1,2}.ppf`) |
| `repro32raw` | the `ROW_H` fix alone: the load-delay `nop`, in place, every group on; 32 PPFs, ZIP `a2ebded6…` |
| `repro21raw` | the full raw set before any of this; what the first (broken) images came from |
| `repro25nocounts` | the bisect's clean build - `INTEGRAL_BRF_NO_COUNTS=1`, briefing clean, row spacing wrong; superseded |
| `repro29control` | the stub running retail's own row-box code; proved the append-and-jump mechanism, which is no longer used |
| `repro30`, `repro31above`, `repro31height` | the three failed fix attempts - each carried the hazard (§24, top) |
| `repro20` | the last **collection** build (not raw). The collection build shares `en_brf` byte for byte, so it carries the same bug until it is rebuilt and redeployed; MC hides it, but the fix should go there too |

Every `mkimage.py` run used `--english-default yes`.

### The emulator, and why its settings matter

SwanStation in RetroArch, and the settings are the accurate end of the scale -
`GPU_Renderer = Software`, `ResolutionScale 1`, `TextureFilter Nearest`, no
PGXP, no TrueColor, no dithering, no widescreen hack, texture replacements
off. Core options live at
`C:\Users\Tideg\My Drive\RetroArch\Core Config\retroarch-core-options.cfg`,
not in the RetroArch folder. **Retail Integral and retail USA both render the
briefing correctly under these settings**, which is what proves the port is at
fault rather than the emulator.

### What the Master Collection has to do with it

Nothing, in the end. MC renders the same bytes cleanly, so the bug hid there
for as long as the family has existed - but MC is not inaccurate, it just left
different garbage in the primitive buffer. M2 shipped a final patch months ago
and will not change, so the collection build is not at risk of regressing; the
reason to fix `ROW_H` is that it is wrong, and that the raw disc shows it.

The fix belongs in `en_brf`, **not** in MGSM2Fix. The two builds share that
patch byte for byte (276,482 bytes, zero differing offsets), and repairing it
in the ASI would fix one environment, leave every other one broken, and split
a file that is currently identical.

### Where the session's own mistakes are recorded

§24 has them, and they are worth reading before repeating them: six formula
guesses, a fix tested in a full build where other disabled groups could mask
it, a mechanism trusted because its *bytes* disassembled correctly rather than
because it had been shown to *execute*, and a screenshot metric that scored a
broken build cleaner than the control. The two results that actually moved
this forward both came from controls - removing `en_brf` entirely, and running
retail's own code through the new stub.

One more, from the user rather than the assistant, and it is the reason the
bug was found at all: the 26-shot-pair verification that signed `en_brf` off
compared Integral-on-MC against USA-on-MC. Both sides were drawn by the same
renderer, so its behaviour cancels out of the difference. That check proves
agreement, never correctness.

## 26. The 2026-09-10 late pass: the connectors' left ends, and the collection's dim line

Two things the user saw once the briefing rendered on an accurate emulator.

### The horizontal connectors started in the wrong place - a port bug, fixed

The line from the selected FILE button to the submenu's rule begins at a
hardcoded x per submenu. Integral: -46 / -24 / -46; USA: -39 / -32 / -27. The
port kept Integral's, on the recorded belief that they "anchor to the FILE
column" - but the FILE column's boxes are USA's now, so the outline and
detailed lines ran 7 px into their box and the member line stopped 8 px short
of its. Retail USA on SwanStation starts the line exactly at the box edge; so
did retail Integral against its own, wider box. The 26 shot pairs that signed
`en_brf` off compared game x 150-320 and never looked at x 114-136.

All six writers (layout and reveal animation, three submenus) now take USA's
value; the detailed one needed its own register because its layout store
shared `s7` with the outline's. `hazards.py` clean. README, "The connectors'
left ends". Built as `repro33raw` / `repro33` (see §25 for what was written
where). **Seen on the collection 23:26** in all three submenus: the line now
begins at the box edge, and the member gap is gone. **Seen on SwanStation
from the `repro33raw` disc 1 image at 23:45** as well: both targets confirmed.

### The dim connector on the Master Collection is the collection's, not ours

On MC the same connector draws faint (USA and Integral alike, patched or not);
on SwanStation and hardware it is full brightness. Its texture `br_line1` is
4x2 with a bright row (grey 135) over a dark one (grey 23) and the quad is one
pixel tall with V spanning both rows, so the renderer's texture-coordinate
rounding picks the row: hardware takes the bright one, M2's renderer the dark
one (measured: MC's line sits +25 over the background, the dark texel's
value). Not dithering - that cannot move a texel from 135 to 23 - and not the
USA `BrightnessText` patch, which never touches this stage. **Not the
upscaler either**: the user's 23:25 shots were taken at Resolution
"Original", Smoothing off, Pixel Perfect, Screen Filter off, and the line is
just as dim, so it is M2's base renderer. MGSM2Fix's own renderer options
(internal-resolution override, widescreen) are off in the live ini. README, "The
horizontal connector is dim on the collection". Fixable in MC by pinning the
three connectors' UVs to the bright texel (code, needs room; a USA stub would
ride the built-in disc-patch mechanism) - offered, not done: it is cosmetic,
MC-only, and outside the port's scope until the user says otherwise.

**Is it only the briefing?** Surveyed 23:42: every `*line*` texture on disc 1
drawn as a stretched quad. The two-row line texture is a house style - the
option screen's `line` (4x2, 135 over 23) and the title's `sp_line` use it
as **2-px** quads, which show bright row plus shadow on every renderer;
`b_line1`, `cam_line1..3` and `sub_sline` have uniform rows and cannot
disagree. The briefing is the only place that squeezes a two-row texture
into a **1-px** quad (`br_line1` connectors, `br_line2` bars), so within the
main disc it is one menu's misuse of a shared asset.

**The user's objection, and it is right:** pinning UVs in the disc bytes is a
band-aid that would only ever run in the collection, because the raw disc is
already correct - and anything that runs only in the collection can live in
MGSM2Fix, which already hooks M2's emulated GPU (smoothing, internal
resolution). The fix at the source is an ASI rule on the GPU's primitive
path: a textured quad one pixel tall whose V spans more than one texel gets
its V collapsed to the first row, which is what hardware draws. One rule,
both titles, no disc patch, upstream-worthy. Cost unknown: it needs the point
in M2's GPU code where primitives are consumed, which MGSM2Fix has not
mapped. Next step if wanted: a scoping pass to find that hook point before
writing anything.

**Done, 2026-09-10 late: `[Patches] ThinTexturedQuads` in MGSM2Fix.** The
scoping pass took the emulator apart from the log's `gpu` system-module
record: constructor `0x140102B30` (registers `dev/gpu`, `gpu:vram`), the GPU
struct's FIFO at `+0x2C` and depth at `+0x6C` (both in the header already),
the FIFO push at `0x1401064D0` whose tail dispatches on the command byte -
polygons via an eight-entry word-count table `[4,7,5,9,6,9,8,12]` at
`0x1407689E8`, indexed by the textured/quad/gouraud bits - to the polygon
handler `0x140103EE0`, which has exactly one caller: `mov rcx, rbx; call` at
`0x140106AF9`, with `rdx` still pointing at the FIFO words. That is the hook.
`PSX::GPU_PolygonCommand` (psx.cpp) rewrites the words in place before M2
reads them: textured polygon, Y extent exactly 1, top-edge vertices agreeing
on V -> every vertex takes that V; the same for U with X extent 1; nothing
else touched, so two-pixel lines keep their shadow row. Signature
`48 C1 E8 02 4C 8D 15 ?? ?? ?? ?? 83 E0 07 42 0F B6 84 10 ?? ?? ?? ?? 44 3B
C0 7C 0F 48 8B CB E8` +0x1C, one hit in the MGS1 executable. Default on;
`iEmulatorLevel >= 2` logs each snap. UPSTREAM.md has the entry. The
scanning scripts are in the session scratchpad only (`gpu_scan1..5.py`);
the method is what matters: log address -> module record -> constructor ->
displacement scan over `.pdata` function ranges -> single caller.

**Deployed 2026-09-11 00:02** as `MGSM2Fix64.asi` (SHA-256 `9ea87429…`), the
previous ASI kept beside it as `MGSM2Fix64.asi.bak-before-thinquads`. **Seen
on screen 00:07**: the log has `[PSX] GPU_PolygonCommand hook succeeded`, the
outline connector on the collection measures +205 over the background (it was
+0 the night before, and +25 on the detailed one), it starts at the box edge,
and nothing else in the shot changed. The collection's briefing now matches
SwanStation's.

**00:13-00:17, every briefing unlocked (`UnlockBriefing`, achievements off),
twenty Integral shots against twenty-one USA shots on the collection.** They
pair one-to-one in order (one spare USA shot is a repeated state) and the
submenu column differs **0.00%** in every pair at a 40/255 threshold; the
FILE column's 0.1% is JPEG noise on the box borders. Seen for the first time
with the fix: all six flag-gated items - `time limit`, `support crew`,
`Meryl`, `genetic strengthening`, `the reason for unanimous approval`,
`Liquid Snake` - each with a complete L-connector, drop **and** foot, where
the 09-02 shots had the drop alone; the member submenu on its five-item
17-row branch; the detailed submenu with nine items. So `ThinTexturedQuads`
covers the bars too, as predicted. Caveat, the §24 one: both sides of these
pairs are the collection with the same ASI, so they prove Integral equals
USA there, not hardware; hardware truth for the connectors is the
SwanStation measurement, and the flag-gated items are still unseen on
SwanStation because the raw disc has no unlock aid.

### 11:11, the VR disc with the three unlock aids: 22 shots, and a family nobody had seen

Seen and right: all four EXTRA help lines (`View the movie.`, `Take a
picture.`, `See the album.`, `Return to the title screen.`; PocketStation's
Japanese is the open §6 item), the RESULT window in every highlight state
with its 1ST/2ND/3RD and RECORD, `SAVE REPLAY DATA` with `NEW FILE [NEED 1
BLOCK]`, LOAD DATA and SAVE DATA with `LOADING...` / `SAVING...` /
`COMPLETE`, the by-rule Japanese at indices 1 and 9, and the `OVERWRITE OK?`
caption that is Integral-only. That closes §5.5's list except the moved
MOVIE EXIT box, which was not in the set.

**And one thing wrong: CLEAR DATA's `NO FILE` caption is Japanese.** Chasing
it found that the memory-card module is compiled into three VR overlays as
well as the executable, each with its own caption tables, and only the
executable's had been ported. `vr_en_memcard` (new, `vr_memcard.py`) ports
`vrsave` and `selectvr`, where USA's copies are English; `vrtitle` - the
CLEAR DATA screen itself - is Japanese in USA's own table and stays. README,
"The memory-card modules". Built, deployed to `mods\INTEGRAL\VR-DISK\`
(31 files clean), in `repro34raw` (33 PPFs, 24 main + 9 VR) and the disc 3
image rewritten from it at 11:37.

**How it was missed, and what else could be.** Asked directly, so answered
directly. The sweeps read GCL records; overlay pools are not records. The
byte inventory saw the strings but judged each *string*, and this one also
lives in `vrtitle` where USA has the same Japanese, so all three copies
inherited that verdict. The general form of the hole is "text USA has in
*this stage* that Integral's copy lacks", and `overlaydiff.py` now asks
exactly that, per stage, both discs, net of deployed PPFs. Its full run on
2026-09-11: disc 1 clean (22 candidates, all debug strings); VR disc clean
apart from this family (881 candidates, all USA's debug symbol tables and
printf strings, plus the other four languages in the three memory-card
stages); both executables clean (a boot string, debug prints, and two item
names that ARE ported). One residue: USA `selectvr`'s `SAVE?` menu record
where Integral's is empty - probably an unused window, to be looked at, not
read. Other blind spots that remain by nature: texture lettering (art, not
text - the EXORCISE textures are the known case), English stored in font
codes rather than ASCII in an overlay (none known; GCL text is swept
separately), and the Master Collection's leniency as a CPU and a GPU, which
§24 and §26 record and which only an accurate emulator can catch.

## 27. The 2026-09-11 cross-check against the MGS1 Translation Toolkit

The user pointed at <https://github.com/DoktorDeSparkle/mgs1-translation-toolkit>
(a PySide6 front end) and its library
<https://github.com/drsparklegasm/mgs1-scripts>, asking whether it reveals
anything we did not know - with the rule that nothing is taken from it and
that a useful reference is credited. Both are GPL v3. Nothing was copied;
CREDITS.md has the entry.

**How much weight it carries.** Not much on its own, and the user said so:
its README states that most of the GUI is "vibe coded by Claude", and its
glyph table is OCR (credited to Green_goblin) with "~30 kanji yet to
identify, numerous others are wrong" in its own words. So a disagreement
with it is a prompt to look again, never a verdict. Every change below was
decided by re-reading our tile and re-reading our sentence, and would have
been made the same way had the prompt come from anywhere else.

**What it is.** Tooling for an *undub*: English subtitles into the Japanese
release's RADIO.DAT, DEMO.DAT, VOX.DAT and ZMOVIE.STR, with a font editor for
the 440-slot kana/kanji font and a `.tbl` encoder. Different scope from this
port, which never touches subtitles. Its README lists its own open issues:
Integral's RADIO.DAT does not recompile (sector-aligned calls, extra
graphics padding), "~30 kanji yet to identify, numerous others are wrong".

**What it revealed: all six of our disputed readings were wrong.** Their
`graphicsData` maps 6,877 codec-glyph bitmaps (Japanese disc 1) to
characters. Hashing their bitmaps with `jptext.shape_key` and looking them
up in `bank1-glyphs.tsv`: **all 1,214 of our shapes occur in their table,
1,208 agree, 6 differ.** The six were adjudicated by rendering the tile
beside 12-px reference glyphs of both candidates (three system fonts, ±1 px
alignment, best pixel agreement) and by re-reading the export's sentences:

| id | uses | was | now | pixels | why |
|---|---:|---|---|---|---|
| g518 | 80 | 京 | 涼 | 62.5 / 70.1 | three dots down the left edge, the water radical 氵, which 京 has no room for. The one context is a staff credit, `モーション 吉村京子`, and 吉村涼子 is as good a name, so the sentence cannot decide and the pixels do |
| g1030 | 8 | 綺 | 華 | 54.9 / 71.5 | horizontal bars the full width with one central vertical, the shape of 華; 綺 would have a thread radical 糸 down the left, and there is none. `確かに綺麗すぎです` ("too clean") and 確かに華麗すぎです ("too showy") both read as self-criticism, so sense does not rule it out |
| g1156 | 2 | 瀕 | 餓 | 59.0 / 66.0 | the left component is boxed like 食, not the three dots of 氵. The two uses are in text the export does not cover, so pixels alone decided |
| g210 | 624 | 綿 | 緻 | 51.0 / 54.6 | **right half only** - 綿 and 緻 share 糸, so a whole-tile score is mostly agreement about the half not in dispute. On the right half the tile is dense with diagonals, which is 攵 in 致; 帛 would leave a clean white box interior and there is none. 緻密な配慮 and 綿密な配慮 are equally real, so again the pixels decide |
| g1045 | 6 | 輌 | 輛 | 53.6 / 54.1 | **a tie on pixels, adopted for want of anything against it.** At 12x12 両 and 兩 differ only by an inner stroke and no reading is measurably better; 0.5 points is noise. Nothing supported 輌 either, and the two are one word, so nothing turns on it |
| g658 | 44 | 〝 | ” | 12.5 / 66.7 | **the clearest of the six, and the one called a "variant" without looking.** The tile's marks are thick at the top and step down to the left, which is ”; 〝 leans the other way. Decisive corroboration: the table holds **one** quote glyph and the text uses it at *both* ends - `彼の髪形は〝タコ〝`, `〝隠れる事〝` - and a 〝…〟 pair needs two. One symmetric mark used for open and close is ”, not 〝 |

**Do the sentences still make sense?** Asked twice, and checked both times.
吉村涼子 is a name where 吉村京子 was one. 華麗すぎ is a real word where 綺麗すぎ
was. 緻密な配慮が必要 means what 綿密な配慮が必要 meant. 輛 is the same word as
輌. `彼の髪形は"タコ"ということで` and `ゲームの基本ルールである"隠れる事"を` read
*better* than before, because 〝…〝 was never valid typography and "…" is.
餓 has no sentence to fit. Sense never objected to any of the six, which is
exactly the point: the sentence test cannot see a misread that lands on a
name, a near-synonym, a variant form or a quotation mark.

`bank1-glyphs.tsv` is corrected for all six (the table loads and
`selftest.py` passes). The export in `work/jpdump/` was written with the old
readings and is not regenerated - it is a reading aid outside the repository,
and the three characters occur in 90 of its 68,242 lines; `py radiotext.py
--dump` rewrites it whenever it is next wanted. §19's claim stands as
written - zero *unresolved* codes - but its 78/78 holdout and "read against a
full sentence" could not see these, because a name, a near-synonym, a variant
form and a quotation mark all pass a sentence test either way.

**The second lesson is about the first pass at this section, not the table.**
It first adopted three of the six and kept three, calling 綿/緻 "undecided"
and 輌/輛 and 〝/” "variants" - which sounds like judgment and was closer to
defending the existing entry. The user looked at the picture and said all
three of those were the toolkit's to win. Re-tested properly they were: the
whole-tile score for a compound character is dominated by the radical both
candidates share, so **the test has to exclude the shared component**, and on
the right half alone 緻 wins; and the "variant" 〝/” was never scored at all,
which is how a 12.5-versus-66.7 miss stayed in the table wearing the word
"variant". A disagreement is not settled by being renamed. Score the half
that differs, score every row, and keep the count honest: **six of six.**

**What it corroborates, from its docs, without changing anything here.**
The font block layout (12-byte header, 96-entry variable-width ASCII table,
12-px 2bpp glyphs, 36-byte kana/kanji tiles) matches what `widths.py` and
`jptext.py` model. The 0x80-prefixed style-flag bytes (`0x80 0x22`, `0x80
0x2D`) they note as USA/Integral-specific are the ones `game_text` strips,
as `font.c` does. Their codec subtitle limit is **260 px and 4 lines per
block**; this port's 240-px figure is the *menu* renderer's `u8 max_width`
path and a different limit, so neither corrects the other. Their DEMO.DAT
parser names chunk type `0x04` "a second language chunk" - the dual-language
mechanism behind Integral's (En,Ja) cutscenes, which §23's language bit
selects. Integral's RADIO.DAT calls are 0x800-aligned with graphics padding,
which is the fragment geometry `radiomap.py` walks.

**Nothing to take.** Their tables are their transcription work under GPL v3
and this port's are its own; the digests met in the middle and that is all.

## 28. The 2026-09-11 late pass: the VR number-substitution re-check, and the grenade DELAY texture

**Technical handoff corrected by §29 below.** The alleged texture in `vab_grn`
was actually `scenerio.gcx`. The texture is now located, ported and deployed;
the unfinished investigation below is historical, not a confirmed asset map.

**Part one: the three "number substitution" claims from §6/README were checked against
real gameplay, not just stage data, and one of three survived.** The user captured
matching screenshots of all three flagged VR missions on both discs (SwanStation).
SNEAKING MODE / NO WEAPON LEVEL 10 and SNEAKING MODE / SOCOM LEVEL 03 showed **no
match** to the documented figures — the live, reachable window for SOCOM LEVEL 03
reads "Enemies 3" on both discs (nothing like the claimed 40/43), and NO WEAPON
LEVEL 10's live window carries no number at all. Re-running `vr_windows.py`'s own
comparison (ground truth, built from real stage data on both discs) confirmed this
is not a screenshot fluke: SOCOM LEVEL 03's key genuinely holds three numbers in
the pool data (40,15,3 vs 43,15,3), of which only one live window's worth
("Enemies 3") is what a real playthrough reaches. The best-supported explanation:
Integral's stages carry every mission a stage family can host as templates, and the
2026-09-07 measurement read a non-live template copy under the same key rather
than the one an actual playthrough shows. Not fully proven, but now backed by an
actual data-level contradiction rather than a guess. WEAPON MODE / GRENADE LEVEL 02
is the one that held up on screen and in the pool data alike: Targets 3 unchanged,
the substituted number sitting inside the sentence (Grenades explode in 5 seconds,
Integral, versus 4 seconds, USA) — which is itself how the fuse-timer investigation
below started. REFERENCE.md ("Numbers that differ between the two versions") and
HANDOFF-ARCHIVE.md §6 both carry this finding now.

**Part two: is GRENADE LEVEL 02's "5 vs 4 seconds" claim even true of the game's own
behaviour, or just its text?** Traced the fuse-timer mechanism through the decomp
(bullet/tenage.c's generic per-instance fuse_time field; the only concrete
literal found, enemy/grnad_e.c's hardcoded 120, turned out to be an enemy's
throw-cooldown, unrelated) and through the vab_grn mission script's full command
tree (573 Integral / 571 USA commands dumped and compared — no second, non-text
numeric parameter anywhere near the briefing window). Neither approach found a
script-level constant. **The user then measured it directly: a real pulled-pin
grenade explodes in ~4 seconds on both discs.** Text says 5 vs 4; the actual game
does not differ at all.

**That reframed the whole finding, and then the user found the corroborating
asset.** If the real mechanic is identical and only Integral's text claims
otherwise, that is not a regional gameplay difference to preserve — it is an
uncorrected error in Integral's own original release, predating this port
entirely. The user then pointed at the rotating 3D grenade model shown in the VR
Missions WEAPON MODE weapon-select menu (WEAPON MODE > GRENADE > LEVEL 02): its
surface texture prints "DELAY 5.2" on Integral and "DELAY 4.0" on USA —
confirmed on screen, both discs, screenshots taken 2026-09-11 16:32-16:33. Two
independent Integral assets (a briefing sentence and a model decal) separately
misstate the same real, unchanged mechanic; USA's originals are correct on both.

**Decision, 2026-09-11: replace Integral's texture with USA's.** This is explicitly
not an amendment to the port's translation rule (§2) — there is no English to
port here, no Japanese to translate, and Integral's number is already in numerals,
not text. It is a correction of an original Konami authoring error, using the
user's own measured gameplay as ground truth. **Packaging, decided the same day:**
this does not belong in the en_* family (bundling it there would make an
asset-accuracy fix hostage to the English patch — a Japanese-only Integral player
gets nothing from en_menu3-style products) and it does not belong in the
MGSM2Fix upstream PR either (UPSTREAM.md tracks C++ mod-loader mechanisms; this
is disc asset data, not code). It ships as its own standalone PPF, named outside
the en_ convention the way the _unlock_ test-aid PPFs are (e.g.
INTEGRAL_disc{1,2}_fix_grenade_delay.ppf), documented on its own rather than
folded into the "What ships" table, and worth naming on its own merits to the wider
Integral/MGN community since it helps any Integral player regardless of language.

**Two separate assets, two separate difficulties.** The mission-briefing text
("Grenades explode in 5 seconds") is well inside this port's existing machinery —
vr_windows.py's substitute_numbers() already carries Integral's number into
USA's sentence; the fix is simply to stop doing that for this one case and let
USA's own correct "4" stand, an easy, bounded change. **The 3D model texture is
the hard, unfinished part**, and is what this section exists to hand off.

**Where the texture lives — confirmed.** Stage vab_grn (the GRENADE weapon-select
stage; a per-weapon stage exists for each of the eight VR weapons — vab_sud,
vab_fms, vab_clm, vab_nkt, vab_psg, vab_scm, vab_stg, vab_grn — found
via vrlib.stage_gcx()/portio.stage() tag enumeration). The relevant tag: index
31, id=0000 mode='c' ext=0xFF, sizes 178,316 bytes (Integral) / 176,816 bytes
(USA) — the ext=0xFF "fake tag" convention this project already knows from GCL
script chunks, though this one is not a GCL script; its content has not been
identified. A raw byte diff of the two payloads (portio.stage()'s payloads[31])
found bytes 0-163,441 byte-identical between discs (this is the shared 3D model
geometry/animation, unsurprisingly unchanged by region) and bytes 163,442 to the
end diverging completely and never realigning (10,400+ differing bytes across the
shared length) — the classic signature of a compressed stream where the encoded
content differs and every subsequent byte shifts. That divergent tail is where the
DELAY decal's difference actually lives; the search space is now this ~13KB region,
not "somewhere on the disc".

**What was ruled out, so the next session does not repeat it.** None of the
following produced a valid decode anywhere in or before that divergent range:

- pcx4.decode() — this project's own 4bpp RLE texture codec (used for
  sc_text, the KEY CONFIG labels)
- pcx4.decode8() — its 8bpp counterpart
- The standard PSX TIM texture header (0x00000010 magic + flag word): several
  byte-sequence matches for the magic were found and parsed structurally, but
  every one produced nonsensical CLUT/pixel dimensions (e.g. pw=0, ph=65535) —
  coincidental 4-byte matches, not real TIM headers
- Raw, uncompressed 8bpp grayscale at widths 64/96/128/160/192/256 — rendered
  and visually inspected; no recognizable structure at any width, for either disc

Also checked and empty: the decomp source tree has no file or identifier
(GRENADE_MODEL, VAB, WeaponView, SelectModel, etc.) that obviously owns this
specific menu's model/texture rendering — the per-weapon vab_*.c files are just
character-registration tables (see vab_grn.c, vab_sud.c in source/stagevr/),
and select.c/selectvr.c are tiny stubs. Whatever renders this menu's rotating
model is shared, generic code this search did not find by name.

**Recommended next steps, in the order I would try them:**

1. Find the real source path. The search so far has been by guessed identifier
   name; a more systematic pass (e.g. tracing what calls the generic weapon-display
   renderer from the VR menu's own top-level stage/proc, or searching the decomp
   for whatever loads a TIM-shaped resource by convention rather than by name)
   is likely to succeed where name-guessing did not.
2. Live inspection over static analysis. An emulator's VRAM/texture viewer
   (or a debug memory dump) while this exact menu is on screen would show the
   decoded texture directly — its real dimensions, bit depth, and VRAM
   address — which then makes finding its on-disc encoding a matter of matching
   a known shape, rather than guessing blind.
3. Once the format is known, the fix is likely mechanical and low-risk: this
   project already has a proven pattern for exactly this shape of problem —
   vr_option.py's build_dar(), written for the KEY CONFIG texture transplant,
   swaps one disc's texture bytes into another's slot by entry id, re-encoding
   losslessly. The same shape of function, once the container format is
   understood, should work here too.

**Session housekeeping the same evening:** the documentation was reorganised for a
model handoff at the user's request — this section written for that purpose, and
HANDOFF-ARCHIVE.md §5 carries a short pointer into it.

## 29. The grenade decal port completed, 2026-09-11

The resumed session checked the cache tags before continuing the earlier search.
`vab_grn`'s entry at 163,440 is extension `g`, id `EA54`: `scenerio.gcx`.
The first difference at 163,442 is inside that script's header. The earlier
claim that the divergent tail proved a compressed texture was incorrect, and
decoding attempts against that tail did not exclude any texture format.

Decoding the actual texture archives in `selectvr` immediately exposed the
grenade label: **second DAR, tag index 2, first entry `4D80.p`**. Integral's
PCX begins at stage offset `0x4F008`; USA's at `0x50008`. Both decode as a
200×125 4bpp image. Direct visual inspection of these decoded assets shows
`GRENADE,DELAY 5.2` and `GRENADE,DELAY 4.0`, with the serial number below.
The complete 128-byte headers match, including VRAM and CLUT coordinates.

`vr_grenade.py` now builds the standalone fix from the two collection disc
containers, without relying on cached stage extracts. It pins the texture hashes:

- Integral: `3385810e2df7b03689fcb09797ac59ed1b83e14dfeac03b0a7c1889f1cb5927a`
- USA: `72589944f74a9005a44c270b7fa7aa84d397d149e677b19a6482aae260bb4d4d`

USA's 10,568-byte PCX re-encodes losslessly with the existing codec to 10,453
bytes. Integral's slot holds 10,512, so 59 zero bytes fill the remainder and no
archive entry or stage allocation moves. The builder verifies decoded indices
and palette against USA and exact stage reconstruction from the serialized PPF.
Only the texture slot changes; no overlay, model, script or other texture does.

Built `INTEGRAL_vr_fix_grenade_delay.ppf`: 704 records / 9,504 payload bytes,
with all records inside sector payloads. Also built
`INTEGRAL_vr_fix_grenade_delay_raw.ppf`, adding verified EDC/ECC tails for the
six affected sectors. Both include Integral VR's block check. Raw records were
read back and applied to original physical sectors to verify serialization too.
The collection PPF was checked for overlap against every other deployed VR PPF,
checked with `ppfcheck.py`, deployed, and read back byte-exact. All 44 existing
data-free self-tests passed.

The collection patch is installed in `mods/INTEGRAL/VR-DISK/`; both outputs
are in `WORK`. This stays outside the English package and upstream PR as
previously decided. The separate briefing sentence was not changed in this
texture-only task. **In-game verification is pending:** reload the selection
stage and inspect WEAPON MODE > GRENADE. The original two decoded textures
are pictured in `WORK/grenade-delay-before.png` for comparison.


## 30. Grenade briefing numeral added to the standalone fix, 2026-09-11

After confirming the texture looked good, the user asked for the remaining
briefing's `5 seconds` to become USA's correct `4 seconds`, as part of the
same fix and also working with the original Japanese briefing. The screenshots
`Integral VR Disc/20260911160634_1.jpg` and
`VR Missions USA/20260911160434_1.jpg` were inspected: they show precisely that
sentence difference, with `Targets 3` identical.

Scanning all 105 Integral stage scripts found the same Level 02 briefing in
**all five** `vr_grn01`-`vr_grn05` stages. The relevant Japanese record contains
`...90FA 8035 90FB...`; the numeral is not an overlay constant or a translation.
`vr_grenade.briefing_offset` parses the GCL window and matches its two titles
and exact sentence, accepting only the known original/corrected forms. It
changes `35` to `34`, preserving the `80` prefix and all other stage bytes.
USA's actual English window in `vr_grn02` independently confirms the 4.

The English port shifts these five digit positions by 0x20 in the current
scripts. A Japanese-address PPF would corrupt the ported script, and an
English-address PPF would write a target-count padding byte in Japanese.
Consequently the default standalone outputs are for Japanese Integral; optional
`_english` outputs resolve the positions from the supplied/installed mission
PPF. Deployment chooses the installed layout and uses the same unsuffixed
mod filename. Users switching mission-text patches must rebuild/redeploy the
addon. Both versions include the already-confirmed texture.

**Load order is not a fix.** Ketchup iterates `std::filesystem::directory_iterator`
without sorting; an old log description claiming filename order does not make
that an API guarantee. Rather than rely on it, `vr_windows.py` now invokes the
same briefing correction after porting the five stages. The full English
rebuild was compared against every byte written by the deployed PPF: identical
address coverage and exactly five differences, each ASCII 5 -> 4. The addon
explicitly writes those same five 4s, so both application orders were verified
to yield the same corrected English scripts. The standalone Japanese patch
requires no English assets or executables.

Both collection PPFs now have 709 records / 9,509 payload bytes. Both raw
variants additionally repair 11 sectors. English raw parity is computed from
the full English mission payload of each touched sector, not from retail
Japanese; use that exact English base and apply the raw addon last. Every
serialized payload PPF was applied back to its corresponding stage base;
every serialized raw PPF was applied to its physical sectors with the proper
base and checked against regenerated EDC/ECC. Five new synthetic GCL tests
prove Japanese/English single-digit scope, idempotence, and rejection of wrong
missions, languages and unexpected numbers. All 49 self-tests pass.

Previous deployed mission and grenade PPFs were backed up under
`WORK/grenade_before_briefing/`. The English compatibility update and matching
combined grenade addon were checked and deployed. The Japanese standalone was
built and verified without applying any English PPF. **Briefing in-game
confirmation remains pending.** The earlier all-missions unlock removal was
preserved; the unlock PPF stays in `unlocks_parked/`.


## 31. Seven optional-patch INI controls, 2026-09-11

The user approved seven proposed switches and explicitly confirmed that
INTEGRAL_vr_unlock_missions.ppf should return to the active folder while being
disabled through the INI. It is now installed again; its parked backup remains.
All four new unlock controls are false, including the extras/movies files that
had previously stayed active. English main/VR sets and the grenade fix default
true. Existing EnglishText, UnlockBriefing and other user settings were retained.

The implementation is a pure loading planner in mgs1_patch_options.h, called
by Ketchup::ProcessDisk, with configuration fields in M2Config. Mapping is by
exact known family plus title/version/disc. Unknown mods are preserved. Main
English controls group all known families on both discs; the VR group is separate.
Native fixtures prove independent title/USA/Integral controls and all eight
VR unlock combinations, and reject stale companions and malformed digit data.

The grenade switch required an additional mechanism: simply skipping its PPF
would leave the English mission PPF's corrected 4 behind. The mission builder
now emits a JSON companion pinning its full-file FNV-1a fingerprint and five
structurally located digit addresses. The loader validates it before loading,
then substitutes 4 or 5 in those five PPF writes according to GrenadeDelayFix.
The external texture/fuse PPFs for both languages are installed with companions,
and only the layout matching the enabled English set is selected. The English
addon pins its base fingerprint too. Missing/stale mission metadata skips the
VR English group with a log explanation; stale addon metadata skips the addon.
No game data was embedded in the ASI. Full fingerprinting here is a stale-layout
guard, not an authentication mechanism.

verify_patch_options.py runs the native planner against the installed assets
and applies the resulting records and overrides to retail disc stages. All four
English/grenade configurations passed: either language with original texture/5,
or either language with USA texture/4; every other briefing byte was exact.
All 49 Python self-tests pass. Release x64 built successfully with
/p:PostBuildEventUseInBuild=false and was deployed through the Vortex symlink
target. Installed ASI SHA-256:
51931ff4d5503e1ce9eee31b6e18d4c734a37cf3094410eba6594ce1b1c98725.

The installed INI and previous ASI were backed up in WORK/ini-toggle-backup.
The actual installed selection report is WORK/verified-patch-options.json.
Eight test-aid PPFs are installed behind the disabled new controls: two Integral
and two USA title files, Integral/USA VR mission files, Integral extras and movies.
Rebuilding an English package now regenerates its companion after final PPF
serialization, and the normal package checksum manifest includes it. INI changes
need a restart; changing English on/off does not require rebuilding the grenade
assets. In-game testing of these new controls remains pending; the native and
real-disc verification is not being represented as a live play test.

## 32. Briefing unlock disabled and confirmed, 2026-09-11

The user reported that Integral's BRIEFING menu was still unlocked and asked
whether the save was responsible. Inspection found `UnlockBriefing = true`
in the active INI, retained from the earlier test session when the seven new
PPF controls were deployed. Its runtime code holds the sixteen briefing flags
set in the title/briefing scenes; `UnlockTitleBonuses` does not control it.

At the user's request, only `UnlockBriefing` was changed to false through the
Vortex symlink target. The active game INI was read back to verify it, and the
user then confirmed: "Good that fixed it." No save was edited or deleted.
The separate briefing control is therefore confirmed off in game. The new
PPF switches retain their native/real-disc validation status; this report does
not establish their full in-game matrix. Saves begun with the briefing control
enabled can retain flags, so disabling it should not be described as clearing
existing save progress.

The subsequent pre-commit review corrected the current-state unlock notes and
kept the old deployment events in this timeline. It also made script-index
generation independent of the working directory and rejected filtered mission
builds before writing a partial PPF with no valid five-digit companion. Manual
checks confirmed both behaviors and that the original NextSteps §9–27 text is
preserved verbatim here. Validation passed: Release x64 build, 49 Python
self-tests, all Python compile checks, native option checks, the four real-disc
English/grenade configurations, and structural checks of all 38 installed PPFs.
No new in-game test or game deployment was performed during this commit review.

## 33. Both grenade corrections included in raw packages, 2026-09-12

The user's newly patched raw VR image still showed DELAY 5.2. Inspection found
all 33 `repro34raw` PPFs applied exactly, but that older package predated the
briefing fix and the raw packager never included the standalone texture addon.
All five English briefing copies still read 5. The user explicitly requested
both corrections in the raw patch.

`rebuild.py --variant raw` now packages the English-layout grenade payload
before computing the complete set's shared ECC patch. The helper verifies the
USA texture round trip and exactly five agreeing writes shared with the English
mission patch. The standalone raw addon's narrower parity is not reused.

Clean build `D:/mgsbuild/repro35raw` produced 34 PPFs. Compared with `repro34raw`,
only the mission PPF, new grenade PPF and VR ECC PPF changed. `mkimage.py` built
the VR BIN from the retail dump with English power-on retained and verified
all 2,015 touched sectors before and after patching. Finished-image readback
proved exact USA decoded pixels, only five 5 -> 4 briefing edits, and no changes
to any other stage. All 34 PPFs pass structural checks; all 49 self-tests pass.

The usual VR BIN in `D:/mgsbuild/patched` was replaced and hash-verified; the
previous BIN is retained in `repro35raw/previous-image`. The existing CUE and
both main-disc images remain valid. Verification report:
`repro35raw/raw-grenade-verification.json`. New image SHA-256:
`90a50dfb7ec1ed99ebae609590e119b4d3cfb8f13227f4268552b2b47380b894`.
ZIP SHA-256: `5d276e7c178ba359c94e66d4dfff72681ef450c538840fcfeb23eff84ea3b523`.
The user confirmed the raw build works on 2026-09-12: "Good it works."
The collection installation was not changed.
