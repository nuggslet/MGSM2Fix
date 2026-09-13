> Historical/reference record. For current instructions see [README.md](README.md),
> [BUILDING.md](BUILDING.md), and [NextSteps.md](NextSteps.md). Dated counts,
> machine paths, deployment state, and completed-work estimates below are not current instructions.

# Rebuilding and packaging the current collection patch

`rebuild.py` builds the whole port in a fresh directory - nine patch families
for both main discs, and since 2026-09-07 the VR disc's (eight since 2026-09-11) as well - and it
never installs patches or changes game files. M2Package packages the ASI
separately and is not the Integral asset packager.

**Two variants, one switch** (also 2026-09-07; before that both constants were
edited by hand):

    py rebuild.py --output <dir>                    # collection: what mods/ gets
    py rebuild.py --output <dir> --variant raw      # for a real PSX disc image

| | collection | raw |
|---|---|---|
| `SC_KEEP_LINES` (`optsctext.py`) | 4 - the collection drops USA's two ○-button lines | 6 - USA's own text |
| `OPTION_MC_CONTROL_SETTINGS` (`opt.c`) | 1 - reproduce the KEY CONFIG doorbell | 0 - nothing to intercept, and no RAM at 0x80200000 |
| `en_menu3` (the `title` disc-swap copy) | **excluded** - the collection patches that block itself and the two layouts kill the title stage | included |
| VR grenade correction | separate texture addon; English mission text includes the corrected numeral | included: DELAY 4.0 texture and all five four-second briefings, with parity computed for the complete package |

The switch is `INTEGRAL_ENGLISH_VARIANT`, resolved in `workdir.py` next to
`WORK`/`GAME`/`DECOMP`, so a tool run by hand honours it too:
`INTEGRAL_ENGLISH_VARIANT=raw py optsctext.py`. `rebuild.py` sets it for every
tool it runs, edits the `opt.c` constant in its isolated decomp export before
compiling, records both values in the report, and names the ZIP for the variant.
`--compare-deployed` is refused with `--variant raw`, because what is deployed
is the collection build.

## Tests

The optional-PPF loader also has a native test executable:
`test_patch_options.cpp`. Compile it with MSVC C++20, `/EHsc` and
`/I ../../src/extern/json/single_include` from this directory, using a Visual
Studio developer prompt. Run it without arguments for data-free classification
checks. For complete selection/companion checks plus real disc validation, run
`py verify_patch_options.py <test.exe> <mods/INTEGRAL/VR-DISK>` after installing
the English set, both grenade collection variants and companions, and the three
VR unlock assets. The verifier does not edit game data or the INI. It tests all
four English/grenade combinations and eight VR unlock combinations; native
scratch fixtures go in the OS temporary directory. The report goes to
`WORK/verified-patch-options.json`.

The English mission builder now emits `.ppf.json` beside its PPF. Collection
packages must include it: Ketchup uses the fingerprint and five digit addresses
to honor `GrenadeDelayFix` independently of the English toggle. `rebuild.py`
regenerates the companion after packaging, so adding a raw block check cannot
leave a stale file fingerprint. SHA256SUMS includes these companion files.
Stage-name filters in `vr_windows.py` are inspection-only. Build/deploy always
uses the full mission set so its companion can validate all five briefing digits;
a filtered build is rejected before it can overwrite the existing PPF.


`py selftest.py` runs 49 tests over the parts that need no game data — the PPF
emitter's two split boundaries (255 bytes and the 2048-byte payload edge), the
record chain, the PCX codec's run cap, the EDC/ECC algebra, the width model,
the language-default patch over a synthetic executable, and the R3000
load-delay scanner (`hazards.py`), which must catch the 2026-09-10 briefing
bug at its own address and pass retail's words and the fix. It takes a tenth
of a second and needs nothing installed, so there is no excuse for skipping it
before a build.

