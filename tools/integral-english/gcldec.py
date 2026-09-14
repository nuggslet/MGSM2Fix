#!/usr/bin/env python
"""A GCL value decoder, written from FoxdieTeam/mgs_reversing source/libgcl.

GCL_GetNextValue (parse.c) advances the pointer per value type.  With p pointing
at the opcode byte, the next value starts at:

    GCL_VAR   (tag 0x10)  p + 4          # opcode byte is part of the BE32 word
    GCL_END   0x00        -              # terminates a value list
    GCL_SHORT 0x01        p + 3
    GCL_BYTE/CHAR/BOOL    p + 2          # 0x02 0x03 0x04
    GCL_STRID/PROCID      p + 3          # 0x06 0x08
    GCL_STRING 0x07       p + 2 + p[1]   # p[1] is the length byte
    GCL_INT/SYMBOL        p + 5          # 0x09 0x0a
    GCL_ARRAY 0x20        p + 2
    GCL_EXPR  0x30        p + 1 + p[1]
    GCL_ARG   0x40        p + 1 + BE16(p+1)      <-- a BLOCK LENGTH, not a target
    GCL_OPTION 0x50       p + 2 + p[2]

The important one is GCL_ARG: `size = GCL_GetShort(p); p += size;` after the
opcode has been consumed.  So a GCL_ARG spans [p+1, p+1+size) and its contents
are themselves a value list.  Growing a string inside such a block therefore
means growing that block's size field - and every enclosing block's, nested all
the way out.  Blocks that do not contain the growth point need no change.
"""
import struct

VAR, END, SHORT, BYTE, CHAR, BOOL = 0x10, 0x00, 0x01, 0x02, 0x03, 0x04
STRID, STRING, PROCID, INT, SYMBOL = 0x06, 0x07, 0x08, 0x09, 0x0a
ARRAY, EXPR, ARG, OPTION, COMMAND, PROC = 0x20, 0x30, 0x40, 0x50, 0x60, 0x70

def be16(d, p): return (d[p] << 8) | d[p+1]

def step(d, p):
    """size of the value at p, or None if p is not a decodable value"""
    op = d[p]
    if (op & 0xF0) == VAR:            return 4
    if op == END:                     return 1
    if op == SHORT:                   return 3
    if op in (BYTE, CHAR, BOOL):      return 2
    if op in (STRID, PROCID):         return 3
    if op == STRING:                  return 2 + d[p+1]
    if op in (INT, SYMBOL):           return 5
    if op == ARRAY:                   return 2
    if op == EXPR:                    return 1 + d[p+1] if d[p+1] else None
    if op == ARG:
        n = be16(d, p+1)
        return 1 + n if n >= 3 else None
    if op == OPTION:                  return 2 + d[p+2]
    return None                        # COMMAND/PROC or junk: not a value

def walk(d, p, end, limit=100000):
    """decode a value list from p; return True iff it lands exactly on end"""
    n = 0
    while p < end and n < limit:
        s = step(d, p)
        if s is None or s <= 0: return False
        p += s; n += 1
    return p == end

def blocks_covering(d, lo, hi, span_lo, span_hi):
    """every GCL_ARG in [lo,hi) whose block strictly encloses [span_lo,span_hi)
    and whose contents decode cleanly to the block end"""
    out = []
    for p in range(lo, hi):
        if d[p] != ARG: continue
        n = be16(d, p+1)
        if n < 3: continue
        start, end = p + 1, p + 1 + n
        if not (start <= span_lo and end >= span_hi): continue
        if walk(d, p + 3, end):
            out.append((p, n, start, end))
    return out

def chain_at(d, p):
    """the run of GCL_STRING records starting at p"""
    out = []
    while d[p] == STRING:
        L = d[p+1]
        if L < 1 or d[p+1+L] != 0: break
        out.append((p, L, bytes(d[p+2:p+1+L])))
        p += 2 + L
    return out

# ---- block level (GCL_ExecBlock, command.c) -------------------------------
# GCL_EXPR    0x30  <u8  size>  next = p+1+size
# GCL_COMMAND 0x60  <BE16 size> next = p+1+size ; GCL_Command(p+3)
# GCL_PROC    0x70  <u8  size>  next = p+1+size ; GCL_Proc(p+2)
# GCL_END     0x00  terminates
def bstep(d, p):
    op = d[p]
    if op == 0x00: return 1
    if op == 0x30: return 1 + d[p+1] if d[p+1] else None
    if op == 0x70: return 1 + d[p+1] if d[p+1] else None
    if op == 0x60:
        n = be16(d, p+1)
        return 1 + n if n >= 3 else None
    return None

def walk_block(d, p, end, limit=200000):
    n = 0
    while p < end and n < limit:
        s = bstep(d, p)
        if s is None or s <= 0: return False
        p += s; n += 1
    return p == end

CONTAINERS = {0x40: ('GCL_ARG', 16), 0x60: ('GCL_COMMAND', 16),
              0x70: ('GCL_PROC', 8),  0x30: ('GCL_EXPR', 8)}

def containers_covering(d, lo, hi, span_lo, span_hi):
    """every container in [lo,hi) enclosing [span_lo,span_hi) whose contents
    decode cleanly to its own end, by either the value or the block grammar"""
    out = []
    for p in range(lo, hi):
        op = d[p]
        if op not in CONTAINERS: continue
        name, bits = CONTAINERS[op]
        n = be16(d, p+1) if bits == 16 else d[p+1]
        if n < 3: continue
        start, end = p + 1, p + 1 + n
        if not (start <= span_lo and end >= span_hi): continue
        body = p + 3 if op in (0x40, 0x60) else p + 2
        how = None
        if walk(d, body, end):       how = 'values'
        elif walk_block(d, body, end): how = 'block'
        elif op == 0x60 and walk(d, p + 6, end): how = 'cmd-args'
        if how: out.append((p, op, name, n, start, end, how))
    return out
