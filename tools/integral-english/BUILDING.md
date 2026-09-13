# Build and package the Integral patches

Run commands from `tools/integral-english` in PowerShell unless stated otherwise.
The builder writes only into a new output directory. Gameplay testing is separate.

## Prerequisites

- Master Collection MGS1 with Integral DLC: `windata/alldata.bin` and
  `windata/dlc/dlc_japan.bin`.
- Five retail executables in one directory: `int1.exe`, `int2.exe`, `us1.exe`,
  `us2.exe`, and `vrus.exe` (USA VR `SLUS-00957`). The collection's zero-filled
  ISO executable extents are not usable substitutes.
- The FoxdieTeam MGS decomp checkout containing commit `7964de7`, and the PSY-Q
  SDK tree accepted by that revision's `build/build.py --psyq_path`.
- Python 3.12+, Git, and packages from `requirements.txt` in this directory.
- For ASI/native tests: Visual Studio C++ build tools, initialized submodules,
  and root `requirements.txt`. CI uses toolset v143; local project defaults are
  inherited from upstream. Pass `/p:PlatformToolset=v143` if that is installed.

```powershell
git submodule update --init --recursive
py -m pip install -r requirements.txt
```

Supported executable SHA-256 values:

| Files | SHA-256 |
|---|---|
| `int1.exe`, `int2.exe` | `4b8252b65953a02021486406cfcdca1c7670d1d1a8f3cf6e750ef6e360dc3a2f` |
| `us1.exe`, `us2.exe` | `615e136083336957ed0b9b3805145bf5bbb35f7a16c2f160dba8f17bb71cc640` |
| `vrus.exe` | `8e8e59a97b5cc7cec137dd782fdeaa09097de1e53b1801c5617aa9132a2fb814` |

The Integral VR executable is built from the decomp and verified against the
hash in `rebuild.py`. All supplied executables are checked before compilation.

## Configure paths

`INTEGRAL_ENGLISH_WORK` names an existing root holding `work/`, not `work/`
itself. Without it, tools use `cwd/work`. Set it for standalone diagnostic tools.
The game is discovered via Steam; the decomp is searched for beside this repo.
Explicit invalid paths fail instead of silently selecting another installation.

```powershell
$env:INTEGRAL_ENGLISH_WORK = 'C:/mgs-work'
$env:INTEGRAL_ENGLISH_GAME = 'C:/Games/Steam/steamapps/common/MGS1'
$env:INTEGRAL_ENGLISH_DECOMP = 'C:/mgs-decomp'
py workdir.py
```

Replace these examples with existing local directories. The SDK defaults to
`<decomp>/../psyq`; use `--psyq` otherwise. Output must be a fresh path without
spaces and at most 65 characters because of the PSY-Q toolchain.

## Build assets

```powershell
py rebuild.py --output C:/mgs-build/collection --executables C:/mgs-inputs --psyq C:/psyq
py rebuild.py --output C:/mgs-build/raw --executables C:/mgs-inputs --psyq C:/psyq --variant raw
```

`--game` and `--decomp` override discovery. `--compare-deployed` is optional
for collection builds: it compares the generated English PPFs with the same
filenames in the installation. It is not needed for a clean build and is refused
for raw builds. It does not certify the state of arbitrary additional mods.

The collection package includes both grenade layouts built against its own
mission PPF. No installed mission PPF is used as a build input. JSON companions
are generated from the packaged bytes. English patch ownership is checked;
grenade addons are alternatives and must not be applied together.

Outputs are `Integral-English-<variant>.zip`, `package/`, `build-report.json`,
logs, and retained inputs. The generated package README lists the actual PPFs
and counts; SHA256SUMS covers the packaged files including JSON companions.
The report records input, source and SDK hashes. Build environment details can
make ZIP hashes differ even when effective patch bytes are equal.

## Build the ASI and run tests

From a Visual Studio developer command prompt at the repository root:

```bat
py -m pip install -r requirements.txt
msbuild MGSM2Fix.sln /p:Configuration=Release /p:Platform=x64 /p:PostBuildEventUseInBuild=false
tools\integral-english\run_native_tests.cmd
```

Disabling the development post-build installer makes this a build-only command.
The ASI is `x64/Release/MGSM2Fix.asi`. Install with an ASI loader; upstream's
local M2Package script is not a complete first-install loader package.

From this tool directory:

```powershell
py selftest.py
py -m compileall -q .
py verify_patch_options.py ../../x64/tests/patch_options.exe C:/mgs-build/collection/package/mods/INTEGRAL/VR-DISK
py rawdisc.py C:/mgs-build/raw/package
```

Native tests cover synthetic patch selection/metadata, scoped ranges and RAM
repair without game data. The real-data verifier checks all English/grenade
plans against the original VR stages. Optional unlock selection is tested when
those assets are supplied. CI runs the data-free checks and gates publication
on their success.

`verify_usa_brightness.py` checks that the source-declared payload occurs in a
built ASI and decodes correctly at source-declared offsets. It does not inspect
the compiled destination table or prove runtime filtering. Set
`INTEGRAL_ENGLISH_ASI` to select an exact binary; invalid paths are rejected.

`optbright.py` is a historical reconstruction tool. It writes only into
`WORK/legacy-optbright/`; its output is not an input to the supported build.
Historical runs and low-level recipes remain in [BUILD-HISTORY.md](BUILD-HISTORY.md).
