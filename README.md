# Metal Gear Solid Master Collection Fix
[![Github All Releases](https://img.shields.io/github/downloads/nuggslet/MGSM2Fix/total.svg)](https://github.com/nuggslet/MGSM2Fix/releases)

This is a fix that adds custom resolutions, pixel perfect scaling and more to the original Metal Gear Solid within the Master Collection.<br />

## Games Supported
- Metal Gear Solid <br />
See Lyall's [MGSHDFix](https://github.com/Lyall/MGSHDFix) for the other games in the collection.

## Installation
- Grab the latest release of MGSM2Fix from [here.](https://github.com/nuggslet/MGSM2Fix/releases)
- Extract the contents of the release zip in to the game folder.<br />(e.g. "**steamapps\common\MGS1**" for Steam).

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
