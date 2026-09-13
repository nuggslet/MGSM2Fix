"""Inspect a stage's tags, DAR entries and GCL chain in either build.

Written to answer where the Option -> SCREEN brightness paragraph lives.  The
answer, and the reason the two builds had to be compared entry by entry: USA
draws it as one 232x70 texture, `sc_text`, that Integral's DAR does not contain
and Integral's overlay never names; Integral draws it as font text from the
option chain (records 13-16 plus 24).  Run as a script it diffs the two option
DARs, which is how sc_text turned up - it is the only large texture USA has and
Integral does not.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
import struct, sys

def pad(x, a=2048): return (x + a - 1) // a * a

def ents(d):
    h = struct.unpack('<I', d[:4])[0]; o = []
    for p in range(4, h + 12, 12):
        n = d[p:p+8].rstrip(b'\x00')
        if n: o.append((n.decode('latin1', 'replace'), struct.unpack('<I', d[p+8:p+12])[0]))
    return o

def stage(path, name):
    d = open(path, 'rb').read(); base = dict(ents(d))[name] * 2048
    ver, pdb, sect = struct.unpack('<BBh', d[base:base+4])
    tags = []; p = base + 4
    while True:
        tid, mode, ext, sz = struct.unpack('<HBBi', d[p:p+8])
        if mode == 0: break
        tags.append([tid, mode, ext, sz]); p += 8
    F = [k for k, t in enumerate(tags) if not (chr(t[1]) == 'c' and chr(t[2]) in 'klhg')]
    off, pay = 2048, {}
    for k in F:
        pay[k] = d[base+off:base+off+tags[k][3]]; off += pad(tags[k][3])
    return sect, tags, F, pay

def parse(b):
    out = []; p = 0
    while p + 8 <= len(b):
        tid, ext, size = struct.unpack('<HhI', b[p:p+8])
        if size <= 0 or p + 8 + size > len(b): break
        out.append([tid, ext, size, b[p+8:p+8+size]]); p += 8 + size
    return out, b[p:]

def geo(b):
    bpp = b[3] * b[65]
    maxx, maxy = struct.unpack('<HH', b[8:12]); minx, miny = struct.unpack('<HH', b[4:8])
    st, fl, px, py, cx, cy, nc = struct.unpack('<7H', b[74:88])
    assert st == 12345, 'no PCXINFO'
    return dict(w=maxx-minx+1, h=maxy-miny+1, bpp=bpp, px=px, py=py, cx=cx, cy=cy, nc=nc)

def strcode(s):
    i = 0
    for ch in s.encode(): i = (((i << 5) | (i >> 11)) & 0xFFFF); i = (i + ch) & 0xFFFF
    return i

NAMES = ('sc_back_l', 'sc_back_r', 'sc_option', 'sc_text', 'op_screen', 'op_exit')

def find(path, stg='option', names=NAMES):
    want = {strcode(n): n for n in names}
    sect, tags, F, pay = stage(path, stg)
    hits = []
    for k in F:
        if chr(tags[k][2]) != 'd': continue
        e, _ = parse(pay[k])
        for j, x in enumerate(e):
            if x[0] in want:
                hits.append((want[x[0]], k, j, x))
    return hits

def chain(path, stg='option', off=0x1B8, tag=6):
    """The option stage's text records: `07 <len> <payload> 00`, inline in the
    0xFF chunk.  GCL_GetString returns a pointer into this, so what is here is
    what the KCB entries draw."""
    sect, tags, F, pay = stage(path, stg)
    scr = pay[tag]; p = off; out = []
    while p < len(scr) and scr[p] == 7:
        n = scr[p+1]; out.append(scr[p+2:p+2+n]); p += 2 + n
    return out


def dar(path, stg='option'):
    sect, tags, F, pay = stage(path, stg)
    e, _ = parse(pay[1])
    out = {}
    for j, x in enumerate(e):
        try: out[x[0]] = (j, geo(x[3]), x[2])
        except Exception: pass
    return out


if __name__ == '__main__':
    import sys
    stg = sys.argv[1] if len(sys.argv) > 1 else 'option'
    D = {}
    for path, tag in ((WORK + '/us1_stage.dir', 'USA'), (WORK + '/int1_stage.dir', 'INTEGRAL')):
        D[tag] = dar(path, stg)
        print('=== %s %s: %d PCX entries' % (tag, stg, len(D[tag])))
        for name, k, j, x in find(path, stg):
            g = geo(x[3])
            print('   %-10s id=%04X [%2d] %3dx%-3d %dbpp vram(%d,%d) clut(%d,%d) %d colours'
                  % (name, x[0], j, g['w'], g['h'], g['bpp'], g['px'], g['py'], g['cx'], g['cy'], g['nc']))
        for i, r in enumerate(chain(path, stg)):
            b = r.rstrip(bytes(1))  # strip the trailing NUL
            ascii_ = all(32 <= c < 127 for c in b) and b
            print('   chain[%2d] len=%-3d %s' % (i, len(r),
                  b.decode('latin1') if ascii_ else
                  ' '.join('%04X' % int.from_bytes(b[k:k+2], 'big') for k in range(0, len(b), 2))))
    only_usa = sorted(set(D['USA']) - set(D['INTEGRAL']))
    only_int = sorted(set(D['INTEGRAL']) - set(D['USA']))
    print('=== textures only in USA:      %s' % [('%04X %dx%d' % (i, D['USA'][i][1]['w'], D['USA'][i][1]['h'])) for i in only_usa])
    print('=== textures only in Integral: %s' % [('%04X %dx%d' % (i, D['INTEGRAL'][i][1]['w'], D['INTEGRAL'][i][1]['h'])) for i in only_int])
