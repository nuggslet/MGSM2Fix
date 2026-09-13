# Upstream PR allocation and final checks

Reviewed 2026-09-13 against fetched upstream/master `97172f5`. The original contribution had
189 commits beyond that base and no missing upstream commits. The allocation
below includes the complete contribution, review fixes, and extracted native tests.
Generated PPFs/ZIPs are not tracked; include the built distributions with the
patch contribution rather than assuming a source PR contains those artifacts.

## Eight PRs, complete scope

| PR | Scope | Dependencies |
|---|---|---|
| 1 | Zydis toolset propagation and cache invalidation | Independent |
| 2 | Ketchup PPF validation, ordering, overlap reports, deferred RAM mirroring and repair | Independent; carry its own native tests |
| 3 | Configuration preservation and Integral language selection | Carry required RAM hooks with this PR |
| 4 | USA brightness, built-in disc patches and scoped filtering | Prefer after 2; carry brightness test dependencies |
| 5 | Thin-polygon rendering correction | Independent; claim only implemented MGS1 x64 support |
| 6 | Integral story/VR English, grenade correction and controls, all builders/analysis tools, raw support, packaging, CI, provenance and complete historical/current documentation | After 2–4; include the shared patch planner here |
| 7 | Optional unlock assets/controls, stage selection, item and weapon grants | After 3 and 6 |
| 8 | Optional diagnostics and debugger tooling, including remaining watches/tracing and RAM audit | After the mechanisms it observes, chiefly 2–4 |

The former separate English/grenade PR boundary is unsafe as a file-only split:
`rebuild.py`, `selftest.py`, and the English mission builder already consume
grenade functions. Keeping them in PR 6 preserves a coherent patch package.
Testing aids and diagnostics get separate reviews instead. Nothing is excluded.

Shared files are split by feature hunks. Standalone RAM tests arrive with PR 2
and range tests with PR 4; neither depends on the Integral toolchain in PR 6.
CI jobs arrive with their scripts. Earlier feature settings are documented in
the INI; the complete current and historical documentation arrives in PR 6.

## Readiness

- Fetched upstream and verified the branch is current with that base.
- Re-ran all 52 Python tests and native tests successfully.
- Fixed the remaining Zydis cache omission; verified rebuilding the old stamp
  and skipping the subsequent identical-toolset invocation.
- Prior full-tree Release x64/x86 builds and real-disc static patch/brightness
  validation are recorded in `tools/integral-english/NextSteps.md`.
- Gameplay gaps remain intentionally open for testers. A fresh raw-disc build
  was not part of the maintenance validation.
- All eight branches were extracted and passed Release x64/x86 builds with
  the development installer disabled. Each available native test suite passed;
  PRs 6 through 8 also passed all 52 Python self-tests.
- The final stack tree exactly matches the complete source snapshot. Nothing
  from the contribution was excluded. The inventory below records actual diffs.
- PR descriptions provide the submission status, links and merge order. Each
  draft targets upstream master, so later diffs are cumulative until earlier
  steps land; use the linked incremental comparisons for focused review.

## File inventory

128 changed/new files covered. Numbers refer to the PRs above.

