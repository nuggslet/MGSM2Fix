Integral English text - Master Collection build (collection variant)

Copy the mods folder into your MGS1 game folder. The generated inventory at
the end of this README lists the exact files. Include all .ppf.json companions.
Both grenade layouts are included; MGSM2Fix selects the matching language.
Replace the previous package as a complete set, removing its files using its
SHA256SUMS manifest first. Retain unrelated mods and your settings. Do not mix
collection/raw variants or older experimental patches such as en_menu3.

This package requires the MGSM2Fix integral-english-text branch (the deferred
executable RAM mirror and EnglishText) with EnglishText = true. It does not
include MGSM2Fix itself. Keep DisableRAM and DisableCDROM false for
achievements; the patches need neither.

This is the collection variant: four brightness-help lines, KEY CONFIG handed to
the collection's Control Settings panel, and no en_menu3 (the collection patches
the title screen's disc-swap block itself, and the two layouts do not mix). It is
not the raw-PSX variant. Do not apply this package to an original PlayStation
disc; that needs a build made with --variant raw.

Verification at packaging (NextSteps.md in the repository has the current list):
every main-disc family has been seen on screen on disc 1 except the game's own
disc-swap prompts, which only the story's disc change can reach; the controller-
port subtitle in the Psycho Mantis room, which needs a second controller in port
2; and the four MISSION LOG location names that now read USA's spelling
(Tank Hangar, Medi room, Cmnder room, Cmnd room). Disc 2 is
byte-identical wherever the patches touch it but has not been played across
that change. On the VR disc the option screen, KEY CONFIG, mission windows, item
and weapon descriptions and the MOVIE captions have been seen; a mission RESULT
window, the save and load messages and the PHOTOGRAPHING memory-card messages
have not. Some Japanese is retained on purpose because USA provides no English
counterpart; no new translation has been made, and USA's spelling is kept.

Of the text this port covers - menus, screens and the executable's own strings
- nothing with a USA counterpart is still Japanese. This is not a complete
translation of the disc: several megabytes of Integral-exclusive developer
commentary and story narration (in RADIO.DAT, DEMO.DAT and VOX.DAT) have no
English source and remain in Japanese by design. Producing English for them
would be new translation, which this port does not do.


CREDITS

The option screen, Previous Operations and the MISSION LOG contain code
compiled from the MGS1 decompilation by FoxdieTeam:

    https://github.com/FoxdieTeam/mgs_reversing   (commit 7964de7)

That work is theirs and this port would not exist without it. The mod loader
and the fix this port targets are MGSM2Fix by nuggslet:

    https://github.com/nuggslet/MGSM2Fix

Every English string is copied verbatim from Konami's USA release. Nothing has
been translated, and no game data is redistributed with these patches.

Uninstall: remove this package's mods files listed in SHA256SUMS.txt, including
JSON companions. Leave unrelated mods and saves alone.
SHA256SUMS.txt and build-report.json identify the packaged build and validation.

OPTIONAL PATCH CONTROLS (current branch)
[Patches] IntegralEnglishPatch and IntegralVREnglishPatch enable the installed
story/VR English sets independently. EnglishText selects Integral's own language
bit separately. Copy the VR mission PPF's .json companion too; it is part of this
package. Missing/stale companions are reported in MGSM2Fix.log and the affected
VR English mission patch is skipped, rather than guessing where its grenade digits moved.
GrenadeDelayFix controls the grenade briefing numeral and its corrected texture.
False restores 5 in English as well as Japanese.
This package includes both grenade texture/text variants and their companions.
Other VR English screens remain active if the mission companion is invalid. Restart after INI changes.
UnlockVRMissions, UnlockVRExtras, UnlockVRMovies and UnlockTitleBonuses default
false and gate separately installed test-aid PPFs. They are not in this package.
