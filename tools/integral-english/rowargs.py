"""Linear register simulation over a brf overlay, reporting the arguments of
every call to the row positioner.

Evaluates the full arithmetic set, not just addiu/addu: USA sets one advance
with `ori a3, s6, 10` (17 | 10 = 27), which a simpler simulator reports as
"unknown" and invites a wrong guess. $zero is hardwired - writing it corrupts
every later value, so setr() discards those writes.
"""
import struct

CALLER_SAVED = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25]


def run(path, base, fn):
    return run_bytes(open(path, 'rb').read(), base, fn)


def run_bytes(b, base, fn):
    W = list(struct.unpack('<%dI' % (len(b) // 4), b[:len(b) // 4 * 4]))
    jal = 0x0C000000 | ((fn >> 2) & 0x03FFFFFF)
    reg = [None] * 32
    reg[0] = 0
    stack = {}
    out = []
    pending = None

    def setr(r, v):
        if r:
            reg[r] = v

    for i, w in enumerate(W):
        a = base + 4 * i
        op = w >> 26
        rs, rt, rd = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
        sa, f6 = (w >> 6) & 31, w & 0x3F
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm >= 0x8000 else imm
        x, y = reg[rs], reg[rt]

        if op == 9:                                     # addiu
            setr(rt, None if x is None else x + simm)
        elif op == 0x0D:                                # ori
            setr(rt, None if x is None else x | imm)
        elif op == 0x0C:                                # andi
            setr(rt, None if x is None else x & imm)
        elif op == 0x0E:                                # xori
            setr(rt, None if x is None else x ^ imm)
        elif op == 0x0F:                                # lui
            setr(rt, imm << 16)
        elif op == 0x2B and rs == 29:                   # sw rt, n(sp)
            stack[simm] = y
        elif op == 0:
            if f6 in (0x20, 0x21):                      # add / addu
                setr(rd, None if (x is None or y is None) else x + y)
            elif f6 in (0x22, 0x23):                    # sub / subu
                setr(rd, None if (x is None or y is None) else x - y)
            elif f6 == 0x25:                            # or
                setr(rd, None if (x is None or y is None) else x | y)
            elif f6 == 0x24:                            # and
                setr(rd, None if (x is None or y is None) else x & y)
            elif f6 == 0x00:                            # sll
                setr(rd, None if y is None else (y << sa) & 0xFFFFFFFF)
            elif f6 == 0x02:                            # srl
                setr(rd, None if y is None else (y & 0xFFFFFFFF) >> sa)
            elif f6 == 0x03:                            # sra
                setr(rd, None if y is None else y >> sa)
            elif f6 in (8, 9):                          # jr / jalr
                pass
            else:
                setr(rd, None)
        elif op in (0x20, 0x21, 0x23, 0x24, 0x25, 0x0A, 0x0B):
            setr(rt, None)                              # loads, slti

        if pending is not None:      # arguments are final after the delay slot
            out.append((pending, reg[5], reg[7], stack.get(16), stack.get(20)))
            pending = None
            for r in CALLER_SAVED:
                reg[r] = None
        if w == jal:
            pending = a
    return out
