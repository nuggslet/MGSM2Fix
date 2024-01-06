# Metal Gear Solid Master Collection Fix
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z7S6BTK)</br>
[![Github All Releases](https://img.shields.io/github/downloads/nuggslet/MGSM2Fix/total.svg)](https://github.com/nuggslet/MGSM2Fix/releases)

This is a fix that adds custom resolutions, analog input, mod support and more to the original Metal Gear Solid within the Master Collection.<br />

## Games Supported
- Metal Gear Solid <br />
- Metal Gear / Snake's Revenge (Vol.1 Bonus Content) <br />
See Lyall's [MGSHDFix](https://github.com/Lyall/MGSHDFix) for the other games in the collection.

## Features
- Borderless/windowed mode.
- Control over built-in filters and Master Collection game patches.
- Analog input (MGS1).
- Launcher skip (MGS1, boots last launched game version).
- Skip intro logos.
- Modding support (MGS1, via Ketchup - see below).
- Debug features (including stage select menu in MGS1).

## Installation
- Grab the latest release of MGSM2Fix from [here.](https://github.com/nuggslet/MGSM2Fix/releases)
- Make sure to download the correct zip for the game. A separate fix is required for Bonus Content as it's 64 bit, while MGS1 is 32 bit.
- Extract the contents of the release zip in to the game folder.<br />(e.g. "**steamapps\common\MGS1**" or "**steamapps\common\MGS Master Collection Bonus Content**" for Steam).

### Steam Deck/Linux additional instructions
- Open up the Steam properties of MGS1 and put `WINEDLLOVERRIDES="d3d11=n,b" %command%` in the launch options.

## Configuration
- See **MGSM2Fix.ini** to adjust settings for the fix.

## Analog (MGS 1)
Recommended Steam controller profile: steam://controllerconfig/2131630/3087945618 (copy to browser address bar).

## Modding (MGS 1; Ketchup)
'Ketchup' is a mod loader for MGS1 in the Master Collection.

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

If your mods conflict with the built-in Master Collection patches, for the time being it may be useful to disable the `GlobalRAM` and `GlobalCDROM` settings in **MGSM2Fix.ini**.
Once all of the Master Collection patches have been identified and grouped (please help, there are lots!) this heavy-handed approach should no longer be necessary.

Additional mod formats may be supported in future.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

## Credits
Many thanks to [@Lyall](https://github.com/Lyall) and co. for MGSHDFix, from which this project derives its general structure. <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[Loguru](https://github.com/emilk/loguru) for logging. <br />
[length-disassembler](https://github.com/Nomade040/length-disassembler) for length disassembly.
