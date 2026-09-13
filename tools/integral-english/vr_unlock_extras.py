#!/usr/bin/env python
"""Unlock the EXTRA menu's items: a test aid, not part of the English port.

WHY THE OTHER TWO AIDS DO NOT DO THIS
-------------------------------------
There are three separate gates on this disc and each needed its own aid:

    vr_unlock.py          the MISSION menu          `selectvr` overlay
    vr_unlock_movies.py   the EXTRA clips           `movie` overlay
    this                  the EXTRA MENU's items    `vrtitle` overlay

Found 2026-09-07, when the user reported that only MOVIE, ALBUM and EXIT were
on the menu, so three of `vr_en_title`'s four ported help lines could not be
reached. PHOTOGRAPHING was simply not there to select.

THE GATE
--------
The EXTRA menu keeps a visibility bitmask at `work+0x1e` and builds it from
progress flags in the word at `+0x1a1c`, one item at a time, each test also
bumping the item count in `s2` (vrtitle overlay, base 0x800C11A0):

    +0639C  lhu   v1, 0x1e(s6)        ; the mask so far
    +063A4  ori   v0, v1, 1           ; MOVIE - unconditional
    +063A8  sh    v0, 0x1e(s6)
    +063AC  lw    v0, 0x1a1c(s1)      ; the progress word
    +063B4  andi  v0, v0, 3           ; PHOTOGRAPHING     <-- PATCHED
    +063B8  beqz  v0, skip
    +063C0  ori   v0, v1, 3
    ...     the same shape twice more:
    +063D4  andi  v0, v0, 0x10        ; ALBUM             <-- PATCHED
    +063F8  andi  v0, v0, 0x40        ; PocketStation     <-- PATCHED

Each `andi` becomes `addiu v0, zero, 1`, so every test passes, every bit is set
and the count is right. Patching the *tests* rather than the mask matters: the
blocks that set the bits also increment the item count, and a mask forced from
outside would leave the menu's layout and navigation disagreeing with it.

    overlay +063B4   30420003   andi v0, v0, 3      ->  24020001  addiu v0, zero, 1
    overlay +063D4   30420010   andi v0, v0, 0x10   ->  24020001
    overlay +063F8   30420040   andi v0, v0, 0x40   ->  24020001

WHAT YOU GET, AND WHAT STAYS JAPANESE
-------------------------------------
All five items: MOVIE, PHOTOGRAPHING, ALBUM, PocketStation, EXIT - which is the
four help lines `vr_en_title` ports (EXIT / MOVIE / PHOTOGRAPHING / ALBUM) plus
PocketStation's, which is deliberately NOT ported: Integral's fifth item is
PocketStation where USA's is STAFF CREDIT, so USA's `See the staff credits.` is
not its translation (README, "The VR disc"). Seeing Japanese there is correct.

WHY THIS IS SAFE TO REMOVE
--------------------------
It writes no progress. The mask is rebuilt from the progress word every time the
menu is entered, and this only changes what three tests see; the progress word
itself is never touched. Delete the PPF and the menu gates exactly as before. It
patches the `vrtitle` overlay, which `vr_en_title` does not touch (that patch
rewrites the script chunk's help-line records), so the two cannot interfere.

    py vr_unlock_extras.py            # stages the PPF
    py vr_unlock_extras.py --deploy   # and installs it
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
import struct, sys

import portio
from vrlib import INT_STAGE, int_disc, stage_lba, stage_bytes, inplace_records, write_ppf, deploy

NAME = 'INTEGRAL_vr_unlock_extras.ppf'
DESC = 'MGS Integral VR-DISC: all EXTRA menu items shown'
assert len(DESC) <= 50
STAGE = 'vrtitle'
FORCE = 0x24020001                      # addiu v0, zero, 1
GATES = {                               # overlay offset: (retail word, what it gates)
    0x063B4: (0x30420003, 'PHOTOGRAPHING'),
    0x063D4: (0x30420010, 'ALBUM'),
    0x063F8: (0x30420040, 'PocketStation'),
}


def build():
    sd = open(INT_STAGE, 'rb').read()
    original = stage_bytes(sd, STAGE)
    tags, pay, offsets = portio.stage(original)
    ov = bytearray(pay[0])
    for off, (want, what) in sorted(GATES.items()):
        got = struct.unpack_from('<I', ov, off)[0]
        assert got == want, 'overlay +%X holds %08X, not %08X - the %s gate moved' % (off, got, want, what)
        struct.pack_into('<I', ov, off, FORCE)
        print('  +%05X  %08X -> %08X   %s always shown' % (off, want, FORCE, what))
    payloads = dict(pay)
    payloads[0] = bytes(ov)
    modified = portio.pack_stage(tags, payloads)
    assert len(modified) == len(original), 'stage changed size'
    return original, modified


def main():
    original, modified = build()
    lba = stage_lba(int_disc(), open(INT_STAGE, 'rb').read(), STAGE)
    records = inplace_records(lba, original, modified)
    path = _os.path.join(WORK, NAME)
    write_ppf(path, records, DESC)
    print('%s: %d records, %d bytes' % (NAME, len(records), sum(len(d) for _, d in records)))
    if '--deploy' in sys.argv:
        print('deployed', deploy(NAME, open(path, 'rb').read()))
        print('a TEST AID: delete it when the help lines have been seen')


if __name__ == '__main__':
    main()
