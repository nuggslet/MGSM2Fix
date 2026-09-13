# Coverage and verification limits

The port copies existing USA English counterparts into Integral's story and VR
menus, screens, and executable strings. Integral-exclusive text with no English
source remains Japanese. It is not a full translation of commentary/narration.

Current patch filenames and counts are generated in each package README and
build report. Historical counts are not a release manifest.

| Area | Evidence and limits |
|---|---|
| Story English assets | Structural/byte checks and recorded screen comparisons; the real disc transition, controller-port subtitle, and location-name visual checks remain open |
| VR English assets | Structural checks and recorded comparisons for options, KEY CONFIG, missions, descriptions and MOVIE screens; RESULT and specified memory-card flows remain open |
| Grenade correction | Source texture/payload checks and four static INI plans; full in-game toggle matrix and Japanese visual confirmation remain open |
| USA brightness | Static payload/texture check and historical disc-1 observations; compiled routing and current filter changes require runtime regression |
| Raw discs | Sector EDC/ECC checks and historical emulator observations; retain the raw-specific visual gaps recorded in the history |
| RAM repair | Native synthetic tests cover interior corruption and last-writer precedence; actual recurring collection writes still need gameplay regression |

See [NextSteps.md](NextSteps.md) for the tester checklist. A successful build,
matching disc-1/disc-2 bytes, or unchanged patch count does not close a game-flow
test. No new gameplay verification is claimed by this maintenance update.

The detailed inventories, scanner limitations, corrections, and source evidence
are preserved in [COVERAGE-RECORD.md](COVERAGE-RECORD.md), [REFERENCE.md](REFERENCE.md),
and [HISTORY.md](HISTORY.md). Dated results there apply to their recorded builds.
