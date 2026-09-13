"""Find MIPS I load-delay hazards in hand-written code.

    py hazards.py <built.bin> <base-hex> [--retail <retail.bin>] [--range lo-hi ...]

The R3000 has a one-instruction load delay: the instruction right after a
load (lb/lbu/lh/lhu/lw/lwl/lwr) still sees the register's OLD value. Every
compiler for the machine schedules around it - a nop or an unrelated
instruction between a load and its first use - so retail code has none. Code
written by hand for this port did not always do that, and the Master
Collection's emulator does not model the delay, so nothing was ever wrong
there. SwanStation and real hardware do model it, and the briefing's right
column was the result (NextSteps 24: the row box's `lbu a1` then `subu ..a1`
read the caller's `idx` instead of the texture's `v0`).

What is reported, for the built image only where a retail image is given
(instructions that are byte-identical in both are skipped, so this is a check
on what the port wrote and never on the compiler's output):

  load-use     a load into R followed immediately by an instruction reading R
  load-write   a load into R followed immediately by a non-load writing R
               (the load's value lands last on the R3000 and overwrites it;
               a second load into R is the compiler's own idiom and is fine)
  branch-slot  a branch or jump in the delay slot of a branch or jump
  mfhilo       mfhi/mflo followed within two instructions by mult/div, whose
               result is then undefined

The exit status is the number of findings, so a build can assert on it.
"""
import struct, sys, argparse

LOADS = {0x20: 'lb', 0x21: 'lh', 0x22: 'lwl', 0x23: 'lw', 0x24: 'lbu', 0x25: 'lhu', 0x26: 'lwr'}
STORES = {0x28: 'sb', 0x29: 'sh', 0x2A: 'swl', 0x2B: 'sw', 0x2E: 'swr'}
IMM = {0x08: 'addi', 0x09: 'addiu', 0x0A: 'slti', 0x0B: 'sltiu', 0x0C: 'andi', 0x0D: 'ori', 0x0E: 'xori'}
SPECIAL = {0x00: 'sll', 0x02: 'srl', 0x03: 'sra', 0x04: 'sllv', 0x06: 'srlv', 0x07: 'srav',
           0x08: 'jr', 0x09: 'jalr', 0x0C: 'syscall', 0x0D: 'break', 0x10: 'mfhi', 0x11: 'mthi',
           0x12: 'mflo', 0x13: 'mtlo', 0x18: 'mult', 0x19: 'multu', 0x1A: 'div', 0x1B: 'divu',
           0x20: 'add', 0x21: 'addu', 0x22: 'sub', 0x23: 'subu', 0x24: 'and', 0x25: 'or',
           0x26: 'xor', 0x27: 'nor', 0x2A: 'slt', 0x2B: 'sltu'}
REGS = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3', 't0', 't1', 't2', 't3', 't4', 't5',
        't6', 't7', 's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7', 't8', 't9', 'k0', 'k1',
        'gp', 'sp', 'fp', 'ra']


