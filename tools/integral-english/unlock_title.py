#!/usr/bin/env python
"""Force every title-screen unlock: a test aid, not part of the English port.

The title overlay (`onoda/open/open.c`) scans the memory card's save NAMES once
per boot (`title_open_800D1CB4`) and derives four flags from them:

    photo_flag   a save named ...'C'...      -> PHOTO ALBUM on the SPECIAL menu
    vr_flag      a save named ...'V'...      -> passed to the title scripts
    demo_rank    a game save 'G' with a non-zero clear-rank nibble
                                             -> EXTREME difficulty, DEMO THEATER
    has_clear_data  the same cleared save    -> 1P MODE (SPECIAL page +4), clear_proc
    spe_rank = photo + 2*(demo_rank != 0) + 4*has_clear_data   (SPECIAL page 0..7)

Rather than fake save files, this patches the scan's RESULT derivation in the
overlay, at three places, so a fresh save sees everything:

  1. the `if (photo == 1)` / `if (vr == 1)` guards on the work stores -> nop
  2. the "no clear save" branch that stores demo_rank = 0 -> store 6 (rank 6:
     every clear bonus); a real clear save still yields its own rank
  3. at the tail, where `if (has_clear_data == 1) spe_rank += 4` reads the
     global, store 1 into has_clear_data instead and drop the test, so the
     later consumers (argv[2] to the scripts, the -k clear_proc) see it too

Every patched word is asserted against the retail overlay first.  The PPFs
address the `title` stage's sectors on each disc image (the stage is not
relocated), one per disc, and are named `_unlock_` so they are obviously not
part of `en_*`.  Remove them to restore normal gating.

USA's title code lacks Integral's additions (1P MODE and the has_clear_data
mechanism are Integral's), so its table is the photo/vr guards and the
demo_rank branch; photo + demo_rank give spe_rank 3 (PHOTO ALBUM + DEMO
THEATER), USA's maximum, and EXTREME.

usage: unlock_title.py [--deploy]
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from optscan import ents

HDR = 24
MODS = (GAME + '/mods') if GAME else None

# (address, old word, new word, note) per game; all addresses in the overlay
INT_SITES = [
    (0x800D21DC, 0x15230002, 0x00000000, 'photo_flag = 1 unconditionally (bne -> nop)'),
    (0x800D21F0, 0x15430002, 0x00000000, 'vr_flag = 1 unconditionally (bne -> nop)'),
    (0x800D2214, 0x14400018, 0x24020006, 'no clear save: demo_rank = 6 (addiu v0,zero,6)'),
    (0x800D2218, 0x24020002, 0x0803489F, '  j 800D227C'),
    (0x800D221C, 0x0803489F, 0xAE420B48, '  sw v0, demo_rank (delay slot)'),
    (0x800D2220, 0xAE400B48, 0x00000000, '  (unreachable) nop'),
    (0x800D22C0, 0x8C4292D0, 0x24100001, 'has_clear_data: addiu s0,zero,1 (was lw v0,has_clear_data)'),
    (0x800D22C4, 0x24100001, 0xAC5092D0, '  sw s0, has_clear_data (=1)'),
    (0x800D22C8, 0x14500005, 0x00000000, '  spe_rank += 4 unconditionally (bne -> nop)'),
]
# USA: the title overlay's code sits 8 bytes later in the payload than the labels
# below assume (its header entry, printf references and every `j` target all agree
# on -8), so file offsets are label-based while encoded jump targets are label-8.
# USA has no has_clear_data / 1P MODE; photo + demo_rank give spe_rank 3, its max.
USA_SITES = [
    (0x800D1250, 0x16C30002, 0x00000000, 'photo_flag = 1 unconditionally (bne -> nop)'),
    (0x800D125C, 0x16E30002, 0x00000000, 'vr_flag = 1 unconditionally (bne -> nop)'),
    (0x800D1280, 0x14400018, 0x24020006, 'no clear save: demo_rank = 6 (addiu v0,zero,6)'),
    (0x800D1284, 0x24020002, 0x080344B8, '  j past the store (true 800D12E0 = label 800D12E8, lw photo_flag)'),
    (0x800D1288, 0x080344B8, 0xAE420B1C, '  sw v0, demo_rank (delay slot)'),
    (0x800D128C, 0xAE400B1C, 0x00000000, '  (unreachable) nop'),
]

TARGETS = [
    # name, stage dir, STAGE.DIR lba, overlay base, sites, PPF path (under MODS)
    ('Integral disc 1', WORK + '/int1_stage.dir', 136654, 0x800C3208, INT_SITES, 'INTEGRAL/INTEGRAL/0/INTEGRAL_disc1_unlock_title.ppf'),
    ('Integral disc 2', WORK + '/int2_stage.dir', 105178, 0x800C3208, INT_SITES, 'INTEGRAL/INTEGRAL/1/INTEGRAL_disc2_unlock_title.ppf'),
    ('USA disc 1',      WORK + '/usa1_stage.dir', 132344, 0x800C5970, USA_SITES, 'MGS1_US/0/MGS1_disc1_unlock_title.ppf'),
    ('USA disc 2',      WORK + '/usa2_stage.dir', 100801, 0x800C5970, USA_SITES, 'MGS1_US/1/MGS1_disc2_unlock_title.ppf'),
]


def ppf3(records, desc):
    out = bytearray(b'PPF30' + bytes([2]) + desc.encode('ascii')[:50].ljust(50, b'\x00') + bytes(4))
    for off, data in records:
        for i in range(0, len(data), 255):
            c = data[i:i+255]; out += struct.pack('<Q', off + i) + bytes([len(c)]) + c
    return bytes(out)


def main():
    deploy = '--deploy' in sys.argv
    for name, sdpath, sd_lba, base, sites, rel in TARGETS:
        if not sites:
            print('%s: no sites configured, skipped' % name); continue
        d = open(sdpath, 'rb').read()
        tsec = dict(ents(d))['title']; tbase = tsec * 2048
        ver, _p, sect = struct.unpack('<BBh', d[tbase:tbase+4])
        tid, mode, ext, osize = struct.unpack('<HBBi', d[tbase+4:tbase+12])
        assert chr(mode) == 's', 'title tag 0 is not the overlay'
        recs = []
        for addr, old, new, note in sites:
            fo = 2048 + (addr - base)                      # overlay is tag 0, payload at +2048
            assert 0 <= fo - 2048 < osize, 'address outside the overlay'
            have = struct.unpack_from('<I', d, tbase + fo)[0]
            assert have == old, '%s: %08X holds %08X, expected %08X' % (name, addr, have, old)
            img = (sd_lba + tsec + fo // 2048) * 2352 + HDR + fo % 2048
            recs.append((img, struct.pack('<I', new)))
            print('  %s  %08X: %08X -> %08X  %s' % (name, addr, old, new, note))
        blob = ppf3(recs, 'MGS1: force title-screen unlocks (test aid)')
        out = WORK + '/' + os.path.basename(rel); open(out, 'wb').write(blob)
        print('%s: %d words -> %s' % (name, len(recs), out))
        if deploy:
            if not MODS:
                raise SystemExit('Cannot find the Master Collection MGS1 directory; '
                                  'run workdir.py to see what was searched, or set '
                                  'INTEGRAL_ENGLISH_GAME.')
            p = os.path.join(MODS, rel); os.makedirs(os.path.dirname(p), exist_ok=True)
            open(p, 'wb').write(blob); print('   deployed %s' % p)
    if not deploy: print('\nNOT DEPLOYED. Re-run with --deploy to install.')


if __name__ == '__main__':
    main()
