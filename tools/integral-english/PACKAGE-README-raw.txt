Integral English text - RAW PSX DISC build

This is the raw-disc variant. It patches original PlayStation disc images, not
the Master Collection. If you are playing the Master Collection, stop here and
use the collection build instead: this one will not work there, and the title
screen's disc-swap patch (en_menu3) actively breaks it, because the collection
patches those same bytes itself.

WHAT IT IS FOR

  Integral disc 1   SLPM-86247
  Integral disc 2   SLPM-86248
  Integral VR disc  SLPM-86249

Each folder in mods/ holds one disc's patches. The folder names are the
collection's and are kept only so the two builds can be compared file by file.

THE EASY WAY: mkimage.py

  py mkimage.py --redump "<your disc>.bin" --ppfs <this folder>       --output "MGS Integral English (Disc 1).bin" --cue

That is the whole command. It works out which of the three discs the image is
by reading the executable inside it, picks the matching folder out of this
package, applies every patch, and writes a MODE2/2352 image and its .cue.
You do not have to say which disc it is, and you cannot pair the wrong
patches with the wrong image - it checks.

It is in tools/integral-english/ in the source repository and needs Python 3
and nothing else. Before it writes anything it verifies:

  - every patch's block check against your image, so a patch built for a
    different pressing is refused rather than applied;
  - that every sector it is about to touch already matches its own stored
    error-correction data - which is what proves your dump is the pressing
    these patches were computed against;
  - that every one of those sectors still verifies afterwards.

If any of that fails it stops and writes nothing at all, and says why.

It will also offer to make English the power-on language. That is optional and
it is not about this port's text - see LANGUAGE below.

APPLYING BY HAND INSTEAD

Any PPF3 tool will do it, but you take on three things mkimage.py does for
you.

Every PPF for a disc must be applied, and the *_zz_ecc.ppf one must be applied
last. It carries the recomputed EDC and ECC of every sector the other patches
touch, worked out from the finished state of the complete set. Apply only some
of them and the error-correction data will describe a disc you do not have.
Order among the others does not matter.

Each patch carries a PPF3 block check, so a tool that honours it will refuse an
image that is not the disc the patch was built against. Not every tool honours
it. If yours does not, nothing will tell you that you used the wrong image.

And you must give it a MODE2/2352 image of the whole disc - the offsets are
raw-sector offsets from LBA 0. A 2048-byte-per-sector .iso will be corrupted
by these patches, silently.

LANGUAGE

Integral is a bilingual disc. One bit in its own configuration decides which
language the *game* uses - the codec dialogue and the cutscene subtitles - and
it is clear at power-on, so a retail disc starts in Japanese. The player turns
it on in Integral's own OPTION screen, where it saves to the memory card.

None of this port's text depends on that bit: the menus, item descriptions,
briefings and mission log are English either way. So if you do nothing, you
get English menus and a Japanese story until you visit OPTION once.

mkimage.py offers to set the bit at boot instead, so the disc starts in
English. The OPTION screen and the memory card still override it afterwards,
so a player who prefers Japanese still gets Japanese. Say no and the image is
byte-for-byte what it would have been without the offer.

WHAT DIFFERS FROM THE COLLECTION BUILD

  - the brightness help text keeps all six of USA's lines, including
    "Press the O button to return to the option screen.", which is true on a
    real PlayStation and is why the collection drops it;
  - KEY CONFIG is Integral's own screen with USA's English labels, because
    there is no Control Settings panel here to intercept it;
  - en_menu3 is included: the title screen's disc-swap prompt in English.
  - the VR grenade correction is included: USA's DELAY 4.0 texture and
    "4 seconds" in all five copies of the grenade briefing. The texture/fuse
    PPF is INTEGRAL_vr_fix_grenade_delay_english.ppf; its sector parity is
    included in the package's final INTEGRAL_vr_zz_ecc.ppf. No separate grenade
    addon is needed. MGSM2Fix.ini does not control raw images.

VERIFIED, AND NOT

The text itself is the same port as the collection build, which has been read
on screen extensively - see NextSteps.md in the repository for exactly which
screens. What has NOT been done is running this variant on a real disc image:
it builds, it packages, and every sector it rewrites was checked against that
sector's own stored parity before the new parity was computed, but nobody has
applied it to a disc image and booted it. Treat it as untested in that specific
sense, and please report what you find.

Some Japanese is retained on purpose, wherever the USA release has no English
counterpart. No new translation has been made, and USA's spelling is kept.

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

UNINSTALL

PPF3 patches here carry no undo data. Keep an unpatched copy of your disc
images before applying anything.
