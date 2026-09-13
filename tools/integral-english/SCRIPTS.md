# Script index

One line per script in this directory, pulled from its module docstring's first sentence by `gen_scripts_index.py`. Each script's own docstring has the full picture and usually a worked example; `REFERENCE.md` has the mechanisms, and `NextSteps.md` has how the pieces fit together and what remains.

| Script | What it does |
|---|---|
| `abst_build.py` | Port the MISSION LOG into MGS Integral's `abst` stage (en_abst). |
| `abstscan.py` | Scope the MISSION LOG (`abst` stage) in both games - the data the port needs. |
| `align.py` | Measure submenu rows relative to the vertical divider line to its left. |
| `audit_text.py` | Inventory remaining text in both main discs and the VR disc. |
| `brf_build.py` | Build the brf stage with the briefing labels at USA proportions. |
| `brf_widen.py` | Widen the briefing menu labels to USA proportions. |
| `bridge.py` | Long-lived sqdbg bridge: reads expressions from cmd/*.sq, writes replies to out/*.txt. |
| `camsave.py` | Port MGS1 (USA)'s PHOTO ALBUM memory-card messages into MGS Integral. |
| `cdecc.py` | EDC and ECC for raw CD-ROM Mode 2 Form 1 sectors. |
| `discaudit.py` | Every file on every disc: size, Integral-vs-USA delta, and does it hold text? |
| `dumpjp.py` | Dump every Japanese line on the Integral discs, exactly as the game draws it. |
| `gcldec.py` | A GCL value decoder, written from FoxdieTeam/mgs_reversing source/libgcl. |
| `gcldump.py` | Dump a stage script's command tree with decoded values. |
| `gclparse.py` | Top-down GCL script parser, per FoxdieTeam/mgs_reversing source/libgcl. |
| `gclprocs.py` | Dump every proc of a stage script (GCL_LoadScript layout: BE32 proclen, proc table of (id:BE16, offset:BE16) ending in a zero word, then proc bodies, then th... |
| `glyphfill.py` | Merge a glyph transcription into work/glyphs-to-identify.tsv, and score it. |
| `glyphocr.py` | Identify the game's 12x12 glyph bitmaps by matching them against a real font. |
| `glyphreview.py` | Print the FULL lines a glyph appears in, with everything else decoded. |
| `glyphsheets.py` | Render every unidentified game glyph for transcription, with its contexts. |
| `hazards.py` | Find MIPS I load-delay hazards in hand-written code. |
| `iso.py` | (no module docstring) |
| `items.py` | Port MGS1 (USA) English item/weapon descriptions into MGS Integral disc 1 & 2. |
| `jplist.py` | Dump EVERY untranslated Japanese string on all three Integral discs. |
| `jpremain.py` | What Japanese is still on the three DEPLOYED Integral discs, and where? |
| `jpsweep.py` | Sweep disc 1 for UI text still Japanese that USA has in English. |
| `jptext.py` | Turn the game's font codes into Japanese you can read. |
| `kcplace.py` | Allocate VRAM and CLUT slots for USA's eight KEY CONFIG labels inside Integral's option stage. |
| `kcquads.py` | Extract the KEY CONFIG label quads from an option overlay. |
| `kcrects.py` | Extract the four KEY CONFIG label rectangles, per button type, from the option overlay's per-type function (opt.c ~line 778: it rewrites poly[13..16] every f... |
| `langdefault.py` | Make English the power-on default, by rewriting one function. |
| `m2archive.py` | Read the collection's own data archive, and its CD-ROM patches. |
| `mainsweep.py` | Is any main-disc UI text still Japanese where the USA release has English? |
| `measure.py` | Measure the briefing labels from a screenshot: threshold the green text, group rows into bands, report each band's x extent and cap height. |
| `menu2.py` | Rebuild en_menu and en_menu2 from retail, preserving shipped payloads. |
| `menu3.py` | Port the `title` stage's copy of the disc-swap messages (`en_menu3`). |
| `mkimage.py` | Write a patched PSX disc image from a raw-variant patch set. |
| `optbright.py` | Historical brightness reconstruction; writes only WORK/legacy-optbright. |
| `optlabel2.py` | Rebuild the shipped option caption chain directly from retail. |
| `optscan.py` | Inspect a stage's tags, DAR entries and GCL chain in either build. |
| `optsctext.py` | Option -> SCREEN: draw USA's sc_text texture, making the brightness paragraph pixel-exact instead of re-wrapped font text. |
| `overlaydiff.py` | English that USA's copy of a stage overlay has and Integral's does not. |
| `pad2.py` | Port the controller-port message `second.c` draws - the last main-disc string that had a USA counterpart and no patch family. |
| `pcx4.py` | Encode/decode the 4-plane 1bpp RLE PCX the MGS1 texture loader expects (source/libdg/loader.c, PcxInflate4; PCX_RLE_CODE = 0xC0). |
| `portio.py` | Shared, read-only disc access and deterministic PPF/stage serialization. |
| `ppfcheck.py` | Parse a PPF3 the way Ketchup does, and refuse anything it would choke on. |
| `ppfgen.py` | Emit a PPF3 for a modified STAGE.DIR against a retail Integral disc image. |
| `preope_both.py` | Both recaps in English, in one grown `preope` stage. |
| `preope_usa.py` | Build Previous Operations from retail Integral and USA, without a baseline. |
| `quadscan.py` | Report brf_800C983C(prim, tex_id, poly, xl, yt, xr, yb, ...) arguments, labelling each call by the resource-name string loaded just before it. |
| `radiomap.py` | Map RADIO.DAT: where every fragment starts, and where its bank-1 font is. |
| `radiotext.py` | Pull every subtitle out of RADIO.DAT by walking the game's own records. |
| `rawdisc.py` | Recompute the EDC/ECC tail of every sector a raw-disc patch set touches. |
| `rebuild.py` | Clean collection-patch build and local packaging; never writes to the game. |
| `reloc_ppf.py` | Emit a PPF that parks a rebuilt stage in DUMMY3M.DAT and repoints its STAGE.DIR entry. |
| `rendertext.py` | Read the game's own Japanese by drawing it with the game's own font. |
| `rowargs.py` | Linear register simulation over a brf overlay, reporting the arguments of every call to the row positioner. |
| `rows.py` | Report the right-hand submenu rows: top edge of each band and the gap between consecutive tops (the row pitch). |
| `savemsg.py` | Port MGS1 (USA)'s memory-card messages into MGS Integral's executable. |
| `selftest.py` | Tests for the parts of the toolchain that need no game data. |
| `shotcmp_brightness.py` | Measure the Option -> SCREEN brightness paragraph in one or two screenshots. |
| `unlock_title.py` | Force every title-screen unlock: a test aid, not part of the English port. |
| `verify_integral_option.py` | Read the DEPLOYED Integral option PPFs back and prove what they install. |
| `verify_patch_options.py` | Apply the native INI planner's four language/grenade plans to real VR stages. |
| `verify_usa_brightness.py` | Static payload/texture check for the USA brightness fix. |
| `vr_camera.py` | Port the USA VR Missions ALBUM / SAVE PHOTO captions into Integral's VR `camera` overlay. |
| `vr_exe.py` | Port the USA VR Missions executable's English into Integral's VR-DISC executable. |
| `vr_grenade.py` | Correct Integral VR's grenade decal and briefing to USA's four-second delay. |
| `vr_kcgeom.py` | KEY CONFIG geometry of a VR-DISC option overlay, read from its code. |
| `vr_memcard.py` | Port the USA VR Missions memory-card captions into Integral's `vrsave` and `selectvr` overlays. |
| `vr_menus.py` | Port the VR-DISC title (EXTRA menu) and option-screen help lines. |
| `vr_movie.py` | Port the VR MOVIE selection captions from USA's VR disc. |
| `vr_option.py` | Port the USA VR Missions option screen into Integral's VR-DISC `option` stage. |
| `vr_sweep.py` | Is any VR-disc text still Japanese where USA has English? |
| `vr_unlock.py` | Unlock every VR mission: a test aid, not part of the English port. |
| `vr_unlock_extras.py` | Unlock the EXTRA menu's items: a test aid, not part of the English port. |
| `vr_unlock_movies.py` | Unlock the EXTRA -> MOVIE clips: a test aid, not part of the English port. |
| `vr_windows.py` | Port the USA VR Missions window text into Integral's VR-DISC stages. |
| `vrlib.py` | Shared code for the Integral VR-DISC (SLPM-86249) English port. |
| `widths.py` | How wide a ported line renders, and how wide it is allowed to be. |
| `workdir.py` | Resolve build inputs: explicit arguments, environment overrides, then discovery. |
