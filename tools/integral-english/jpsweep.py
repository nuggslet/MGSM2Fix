"""Sweep disc 1 for UI text still Japanese that USA has in English.

This historical scanner compares pointer slots at equal overlay offsets.
That established the camera caption pairing only after independent inspection;
equal offsets alone do not prove homologous tables in other overlays.
It scans disc 1 only and misses GCL, instruction references and relocated data.
Use audit_text.py for a broader candidate inventory, with explicit limitations.

Guards, both learned the hard way: a target must look like text, not code (a
MIPS prologue reads as high bytes), and must terminate.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
import sys, struct
from optscan import ents, stage

BASE_I, BASE_U = 0x800C3208, 0x800C5968

def jp_text(b):
    """The game's encoding is two-byte pairs whose FIRST byte is 0x80-0xdf and
    whose second is often low (0x82 0x1b, 0xd0 0x06 ...). Testing all bytes for
    the high range only reached ~50% and missed almost everything - test the
    lead byte of each pair instead."""
    if not (4 <= len(b) <= 80) or len(b) % 2: return False
    lead = [b[i] for i in range(0, len(b), 2)]
    return sum(1 for x in lead if 0x80 <= x <= 0xdf) >= len(lead) * 0.8

def en_text(b):
    if not (4 <= len(b) <= 70): return False
    if not all(32 <= x < 127 for x in b): return False
    return any(c.islower() for c in b.decode()) and b.count(b' ') >= 1

def targets(ov, base, ok):
    out = {}
    for i in range(0, len(ov) - 4, 4):
        w = struct.unpack_from('<I', ov, i)[0]
        if not (base <= w < base + len(ov)): continue
        off = w - base
        e = ov.find(b'\x00', off)
        if e < 0 or e - off > 80: continue
        s = ov[off:e]
        if ok(s): out[i] = s
    return out

def sweep():
    di = dict(ents(open(WORK + '/int1_stage.dir', 'rb').read()))
    du = dict(ents(open(WORK + '/usa1_stage.dir', 'rb').read()))
    hits = []
    for name in sorted(set(di) & set(du)):
        try:
            _c, ti, Fi, pi = stage(WORK + '/int1_stage.dir', name)
            _c, tu, Fu, pu = stage(WORK + '/usa1_stage.dir', name)
        except Exception:
            continue
        for k in Fi:
            if k not in Fu or chr(ti[k][1]) != 's' or chr(tu[k][1]) != 's':
                continue
            I = targets(pi[k], BASE_I, jp_text)
            U = targets(pu[k], BASE_U, en_text)
            common = sorted(set(I) & set(U))
            if common:
                hits.append((name, k, common, I, U))
    return hits

if __name__ == '__main__':
    hits = sweep()
    if not hits:
        print('no stage has a pointer slot with Japanese in Integral and English in USA')
    for name, k, common, I, U in hits:
        print('== %s tag %d: %d slot(s)' % (name, k, len(common)))
        for s in common[:14]:
            print('   0x%04X  %-22s -> %r' % (s, I[s][:18].hex(' ')[:22], U[s].decode('latin1')))
