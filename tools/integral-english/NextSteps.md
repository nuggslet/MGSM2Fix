# Current work — Integral English patches

This is the current contributor handoff. Installation is in [README.md](README.md),
build commands in [BUILDING.md](BUILDING.md), and test scope in [COVERAGE.md](COVERAGE.md).
The dated session record and prior local deployment details are preserved in
[HANDOFF-ARCHIVE.md](HANDOFF-ARCHIVE.md) and [HISTORY.md](HISTORY.md). They do not
describe the current installed state of any machine.

## Current source state

- The branch already contains upstream commit `97172f5`; the old instruction to
  rebase from 3.6.0 is obsolete. Read `resource.h` for the ASI version and use
  `git log`/`git merge-base` to assess any future upstream changes.
- `rebuild.py` is the supported asset builder. It builds from original inputs
  into a new directory, without installing anything. Collection packages include
  the English sets, both grenade addons, and matching companions. Raw packages
  include their correction and sector parity.
- Package contents and counts come from the generated README, build report,
  and SHA256SUMS. A historical `reproNN` artifact is not the current release.
- Optional unlocks are off in the committed INI. Actual game settings are local
  state; inspect the user's installation rather than assuming a recorded state.

## Maintenance validation recorded 2026-09-12

Release x64 and x86 ASIs built successfully. All 52 data-free Python tests and
the native patch/range/RAM tests passed. A clean collection build produced its
English assets and both grenade layouts; all four English/grenade plans passed
against the real VR stages, including exact non-digit bytes and decoded textures.
Both USA discs passed the static brightness payload/texture check. The package
manifest, ZIP contents/integrity, and recorded source hashes were verified.

The tested collection ZIP has SHA-256
`a41a447222fb6fa2cc81c5737864ff20f5a9b0722178756ea784649f80a2248a`.
This identifies that test artifact, not a future release. No patches or ASIs
were installed into the game, and no gameplay or new raw-disc build was tested.

## Gameplay testing requested — still outstanding

The user will seek testers. Do not turn the following into completed checks
based on a successful build or static comparison:

- Play the real story transition onto disc 2; inspect disc-swap prompts and
  continued text behavior. Direct stage selection is not an equivalent test.
- Check the Psycho Mantis controller-port subtitle with a second controller.
- Inspect the four MISSION LOG location-name spellings on screen.
- Check VR mission RESULT, save/load, and PHOTOGRAPHING memory-card messages.
- Exercise the complete in-game English/grenade and optional-unlock toggle
  matrix, including restarts, and the Japanese grenade visual check.
- Keep the recorded raw-disc-specific visual gaps in the historical coverage
  record open until an actual test supplies evidence.
- After the scoped brightness filter and RAM repair changes, regress USA
  brightness modes and Integral text behavior with collection patches enabled.

For each report record source commit, package hashes, game version, disc,
collection/raw variant, relevant INI settings, exact route, and observed result.
Record fresh-boot and saved-game behavior separately when relevant. Do not
require testers to delete their saves.

## Maintenance rules

Preserve the port's scope: copy existing English counterparts without shortening
them; retain Japanese where no counterpart exists. Preserve original evidence
when correcting historical claims. Keep general MGSM2Fix changes documented in
[UPSTREAM.md](../../UPSTREAM.md), separate from the game-data build toolchain.
Run the Python and native tests before packaging. Verify packages against real
disc data using the commands in BUILDING.md. Do not infer release readiness from
file counts, matching hashes, or a tool's own success message alone.

There is no pending instruction to push, publish, deploy, or submit an upstream
PR. Those actions require the user's task authorization. Local work paths are
explicit environment/CLI settings, never public defaults.
