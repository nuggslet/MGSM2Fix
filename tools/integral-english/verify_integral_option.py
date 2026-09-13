"""Read the DEPLOYED Integral option PPFs back and prove what they install.

    py verify_integral_option.py          (run from the directory holding work/)

The Integral counterpart of verify_usa_brightness.py. Trusts nothing the build
claims: reconstructs the 78 DUMMY3M sectors from each deployed PPF's own
records, walks the stage's tag table and DAR, decodes sc_text, and measures
where its lines sit and that the (8,8,8) seam filler is still in rows 0..1.
DUMMY3M is blank on the retail image, which emit() asserts, so the records
alone rebuild the stage.
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pcx4
from optsctext import (dar_entries, img_off, SC_TEXT, SC_KEEP_LINES, SC_ROWS,
                       SLOT, DISCS, MODS, HDR, pad)

for D in DISCS:
    disc, du = D['disc'], D['du']
    d = open('%s/%d/%s' % (MODS, disc, D['ppf']), 'rb').read()
    assert d[:6] == b'PPF30\x02', d[:6]
    recs, p = [], 60
    while p + 9 <= len(d):
        off, n = struct.unpack_from('<QB', d, p)
        recs.append((off, d[p + 9:p + 9 + n]))
        p += 9 + n
    assert p == len(d), 'trailing %d bytes' % (len(d) - p)

    base = img_off(du + SLOT, 0)
    end = img_off(du + SLOT + 78, 0)
    stage, outside = bytearray(78 * 2048), 0
    for off, b in recs:
        if not (base <= off < end):
            outside += 1
            continue
        # base already carries the 24-byte sector header, so pos is an offset
        # into that sector's 2048 bytes of user data
        sec, pos = divmod(off - base, 2352)
        assert pos + len(b) <= 2048, (sec, pos, len(b))
        stage[sec * 2048 + pos:sec * 2048 + pos + len(b)] = b

    ver, _p, sect = struct.unpack('<BBh', stage[:4])
    assert sect == 78, 'header says %d sectors' % sect
    tags, p = [], 4
    while True:
        tid, mode, ext, sz = struct.unpack('<HBBi', stage[p:p + 8])
        if mode == 0:
            break
        tags.append((tid, chr(mode), chr(ext) if 32 <= ext < 127 else '?', sz))
        p += 8
    FILE = [k for k, t in enumerate(tags) if not (t[1] == 'c' and t[2] in 'klhg')]
    off, pay = 2048, {}
    for k in FILE:
        pay[k] = bytes(stage[off:off + tags[k][3]])
        off += pad(tags[k][3])
    assert off == len(stage), 'payloads end at %d, block is %d' % (off, len(stage))
    n = len(tags)
    got = {t: (x, b) for t, x, b in dar_entries(pay[1])}
    w, h, pal, rows = pcx4.decode(got[SC_TEXT][1])
    prof = [sum(1 for x in range(2, 227) if rows[y][x] not in (12, 0, 11, 5)) for y in range(h)]
    starts = [y for y in range(h) if prof[y] and (y == 0 or not prof[y - 1])]
    bar = (rows[0].count(0), rows[1].count(0))
    pcxv = struct.unpack_from('<7H', got[SC_TEXT][1], 74)

    print('disc %d: %d records (%d outside DUMMY3M = the STAGE.DIR repoint),'
          ' %d tags, sc_text %dx%d at vram(%d,%d) clut(%d,%d)'
          % (disc + 1, len(recs), outside, n, w, h, pcxv[2], pcxv[3], pcxv[4], pcxv[5]))
    print('        inked lines start at rows %s; (8,8,8) bar rows 0/1 = %d/%d px'
          % (starts, bar[0], bar[1]))
    assert outside == 1, outside
    assert (w, h) == (232, SC_ROWS), (w, h)
    assert len(starts) == SC_KEEP_LINES, starts
    assert starts[0] == 0, 'line 1 is not at row 0 - the block moved'
    assert min(bar) > 150, bar
    assert all(prof[y] == 0 for y in range(starts[-1] + 12, h)), 'ink past the last kept line'
    print('        OK: %d lines, first at row 0, bar hidden at the top, nothing below'
          % len(starts))
