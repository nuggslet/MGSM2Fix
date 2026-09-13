"""KEY CONFIG geometry of a VR-DISC option overlay, read from its code.

Two things position the eight `key_*` labels (the same program as the main
game's opt.c, whose findings this follows - README "How the KEY CONFIG port
was built"):

  * `Init_Res(work, strcode, poly, x0, sp+16=y0, sp+20=x1, sp+24=y1, abe,
    orient)` calls set every label's first quad and its blend flag;
  * a per-button-type function (entered through `lw <type>, 9760(work)`,
    then three blocks) rewrites the four movable labels' rectangles every
    frame, so for key_action / key_buki / key_hohuku / key_syukan that
    function is the authority, not Init_Res.

`init_res_calls` simulates the overlay linearly (kcquads.py's method, with
the offset of the instruction that produced each value); `type_rects`
executes the per-type function for one button type and returns the poly
fields it stores, work-relative. Overlays load at 0x800C11A0 (Integral VR)
and 0x800C4350 (USA VR) - the executables' _bss_objend.
"""
import re
import struct

INT_BASE, USA_BASE = 0x800C11A0, 0x800C4350
LABELS = ['key_button', 'key_sykan', 'key_syukan', 'key_normal', 'key_reverse', 'key_action', 'key_buki', 'key_hohuku']
TYPE_FIELD = 9760          # work->button type, read by the per-type function
POLY_BASE = 1612           # work + 1612 = poly[0] (POLY_FT4, 40 bytes each; x0 +8, y0 +10, x1 +16, y1 +18, x2 +24, y2 +26, x3 +32, y3 +34)


def key_strings(ov):
    return {m.group(1).decode(): m.start() for m in re.finditer(rb'(key_[a-z_0-9]{1,14})\x00', ov)}


def init_res_calls(ov, base):
    """[(jal offset, label, x0, y0, x1, y1, abe, orient)] with every value a
    (value, offset of the addiu/lui that made it) pair or None"""
    byaddr = {base + o: n for n, o in key_strings(ov).items()}

    class Regs(dict):
        def __setitem__(self, k, v):
            if k:
                dict.__setitem__(self, k, v)
    reg = Regs({0: (0, None)}); stack = {}; out = []; pending = None
    for i in range(0, len(ov) - 4, 4):
        w = struct.unpack_from('<I', ov, i)[0]
        op = w >> 26; rs = (w >> 21) & 31; rt = (w >> 16) & 31; rd = (w >> 11) & 31
        fn = w & 0x3F; imm = w & 0xFFFF
        simm = imm - 0x10000 if imm >= 0x8000 else imm
        if op == 0x0F:
            reg[rt] = (imm << 16, i)
        elif op == 0x09:
            src = reg.get(rs)
            reg[rt] = None if src is None else (src[0] + simm, i if rs == 0 else src[1])
        elif op == 0x2B and rs == 29:
            stack[simm] = reg.get(rt)
        elif op == 0 and fn == 0x21:
            a, b = reg.get(rs), reg.get(rt)
            reg[rd] = b if rs == 0 else (a if rt == 0 else None)
        elif op == 3:
            tgt = 0x80000000 | ((w & 0x03FFFFFF) << 2)
            # the delay slot runs before the callee: a `sw rt, imm(sp)` there is an argument too
            w2 = struct.unpack_from('<I', ov, i + 4)[0]
            if (w2 >> 26) == 0x2B and ((w2 >> 21) & 31) == 29:
                imm2 = w2 & 0xFFFF
                stack[imm2 - 0x10000 if imm2 >= 0x8000 else imm2] = reg.get((w2 >> 16) & 31)
            if base <= tgt < base + len(ov):
                out.append((i, pending, reg.get(7), stack.get(16), stack.get(20), stack.get(24), stack.get(28), stack.get(32)))
                pending = None
            else:
                a4 = reg.get(4)
                pending = byaddr.get(a4[0]) if a4 else None
            for r in list(reg):
                if r not in (0, 16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30, 31):
                    reg[r] = None
        elif op == 0:
            if fn not in (8, 9):
                reg[rd] = None
        elif op in (0x20, 0x21, 0x23, 0x24, 0x25, 0x0C, 0x0D, 0x0E, 0x0A, 0x0B):
            reg[rt] = None
    return out


def label_quads(ov, base):
    """{label: (x0, y0, x1, y1, abe, orient)} from the Init_Res calls"""
    q = {}
    for call, lab, x0, y0, x1, y1, abe, orient in init_res_calls(ov, base):
        if lab in LABELS and lab not in q:
            q[lab] = tuple(v[0] if v else None for v in (x0, y0, x1, y1, abe, orient))
    return q