The build itself runs the scanner: `brf_build.py` asserts zero load-delay
hazards in the rewritten overlay against retail, and the same command works on
any hand-patched object - `py hazards.py <built> <base-hex> --retail <retail>`.
The Master Collection's emulator does not model the delay, so this is the only
check that class of bug has.

It is deliberately not a check against ground truth. `py cdecc.py` is that, for
the checksums, against the real discs; `rebuild.py --compare-deployed` is that
for the whole build. Each mutation of the three modules the suite covers was
confirmed to make it fail, so it is known to have teeth.

## Inputs

- A Windows installation of the Master Collection MGS1, including Integral DLC:
  `windata/dlc/dlc_japan.bin` and `windata/alldata.bin`.
- Four original retail executables in one directory: `int1.exe`, `int2.exe`,
  `us1.exe`, `us2.exe`. The collection's ISO executable extents are zero-filled;
  extracting them does **not** supply usable retail code. No game data is
  distributed in this repository.

  **The Redump dumps supply the Integral two.** `SLPM_862.47` and `SLPM_862.48`
  extracted from `Metal Gear Solid - Integral (Japan, Asia) (En,Ja) (Disc 1/2)`
  both hash to the `4b8252b6…` the table below requires (measured 2026-09-10,
  §22 of `HANDOFF-ARCHIVE.md`), so a copy of that set is a complete source for these
  inputs.
- The local MGS decomp Git repository containing commit `7964de7`, and a PSYQ
  SDK tree accepted by that revision's `build/build.py --psyq_path` (the local
  tree contains `psyq_4.3`, `psyq_4.4`, `psyq_4.5` and `aspsx`).
- Python with `tarfile.extractall(filter='data')` support, Git, Pillow and the
  Python `ninja` package. The report records Python/package versions and hashes
  every SDK file outside `.git`; reproduce those inputs for matching output.
- Tracked scripts, `decomp-overlay-changes.patch` and `brf_quads_all.json`.
  Their hashes are recorded in the build report.

The builder rejects unsupported executable hashes:

| Files | Bytes | SHA-256 |
|---|---:|---|
| `int1.exe`, `int2.exe` | 641024 each | `4b8252b65953a02021486406cfcdca1c7670d1d1a8f3cf6e750ef6e360dc3a2f` |
| `us1.exe`, `us2.exe` | 651264 each | `615e136083336957ed0b9b3805145bf5bbb35f7a16c2f160dba8f17bb71cc640` |

Stage files are extracted from the collection and hashed in the report with
their container, image base, ISO path, LBA and size. Main-disc image bases are
Integral `0`, `0x2AE54800`; USA `0xF12F8000`, `0x11B3E5800`.

## Command

From this directory in PowerShell. **Only `--output` is required** - the game
is found through Steam (registry, then `libraryfolders.vdf`, then the ordinary
install paths on every drive), the decomp is looked for beside this repository,
and the SDK defaults to `<decomp>/../psyq`:

```powershell
py rebuild.py --output D:/mgsbuild/repro22
```

Pass any of them explicitly to override, and a path that is wrong is refused
rather than quietly replaced by a search result:

```powershell
py rebuild.py --output D:/mgsbuild/repro22 --game D:/Steam/SteamApps/common/MGS1 --decomp D:/mgsbuild/d --psyq D:/mgsbuild/psyq --executables D:/mgsbuild/integral-english-work/work --compare-deployed
```

`py workdir.py` prints everything that resolved and how, and lists the Steam
libraries it searched when it cannot find the game. That is the first thing to
run when a tool cannot find something.

The output directory must not exist and must have a short path without spaces.
`--compare-deployed` requires the 25 deployed PPFs: the 18 under the game's
`mods/INTEGRAL/INTEGRAL/{0,1}` and the 7 under `mods/INTEGRAL/VR-DISK`. Omit it
for an independent build without that reference set. Existing PPFs are read only
as comparison references.

