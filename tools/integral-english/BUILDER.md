# M2FixGUI

Double-click `M2FixGUI.exe`. It opens **M2Fix Settings** by default.
Use **Open Patch & Disc Builder** for the separate build window.

**Appearance** defaults to **System**, following Windows' current app light/dark
setting, including changes while the app is open. Choose **Light** or **Dark**
to override it for both windows. This preference is remembered in HKCU\Software\M2FixGUI and does
not change `MGSM2Fix.ini`.

## Edit M2Fix settings

The app first looks for `MGSM2Fix.ini` beside its EXE, then in the detected Steam
MGS1 folder. If neither exists, it loads the embedded release defaults. Use
**Open INI...** to choose another file; the full path is displayed so you can
verify which file you are editing. Use **Save As...** to create a new INI.

Choose a section at the left, select a feature, and edit its value below the list.
The selected feature shows its INI help comments. **Reset to release defaults**
stages the distributed defaults in the current document; nothing is written
until **Save settings** or **Save As**.

All active settings in the release INI are exposed, plus the optional Windows
slideshow-warning switch. Missing entries are shown as not set and stay absent
unless edited. Unknown entries are preserved. Saves retain comments, ordering,
line endings and untouched values, create a numbered `.bak` backup, and detect
external file changes. Unsaved edits prompt before closing or switching files.
Restart the game after saving. Enabling PPF controls does not install patches.

Enabled features with missing or empty installed PPFs (including required JSON
companions) show a **Patch files needed** notice. It checks `mods` beside the
selected INI, updates when settings change and rechecks automatically after
installation. The notice opens the builder in PPF mode and lists missing paths.
Save a new INI in the game folder before checking its installation. Optional
unlock PPFs are also reported, but are not included in this builder. These checks
detect missing files, not their contents or compatibility.

## Build patches or disc images

No terminal, Python, Git, SDK, decomp checkout,
previously built PPFs, or manually extracted PlayStation executables are needed.

1. Choose **Steam installation** or **Redump images**.
2. Browse to your MGS1 installation (with Integral DLC), or a folder containing
   unpatched Integral BINs or ZIPs. Steam is detected when possible; every path
   can be selected manually. No developer's directory layout is assumed.
3. Choose **Disc images (BIN/CUE)** or **M2Fix patch package (PPF)**.
   Images can include disc 1, disc 2, the VR disc, or all three. A complete patch
   package requires all three Integral discs. No USA Redump discs are needed.
4. Select the checkboxes for **Story English**, **VR English**, and the
   **grenade display correction**. Disc images also have **Start in English**.
5. Choose an existing **Save to** folder for the built package. This is where the
   package is created; it is separate from the game installation folder.
6. For PPF builds, optionally check **Install into MGS1 after building**. The
   **Install to** field is prefilled with the detected game folder and can be
   changed with **Browse**. If checked, M2FixGUI verifies and installs the built
   PPFs with their JSON companions after the build succeeds. Existing matching
   files are backed up in `M2FixGUI-patch-backup-*` inside the game folder;
   unrelated or unchecked patches and INI settings are preserved. Close the game
   first, then restart it after installation. If unchecked, the package remains
   available for manual installation. Disc-image builds do not install PPFs.

English text is selected as complete story/VR groups, matching the supported
M2Fix controls. These groups include the associated menus, options, descriptions,
save messages and briefing text. The grenade correction changes displayed timing,
not gameplay. Without VR English, the builder uses the original Japanese-layout
grenade patch automatically. Sector parity is recalculated for the exact selected
image patches. English VR briefing text itself already describes the USA four-second
timing; disabling the separate grenade addon does not rewrite the English translation.

Patch packages only require the discs used by checked groups: story discs 1/2 for
Story English, and the VR disc for VR English or the grenade correction. At least
one group must be checked. Disc builds can leave everything unchecked to extract
an unmodified image. Start in English is disabled for PPF packages because the
collection's `EnglishText` INI setting controls that behavior.

Unchecked PPFs and their companions are omitted from generated packages. When
replacing an installed package, remove the previous files using its manifest;
the builder does not remove already installed patches. Each output records the
selected patches, and collection manifests cover only the generated selection.

The destination can contain spaces. Intermediate files are managed automatically
and removed when the operation ends. Allow roughly 6 GB free for an all-disc
image build, plus temporary space for unpacking one ZIP. Original inputs and the
Steam installation are never changed. Existing outputs are never overwritten.

## Outputs

Image builds create `Integral-English-images` when an English patch or English
startup is selected; Japanese-only builds use `Integral-images`. Each contains
one BIN/CUE pair per selected disc. By default they include the English text port,
English at startup, the grenade display correction, and corrected sector parity. They
include no unlock cheats. Integral-only text without a counterpart can remain
Japanese; see [COVERAGE.md](COVERAGE.md).

Patch builds create `Integral-English-patches` when Story or VR English is selected,
or `Integral-patches` for Japanese-only/grenade-only selections, containing:

- `mods/INTEGRAL/INTEGRAL/0`: disc 1 PPFs
- `mods/INTEGRAL/INTEGRAL/1`: disc 2 PPFs
- `mods/INTEGRAL/VR-DISK`: VR PPFs and JSON companions
- `Integral-English-collection.zip` (or `Integral-collection.zip` for Japanese-only selections): the same installable package
- README, build provenance and SHA-256 manifest

Copy the package's `mods` folder into MGS1 to install it manually. PPF builds can
also install automatically when **Install into MGS1 after building** is checked.
With VR English and the grenade fix enabled, patch packages include both
Japanese- and English-layout grenade addons; M2Fix selects the matching layout.

## Validation and supported sources

The tool checks complete unpatched disc SHA-256 hashes before generating any
patches. Steam's washed disc images are restored using binary differences
against its startup RAM snapshots. Each restored boot program and complete disc
must match the original retail hash. Unsupported snapshots or modified discs
are refused. Every generated package file is checked against the release hash;
image creation additionally verifies touched sectors before and after patching.

All selected outputs are published together only after verification succeeds.
Failures preserve a diagnostic log, not a seemingly complete output folder.
This verifies file construction, not gameplay compatibility on every emulator.

## Release preparation (maintainers only)

`release-recipes.zip` contains binary differences, PPF structure, metadata and
hashes. It does not contain complete disc images or complete retail executable
files. PPF payloads are stored as XOR differences against original disc bytes;
boot restoration uses differences against Steam RAM snapshots. These are patch
assets derived from the existing port, including its compiled overlay changes.
See [CREDITS.md](CREDITS.md) for provenance and the unresolved upstream license.

The existing `rebuild.py` remains the developer workflow. To refresh recipes:

1. Build and verify collection and raw packages using [BUILDING.md](BUILDING.md).
2. Run `prepare_release.py` with `--game`, `--redump` (three extracted Integral
   BINs), `--collection` and `--raw` (build directories), and `--output` (a new
   recipe ZIP path).
3. Validate the candidate recipes against both source routes, replace the
   committed `release-recipes.zip`, then package the EXE.

The release is a native C++17 Windows GUI, with no Python or Tk runtime. Recipe
assets are losslessly compressed with Windows LZMS using a 16 MiB block; the
build verifies an exact decompression round trip before embedding them.

From the repository root on Windows x64 with Visual Studio C++ Build Tools,
CMake, and Python (build-time resource preparation only):

```powershell
tools\M2FixGUI\build.cmd
```

The output is `dist/M2FixGUI.exe`, a GUI-only single file. CI and M2Package
include it in release archives. Internal worker arguments support automated
verification; users never need them. Building the EXE from committed recipes
does not require game data or the PlayStation compiler.
