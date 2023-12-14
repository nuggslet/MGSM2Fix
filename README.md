# Metal Gear Solid Master Collection Fix
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z7S6BTK)</br>
[![Github All Releases](https://img.shields.io/github/downloads/nuggslet/MGSM2Fix/total.svg)](https://github.com/nuggslet/MGSM2Fix/releases)

This is a fix that adds custom resolutions, analog input, pixel perfect scaling and more to the original Metal Gear Solid within the Master Collection.<br />

## Games Supported
- Metal Gear Solid <br />
- Metal Gear / Snake's Revenge (Vol.1 Bonus Content) <br />
See Lyall's [MGSHDFix](https://github.com/Lyall/MGSHDFix) for the other games in the collection.

## Features
- Borderless/windowed mode.
- Control over built-in filters.
- Analog input (MGS1).
- Launcher skip (MGS1, boots last launched game version).
- Skip intro logos.
- Debug features.

## Installation
- Grab the latest release of MGSM2Fix from [here.](https://github.com/nuggslet/MGSM2Fix/releases)
- Make sure to download the correct zip for the game. A separate fix is required for Bonus Content as it's 64 bit, while MGS1 is 32 bit.
- Extract the contents of the release zip in to the game folder.<br />(e.g. "**steamapps\common\MGS1**" or "**steamapps\common\MGS Master Collection Bonus Content**" for Steam).

### Steam Deck/Linux additional instructions
- Open up the Steam properties of MGS1 and put `WINEDLLOVERRIDES="d3d11=n,b" %command%` in the launch options.

## Configuration
- See **MGSM2Fix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

## Credits
Many thanks to [@Lyall](https://github.com/Lyall) and co. for MGSHDFix, from which this project derives its general structure. <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[Loguru](https://github.com/emilk/loguru) for logging. <br />
[length-disassembler](https://github.com/Nomade040/length-disassembler) for length disassembly.
