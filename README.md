# Metal Gear Solid Master Collection Fix (M2Fix)
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z7S6BTK)</br>
[![MGSM2Fix](https://github.com/nuggslet/MGSM2Fix/actions/workflows/ci.yml/badge.svg)](https://github.com/nuggslet/MGSM2Fix/actions/workflows/ci.yml)[![Github All Releases](https://img.shields.io/github/downloads/nuggslet/MGSM2Fix/total.svg)](https://github.com/nuggslet/MGSM2Fix/releases)

This is a fix that adds custom resolutions, mod support and more to Metal Gear Solid & Metal Gear / Snake's Revenge within the MGS Master Collection, and many more M2ENGAGE titles.<br />

## Games Supported
- [Metal Gear Solid](https://store.steampowered.com/app/2131630/METAL_GEAR_SOLID__Master_Collection_Version/)
- [Metal Gear / Snake's Revenge (Vol.1 Bonus Content)](https://store.steampowered.com/app/2306740/METAL_GEAR_SOLID_MASTER_COLLECTION_Vol1_BONUS_CONTENT/)
- [Metal Gear Solid 4 (Metal Gear Solid flashback)](https://store.steampowered.com/app/2492670/METAL_GEAR_SOLID_4_Guns_of_the_Patriots__Master_Collection_Version/)
- [Metal Gear Solid: Ghost Babel (Vol.2 Bonus Content)](https://store.steampowered.com/app/3036720/METAL_GEAR_SOLID_MASTER_COLLECTION_Vol2_BONUS_CONTENT/)
- [Contra Anniversary Collection](https://store.steampowered.com/app/1018020/Contra_Anniversary_Collection/)
- Castlevania Anniversary Collection: [Steam](https://store.steampowered.com/app/1018010/Castlevania_Anniversary_Collection/) / [Epic Games](https://store.epicgames.com/p/castlevania-anniversary-collection-a61f94)
- [Castlevania Advance Collection](https://store.steampowered.com/app/1552550/Castlevania_Advance_Collection/)
- [Castlevania Dominus Collection](https://store.steampowered.com/app/2369900/Castlevania_Dominus_Collection/)
- [Ray’z Arcade Chronology](https://store.steampowered.com/app/2478020/Rayz_Arcade_Chronology/)
- [Darius Cozmic Collection Arcade](https://store.steampowered.com/app/1638330/Darius_Cozmic_Collection_Arcade/)
- [G-Darius HD](https://store.steampowered.com/app/1640160/GDarius_HD/)
- [Gradius Origins](https://store.steampowered.com/app/2897590/GRADIUS_ORIGINS/)
- [Operation Night Strikers](https://store.steampowered.com/app/3099790/Operation_Night_Strikers/)
- Namco Museum Archives: [(Vol. 1)](https://store.steampowered.com/app/1250250/NAMCO_MUSEUM_ARCHIVES_Vol_1/) / [(Vol. 2)](https://store.steampowered.com/app/1254620/NAMCO_MUSEUM_ARCHIVES_Vol_2/)

See Lyall's [MGSHDFix](https://github.com/ShizCalev/MGSHDFix) for the other games in the MGS Master Collection (Vol. 1).

See ShizCalev's [MGSPatriotFix](https://github.com/ShizCalev/MGSPatriotFix) for the other games in the MGS Master Collection (Vol. 2).

## Features
- ~~Upscaled render resolution (MGS 1).~~ Fixed by Konami officially via patch 3.0.0 on 12th February 2026.
- Widescreen support (MGS 1).
- Borderless/windowed mode.
- Corrects the monitor going to sleep during long periods with no input (e.g. during cutscenes).
- Control over Master Collection game patches.
- ~~Analog input (MGS 1).~~ - Fixed by Konami officially via patch 1.5.0 on 13th March 2024.
- Deadzone removal (MGS 1).
- Launcher skip (MGS 1, boots last launched game version).
- Skip intro logos.
- Modding support (MGS 1, via Ketchup - see below).
- Debug features (including stage select menu in MGS 1).

## Installation
- Grab the latest release of MGSM2Fix from [here](https://github.com/nuggslet/MGSM2Fix/releases) - or the preview release if you're feeling adventurous.
- Extract the contents of the release into the game folder.<br />(e.g. "**steamapps\common\MGS1**" or "**steamapps\common\METAL GEAR SOLID 4\MGS1**" for Steam games).

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
