# Metal Gear Solid Master Collection Fix (M2Fix)
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z7S6BTK)</br>
[![MGSM2Fix](https://github.com/nuggslet/MGSM2Fix/actions/workflows/ci.yml/badge.svg)](https://github.com/nuggslet/MGSM2Fix/actions/workflows/ci.yml)[![Github All Releases](https://img.shields.io/github/downloads/nuggslet/MGSM2Fix/total.svg)](https://github.com/nuggslet/MGSM2Fix/releases)

This is a fix that adds custom resolutions, mod support and more to Metal Gear Solid & Metal Gear / Snake's Revenge within the MGS Master Collection, and many more M2ENGAGE titles.<br />

## Games Supported
- Metal Gear Solid
- Metal Gear / Snake's Revenge (Vol.1 Bonus Content)
- Contra Anniversary Collection
- Castlevania Anniversary Collection
- Castlevania Advance Collection
- Castlevania Dominus Collection
- Ray’z Arcade Chronology
- Darius Cozmic Collection Arcade
- G-Darius HD

See Lyall's [MGSHDFix](https://github.com/ShizCalev/MGSHDFix) for the other games in the MGS Master Collection.

## Features
- Custom internal render resolution & widescreen support (MGS 1).
- Borderless/windowed mode.
- Corrects the monitor going to sleep during long periods with no input (e.g. during cutscenes).
- Control over Master Collection game patches.
- ~~Analog input (MGS 1).~~ - Fixed by Konami officially via patch 1.5.0 on 13th March 2024.
- Launcher skip (MGS 1, boots last launched game version).
- Skip intro logos.
- Modding support (MGS 1, via Ketchup - see below).
- Debug features (including stage select menu in MGS 1).

## Installation
- Grab the latest release of MGSM2Fix from [here](https://github.com/nuggslet/MGSM2Fix/releases).
- Extract the contents of the release into the game folder.<br />(e.g. "**steamapps\common\MGS1**" or "**steamapps\common\MGS Master Collection Bonus Content**" for Steam games).

### Steam Deck/Linux additional instructions
- Open up the Steam game properties and put `WINEDLLOVERRIDES="dinput8=n,b;d3d11=n,b" %command%` in the launch options.

## Configuration
- See **MGSM2Fix.ini** to adjust settings for the fix.

## Modding (MGS 1; Ketchup)
'Ketchup' is a mod loader for MGS 1 in the Master Collection.

It currently supports PPF3 format mods to each ISO under the following folders in the "**steamapps\common\MGS1**" directory:
```
  mods\INTEGRAL\INTEGRAL\0\
  mods\INTEGRAL\INTEGRAL\1\
  mods\INTEGRAL\VR-DISK\
  mods\VR-DISK_US\
  mods\VR-DISK_EU\
  mods\MGS1_JP\0\
  mods\MGS1_JP\1\
  mods\MGS1_US\0\
  mods\MGS1_US\1\
  mods\MGS1_UK\0\
  mods\MGS1_UK\1\
  mods\MGS1_DE\0\
  mods\MGS1_DE\1\
  mods\MGS1_FR\0\
  mods\MGS1_FR\1\
  mods\MGS1_IT\0\
  mods\MGS1_IT\1\
  mods\MGS1_ES\0\
  mods\MGS1_ES\1\
```
Where `0` and `1` refer to disk 1 and disk 2 respectively.

See [makeppf](https://github.com/meunierd/ppf) for creating PPF3 patches/mods. PPF3 mods derived from original PSX CD releases should work correctly with Master Collection.

If your mods conflict with the built-in Master Collection patches, for the time being it may be useful to enable the `DisableRAM` and `DisableCDROM` settings in **MGSM2Fix.ini**.
Once all of the Master Collection patches have been identified and grouped (please help, there are lots!) this heavy-handed approach should no longer be necessary.

Additional mod formats may be supported in future.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

## Screenshots

| ![MGS 1](https://github.com/user-attachments/assets/e8c0f73b-24df-4264-a86a-0f20a87e3dd8) <br /> ![MGS 1](https://github.com/user-attachments/assets/65d14662-95f4-49e4-9aad-2f2cdeaaaa06) |
|:--:|
| Metal Gear Solid |

## Credits
Many thanks to [@Lyall](https://github.com/Lyall) and co. for MGSHDFix, from which this project derives its general structure; <br />
[@ShizCalev/Afevis](https://github.com/shizcalev), [@Bud11](https://github.com/bud11) and [@orzcode](https://github.com/orzcode) for contributing fixes and being generally helpful; <br />
Countless people in the MGN Discord for testing and encouraging the project along; <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading; <br />
[inipp](https://github.com/mcmtroffaes/inipp) for **MGSM2Fix.ini** reading; <br />
[spdlog](https://github.com/gabime/spdlog) for logging; <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking; <br />
[Squirrel](http://squirrel-lang.org/) for one at least two virtual machines this project tinkers with; <br />
[Sqrat](https://scrat.sourceforge.net/index.html) for Squirrel bindings in C++; <br />
[FunctionTraits](https://github.com/HexadigmSystems/FunctionTraits) for improving the safetyhook calling convention experience.