| File | PR allocation |
|---|---|
| `.gitattributes` | 6 |
| `.github/workflows/ci.yml` | 1, 2, 6 |
| `.gitignore` | 6 |
| `MGSM2Fix.ini` | 3, 4, 5, 6, 7 |
| `MGSM2Fix.vcxproj` | 1, 2, 4, 6 |
| `MGSM2Fix.vcxproj.filters` | 1, 2, 4, 6 |
| `README.md` | 6 |
| `UPSTREAM-PRS.md` | 6 |
| `UPSTREAM.md` | 6 |
| `build_zydis.cmd` | 1 |
| `src/games/mgs1.cpp` | 3, 7 |
| `src/games/mgs1.h` | 3, 4, 7, 8 |
| `src/games/mgs1_patch_options.h` | 6 |
| `src/m2fix/ketchup.cpp` | 2, 4, 6, 8 |
| `src/m2fix/ketchup.h` | 2, 4, 6, 8 |
| `src/m2fix/m2config.cpp` | 3, 4, 5, 6, 7 |
| `src/m2fix/m2config.h` | 3, 4, 5, 6, 7 |
| `src/m2fix/m2game.h` | 3, 4, 8 |
| `src/m2fix/patch_range.h` | 4 |
| `src/m2fix/ram_patch.h` | 2 |
| `src/machines/psx.cpp` | 5 |
| `src/machines/psx.h` | 5 |
| `src/modules/sqhook.cpp` | 2, 3, 4, 8 |
| `src/modules/sqhook.h` | 3, 4, 8 |
| `tests/run_native_tests.cmd` | 2 |
| `tests/test_patch_range.cpp` | 4 |
| `tests/test_ram_patch.cpp` | 2 |
| `tools/integral-english/BUILD-HISTORY.md` | 6 |
| `tools/integral-english/BUILDING.md` | 6 |
| `tools/integral-english/COVERAGE-RECORD.md` | 6 |
| `tools/integral-english/COVERAGE.md` | 6 |
| `tools/integral-english/CREDITS.md` | 6 |
| `tools/integral-english/HANDOFF-ARCHIVE.md` | 6 |
| `tools/integral-english/HISTORY.md` | 6 |
| `tools/integral-english/NextSteps.md` | 6 |
| `tools/integral-english/PACKAGE-README-raw.txt` | 6 |
| `tools/integral-english/PACKAGE-README.txt` | 6 |
| `tools/integral-english/README.md` | 6 |
| `tools/integral-english/REFERENCE.md` | 6 |
| `tools/integral-english/SCRIPTS.md` | 6 |
| `tools/integral-english/abst_build.py` | 6 |
| `tools/integral-english/abstscan.py` | 6 |
| `tools/integral-english/align.py` | 6 |
| `tools/integral-english/audit_text.py` | 6 |
| `tools/integral-english/bank1-glyphs.tsv` | 6 |
| `tools/integral-english/brf_build.py` | 6 |
| `tools/integral-english/brf_quads.json` | 6 |
| `tools/integral-english/brf_quads_all.json` | 6 |
| `tools/integral-english/brf_widen.py` | 6 |
| `tools/integral-english/bridge.py` | 8 |
| `tools/integral-english/camsave.py` | 6 |
| `tools/integral-english/cdecc.py` | 6 |
| `tools/integral-english/decomp-overlay-changes.patch` | 6 |
| `tools/integral-english/discaudit.py` | 6 |
| `tools/integral-english/dumpjp.py` | 6 |
| `tools/integral-english/gcldec.py` | 6 |
| `tools/integral-english/gcldump.py` | 6 |
| `tools/integral-english/gclparse.py` | 6 |
| `tools/integral-english/gclprocs.py` | 6 |
| `tools/integral-english/gen_scripts_index.py` | 6 |
| `tools/integral-english/glyphfill.py` | 6 |
| `tools/integral-english/glyphocr.py` | 6 |
| `tools/integral-english/glyphreview.py` | 6 |
| `tools/integral-english/glyphsheets.py` | 6 |
| `tools/integral-english/hazards.py` | 6 |
| `tools/integral-english/iso.py` | 6 |
| `tools/integral-english/items.py` | 6 |
| `tools/integral-english/jplist.py` | 6 |
| `tools/integral-english/jpremain.py` | 6 |
| `tools/integral-english/jpsweep.py` | 6 |
| `tools/integral-english/jptext.py` | 6 |
| `tools/integral-english/kcplace.py` | 6 |
| `tools/integral-english/kcquads.py` | 6 |
| `tools/integral-english/kcrects.py` | 6 |
| `tools/integral-english/langdefault.py` | 6 |
| `tools/integral-english/m2archive.py` | 6 |
| `tools/integral-english/mainsweep.py` | 6 |
| `tools/integral-english/measure.py` | 6 |
| `tools/integral-english/menu2.py` | 6 |
| `tools/integral-english/menu3.py` | 6 |
| `tools/integral-english/mkimage.py` | 6 |
| `tools/integral-english/optbright.py` | 6 |
| `tools/integral-english/optlabel2.py` | 6 |
| `tools/integral-english/optscan.py` | 6 |
| `tools/integral-english/optsctext.py` | 6 |
| `tools/integral-english/overlaydiff.py` | 6 |
| `tools/integral-english/pad2.py` | 6 |
| `tools/integral-english/pcx4.py` | 6 |
| `tools/integral-english/portio.py` | 6 |
| `tools/integral-english/ppfcheck.py` | 6 |
| `tools/integral-english/ppfgen.py` | 6 |
| `tools/integral-english/preope_both.py` | 6 |
| `tools/integral-english/preope_usa.py` | 6 |
| `tools/integral-english/quadscan.py` | 6 |
| `tools/integral-english/radiomap.py` | 6 |
| `tools/integral-english/radiotext.py` | 6 |
| `tools/integral-english/rawdisc.py` | 6 |
| `tools/integral-english/rebuild.py` | 6 |
| `tools/integral-english/reloc_ppf.py` | 6 |
| `tools/integral-english/rendertext.py` | 6 |
| `tools/integral-english/requirements.txt` | 6 |
| `tools/integral-english/rowargs.py` | 6 |
| `tools/integral-english/rows.py` | 6 |
| `tools/integral-english/run_native_tests.cmd` | 6 |
| `tools/integral-english/savemsg.py` | 6 |
| `tools/integral-english/selftest.py` | 6 |
| `tools/integral-english/shotcmp_brightness.py` | 6 |
| `tools/integral-english/test_patch_options.cpp` | 6 |
| `tools/integral-english/unlock_title.py` | 7 |
| `tools/integral-english/verify_integral_option.py` | 6 |
| `tools/integral-english/verify_patch_options.py` | 6 |
| `tools/integral-english/verify_usa_brightness.py` | 6 |
| `tools/integral-english/vr_camera.py` | 6 |
| `tools/integral-english/vr_exe.py` | 6 |
| `tools/integral-english/vr_grenade.py` | 6 |
| `tools/integral-english/vr_kcgeom.py` | 6 |
| `tools/integral-english/vr_memcard.py` | 6 |
| `tools/integral-english/vr_menus.py` | 6 |
| `tools/integral-english/vr_movie.py` | 6 |
| `tools/integral-english/vr_option.py` | 6 |
| `tools/integral-english/vr_sweep.py` | 6 |
| `tools/integral-english/vr_unlock.py` | 7 |
| `tools/integral-english/vr_unlock_extras.py` | 7 |
| `tools/integral-english/vr_unlock_movies.py` | 7 |
| `tools/integral-english/vr_windows.py` | 6 |
| `tools/integral-english/vrlib.py` | 6 |
| `tools/integral-english/widths.py` | 6 |
| `tools/integral-english/workdir.py` | 6 |
