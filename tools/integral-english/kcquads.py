"""Extract the KEY CONFIG label quads from an option overlay.

A label is drawn by:
    lui/addiu a0, <label string>      ; jal GV_StrCode (in the executable)
    a1 = v0 ; a3 = x0 ; sp+16 = y0 ; sp+20 = x1 ; sp+24 = y1
            ; sp+28 = abe ; sp+32 = orient
    jal Init_Res (inside the overlay)

Registers carry between calls, so the whole overlay is simulated linearly and
values are read at each Init_Res call rather than scanned per label.

The overlay's load address is not in its header; it is derived as the base that
gives every `key_*` string an adjacent lui+addiu reference, which is only true
of the right one (all sixteen must resolve).
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
import sys, struct, re, json
from optscan import stage, parse, strcode
import pcx4

LAB = ['key_button', 'key_sykan', 'key_syukan', 'key_normal', 'key_reverse',
       'key_action', 'key_buki', 'key_hohuku']

def derive_base(ov, strs):
    P = {}
    for i in range(0, len(ov) - 8, 4):
        a = struct.unpack_from('<I', ov, i)[0]; b = struct.unpack_from('<I', ov, i + 4)[0]
        if (a >> 26) == 0x0F and (b >> 26) == 0x09:
            rt = (a >> 16) & 31
            if ((b >> 21) & 31) == rt and ((b >> 16) & 31) == rt:
                imm = b & 0xFFFF; imm = imm - 0x10000 if imm >= 0x8000 else imm
                P.setdefault(((a & 0xFFFF) << 16) + imm, i + 4)
    for cand in sorted({a - o for a in P for o in strs.values()}):
        if 0x80080000 <= cand <= 0x80110000 and all((cand + o) in P for o in strs.values()):
            return cand
    raise SystemExit('no base resolves every key_* string')

def simulate(ov, base, strs):
    """-> list of (label_or_None, x0, y0, x1, y1, abe, orient) per Init_Res call"""
    byaddr = {base + o: n for n, o in strs.items()}
    # r0 is hardwired zero: never let a write reach it (a `nop` is
    # `sll zero,zero,0`, which would otherwise poison every addiu from zero).
    class Regs(dict):
        def __setitem__(self, k, v):
            if k: dict.__setitem__(self, k, v)
    reg = Regs({0: 0}); stack = {}; out = []; pending = None
    for i in range(0, len(ov) - 4, 4):
        w = struct.unpack_from('<I', ov, i)[0]
        op = w >> 26; rs = (w >> 21) & 31; rt = (w >> 16) & 31; rd = (w >> 11) & 31
        fn = w & 0x3F; imm = w & 0xFFFF
        simm = imm - 0x10000 if imm >= 0x8000 else imm
        if op == 0x0F:                                    # lui
            reg[rt] = imm << 16
        elif op == 0x09:                                  # addiu
            reg[rt] = None if reg.get(rs) is None else reg[rs] + simm
        elif op == 0x2B and rs == 29:                     # sw rt, imm(sp)
            stack[simm] = reg.get(rt)
        elif op == 0 and fn == 0x21:                      # addu rd, rs, rt
            a, b = reg.get(rs), reg.get(rt)
            reg[rd] = b if rs == 0 else (a if rt == 0 else None)
        elif op == 3:                                     # jal
            tgt = 0x80000000 | ((w & 0x03FFFFFF) << 2)
            if base <= tgt < base + len(ov):              # overlay-local: Init_Res
                out.append((pending, reg.get(7), stack.get(16), stack.get(20),
                            stack.get(24), stack.get(28), stack.get(32)))
                pending = None
            else:                                         # executable: GV_StrCode
                pending = byaddr.get(reg.get(4))
            for r in list(reg):
                if r not in (0, 16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30, 31):
                    reg[r] = None                         # caller-saved
        elif op == 0:
            if fn not in (8, 9): reg[rd] = None
        elif op in (0x20, 0x21, 0x23, 0x24, 0x25, 0x0C, 0x0D, 0x0E, 0x0A, 0x0B):
            reg[rt] = None
    return out

def extract(sd):
    sect, tags, F, pay = stage(sd, 'option')
    ov = pay[0]; e, _ = parse(pay[1])
    tex = {}
    for x in e:
        for n in LAB:
            if (x[0] & 0xFFFF) == (strcode(n) & 0xFFFF):
                w, h, _p, _r = pcx4.decode(x[3]); tex[n] = (w, h)
    strs = {m.group(1).decode(): m.start()
            for m in re.finditer(rb'(key_[a-z_0-9]{1,14})\x00', ov)}
    base = derive_base(ov, strs)
    calls = simulate(ov, base, strs)
    q = {}
    for lab, x0, y0, x1, y1, abe, orient in calls:
        if lab in LAB and lab not in q:
            q[lab] = (x0, y0, x1, y1, abe, orient)
    return base, tex, q

if __name__ == '__main__':
    res = {}
    for sd, label in ((WORK + '/usa1_stage.dir', 'USA'), (WORK + '/int1_stage.dir', 'INTEGRAL')):
        base, tex, q = extract(sd)
        print('== %s  overlay base 0x%08X' % (label, base))
        print('   %-12s %-9s %-26s %s' % ('texture', 'size', 'quad (x0,y0,x1,y1)', 'abe,or'))
        ok = 0
        for n in LAB:
            a = q.get(n, (None,) * 6)
            good = None not in a[:4] and (a[2] - a[0], a[3] - a[1]) == tex[n]
            ok += good
            print('   %-12s %-9s (%4s,%4s,%4s,%4s)      %-7s %s'
                  % (n, '%dx%d' % tex[n], a[0], a[1], a[2], a[3],
                     '%s,%s' % (a[4], a[5]), '' if good else '<-- quad != texture'))
        print('   %d/8 quads equal their texture size' % ok)
        res[label] = {'base': base, 'tex': tex, 'quads': q}
    json.dump(res, open(WORK + '/keyconfig_quads.json', 'w'), indent=1)
    print('\nwrote work/keyconfig_quads.json')
