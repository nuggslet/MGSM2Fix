#!/usr/bin/env python
"""Unlock every VR mission: a test aid, not part of the English port.

WHAT GATES A VR MISSION
-----------------------
The VR discs keep their progress as a **bitmap in VRAM**, not in GCL variables.
`selectvr`'s window manager (`nobu/vr_slct/winmngr.c`, chara 0xAE06) owns a
384-byte buffer - 16 rows of 6 words, 192 bits per row - that it copies to and
from the video RAM rectangle **(160, 224) 12x16** with `LoadImage` / `StoreImage`.
Every one of the ninety mission stages carries that same rectangle in its own
overlay, so a mission sets its bit in VRAM as it is cleared and the bit is still
there when `selectvr` comes back: VRAM is what survives a stage load. The
memory-card save is built from the same buffer (plus the 0x226 record shorts and
one flag word), so the bitmap - not the item flags - is the progress.

Two static byte tables turn (group, item) into a bit:

    row = A[group] - 1      A at INT 0x800CE944 / USA 0x800D1DF0; A == 0 means
                            "this group has no missions" and is never cleared
    bit = B[group] + item   B at INT 0x800CE98C / USA 0x800D1E38

and the menu itself is a table of 70 groups x 28 bytes (INT 0x800CDB54, USA
0x800D0E08; count at +4, item array at +8, items 60 bytes each). The item's
first word is its flags:

    0x80000000  drawn
    0x40000000  UNLOCKED - selectable, and drawn bright instead of grey
    0x20000000  gated: consult the clear bitmap for this item
    0x10000000  a menu entry rather than a mission
    0x00080000  cleared (set at run time, never stored)

Once per frame the manager walks all 70 groups. For each item with 0x20000000 it
asks the bitmap; a set bit marks the item cleared (0x00080000) and **unlocks the
next item in the group** (0x40000000, `800DBE2C` / USA `800DF3B8`). If every item
of a group is cleared the group's bit is set in a 70-bit array (INT 0x800C12AC,
USA 0x800C445C), and a long tail then hands 0x40000000 to the items of other
groups - the cascade that opens SPECIAL, the harder modes and the rest.

A third gate scores the bitmap - `count / 3` against 80 / 70 / 60 / 50, where
one bit (row 12, word 4, bit 12) counts ten - and opens the first item of the
last four categories.

All three questions go through one instruction each:

    is group G fully cleared?          INT 0x800DBD64   USA 0x800DF2F0
    is mission I of group G cleared?   INT 0x800DBDA4   USA 0x800DF330
    the progress score                 INT 0x800DC514   USA 0x800DFA1C

so three words unlock everything. The first two patches replace the `and` in a
predicate's `jr ra` delay slot with `addiu v0, zero, 1`; the third replaces the
`subu` that computes the score with `addiu v1, zero, 0x100`, over every
threshold. The mission-cleared predicate keeps its `A[group] == 0` early return,
so groups that have no missions are not falsely marked.

VERIFIED BY EMULATION
---------------------
Running the real overlay image through all five unlock passes to a fixpoint
(a small MIPS interpreter, `scratchpad/vr/emu.py`) gives 46 of 373 menu items
unlocked stock and **361** with these three words patched; on the USA disc,
45 of 372 stock and 357 with the cascade pass alone. The twelve that never
open are group 67, whose menu id is 0xFFFF and whose item-count table entry is
zero: a placeholder, not a reachable menu.

WHY THIS IS SAFE TO REMOVE
--------------------------
Nothing here writes progress. The save file is made from the VRAM bitmap, which
only a genuinely cleared mission ever touches, so a save written while the patch
is deployed still records real progress only. Delete the PPF and the disc gates
exactly as before - the "relock, not permanently" the test procedure needs.

Named `_unlock_`, not `en_`, so it is obviously not part of the English port.
Integral's PPF goes to `mods/INTEGRAL/VR-DISK/`, the USA VR disc's to
`mods/VR-DISK_US/` (Ketchup's RootPath adds a version folder only when a title
has more than one version, and a disk folder only when a version has more than
one disk; VR-DISK_US has one of each).

TEST PROCEDURE (the standing rule: never unlock with achievements live)
    1. MGSM2Fix.ini [Patches]: DisableRAM = true, DisableCDROM = true
       (also what makes Integral's own KEY CONFIG visible - the collection stops
       intercepting it)
    2. vr_unlock.py --deploy
    3. test
    4. delete the deployed INTEGRAL_vr_unlock_missions.ppf (and the USA one)
    5. DisableRAM = false, DisableCDROM = false

usage: vr_unlock.py [--deploy]
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import struct
import portio
from workdir import WORK, GAME
from vrlib import (INT_STAGE, USA_STAGE, int_disc, usa_disc, stage_lba, stage_bytes,
                   inplace_records, write_ppf)

AND_A0_V0 = 0x00821024      # and v0, a0, v0
AND_V1_V0 = 0x00621024      # and v0, v1, v0
SUBU_V1 = 0x00C41823        # subu v1, a2, a0     (the score, count / 3)
RETURN_1 = 0x24020001       # addiu v0, zero, 1   (in the jr ra delay slot)
MAX_SCORE = 0x24030100      # addiu v1, zero, 0x100  (over every threshold)

# disc: (stage dir, disc reader, overlay load address, [(address, expected word, note)],
#        PPF name, PPF description, mods folder relative to the game root)
TARGETS = {
    'INT': (INT_STAGE, int_disc, 0x800C11A0, [
        (0x800DBDA0, AND_A0_V0, 'group-cleared predicate (0x800DBD64) always says yes'),
        (0x800DBE28, AND_V1_V0, 'mission-cleared predicate (0x800DBDA4) always says yes'),
        (0x800DC528, SUBU_V1, 'score gate (0x800DC514) sees a full score'),
    ], 'INTEGRAL_vr_unlock_missions.ppf',
       'MGS Integral VR-DISC: all VR missions unlocked',
       'mods/INTEGRAL/VR-DISK'),
    'USA': (USA_STAGE, usa_disc, 0x800C4350, [
        (0x800DF32C, AND_A0_V0, 'group-cleared predicate (0x800DF2F0) always says yes'),
        (0x800DF3B4, AND_V1_V0, 'mission-cleared predicate (0x800DF330) always says yes'),
        (0x800DFA30, SUBU_V1, 'score gate (0x800DFA1C) sees a full score'),
    ], 'VRUS_unlock_missions.ppf',
       'MGS VR Missions USA: all VR missions unlocked',
       'mods/VR-DISK_US'),
}


def patch(which):
    """-> (ppf bytes, ppf name, mods folder, [(address, note)])"""
    stage_dir, disc_fn, base, sites, name, desc, mods = TARGETS[which]
    sd = open(stage_dir, 'rb').read()
    data = stage_bytes(sd, 'selectvr')
    tags, payloads, offsets = portio.stage(data)
    ov_index = [k for k, (tid, mode, ext, sz) in enumerate(tags)
                if mode == ord('s') and ext == ord('b')]
    assert len(ov_index) == 1, 'selectvr has %d overlays' % len(ov_index)
    k = ov_index[0]
    ov_off = offsets[k]
    new = bytearray(data)
    for addr, expect, note in sites:
        p = ov_off + addr - base
        was = struct.unpack_from('<I', new, p)[0]
        assert was == expect, '%s %08X: expected %08X, found %08X' % (which, addr, expect, was)
        word = MAX_SCORE if expect == SUBU_V1 else RETURN_1
        struct.pack_into('<I', new, p, word)
        print('  %08X (stage +%06X) %08X -> %08X  %s' % (addr, p, was, word, note))
    assert len(new) == len(data)
    lba = stage_lba(disc_fn(), sd, 'selectvr')
    recs = inplace_records(lba, data, bytes(new))
    return write_ppf(_os.path.join(WORK, name), recs, desc), name, mods, sites


def main():
    for which in TARGETS:
        print('== %s VR disc, selectvr' % which)
        try:
            ppf, name, mods, sites = patch(which)
        except FileNotFoundError as e:
            print('  skipped: %s' % e)
            continue
        print('  %s: %d bytes' % (name, len(ppf)))
        if '--deploy' in _sys.argv:
            folder = _os.path.join(GAME, mods)
            _os.makedirs(folder, exist_ok=True)
            with open(_os.path.join(folder, name), 'wb') as f:
                f.write(ppf)
            print('  deployed %s' % _os.path.join(folder, name))
    if '--deploy' not in _sys.argv:
        print('\nnot deployed. Disable achievements first (MGSM2Fix.ini [Patches]:')
        print('DisableRAM = true, DisableCDROM = true), then run with --deploy;')
        print('delete the PPFs and set both back to false when the test is over.')


if __name__ == '__main__':
    main()