The builder exports the pinned decomp revision into the output directory,
applies the tracked patch, generates the build graph, and compiles only the
requested `option.bin`, `preope.bin` and `abst.bin` targets and their dependencies. The
original decomp checkout is not modified. It then runs the asset builders,
stages the two discs, validates PPF framing/sector boundaries and checks
conflicting writes across each complete patch set.

Comparison is by effective changed bytes against the original image, so PPF
description text and record grouping may differ without changing game data.
A failed comparison retains the report and does not create a ZIP.

## Outputs

- `Integral-English-<variant>.zip`: the PPFs in installation paths (collection: 20 main + 8 VR; raw adds `en_menu3` × 2 and a
  `zz_ecc` per disc, 33 in all), README, `build-report.json` and `SHA256SUMS.txt`.
- `package/`: the same unpacked files for review.
- `work/`, `decomp/`, `build.log`: extracted inputs, intermediate assets and
  compiler/build evidence, retained for diagnosis.

The ZIP uses fixed metadata/order. Its report records the environment, so a
different SDK, Python version or source checkout may change the ZIP even if
the resulting patch effects match. Inspect `reference_effect_equal` for every
output when comparing against the known deployed set. The package README
lists installation, removal, ASI requirements and incomplete features.

## Writing a patched disc image

A raw build stops at PPFs. `mkimage.py` applies a disc's folder to an image and
writes the patched `MODE2/2352` `.bin` and its `.cue`:

```powershell
py mkimage.py --redump "Metal Gear Solid - Integral (Japan, Asia) (En,Ja) (Disc 1).bin" `
    --ppfs D:/mgsbuild/repro21raw --output "D:/out/MGS Integral English (Disc 1).bin" --cue
```

**Which disc it is, and which folder to use, are both worked out.** The disc is
read from the executable inside the image, not from `--disc` and not from the
filename, so the wrong pairing is impossible rather than merely unlikely - pass
`--disc` and a disagreement is an error. `--ppfs` accepts the build directory,
its `package/`, the `mods/` tree, or the leaf folder itself, and picks the leaf
that matches the disc. The same command therefore does disc 2 and the VR disc
with only the image changed.

The other source is the collection's own image, which needs the retail
executable put back, refuses without it, and does need `--disc` (all three
images live in one container, so there is nothing to detect):

```powershell
py mkimage.py --collection --disc 1 `
    --exe D:/mgsbuild/integral-english-work/work/int1.exe `
    --ppfs D:/mgsbuild/repro21raw --output "D:/out/MGS Integral English (Disc 1).bin" --cue
