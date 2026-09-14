#!/usr/bin/env python
"""Top-down GCL script parser, per FoxdieTeam/mgs_reversing source/libgcl.

Layout (GCL_LoadScript / GCL_ExecScript, command.c):

    script_len : u32 big-endian, immediately before the body
    script_body: starts with GCL_ARG (0x40); GCL_ExecScript asserts this
                 font lives at script_body + script_len
    GCL_ExecBlock(script_body + 3) walks the command list

Every container carries its own size, so a parse that lands exactly on each
declared end is self-checking: if the grammar or an edit is wrong, the walk
desynchronises and fails loudly instead of producing a plausible tree.
"""

def be16(d, p): return (d[p] << 8) | d[p+1]
def be32(d, p): return (d[p] << 24) | (d[p+1] << 16) | (d[p+2] << 8) | d[p+3]

class Bad(Exception): pass

class Node:
    __slots__ = ('kind', 'pos', 'end', 'size_at', 'size_bits', 'kids', 'info')
    def __init__(self, kind, pos, end, size_at=None, size_bits=0, info=''):
        self.kind, self.pos, self.end = kind, pos, end
        self.size_at, self.size_bits, self.info = size_at, size_bits, info
        self.kids = []
    def __repr__(self):
        return '%s[0x%X,0x%X)%s' % (self.kind, self.pos, self.end, self.info)

def parse_values(d, p, end, out):
    """a GCL value list (GCL_GetNextValue, parse.c). Returns the end position."""
    while p < end:
        op = d[p]
        if (op & 0xF0) == 0x10:                       # GCL_VAR: BE32 incl opcode
            out.append(Node('VAR', p, p+4)); p += 4; continue
        if op == 0x00:
            out.append(Node('END', p, p+1)); return p + 1
        if op == 0x01:   n = Node('SHORT',  p, p+3)
        elif op in (0x02, 0x03, 0x04): n = Node('BYTE', p, p+2)
        elif op in (0x06, 0x08):       n = Node('STRID', p, p+3)
        elif op == 0x07:
            L = d[p+1]
            n = Node('STRING', p, p+2+L, size_at=p+1, size_bits=8,
                     info=' %r' % bytes(d[p+2:p+1+L])[:28])
        elif op in (0x09, 0x0a):       n = Node('INT', p, p+5)
        elif op == 0x20:               n = Node('ARRAY', p, p+2)
        elif op == 0x30:
            L = d[p+1]
            if not L: raise Bad('EXPR size 0 at 0x%X' % p)
            n = Node('EXPR', p, p+1+L, size_at=p+1, size_bits=8)
        elif op == 0x40:
            L = be16(d, p+1)
            if L < 3: raise Bad('ARG size %d at 0x%X' % (L, p))
            n = Node('ARG', p, p+1+L, size_at=p+1, size_bits=16)
            parse_block(d, p+3, n.end, n.kids)         # contents are a block
        elif op == 0x50:
            L = d[p+2]
            n = Node('OPTION', p, p+2+L, size_at=p+2, size_bits=8,
                     info=" '%c'" % chr(d[p+1]) if 32 <= d[p+1] < 127 else '')
        else:
            raise Bad('bad value opcode 0x%02X at 0x%X' % (op, p))
        if n.end > end: raise Bad('%s overruns at 0x%X' % (n.kind, p))
        out.append(n); p = n.end
    if p != end: raise Bad('value list ended at 0x%X, wanted 0x%X' % (p, end))
    return p

def parse_block(d, p, end, out):
    """a GCL command block (GCL_ExecBlock, command.c)"""
    while p < end:
        op = d[p]
        if op == 0x00:
            out.append(Node('END', p, p+1)); p += 1; continue
        if op == 0x30:
            L = d[p+1]
            if not L: raise Bad('EXPR size 0 at 0x%X' % p)
            n = Node('EXPR', p, p+1+L, size_at=p+1, size_bits=8)
        elif op == 0x60:
            L = be16(d, p+1)
            if L < 3: raise Bad('COMMAND size %d at 0x%X' % (L, p))
            n = Node('COMMAND', p, p+1+L, size_at=p+1, size_bits=16,
                     info=' id=0x%04X ofs=%d' % (be16(d, p+3), d[p+5]))
            # GCL_Command(top+3): id BE16, then u8 ofs, then the arg value list.
            # The option list starts at ofs_pos + ofs.
            n.kids.append(Node('OFS', p+5, p+6, size_at=p+5, size_bits=8,
                               info=' -> 0x%X' % (p + 5 + d[p+5])))
            try: parse_values(d, p+6, n.end, n.kids)
            except Bad: pass                            # args may end before block end
        elif op == 0x70:
            L = d[p+1]
            if not L: raise Bad('PROC size 0 at 0x%X' % p)
            n = Node('PROC', p, p+1+L, size_at=p+1, size_bits=8,
                     info=' id=0x%04X' % be16(d, p+2))
            try: parse_values(d, p+4, n.end, n.kids)
            except Bad: pass
        else:
            raise Bad('bad block opcode 0x%02X at 0x%X' % (op, p))
        if n.end > end: raise Bad('%s overruns at 0x%X' % (n.kind, p))
        out.append(n); p = n.end
    if p != end: raise Bad('block ended at 0x%X, wanted 0x%X' % (p, end))
    return p

def parse_script(d, body):
    """body must point at the GCL_ARG that GCL_ExecScript checks for"""
    if d[body] != 0x40: raise Bad('script body at 0x%X is not GCL_ARG' % body)
    slen = be32(d, body-4)
    root = Node('SCRIPT', body, body + slen, size_at=body-4, size_bits=32)
    arg = Node('ARG', body, body + 1 + be16(d, body+1), size_at=body+1, size_bits=16)
    parse_block(d, body+3, arg.end, arg.kids)
    root.kids.append(arg)
    return root, slen

def walk_tree(n, depth=0):
    yield n, depth
    for k in n.kids:
        yield from walk_tree(k, depth+1)

def containers_over(root, lo, hi):
    """every sized node whose span encloses [lo,hi) - these are the fields that
    must grow when bytes are inserted in that range"""
    out = []
    for n, _ in walk_tree(root):
        if n.size_at is None or n.kind == 'STRING': continue
        if n.pos <= lo and n.end >= hi: out.append(n)
    return out