def find_type_function(ov):
    """offset of `lw rX, 9760(a0)` that opens the per-type function"""
    for i in range(0, len(ov) - 16, 4):
        w = struct.unpack_from('<I', ov, i)[0]
        if (w >> 26) == 35 and ((w >> 21) & 31) == 4 and (w & 0xFFFF) == TYPE_FIELD:
            nxt = struct.unpack_from('<I', ov, i + 8)[0]
            if (nxt >> 26) == 5:                      # bne on the loaded type
                return i
    raise LookupError('per-type function not found')


def function_end(ov, start):
    """offset just past the LAST `jr ra` + delay slot before the next function's
    first instruction (the per-type function has three early returns)"""
    # the next function begins with `lw rX, 9764(a0)` in both overlays; stop there
    for i in range(start + 4, len(ov) - 4, 4):
        w = struct.unpack_from('<I', ov, i)[0]
        if (w >> 26) == 35 and ((w >> 21) & 31) == 4 and (w & 0xFFFF) == TYPE_FIELD + 4:
            return i
    raise LookupError('end of the per-type function not found')


def type_rects(ov, start, btype, base):
    """execute the per-type function for button type `btype`:
    -> ({poly index: {field offset: (value, source offset)}}, {colour byte field: value})"""
    reg = {i: None for i in range(32)}
    reg[0] = (0, None); reg[4] = ('work', None)
    fields, bytes_ = {}, {}
    pc, steps = start, 0

    def step(pc, w):
        op = w >> 26; rs = (w >> 21) & 31; rt = (w >> 16) & 31; rd = (w >> 11) & 31; fn = w & 63
        imm = w & 0xFFFF; simm = imm - 65536 if imm >= 32768 else imm
        if op == 35:
            v = reg[rs]
            reg[rt] = (btype, pc) if v and v[0] == 'work' and simm == TYPE_FIELD else None
        elif op == 9:
            v = reg[rs]
            if v is None: reg[rt] = None
            elif v[0] == 'work': reg[rt] = (('work', simm), pc)
            elif isinstance(v[0], tuple): reg[rt] = ((v[0][0], v[0][1] + simm), v[1])
            else: reg[rt] = (v[0] + simm, pc if rs == 0 else v[1])
        elif op == 15:
            reg[rt] = (imm << 16, pc)
        elif op in (41, 40):
            b = reg[rs]; v = reg[rt]
            if b and isinstance(b[0], tuple):
                f = b[0][1] + simm
                if op == 41:
                    fields[f] = (v[0] if v else None, v[1] if v else None)
                else:
                    bytes_[f] = v[0] if v else None
        elif op == 0 and fn == 33:
            a, b = reg[rs], reg[rt]
            reg[rd] = b if rs == 0 else (a if rt == 0 else None)
        elif op == 0:
            if rd: reg[rd] = None
        elif op in (32, 33, 36, 37, 12, 13, 14, 10, 11, 8):
            reg[rt] = None
    while steps < 4000:
        w = struct.unpack_from('<I', ov, pc)[0]
        op = w >> 26; rs = (w >> 21) & 31; rt = (w >> 16) & 31; fn = w & 63
        imm = w & 0xFFFF; simm = imm - 65536 if imm >= 32768 else imm
        if op == 0 and fn == 8:                        # jr ra: done (delay slot is a nop)
            break
        if op in (4, 5):                               # beq / bne
            a, b = reg[rs], reg[rt]
            if a is None or b is None or isinstance(a[0], tuple) or isinstance(b[0], tuple):
                raise RuntimeError('branch on an unknown value at %X' % pc)
            taken = (a[0] != b[0]) if op == 5 else (a[0] == b[0])
            step(pc + 4, struct.unpack_from('<I', ov, pc + 4)[0])      # delay slot
            pc = pc + 4 + simm * 4 if taken else pc + 8
        elif op == 2:                                  # j (within the overlay)
            target = (base & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            step(pc + 4, struct.unpack_from('<I', ov, pc + 4)[0])
            pc = target - base
        elif op == 3:
            raise RuntimeError('jal at %X' % pc)
        else:
            step(pc, w)
            pc += 4
        steps += 1
    polys = {}
    for f, v in fields.items():
        i, off = divmod(f - POLY_BASE, 40)
        polys.setdefault(i, {})[off] = v
    return polys, bytes_


def rect_of(poly):
    """(x0, y0, x3, y3) of a poly's fields, None where a field is unset"""
    g = lambda k: poly.get(k, (None, None))[0]
    return g(8), g(10), g(32), g(34)
