# Integral English patches

This port brings existing USA English text into Integral's story and VR discs.
Integral-only material without an English counterpart stays Japanese. This is
a text port, not a complete translation. The patch files are built from the
user's game data; the source checkout does not contain ready-to-install PPFs.

## Install the collection version

1. Obtain an MGSM2Fix build carrying the patch controls, and the matching
   `Integral-English-collection.zip`. The controls are in upstream `master`;
   the current tagged releases predate them, so until the next release use a
   CI build of `master` or build it yourself. The patch ZIP is never
   distributed — build it from your own game data with [BUILDING.md](BUILDING.md).
2. Install the ASI loader and the ASI as described in the root
   [installation instructions](../../README.md#installation). If using CI,
   its artifact includes the loader. A local ASI build needs a loader installed.
3. Extract the patch ZIP's `mods` folder into the Master Collection MGS1 folder.
   Include every `.ppf.json` companion. The package contains both grenade
   texture/text layouts; the loader selects the appropriate one automatically.
4. Keep `EnglishText`, `IntegralEnglishPatch`, `IntegralVREnglishPatch`, and
   `GrenadeDelayFix` enabled for the default English experience. Keep
   `DisableRAM` and `DisableCDROM` false. Restart after changing patch controls.

The package's README and SHA256SUMS identify its exact contents. Use one complete
build per disc. Before upgrading, remove the previous package's files using its
manifest, then extract the new package; retain unrelated mods and your INI.
Do not combine collection and raw-disc packages or older experimental PPFs.
To uninstall, remove only the files listed by that package's manifest. Installing
or removing these text patches does not require deleting saves.

## Controls and failure messages

| Control | Effect |
|---|---|
| `EnglishText` | Select Integral's existing English language option; separate from PPF selection |
| `IntegralEnglishPatch` / `IntegralVREnglishPatch` | Enable installed story / VR English assets |
| `GrenadeDelayFix` | Correct the displayed delay in briefings and the grenade decal; does not alter gameplay timing |
| `BrightnessText` | USA story-disc brightness help: `fixed`, `original`, or `collection`; Integral uses its own English option patch |
| Unlock controls | Optional test aids, off by default; their assets are not part of the English package |

If a mission companion is missing or stale, the loader skips that mission PPF
and reports it in `MGSM2Fix.log`; other VR English screens remain enabled.
Reinstall the matching PPF/JSON pair. Invalid grenade addons are skipped with a
separate log message. Do not rename controlled PPFs: their filenames identify
their layout and INI setting.

## Raw PlayStation discs

Build with `--variant raw` and follow `PACKAGE-README-raw.txt` in the resulting
package. It includes the English grenade correction and sector EDC/ECC. It is
not an ASI/INI-controlled collection package. Never install its patches into
the collection's mods folder.

[mkimage.py](mkimage.py) writes the patched image from that package, taking the
disc either from a Redump dump (`--redump`) or from a Master Collection
installation (`--collection`, which also needs that disc's own executable,
since the collection zero-fills the extent). Both sources have been measured to
produce the same image; `PACKAGE-README-raw.txt` covers both.

## Project information

- [Build and package](BUILDING.md)
- [Current work and testing needs](NextSteps.md)
- [Coverage and verification limits](COVERAGE.md)
- [Tool index](SCRIPTS.md)
- [Technical reference](REFERENCE.md), [history](HISTORY.md), and [credits](CREDITS.md)

The user is seeking gameplay testers. Static validation is not a claim that the
remaining game flows have been tested.
