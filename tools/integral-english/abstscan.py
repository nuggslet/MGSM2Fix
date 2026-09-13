#!/usr/bin/env python
"""Scope the MISSION LOG (`abst` stage) in both games - the data the port needs.

    py abstscan.py            summary of both games
    py abstscan.py page N     dump page N (0-based) of both games as a GCL value walk

The mission log is the story-so-far shown after loading a save (READ MISSION
LOG?). USA has it in English; Integral's is game-encoded Japanese and is NOT yet
ported (2026-09-04). This prints what was measured while scoping it, from the
extracted STAGE.DIRs, so nobody re-derives it.

WHICH ACTOR: the mission log is drawn by onoda/abst/abst.c (NewAbstract) - NOT
ab_ch.c, which is the disc-change abstract (NewAbstractChange, the eight
disc-swap messages: a FOURTH copy of those, also Japanese in Integral). abst.c:
KCB kcb[12], rect 128x21 at font 704/256 clut 704/276, += 21 per line; reads
options l (left image), r (right image), e (end proc) and i. The `i` option's
payload is: an INT = line count (default 12), then count+1 STRING records
(record 0 is the header line, D_800C3238[0] = {88, 180}; lines 1.. at x 16,
y 35 + 19*k). Integral pages carry count 8 (9 records); USA pages count 14
(15 records, drawn as two screens of 7 at x 8, y 35 + 22*k - see the 14-entry
table at USA overlay +0x34 - in 128x20 KCBs with a VRAM column wrap at 512).

THE LENGTH BYTE: the `i` option's u8 length reads 49 (USA) / 234 (Integral)
against a ~560-byte payload - the low byte of an overflowed length. It is never
used: every GetOption() scan (l, r, e, i) stops at its letter, and i is last, so
nothing skips past it. Leave it exactly as the game wrote it; resize the STRING
records and the COMMAND's BE16 size instead.

Requires work/int1_stage.dir and work/usa1_stage.dir (see workdir.py).
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
import struct, sys
from collections import Counter
from optsctext import ents, pad
import gcldec

GAMES = [('Integral', WORK + '/int1_stage.dir', 0x800C3208),
         ('USA',      WORK + '/usa1_stage.dir', 0x800C5968)]
PAGE_CMD = 0x9906


def stage(path, name='abst'):
    d = open(path, 'rb').read()
    sec = {n: v for n, v, _p in ents(d)}[name]
    b = sec * 2048
    _v, _p, nsect = struct.unpack('<BBh', d[b:b + 4])
    tags, p = [], b + 4
    while True:
        tid, mode, ext, sz = struct.unpack('<HBBi', d[p:p + 8])
        if mode == 0:
            break
        tags.append((chr(mode) + (chr(ext) if 32 <= ext < 127 else '?'), sz))
        p += 8
    FILE = [k for k, (t, _s) in enumerate(tags) if not (t[0] == 'c' and t[1] in 'klhg')]
    off, pay = 2048, {}
    for k in FILE:
        pay[k] = d[b + off: b + off + tags[k][1]]
        off += pad(tags[k][1])
    return sec, nsect, tags, pay


def script_chunk(tags, pay):
    return next(pay[k] for k in pay if tags[k][0] == 'c?')


def page_blocks(c):
    """every `60 <BE16 size> 99 06` block: (start, size)"""
    out, i = [], 0
    while i < len(c) - 5:
        if c[i] == 0x60 and ((c[i + 3] << 8) | c[i + 4]) == PAGE_CMD:
            size = (c[i + 1] << 8) | c[i + 2]
            out.append((i, size))
            i += 1 + size
        else:
            i += 1
    return out


def options(c, start, size):
    """the option letters of a block, in order"""
    end = start + 1 + size
    p = start + 6
    while p < end and c[p] != 0x50:
        p += gcldec.step(c, p)
    out = ''
    while p < end and c[p] == 0x50:
        out += chr(c[p + 1])
        p += 2 + c[p + 2]
    return out


def page_strings(c, start, size):
    """inside the `i` option: (list of record bytes, (i length byte, line count))"""
    end = start + 1 + size
    p = start + 6
    while p < end and c[p] != 0x50:
        p += gcldec.step(c, p)
    while p < end and c[p] == 0x50 and c[p + 1] != ord('i'):
        p += 2 + c[p + 2]
    if p >= end or c[p] != 0x50:
        return [], (None, None)
    i_len = c[p + 2]
    p += 3                                          # into i's payload: first an INT, the line count
    count = None
    if p < end and c[p] in (0x01, 0x02, 0x09, 0x0a):
        w = gcldec.step(c, p)
        count = int.from_bytes(c[p + 1:p + w], 'big')
        p += w
    recs = gcldec.chain_at(c, p) if p < end and c[p] == 0x07 else []
    return [s for _, _, s in recs], (i_len, count)


def render(s):
    if all(32 <= b < 127 for b in s):
        return s.decode('ascii')
    return ''.join(chr(s[i + 1]) if s[i] == 0x80 and 32 <= s[i + 1] < 127 else '<%02x%02x>' % (s[i], s[i + 1])
                   for i in range(0, len(s) - 1, 2))


def summary():
    for label, path, base in GAMES:
        sec, nsect, tags, pay = stage(path)
        c = script_chunk(tags, pay)
        blocks = page_blocks(c)
        recs_per, counts, ilens, letters = Counter(), Counter(), Counter(), Counter()
        longest = b''
        for start, size in blocks:
            strs, (i_len, count) = page_strings(c, start, size)
            recs_per[len(strs)] += 1
            counts[count] += 1
            ilens[i_len] += 1
            letters[options(c, start, size)] += 1
            for s in strs:
                if len(s) > len(longest):
                    longest = s
        print('%s abst: STAGE.DIR sector %d, %d sectors, overlay base 0x%08X' % (label, sec, nsect, base))
        print('   tags: %s' % tags)
        print('   script chunk %d bytes; %d `0x9906` blocks (first +0x%X, last +0x%X)'
              % (len(c), len(blocks), blocks[0][0], blocks[-1][0]))
        print('   option letters per block: %s' % dict(letters.most_common(6)))
        print('   `i` line count (INT before the strings): %s' % dict(sorted(counts.items(), key=lambda kv: -kv[1])))
        print('   records per block (count + 1 header): %s' % dict(sorted(recs_per.items())))
        print("   `i` length bytes seen: %s  (NOT the payload size - see the docstring)" % dict(ilens.most_common(4)))
        print('   longest record: %d bytes: %r' % (len(longest), render(longest)[:80]))
        print()


def dump_page(n):
    NAMES = {0x00: 'END', 0x01: 'SHORT', 0x02: 'BYTE', 0x03: 'CHAR', 0x04: 'BOOL', 0x06: 'STRID', 0x07: 'STRING',
             0x08: 'PROCID', 0x09: 'INT', 0x0a: 'SYMBOL', 0x20: 'ARRAY', 0x30: 'EXPR', 0x40: 'ARG', 0x50: 'OPTION'}
    for label, path, base in GAMES:
        sec, nsect, tags, pay = stage(path)
        c = script_chunk(tags, pay)
        blocks = page_blocks(c)
        if n >= len(blocks):
            print('%s: only %d blocks' % (label, len(blocks)))
            continue
        start, size = blocks[n]
        end = start + 1 + size
        print('%s block %d: +0x%X..+0x%X (size %d), ofs byte %d, options %r'
              % (label, n, start, end, size, c[start + 5], options(c, start, size)))
        p = start + 6
        while p < end:
            op = c[p]
            kind = 'VAR' if (op & 0xF0) == 0x10 else NAMES.get(op, '?%02X' % op)
            try:
                sz = 4 if kind == 'VAR' else gcldec.step(c, p)
            except Exception:
                sz = None
            if op == 0x50 and c[p + 1] == ord('i'):
                strs, (i_len, count) = page_strings(c, start, size)
                print("   +0x%X  OPTION 'i' len byte %d -> INT count %s, %d records:" % (p, i_len, count, len(strs)))
                for k, s in enumerate(strs):
                    print('        %2d %3d  %r' % (k, len(s), render(s)[:76]))
                break
            if sz is None:
                print('   +0x%X  undecodable 0x%02X' % (p, op))
                break
            extra = " '%c' len %d" % (c[p + 1], c[p + 2]) if op == 0x50 else ''
            print('   +0x%X  %-7s %3d%s' % (p, kind, sz, extra))
            p += sz
        print()


if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'page':
        dump_page(int(sys.argv[2]))
    else:
        summary()
