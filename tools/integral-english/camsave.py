#!/usr/bin/env python
"""Port MGS1 (USA)'s PHOTO ALBUM memory-card messages into MGS Integral.

`en_savemsg` ports `menu/datasave.c`'s two caption tables, which live in the
EXECUTABLE and serve the LOAD DATA screen and the in-game save. The PHOTO ALBUM
(the `camera` stage) has its **own copy of the same message family inside its
overlay**, and `en_savemsg` never touches it - so that screen still read
`セーブファイルがありません。` where LOAD DATA correctly read `No save file.`

MECHANISM. The overlay reaches these strings through pointer words in one
region (0x600..0x740 of the overlay payload). Both games' overlays are the same
program, so **the pointer word's offset is the index**: slot 0x648 is the same
message in both, whatever each game's string happens to be or where it sits.
That makes the mapping exact - no ordering or content guesswork - and it is the
whole reason this port is small.

    overlay base   Integral 0x800C3208, USA 0x800C5968 (same as their `option`
                   overlays: stages load at a fixed address)
    Japanese       815 bytes across the paired slots
    English        509 bytes - 306 SHORTER, so the pool repacks in place and
                   the stage stays 38 sectors with no relocation

WHERE USA SHOWS NOTHING, INTEGRAL'S JAPANESE STAYS. Six slots point at an empty
string in USA (0x60C, 0x62C, 0x63C, 0x65C, 0x668, 0x66C). Those are
Integral-only messages, and the rule is verbatim USA text where USA has text,
Integral's own where it does not - the same decision `savemsg.py` made for the
"saving" and "save completed" captions.

Slots already identical in both (OVERWRITE OK?, FORMAT OK?, COMPLETE, ERROR,
MEMORY CARD 1/2, YES, NO, SAVE DATA) are left alone.

usage: camsave.py [--deploy]      (writes work/ always; PPFs to mods with --deploy)
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from optscan import ents, stage, pad

INT = {1: WORK + '/int1_stage.dir', 2: WORK + '/int2_stage.dir'}
USA = WORK + '/usa1_stage.dir'
BASE_INT, BASE_USA = 0x800C3208, 0x800C5968
TAB = (0x600, 0x740)                 # pointer-word region in the overlay payload
POOL = (0x0CB2C, 0x0CD18)            # Integral's Japanese message pool, in the overlay
STR_MIN = 0x0C000                    # strings live past here; below it is code
SD_LBA = {1: 136654, 2: 105178}      # STAGE.DIR lba per disc
MODS = os.path.join(GAME, 'mods/INTEGRAL/INTEGRAL')
NAMES = {1: 'INTEGRAL_disc1_en_camsave.ppf', 2: 'INTEGRAL_disc2_en_camsave.ppf'}
HDR = 24


def overlay(path):
    """-> (the stage's SECTOR in STAGE.DIR, overlay payload).

    `stage()` returns the stage's sector COUNT as its first value, not its
    position - taking that for the position put the first build's records about
    27,000 sectors early, so the patch loaded and wrote nothing useful. The
    position comes from the entry table, via `ents()`.
    """
    _count, tags, F, pay = stage(path, 'camera')
    assert chr(tags[0][1]) == 's', 'camera tag 0 is not the overlay'
    sect = dict(ents(open(path, 'rb').read()))['camera']
    return sect, pay[0]


def read_table(ov, base):
    """-> {slot: (offset, bytes)} for every pointer word aiming at a STRING.

    The same region also holds FUNCTION pointers - slots 0x6E0/0x6E4/0x6E8 aim
    at code, and `addiu sp, sp, -N` reads as high bytes, so a naive "has bytes
    over 0x80 means Japanese" test called them text and would have rewritten
    them. Every real string in these overlays sits past STR_MIN, well clear of
    the code, so that is the guard.
    """
    out = {}
    for i in range(TAB[0], TAB[1], 4):
        w = struct.unpack_from('<I', ov, i)[0]
        if not (base <= w < base + len(ov)):
            continue
        off = w - base
        if off < STR_MIN:                       # a function pointer, not a string
            continue
        e = ov.find(b'\x00', off)
        if e < 0 or e - off > 80:
            continue
        out[i] = (off, ov[off:e])
    return out


def japanese(s):
    return any(b >= 0x80 for b in s)


def build(iov, uov):
    """-> (new overlay bytes, report rows)"""
    it, ut = read_table(iov, BASE_INT), read_table(uov, BASE_USA)
    want, rows = {}, []
    for slot in sorted(set(it) | set(ut)):
        i, u = it.get(slot), ut.get(slot)
        if i is None:
            continue
        if u is not None and u[1] and japanese(i[1]):
            want[slot] = u[1]; rows.append((slot, i[1], u[1], 'USA'))
        elif u is not None and not u[1] and japanese(i[1]):
            want[slot] = i[1]; rows.append((slot, i[1], b'', 'kept (USA blank)'))
        else:
            want[slot] = i[1]; rows.append((slot, i[1], u[1] if u else None, 'unchanged'))

    # Only strings that MOVE are laid into the pool: the ones whose text
    # changes, and the ones that already live in the pool (it gets cleared, so
    # they must be re-laid). Everything else - OVERWRITE OK?, MEMORY CARD 1,
    # SAVE DATA and the rest, all outside the pool and unchanged - keeps its
    # address and its pointer word, so the edit stays as small as the change.
    lo, hi = POOL
    move = {slot: tgt for slot, tgt in want.items()
            if tgt != it[slot][1] or lo <= it[slot][0] < hi}
    new = bytearray(iov)
    for a in range(lo, hi):
        new[a] = 0
    placed, cur = {}, lo
    for slot in sorted(move):
        s = move[slot]
        if s in placed:
            continue
        blob = s + b'\x00'
        assert cur + len(blob) <= hi, ('pool overflow: %d bytes needed, %d available'
                                       % (cur + len(blob) - lo, hi - lo))
        new[cur:cur + len(blob)] = blob
        placed[s] = cur
        cur += len(blob)
    used = cur - lo

    # rewrite only the pointer words whose string moved
    for slot, s in move.items():
        struct.pack_into('<I', new, slot, BASE_INT + placed[s])

    # verify by re-reading the way the game does
    chk = read_table(bytes(new), BASE_INT)
    for slot, s in want.items():
        assert slot in chk and chk[slot][1] == s, 'slot 0x%X reads back wrong' % slot
    # nothing outside the pool and the table may move
    for k in range(len(iov)):
        if lo <= k < hi or TAB[0] <= k < TAB[1]:
            continue
        assert new[k] == iov[k], 'byte 0x%X changed outside the pool and table' % k
    return bytes(new), rows, used, hi - lo, len(placed), len(move)


def ppf3(records, desc):
    out = bytearray(b'PPF30' + bytes([2]) + desc.encode('ascii')[:50].ljust(50, b'\x00') + bytes(4))
    for off, data in records:
        for i in range(0, len(data), 255):
            c = data[i:i+255]
            out += struct.pack('<Q', off + i) + bytes([len(c)]) + c
    return bytes(out)


def main():
    deploy = '--deploy' in sys.argv
    _us_sect, uov = overlay(USA)
    _s1, iov = overlay(INT[1])
    _s2, iov2 = overlay(INT[2])
    assert iov == iov2, "the two discs' camera overlays differ; build them separately"

    new, rows, used, cap, uniq, nmove = build(iov, uov)
    print('%-6s %-30s %-30s %s' % ('slot', 'Integral was', 'now', 'source'))
    for slot, was, now, src in rows:
        if src == 'unchanged':
            continue
        w = was[:26].hex(' ') if japanese(was) else repr(was[:26].decode('latin1'))
        n = repr(now.decode('latin1')) if src == 'USA' else '(unchanged Japanese)'
        print('0x%03X  %-30s %-30s %s' % (slot, w[:30], n[:30], src))
    print('\npool %d of %d bytes used, %d distinct strings, %d pointer words rewritten'
          % (used, cap, uniq, nmove))

    diffs = [k for k in range(len(iov)) if new[k] != iov[k]]
    runs = []
    for k in diffs:
        if runs and k == runs[-1][1]:
            runs[-1][1] = k + 1
        else:
            runs.append([k, k + 1])
    print('%d bytes differ in %d runs' % (len(diffs), len(runs)))

    os.makedirs(WORK, exist_ok=True)
    open(WORK + '/camera_en.bin', 'wb').write(new)
    for disc in (1, 2):
        sect, _ = overlay(INT[disc])
        recs = []
        for a, b in runs:
            p = a
            while p < b:
                fo = 2048 + p                    # the overlay is tag 0, payload at +2048
                room = 2048 - (fo % 2048)        # Ketchup drops bytes past a sector's payload
                take = min(b - p, room)
                img = (SD_LBA[disc] + sect + fo // 2048) * 2352 + HDR + fo % 2048
                recs.append((img, new[p:p + take]))
                p += take
        for off, data in recs:                   # replay Ketchup's own rule
            pos = (off % 2352) - HDR
            assert 0 <= pos and pos + len(data) <= 2048, 'record crosses a sector tail'
        blob = ppf3(recs, 'MGS Integral: English photo album messages')
        p = WORK + '/%s' % NAMES[disc]
        open(p, 'wb').write(blob)
        print('disc %d: %d records, %d bytes -> %s' % (disc, len(recs), sum(len(d) for _, d in recs), p))
        if deploy:
            t = os.path.join(MODS, str(disc - 1), NAMES[disc])
            open(t, 'wb').write(blob)
            print('   deployed %s' % t)
    if not deploy:
        print('\nNOT DEPLOYED. Re-run with --deploy to install.')


if __name__ == '__main__':
    main()