def decode(w):
    """-> (mnemonic, reads:set, writes:set, is_load, is_branch, hilo:'r'|'w'|None)"""
    op = w >> 26
    rs, rt, rd = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    fn = w & 0x3F
    if w == 0:
        return 'nop', set(), set(), False, False, None
    if op == 0:
        m = SPECIAL.get(fn, 'special%02x' % fn)
        if fn in (0x00, 0x02, 0x03):            return m, {rt}, {rd}, False, False, None
        if fn in (0x04, 0x06, 0x07):            return m, {rs, rt}, {rd}, False, False, None
        if fn == 0x08:                          return m, {rs}, set(), False, True, None
        if fn == 0x09:                          return m, {rs}, {rd}, False, True, None
        if fn in (0x0C, 0x0D):                  return m, set(), set(), False, False, None
        if fn in (0x10, 0x12):                  return m, set(), {rd}, False, False, 'r'
        if fn in (0x11, 0x13):                  return m, {rs}, set(), False, False, 'w'
        if fn in (0x18, 0x19, 0x1A, 0x1B):      return m, {rs, rt}, set(), False, False, 'w'
        return m, {rs, rt}, {rd}, False, False, None
    if op == 1:                                 # bltz/bgez/bltzal/bgezal
        return 'b%dz' % rt, {rs}, ({31} if rt & 0x10 else set()), False, True, None
    if op == 2:                                 return 'j', set(), set(), False, True, None
    if op == 3:                                 return 'jal', set(), {31}, False, True, None
    if op in (4, 5):                            return ('beq', 'bne')[op - 4], {rs, rt}, set(), False, True, None
    if op in (6, 7):                            return ('blez', 'bgtz')[op - 6], {rs}, set(), False, True, None
    if op in IMM:                               return IMM[op], {rs}, {rt}, False, False, None
    if op == 0x0F:                              return 'lui', set(), {rt}, False, False, None
    if op in (0x10, 0x11, 0x12, 0x13):          # coprocessor moves: mfc/cfc write rt, mtc/ctc read it
        sub = rs
        if sub in (0, 2):                       return 'mfc%d' % (op & 3), set(), {rt}, True, False, None
        if sub in (4, 6):                       return 'mtc%d' % (op & 3), {rt}, set(), False, False, None
        return 'cop%d' % (op & 3), set(), set(), False, False, None
    if op in LOADS:                             return LOADS[op], {rs}, {rt}, True, False, None
    if op in STORES:                            return STORES[op], {rs, rt}, set(), False, False, None
    if op in (0x32, 0x3A):                      return ('lwc2', 'swc2')[op == 0x3A], {rs}, set(), False, False, None
    return 'op%02x' % op, set(), set(), False, False, None


def words(b):
    n = len(b) // 4
    return list(struct.unpack('<%dI' % n, b[:n * 4]))


def scan(built, base, retail=None, ranges=None):
    W = words(built)
    R = words(retail) if retail is not None else None
    D = [decode(w) for w in W]
    out = []

    def changed(i):
        if R is None: return True
        return i >= len(R) or R[i] != W[i]

    def inrange(a):
        return not ranges or any(lo <= a <= hi for lo, hi in ranges)

    for i in range(len(W) - 1):
        a = base + 4 * i
        if not inrange(a): continue
        # only judge pairs the port wrote at least one instruction of
        if not (changed(i) or changed(i + 1)): continue
        m0, r0, w0, ld0, br0, hl0 = D[i]
        m1, r1, w1, ld1, br1, hl1 = D[i + 1]
        if ld0:
            for reg in w0:
                if reg == 0: continue
                if reg in r1:
                    out.append((a, 'load-use', '%s $%s ... then %s reads $%s' % (m0, REGS[reg], m1, REGS[reg])))
                elif reg in w1 and not ld1:
                    # a second LOAD into the same register is the compiler's own
                    # idiom (retail brf has 71 of them) and is well defined: the
                    # later load lands later. Any other writer in the slot is not.
                    out.append((a, 'load-write', '%s $%s ... then %s writes $%s' % (m0, REGS[reg], m1, REGS[reg])))
        if br0 and br1:
            out.append((a, 'branch-slot', '%s then %s in its delay slot' % (m0, m1)))
        if hl0 == 'r':
            for k in (1, 2):
                if i + k < len(D) and D[i + k][5] == 'w' and D[i + k][0] in ('mult', 'multu', 'div', 'divu'):
                    out.append((a, 'mfhilo', '%s then %s %d later' % (m0, D[i + k][0], k)))
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('built')
    ap.add_argument('base', help='load address of the first byte, hex')
    ap.add_argument('--retail', help='the unpatched image; pairs identical in both are not judged')
    ap.add_argument('--range', action='append', default=[], help='lo-hi, hex; may repeat')
    ap.add_argument('--quiet', action='store_true')
    a = ap.parse_args(argv)
    base = int(a.base, 16)
    ranges = []
    for r in a.range:
        lo, hi = r.split('-')
        ranges.append((int(lo, 16), int(hi, 16)))
    built = open(a.built, 'rb').read()
    retail = open(a.retail, 'rb').read() if a.retail else None
    found = scan(built, base, retail, ranges)
    if not a.quiet:
        for addr, kind, what in found:
            print('%08X  %-11s %s' % (addr, kind, what))
        print('%d hazard%s' % (len(found), '' if len(found) == 1 else 's'))
    return len(found)


if __name__ == '__main__':
    sys.exit(min(main(), 255))
