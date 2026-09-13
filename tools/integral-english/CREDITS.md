# Credits and provenance — Integral English text port

This port stands on other people's work. This file says whose, exactly what is
used, and what is unresolved. It covers `tools/integral-english/` and the
patches it builds; the rest of MGSM2Fix has its own `LICENSE` and
`LICENSES.txt` at the repository root.

## The MGS1 decompilation — FoxdieTeam/mgs_reversing

**<https://github.com/FoxdieTeam/mgs_reversing>**

Three of the port's nine main-disc patch families contain compiled code, and
that code is built from this project's source:

| patch | overlay | what the port changes there |
|---|---|---|
| `en_option` | `option.bin` | the `sc_text` brightness texture path, and the collection's KEY CONFIG doorbell |
| `en_preope` | `preope.bin` | Previous Operations paginated the way the USA release paginates it |
| `en_abst` | `abst.bin` | the MISSION LOG rebuilt to the USA release's two-screen model |

Nothing else in the port compiles anything. The VR disc's code changes — the
KEY CONFIG label transplant and the MOVIE captions' two-line stub — are
hand-assembled instructions written as byte patches, and every other family is
data.

**Pinned revision:** `7964de7fd2e9e8276a6307d59fc7a0cdf96aae21` (2026-08-27).
`rebuild.py` exports exactly that commit into an isolated directory, applies
`decomp-overlay-changes.patch`, and compiles three targets. It never modifies
the checkout it reads from.

**What of theirs is in this repository.** No decomp source is vendored here and
none ships in a release: a build requires a clone you supply with `--decomp`.
But `decomp-overlay-changes.patch` is committed, and a unified diff quotes the
code it changes — 534 context lines and 425 removed lines of their source,
against 819 lines added by this port, across five files
(`onoda/abst/abst.c`, `onoda/option/opt.c`, `onoda/preope/preope.c`,
`pre_met1.c`, `pre_met2.c`).

**Their work is byte-faithful, and that is measured, not assumed.** Integral's
VR executable built from this decomp reproduces the retail disc's own EDC/ECC
parity for all 308 of its sectors — a 280-byte sum over each 2048-byte block
(`py cdecc.py`). Where the source is unchanged, their build *is* the disc.

**Unresolved: the project states no licence.** There is no `LICENSE` or
`COPYING` file at its root and its README carries no licence or credits
section, so the terms under which its source may be modified, compiled and
redistributed are not stated anywhere. This port therefore:

* vendors none of it, and requires the builder to obtain their own copy;
* pins and records the exact upstream commit in every build report;
* credits the project here, in `README.md`, in the patch file itself and in the
  README that ships inside every release ZIP.

**That is credit, not permission.** If FoxdieTeam would rather their code were
not compiled into a distributed patch, the fallback is to express these three
overlays as byte patches against the retail overlays, the way the VR disc's
code changes already are — larger work for `abst`, which is a reimplementation
rather than a tweak, but possible.

## This port's own code

The scripts under `tools/integral-english/` and the documents beside them
are original to this port — none of them are copied from `mgs_reversing` or
any other project (the decomp's *output* is compiled by three of them, which
is the case above). **No separate licence has been selected or added for the
port's tooling.** The repository retains its existing MIT `LICENSE` under
nuggslet's copyright. An explicit licensing decision for
`tools/integral-english/` remains open; this documentation update does not
choose one or change the repository's licence.

## Other people's work this port relies on

| what | who | how it is used |
|---|---|---|
| MGSM2Fix and Ketchup | nuggslet — <https://github.com/nuggslet/MGSM2Fix> | the mod loader the collection build targets, and the host for this port's runtime changes. MIT; see `LICENSE` |
| PSY-Q SDK | Sony Computer Entertainment | required to compile the three overlays. Not distributed here, and not distributable |
| Metal Gear Solid, MGS Integral, VR Missions | Konami | every English string in this port is copied verbatim from the USA release. Nothing is translated and no game data is distributed in this repository |
| mgs1-scripts and the MGS1 Translation Toolkit | drsparklegasm / DoktorDeSparkle, with OCR work credited there to Green_goblin — <https://github.com/drsparklegasm/mgs1-scripts>, <https://github.com/DoktorDeSparkle/mgs1-translation-toolkit> (GPL v3) | **used once, as an independent cross-check, on 2026-09-11.** Their `graphicsData` table maps 6,877 codec-glyph bitmaps from the Japanese disc to characters; every one of the 1,214 bank-1 shapes in `bank1-glyphs.tsv` occurs in it, 1,208 readings agree, and the six disagreements were re-read here and all six corrected (NextSteps §27). Their table is OCR output with errors their README acknowledges, and the GUI is described there as largely AI-written, so it was treated as a second opinion and not as an authority: **no code and no table entry of theirs is copied here**, the comparison hashed their bitmaps with `jptext.shape_key` and compared characters, and each change was decided by re-reading the tile against reference glyphs and re-reading the sentence, not by taking their value. Their tools target subtitles (RADIO / DEMO / VOX / ZMOVIE) for an undub, which this port does not touch |

## What this port does not take

* **No game data is in this repository.** Disc images, stage archives,
  executables and textures are read from a local installation and from retail
  discs the builder supplies; none is committed and none is redistributed.
  Audited and enforced 2026-09-10: the reference screenshots and the rendered
  key-config art were deleted, and `bank1-glyphs.tsv` - the bank-1 glyph
  table - now carries a digest of each 12x12 bitmap (`jptext.shape_key`)
  instead of the bitmap itself. The digest is enough to look a glyph up, so
  nothing here stopped working; what it deliberately cannot do is reproduce
  the font. `glyphfill.py --publish` is the step that strips the bitmaps, so
  a future transcription pass cannot reintroduce them by accident, and
  `.gitignore` blocks images under `tools/integral-english/`.
  **The one thing that is not stripped is short quotations of game text in
  the documentation** - the sentence each glyph was read against, a subtitle
  that showed a decoder bug. They are there because a finding nobody can
  check is not a finding; they total a few thousand characters against the
  3.9 million the export holds, and the export lives outside the repository.
* **No new translation exists.** Every English string comes from a released
  Konami build. Where the USA release has no counterpart, the Japanese is left
  exactly as it is, on purpose. `COVERAGE.md` lists those cases.
* **The collection's own patch data is read, never redistributed.**
  `m2archive.py` decodes the archive in place so the collection's CD-ROM
  patches can be compared with the port's; it extracts nothing into this
  repository.