```

(`--game` is optional here too; it is found the same way `rebuild.py` finds it.)

Both routes produce the same file. Give it a **raw** build: a collection build
carries no block check and no `zz_ecc`, so its images would be left with
correct data behind stale parity, and `mkimage.py` warns about the first and
refuses on the second. Every touched sector is verified against its own parity
before the write and against the set's recomputed parity after it; §22 of
`HANDOFF-ARCHIVE.md` has the measurements and what they do and do not establish.

## Recovered builders and obsolete experiments

The clean run on 2026-09-04 used Python 3.12.3, Pillow 11.0.0 and ninja 1.13.0.
All 16 patches matched the deployed set's effective changed bytes. Fourteen
also matched byte-for-byte; the two `en_menu2` files differ only in PPF encoding.
ZIP integrity and all 18 manifest entries were independently checked.
The local artifact is `D:/mgsbuild/repro4/Integral-English-collection.zip`, SHA-256
`b052a7105221130f024e0e7e4b1ca5701b66af761333dbbbd6a78b8ef0240366`.
Its full source/SDK/input/output ledger is in `build-report.json` beside the ZIP
and inside it. This is static equivalence evidence, not a new gameplay test.

`items.py` recovers the scratchpad's actual item generator (`mkpatch.py`).
`menu2.py` reconstructs current `en_menu` and `en_menu2` behavior, excluding the
broken historical menu3 mode. `optlabel2.py` rebuilds owned option captions
directly from retail and preserves the colon and all other unowned records.
`preope_usa.py` now builds both recaps directly; `preope_both.py` is an obsolete
experiment, not a prerequisite. Old `optbright.py`/font-text PPF output is not
an input. Briefing construction uses the USA donor; its 16 row and 53 quad
argument tuples were checked against the former European donor and match.

**Clean run 2026-09-05 (nine families).** After the MISSION LOG port, the same
command rebuilt all 18 PPFs in `D:/mgsbuild/repro5`; every one matched the
deployed set's effective changed bytes (16 byte-identical, the two `en_menu2`
differing only in record grouping as before). The exported decomp compiled
`abst.bin` byte-identical to the live checkout's (SHA-256
`a491c1d27a256cb7295da12f543620ef955c33ef3e72fa719c9c9d531283c966`, 48,087
bytes). ZIP SHA-256
`02346ac790a218429220b65f2c8bc930ea07f01cf07eab1f4e090d6f931a42f0`, 21 manifest
entries. `abst_build.py` also reads the mods folder to refuse any overlap with
the other PPFs' bytes, so a clean run wants the game installed even though it
never writes to it.

**Clean run 2026-09-05, later (item fixes).** After `items.py` and `savemsg.py`
changed to own every byte of their pools (README "Three item-text faults"), the
same command in `D:/mgsbuild/repro6` again matched all 18 deployed PPFs by
effect (16 byte-identical); `en_items` is now 26 records / 3518 bytes per disc and
`en_savemsg` 3 records / 618 bytes. ZIP SHA-256 `d8dba9d16b2325f60785ab443f8f7429babd6d7178d175dd6942cc0a6f97e9b5`, 21 manifest entries.

**Clean run 2026-09-05 13:06 (slide fix).** After the abst sprite-width fix
(decomp `26d27f1`, `abst.bin` 48,103 bytes, SHA-256 `f625fc8ece123648…`), `D:/mgsbuild/repro7`
again matched all 18 deployed PPFs by effect (16 byte-identical). ZIP SHA-256
`870a691a4782291c5e92d6a68f3035cb102ed132daf5ce478901a14dc8ec51ca`, 21 manifest entries. This was the deployed state until the VR disc was folded in (repro8, below).

**Clean run 2026-09-07 16:08 (repro8: the VR disc folded in, one switch for both
variants).** The same command in `D:/mgsbuild/repro8` built 25 PPFs, 18 main + 7
VR, with Integral's VR executable compiled from the decomp rather than copied,
and every one matched the deployed set's effective changed bytes. ZIP SHA-256
`a13eefc08fa93b61adcb7c0524d57e6d7e913e0f313262e961c293d13bd5faef`, 2,796,157
bytes, 27 manifest entries.

**Clean run 2026-09-08 (repro17: `en_pad2`, the tenth family).** The same
command in `D:/mgsbuild/repro17` built 27 PPFs, 20 main + 7 VR, and every one
matched the deployed set's effective changed bytes; the two new
`INTEGRAL_disc{1,2}_en_pad2.ppf` are byte-identical to what is deployed
(SHA-256 `ff45bcea…448a` and `4a7af1d1…6f99`, 273 bytes and 159 changed
bytes each). ZIP SHA-256 `3eb2e1058486fceb3f0aa866f3194bc7c07ed70987fc2eea439259d5e8e8a6fb`, 2,798,182
bytes, 30 manifest entries.

**Clean run 2026-09-08 (repro18: USA's `abst` location names).** The four
location names in the MISSION LOG now read USA's (`USA_LOCATION_NAMES` in
`abst_build.py`), which grows the abst chunk by 12 bytes and leaves the stage at
88 sectors. The same command in `D:/mgsbuild/repro18` built 27 PPFs and every
one matched the deployed set, both `en_abst` files byte-identical. ZIP SHA-256
`0044ed814c81d18308e3969b5f342aa10ba4d3b17561881efc399300216eda3e`, 2,797,997
bytes, 30 manifest entries.

**Clean runs 2026-09-08 (repro19 and repro20: the packaged README corrected).**
No patch changed in either; `PACKAGE-README.txt` did, and it ships in the ZIP as
`README.txt`, so the recorded hash had to be re-made to stay reproducible. It had
said each main disc holds *nine* PPFs (it is ten since `en_pad2`), repeated that
in its uninstall instructions, and still told the installer that
`INTEGRAL_vr_en_movie.ppf` is built on top of `INTEGRAL_vr_en_missions.ppf`,
which stopped being true when those two were given one stage each. `repro19`
carried the first two fixes and `repro20` the uninstall line; both built 27 PPFs
matching the deployed set, and **`repro20` is the reproducible one**. ZIP SHA-256
`9dff48498d2be474685f490b12df77f301dc107b5a49e5790898639bb459bce3`,
2,798,169 bytes, 30 manifest entries. This is the deployed state.

The lesson is small and cost two runs: **the packaged README is an input to the
artefact, not a note beside it.** Read it whole before rebuilding for a doc fix.

`--variant raw` flips the two constants and adds `en_menu3`. Since 2026-09-07 it
also does the two things a real disc needs, which the collection never did:

* **it recomputes error correction.** Changing a payload byte invalidates that
  sector's EDC and P/Q parity. `rawdisc.py` rebuilds the tail of every sector the
  set touches and ships them as one more PPF per disc, `INTEGRAL_disc{1,2}_zz_ecc.ppf`
  and `INTEGRAL_vr_zz_ecc.ppf` — 413, 413 and 2003 sectors. Before computing any
  tail it requires the sector, as we believe retail has it, to verify against its
  own **stored** parity, so it cannot invent one for a sector whose true content
  is unknown. The executables are zero-filled in the collection's images, so the
  retail file is substituted first; that they then reproduce the stored parity
  exactly (313/313, 313/313, 308/308) is what proves both the sums and the inputs.
* **it stamps a PPF3 block check** — 1024 bytes of the original image at 0x9320 —
  so a tool that honours it refuses a patch aimed at a different release.

Check a finished raw build end to end with `py rawdisc.py <output>/package`: it
applies the whole set in memory and reports whether every touched sector
verifies. Expect `all verify` on all three discs.

**Still not proven: the raw variant has never been applied to a real disc image
and booted.** The remaining question there is whether the collection's embedded
images equal a retail dump everywhere the patches address, and the strongest
evidence so far is the parity check above, which says they do in the executable
extents. `HANDOFF-ARCHIVE.md` §5.4, §5.10 and §5.13.

## Clean runs, newest first

| run | date | what changed since the previous run | matched the deployed set | ZIP SHA-256 |
|---|---|---|---|---|
| `repro34` | 2026-09-11 | collection build with `vr_en_memcard`; 28 PPFs, 20 main + 8 VR | 28 of 28 | `73838eec29929a26877177adfa57d456f6032462d69e6c704c5e5e274b81100c` |
| `repro34raw` | 2026-09-11 | **raw variant**; `vr_en_memcard` added (`vr_memcard.py`: the `vrsave` and `selectvr` memory-card captions), 33 PPFs, 24 main + 9 VR; disc 3 image rewritten from it | n/a (raw) | `a6fc5a29b43ffca3ca6d8990399082554ec927378041cd7a998c9858cd54b6cd` |
| `repro33raw` | 2026-09-10 | **raw variant**; the three horizontal connectors' left ends at USA's values (`CONNECTOR_LEFT`); what the images in `D:\mgsbuild\patched` are built from | n/a (raw) | `0d2eab58e7141ff8d716c374f1920d6059cb39cdd69dce352c4d1f20594daef5` |
| `repro33` | 2026-09-10 | collection build of the same; `--compare-deployed` reported exactly the two `en_brf` PPFs as different (the intended change) and so made no ZIP; those two were **deployed** from its `package/` at 23:24, the previous pair kept as `workrf_deployed_before_connector_disc{1,2}.ppf` | 25 of 27, the two `en_brf` by design | none (no ZIP) |
| `repro32raw` | 2026-09-10 | **raw variant**; `en_brf` `ROW_H` with the load-delay `nop` (the briefing fix), every geometry group on; 32 PPFs, 24 main + 8 VR | n/a (raw builds are not compared to the deployed collection set) | `a2ebded662372497df4864e0981c17556559f2143ab2832794b86e8324c3173f` |
| `repro20` | 2026-09-08 | packaged README: the uninstall count too | 27 of 27 | `9dff48498d2be474685f490b12df77f301dc107b5a49e5790898639bb459bce3` |
| `repro19` | 2026-09-08 | packaged README corrected (ten PPFs a disc; the split VR pair) | 27 of 27 | `c8df6f1e…556a` (superseded by repro20) |
| `repro18` | 2026-09-08 | USA's four `abst` location names | 27 of 27 | `0044ed814c81d18308e3969b5f342aa10ba4d3b17561881efc399300216eda3e` |
| `repro17` | 2026-09-08 | `en_pad2` added (`pad2.py`), ten families | 27 of 27 (20 main + 7 VR) | `3eb2e1058486fceb3f0aa866f3194bc7c07ed70987fc2eea439259d5e8e8a6fb` |
| `repro8` | 2026-09-07 16:08 | VR disc folded in, VR executable built from the decomp, `--variant` switch | 25 of 25 (18 main + 7 VR) | `a13eefc08fa93b61adcb7c0524d57e6d7e913e0f313262e961c293d13bd5faef` |
| `repro7` | 2026-09-05 13:06 | MISSION LOG slide fix (`abst.bin` 48,103 bytes) | 18 of 18 | `870a691a4782291c5e92d6a68f3035cb102ed132daf5ce478901a14dc8ec51ca` |
| `repro6` | 2026-09-05 | item-text fixes; `en_items` and `en_savemsg` own every byte of their pools | 18 of 18 | `d8dba9d16b2325f60785ab443f8f7429babd6d7178d175dd6942cc0a6f97e9b5` |
| `repro5` | 2026-09-05 | MISSION LOG (`en_abst`), nine families | 18 of 18 | `02346ac790a218429220b65f2c8bc930ea07f01cf07eab1f4e090d6f931a42f0` |
| `repro4` | 2026-09-04 | first clean run, eight families | 16 of 16 | `b052a7105221130f024e0e7e4b1ca5701b66af761333dbbbd6a78b8ef0240366` |

"Matched" is by effective changed bytes against the original image
(`reference_effect_equal`), not by PPF file hash; the paragraphs above hold each
run's details.

## The VR disc (in `rebuild.py` since 2026-09-07)

`rebuild.py` builds the seven VR PPFs in the same isolated run as the main
discs and packages them under `mods/INTEGRAL/VR-DISK/`. The tools below are what
it runs, and they still work standalone for iterating on one patch.

Three things are particular to the VR half of a clean build:

- **Integral's VR executable is built, not copied.** The collection's copy is
  unusable, so `rebuild.py` runs the decomp's generator a second time for the
  `vr_exe` variant, ninjas `obj_vr/_mgsi.exe`, and checks it against
  SHA-256 `c370f8e4…` before the tools see it. USA's `SLUS-00957` is a supplied
  input, hashed like the four main executables.
- **`vr_movie` composes on the run's own output.** It builds on top of
  `vr_en_missions`, which already owns the `movie` stage, and normally reads the
  deployed folder to do it. `rebuild.py` sets `INTEGRAL_ENGLISH_VR_PPF_DIR` to
  its own work directory so an isolated build never depends on what is installed.
- **The VR set has one deliberate overlap.** `vr_en_movie` shares bytes with
  `vr_en_missions` by construction and must land last, which Ketchup's name
  order gives. The packaged-set overlap check allows exactly that pair and no
  other, on top of the main discs' own check.

**Order matters inside the VR half.** `vr_windows.py` ports the `movie` stage
but writes none of its records: it hands the finished stage to `vr_movie.py` as
`work/vr_movie_base.bin`, so that one patch owns that stage and the two no longer
overlap (README, "The composite trap"). `vr_movie.py` therefore has to run after
`vr_windows.py`, which is the order `VR_SCRIPTS` gives. Run by hand, the same
applies. `rebuild.py` now refuses **any** overlap between two VR patches.

The unlock aids (`vr_unlock.py`, `vr_unlock_movies.py`, `vr_unlock_extras.py`)
are **not** built or packaged: they are test aids, they must never ship, and
they are documented under "Unlock every VR mission", "Unlocking the EXTRA
movies" and "Unlocking the EXTRA menu's items".

### Inputs

- The same Master Collection installation. Integral's VR ISO is inside
  `windata/dlc/dlc_japan.bin` at image base `0x57592000`; USA's VR Missions ISO
  is inside `windata/alldata.bin` at `0xD39B7000`.
- Two VR executables in the work directory, because the collection's copies are
  zero-filled:
  - `work/vrint.exe` — Integral's, rebuilt byte-exact from the decomp with
    `py build/build.py --variant vr_exe`, SHA-256 `c370f8e4…`.
  - `work/vrus.exe` — USA's `SLUS-00957`, from a real disc image. It is a
    five-language build: Spanish, Italian, French, German and English pools
    behind tables-of-tables, English first, selected by GCL variable `0x11`
    (0 = English).
- Two stage directories extracted from the two VR ISOs, `work/vrint_stage.dir`
  and `work/vrus_stage.dir` (`/MGS/STAGE.DIR;1`). `vrlib.py` locates the ISOs,
  reads STAGE.DIR and computes the LBA of every named stage itself.

### Commands

A clean build needs none of these - `py rebuild.py --output <dir>` runs them all
in isolation. They are for iterating on one patch against the installed game:

```powershell
py vr_windows.py --build --deploy     # en_missions  (slow: 92 stages rebuilt)
py vr_exe.py --deploy                 # en_items and en_savemsg
py vr_option.py --deploy              # en_option (help lines + KEY CONFIG)
py vr_menus.py --deploy               # en_title
py vr_camera.py --deploy              # en_camsave
py vr_movie.py --deploy               # en_movie (MOVIE captions; needs the
                                      #   deployed en_missions to build on)
