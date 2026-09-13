"""Extract the four KEY CONFIG label rectangles, per button type, from the
option overlay's per-type function (opt.c ~line 778: it rewrites poly[13..16]
every frame, which OVERRIDES whatever Init_Res set).

POLY_FT4: x0 at +8, y0 +10, x1 +16, y1 +18, x2 +24, y2 +26, x3 +32, y3 +34,
40 bytes per poly. Registers are simulated so `sh rt, off(rs)` can be attributed
to a poly index via the base register's offset from work->field_674.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
import sys, struct
from optscan import stage

FIELD = {8: 'x0', 10: 'y0', 16: 'x1', 18: 'y1', 24: 'x2', 26: 'y2', 32: 'x3', 34: 'y3'}

def rects(sd, lo, hi):
    sect, tags, F, pay = stage(sd, 'option')
    ov = pay[0]
    reg = {0: 0}
    out = []            # (code offset, base_reg_value, field, value)
    for i in range(lo, hi, 4):
        w = struct.unpack_from('<I', ov, i)[0]
        op = w >> 26; rs = (w >> 21) & 31; rt = (w >> 16) & 31; rd = (w >> 11) & 31
        fn = w & 0x3F; imm = w & 0xFFFF
        simm = imm - 0x10000 if imm >= 0x8000 else imm
        if op == 0x09:
            if rt: reg[rt] = None if reg.get(rs) is None else reg[rs] + simm
        elif op == 0x0F:
            if rt: reg[rt] = imm << 16
        elif op == 0x29:                       # sh rt, imm(rs)
            out.append((i, reg.get(rs), simm, reg.get(rt)))
        elif op == 0 and fn == 0x21:
            if rd: reg[rd] = reg.get(rt) if rs == 0 else (reg.get(rs) if rt == 0 else None)
        elif op == 0:
            if rd and fn not in (8, 9): reg[rd] = None
        elif op in (0x20, 0x21, 0x23, 0x24, 0x25, 0x0C, 0x0D, 0x0E, 0x0A, 0x0B):
            if rt: reg[rt] = None
    return out

if __name__ == '__main__':
    for sd, label, lo, hi in ((WORK + '/int1_stage.dir', 'INTEGRAL', 0x1200, 0x1900),
                              (WORK + '/usa1_stage.dir', 'USA', 0x1200, 0x1900)):
        print('== %s' % label)
        cur = {}
        for off, basev, fieldoff, val in rects(sd, lo, hi):
            f = FIELD.get(fieldoff % 40)
            idx = fieldoff // 40
            if f is None or val is None: continue
            cur.setdefault((basev, idx), {})[f] = val
        for k in sorted(cur, key=lambda t: (t[0] is None, t[0] or 0, t[1])):
            d = cur[k]
            if {'x0','y0','x3','y3'} <= set(d):
                print('   base=%s poly+%d  (%d,%d)-(%d,%d)  %dx%d'
                      % (k[0], k[1], d['x0'], d['y0'], d['x3'], d['y3'],
                         d['x3']-d['x0'], d['y3']-d['y0']))