py ppfcheck.py --deployed             # always, before the game sees a PPF
```

Without `--deploy` each tool writes only to `work/`. `--deploy` copies into
`mods/INTEGRAL/VR-DISK/`.

Expected output on a clean run: 1808 of 1813 windows ported, 15 031 records and
3 370 955 bytes for `en_missions` with **no stage grown** (ten padded back to
their original sector count); 63 records for `en_items`; 36 records / 431 bytes
for `en_savemsg`; 546 records / 121 471 bytes for `en_option` with the DAR at
120 754 of 120 832 bytes; 7 records / 915 bytes for `en_title`; 4 records /
617 bytes for `en_camsave`; 69 records / 753 bytes for `en_movie`.

`vr_movie.py` is the one with an ordering constraint: it builds on the composite
(retail plus every VR PPF that writes the `movie` stage), so by hand a fresh
sequence has to deploy `en_missions` first, and inside `rebuild.py` it runs last
against `INTEGRAL_ENGLISH_VR_PPF_DIR`, that run's own work directory. It writes
two PPFs and `--deploy` installs the full one, moving the older E3-only file out
of `mods/` - the two overlap and only one may be present (README "The MOVIE
selection captions").

`py vr_unlock.py` builds the removable test aid that unlocks every mission
(README "Unlock every VR mission"). It is deliberately *not* deployed by
default, and must never be deployed with achievements enabled.
