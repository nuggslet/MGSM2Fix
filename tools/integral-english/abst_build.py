#!/usr/bin/env python
"""Port the MISSION LOG into MGS Integral's `abst` stage (en_abst).

    py abst_build.py            build and verify; stage the two PPFs in WORK
    py abst_build.py --deploy   ... and copy them into the game's mods folders
    py abst_build.py --overlay PATH   use another compiled abst overlay

What the stage holds (both discs carry an identical copy at STAGE.DIR sector
139, 80 sectors): the overlay `sb` (mload.c LOAD/SAVE DATA, abst.c the mission
log, ab_demo1/2.c, ab_ch.c the disc-change abstract), the texture DAR `nd`, a
cache section `c?` and two sound files. The cache section is five files whose
tag "size" fields are OFFSETS into the section (libfs/cdstage.c
LoadCacheSection): k @0, l @180, h @184, scenerio.gcx @236, demo.gcx @57900,
and the 0xFF "fake tag" whose size is the section total. Each .gcx is
GCL_LoadScript's layout: BE32 proclen, proc table (id:BE16 offset:BE16, zero
word), contiguous proc bodies (each a GCL ARG `40 BE16`), BE32 script length,
the script body ARG, BE32 font length, the font glyphs. scenerio.gcx carries 80
mission-log pages (one 0x9906 command per proc), the disc-change block and the
English location list; demo.gcx carries the other 42 pages and the Japanese
location list. The pages pair with USA's 1:1 in order (every e/l/r option
equal, checked here).

A page is `OPTION i`: SHORT count, then count+1 STRING records, then the
command's GCL_END byte (which is what stops a GetOption scan) - record 0 is
the caption drawn under READ MISSION LOG? (Integral: 作戦記録を参照しますか？
with furigana; USA: empty), records 1.. are the lines. Integral draws up to 8
lines on one screen (count 7 or 8), USA 7 lines per screen on two screens
(count 14) or one (count 7). This builder gives every page USA's count and
USA's line records verbatim - USA's line breaks, USA's text, nothing re-wrapped
- and keeps Integral's record 0 (KEEP_PROMPT_CAPTION): USA shows nothing there,
so under the no-translation rule the Japanese caption stays. The `i` option's
own length byte is an overflowed u8 in both games and is never read (every
GetOption scan stops at its own letter and i is last); it is left as Integral
wrote it. Each edited COMMAND's BE16 size, the enclosing proc body's ARG
length, the proc table offsets, the proclen and the script length are
recomputed; the font blobs are Integral's (the caption and the Japanese
location names need Integral's glyphs; USA's lines are ASCII, drawn with the
resident font).

The disc-change abstract (ab_ch.c, block `a`+`e` in scenerio.gcx) takes USA's
whole `e` option: PROCID plus the eight English disc-swap strings.

Textures: USA splits the bottom bar into abst_d_l (with the ◄ button),
abst_d_r1 (►) and abst_d_r2 (EXIT at the right); Integral has abst_d_l/abst_d_r
with EXIT centred. The three USA payloads replace Integral's two, placed in
the VRAM/CLUT footprint the two freed (d_r1 and d_r2 are 20 words wide each
and share abst_d_r's 40x30 slot at (0,286)); every other texture, palette
included, stays Integral's - the SOLID strip and the cursor pieces differ only
in palette, which is Integral's own art.

The overlay is the decomp's abst.c compiled by build.py/ninja (obj/abst.bin);
abst.c is grown to USA's model (see decomp-overlay-changes.patch and the
README section "The MISSION LOG port").

The stage grows past its 80 sectors, so it is relocated into DUMMY3M.DAT from
slot 462 (preope 0..89, brf 128..266, option 384..461) and the STAGE.DIR entry
repointed, exactly as optsctext.py does for option. Outputs: work/abst_en.bin
(the packed stage), work/abst_chunk.bin, and INTEGRAL_disc{1,2}_en_abst.ppf.
Run ppfcheck.py on anything before it goes near the game.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME, DECOMP
import os, struct, sys, shutil
from collections import Counter
import portio
from iso import Disc
from optsctext import dar_entries, set_pcxinfo
from gclparse import be16, be32, parse_values, Bad

INT_STAGE = WORK + '/int1_stage.dir'
USA_STAGE = WORK + '/usa1_stage.dir'
OVERLAY = DECOMP + '/obj/abst.bin'
DLC = GAME + '/windata/dlc/dlc_japan.bin'
MODS = GAME + '/mods/INTEGRAL/INTEGRAL'
SLOT = 462
KEEP_PROMPT_CAPTION = True     # False = USA's empty record 0 (no Japanese caption under READ MISSION LOG?)
USA_LOCATION_NAMES = True      # False = keep Integral's own English location names
# The location list is the port's SECOND replacement of Integral's own English
# (amendment 4b in NextSteps.md 2; `SCARF` -> `HANDKER` was the first). Both
# games spell 30 names, aligned 1:1, and four differ - `Tank Hanger`,
# `Medi rm`, `Cmnder rm` and `Cmnd rm` against USA's `Tank Hangar`,
# `Medi room`, `Cmnder room` and `Cmnd room`. The fourth was found only on
# 2026-09-08, by `mainsweep.py --diff-english`; three documents had listed
# three. Asked and approved the same day.
LOCATION_STRIDS = bytes.fromhex('0653c706c8bb085fd9')
PAGE_CMD = 0x9906
DESC = 'MGS Integral: English abst (MISSION LOG)'
assert len(DESC) <= 50


def strcode(s):
    i = 0
    for ch in s.encode():
        i = (((i << 5) | (i >> 11)) & 0xFFFF)
        i = (i + ch) & 0xFFFF
    return i


# --------------------------------------------------------------- GCX scripts

def parse_gcx(buf, start):
    """One GCL_LoadScript file at `start`: -> dict, end offset."""
    proclen = be32(buf, start)
    tstart = start + 4
    q, table = tstart, []
    while be32(buf, q) != 0:
        table.append((be16(buf, q), be16(buf, q + 2)))
        q += 4
    pbody = q + 4
    script_pos = tstart + proclen
    procs, p = [], pbody
    for pid, off in table:
        assert pbody + off == p, 'proc bodies are not contiguous in table order at %X' % p
        assert buf[p] == 0x40, 'proc body at %X is not GCL_ARG' % p
        L = be16(buf, p + 1)
        procs.append((pid, buf[p:p + 1 + L]))
        p += 1 + L
    assert p == script_pos, 'bodies end %X, script at %X' % (p, script_pos)
    slen = be32(buf, script_pos)
    body = buf[script_pos + 4:script_pos + 4 + slen]
    assert body[0] == 0x40 and be16(body, 1) + 1 == slen, 'script body header'
    fpos = script_pos + 4 + slen
    flen = be32(buf, fpos)
    font = buf[fpos + 4:fpos + 4 + flen]
    assert len(font) == flen
    return dict(procs=procs, script=body, font=font), fpos + 4 + flen


def build_gcx(g):
    table, bodies, off = bytearray(), bytearray(), 0
    for pid, body in g['procs']:
        assert body[0] == 0x40 and be16(body, 1) + 1 == len(body)
        assert off < 0x10000, 'proc table offset overflow'
        table += struct.pack('>HH', pid, off)
        bodies += body
        off += len(body)
    table += bytes(4)
    script = g['script']
    assert script[0] == 0x40 and be16(script, 1) + 1 == len(script)
    out = struct.pack('>I', len(table) + len(bodies)) + table + bodies
    out += struct.pack('>I', len(script)) + script
    out += struct.pack('>I', len(g['font'])) + g['font']
    return bytes(out)


def blocks_in(buf):
    """every `60 <BE16> 99 06` command: (start, size)"""
    out, i = [], 0
    while i < len(buf) - 5:
        if buf[i] == 0x60 and be16(buf, i + 3) == PAGE_CMD:
            size = be16(buf, i + 1)
            out.append((i, size))
            i += 1 + size
        else:
            i += 1
    return out


def option_starts(buf, start, size):
    """the option list of a block: {letter: offset of its 0x50} in order, and the block end"""
    end = start + 1 + size
    p = start + 5 + buf[start + 5]           # ofs byte -> first option
    out = []
    while p < end and buf[p] == 0x50:
        out.append((chr(buf[p + 1]), p))
        p += 2 + buf[p + 2]
        if out[-1][0] == 'i':
            break                              # i's length byte is meaningless
    return out, end


def records(buf, p, end):
    """`07 len payload` records from p to end (payload keeps its NUL)"""
    out = []
    while p < end:
        assert buf[p] == 0x07, 'record opcode %02X at %X' % (buf[p], p)
        L = buf[p + 1]
        out.append(buf[p + 2:p + 2 + L])
        p += 2 + L
    assert p == end
    return out


def page_of(buf, start, size):
    """-> (options {letter: payload}, count, records) or None if not a page"""
    opts, end = option_starts(buf, start, size)
    d = dict()
    for letter, p in opts:
        if letter != 'i':
            d[letter] = buf[p + 3:p + 2 + buf[p + 2]]
    if not opts or opts[-1][0] != 'i':
        return None
    ip = opts[-1][1]
    q = ip + 3
    assert buf[q] == 0x01, 'i count is not a SHORT'
    count = be16(buf, q + 1)
    # the command's value list ends with GCL_END (0x00) - GCL_GetOption stops there
    assert buf[end - 1] == 0x00, 'no GCL_END at the block end'
    recs = records(buf, q + 3, end - 1)
    return d, count, recs, ip


def encode_records(recs):
    out = bytearray()
    for r in recs:
        assert 0 < len(r) <= 255 and r[-1] == 0
        out += bytes((7, len(r))) + r
    return bytes(out)


def resize_block(buf, start, size, new_tail_from, new_tail):
    """replace buf[new_tail_from:end] with new_tail, fixing the COMMAND's BE16"""
    end = start + 1 + size
    body = bytearray(buf[start:new_tail_from]) + new_tail
    new_size = len(body) - 1
    assert new_size < 0x10000
    body[1:3] = struct.pack('>H', new_size)
    return bytes(buf[:start]) + bytes(body) + bytes(buf[end:])


def rebuild_body(int_body, usa_pages, disc_change, stats, location=None):
    """int_body: a proc body or the script body. usa_pages: iterator of USA page
    (opts, count, recs). Edits every page block, the disc-change block and - if
    `location` is given and USA_LOCATION_NAMES is set - the location list."""
    out = bytes(int_body)
    if location is not None and USA_LOCATION_NAMES:
        found = location_block(out)
        if found is not None:
            start, size = found
            recs = records(out, start + 15, start + size)
            assert len(recs) == 31, 'Integral location list has %d records' % len(recs)
            out = bytes(out[:start]) + location + bytes(out[start + 1 + size:])
            stats['locations'] += 1
            stats['bytes'] += len(location) - (1 + size)
    shift = 0
    for start, size in blocks_in(int_body):
        start += shift
        pg = page_of(out, start, size)
        if pg is None:
            continue
        opts, count, recs, ip = pg
        if 'a' in opts and 'e' in opts and count is None:
            continue
        if count in (7, 8) and 'e' in opts and 'l' in opts and 'r' in opts:
            uopts, ucount, urecs, _uip = next(usa_pages)
            for k in 'elr':
                assert opts[k] == uopts[k], 'page pairing broke: %s %s vs %s' % (k, opts[k].hex(), uopts[k].hex())
            assert ucount in (7, 14) and len(urecs) >= ucount + 1
            rec0 = recs[0] if KEEP_PROMPT_CAPTION else urecs[0]
            payload = bytes((0x01,)) + struct.pack('>H', ucount) + encode_records([rec0] + list(urecs[1:])) + b'\x00'
            old_end = start + 1 + size
            new = resize_block(out, start, size, ip + 3, payload)
            stats['pages'] += 1
            stats['bytes'] += len(new) - len(out)
            shift += len(new) - len(out)
            out = new
        else:
            stats['skipped'].append((count, sorted(opts)))
    if disc_change is not None:
        # the ab_ch.c block: options a then e; replace e wholesale with USA's
        for start, size in blocks_in(out):
            optl, end = option_starts(out, start, size)
            letters = [l for l, p in optl]
            if letters == ['a', 'e']:
                ep = dict(optl)['e']
                new = resize_block(out, start, size, ep, disc_change)
                stats['disc_change'] += 1
                out = new
    return out


def usa_disc_change(usa_body_list):
    for body in usa_body_list:
        for start, size in blocks_in(body):
            optl, end = option_starts(body, start, size)
            if [l for l, p in optl] == ['a', 'e']:
                ep = dict(optl)['e']
                return bytes(body[ep:end])
    raise AssertionError('USA disc-change block not found')


def location_block(body):
    """(start, size) of the English location-name command, or None.

    It is a `0x9906` command like a mission-log page - `mainsweep.py` calls it
    `chara 53C7` after the actor its first STRID spawns - but it carries no
    option list at all: 31 records sit directly in the value list, 30 of them
    the English names. It is matched on its STRID/PROCID chain rather than on
    a position, because a stride is not a structure (NextSteps.md 14).
    """
    for start, size in blocks_in(body):
        if body[start + 6:start + 6 + len(LOCATION_STRIDS)] != LOCATION_STRIDS:
            continue
        optl, end = option_starts(body, start, size)
        if optl:
            continue
        return start, size
    return None


def location_records(script):
    """the 31 records of a script body's location list"""
    start, size = location_block(script)
    return records(script, start + 15, start + size)


def usa_location_block(usa_body_list):
    """USA's command, taken whole.

    Its two derived fields - the COMMAND's BE16 size and the u8 at start+5 that
    `option_starts` uses to reach the option list - both encode the length of
    the record region, and USA's are 12 bytes larger because three of its four
    differing names are two characters longer. Taking the block whole is what
    keeps those two in step: patching the records alone and forgetting the u8
    would leave a block whose size says one thing and whose offset byte says
    another.
    """
    for body in usa_body_list:
        found = location_block(body)
        if found is None:
            continue
        start, size = found
        end = start + 1 + size
        recs = records(body, start + 15, end - 1)
        assert len(recs) == 31, 'USA location list has %d records' % len(recs)
        return bytes(body[start:end])
    raise AssertionError('USA location-name block not found')


def usa_pages_of(bodies):
    for body in bodies:
        for start, size in blocks_in(body):
            pg = page_of(body, start, size)
            if pg is None:
                continue
            opts, count, recs, ip = pg
            if count in (7, 14) and 'e' in opts and 'l' in opts and 'r' in opts:
                yield opts, count, recs, ip


def rebuild_chunk(int_chunk, usa_chunk, int_tags, usa_tags):
    """-> new chunk bytes, new demo.gcx offset, stats"""
    def cache_offsets(tags):
        # tags: [id, mode, ext, size]; the 'c' tags before the 0xFF one carry offsets
        c = [(chr(t[2]) if t[2] != 0xFF else '?', t[3]) for t in tags if t[1] == ord('c')]
        return c
    ci, cu = cache_offsets(int_tags), cache_offsets(usa_tags)
    assert [x for x, _ in ci] == ['k', 'l', 'h', 'g', 'g', '?'] == [x for x, _ in cu], (ci, cu)
    i_sc, i_demo, i_total = ci[3][1], ci[4][1], ci[5][1]
    u_sc, u_demo, u_total = cu[3][1], cu[4][1], cu[5][1]
    assert i_total == len(int_chunk) and u_total == len(usa_chunk)
    head = int_chunk[:i_sc]
    assert head == usa_chunk[:u_sc], 'k/l/h files differ between games'
    gi1, e1 = parse_gcx(int_chunk, i_sc)
    assert e1 <= i_demo and not any(int_chunk[e1:i_demo]), 'unexpected bytes between the scripts'
    gi2, e2 = parse_gcx(int_chunk, i_demo)
    assert e2 <= len(int_chunk) and not any(int_chunk[e2:]), 'unexpected trailing bytes'
    gu1, _ = parse_gcx(usa_chunk, u_sc)
    gu2, _ = parse_gcx(usa_chunk, u_demo)
    print('scenerio.gcx: %d procs, script %d, font %d | demo.gcx: %d procs, script %d, font %d (Integral)'
          % (len(gi1['procs']), len(gi1['script']), len(gi1['font']), len(gi2['procs']), len(gi2['script']), len(gi2['font'])))
    stats = dict(pages=0, bytes=0, skipped=[], disc_change=0, locations=0)
    out_scripts = []
    for gi, gu in ((gi1, gu1), (gi2, gu2)):
        # demo.gcx: Integral has one extra proc (0x5FD9, the Japanese location list's);
        # the shared procs keep their order, and every page is re-checked by e/l/r below.
        li, lu = [p for p, b in gi['procs']], [p for p, b in gu['procs']]
        assert [x for x in li if x in lu] == [x for x in lu if x in li], 'shared proc order differs'
        assert not [x for x in lu if x not in li], 'USA has procs Integral lacks'
        u_bodies = [b for p, b in gu['procs']] + [gu['script']]
        upages = usa_pages_of(u_bodies)
        dch = usa_disc_change(u_bodies) if gi is gi1 else None
        # The location list is in scenerio.gcx's script body; demo.gcx's own
        # list is Integral's Japanese one and has no USA counterpart, so it is
        # not offered here and stays as it is.
        loc = usa_location_block(u_bodies) if gi is gi1 else None
        procs = [(pid, rebuild_body(body, upages, dch, stats, loc)) for pid, body in gi['procs']]
        script = rebuild_body(gi['script'], upages, None, stats, loc)
        leftover = sum(1 for _ in upages)
        assert leftover == 0, '%d USA pages unused' % leftover
        for pid, body in procs:                       # re-stamp ARG lengths
            pass
        fixed = []
        for pid, body in procs:
            b = bytearray(body)
            b[1:3] = struct.pack('>H', len(b) - 1)
            fixed.append((pid, bytes(b)))
        s = bytearray(script)
        s[1:3] = struct.pack('>H', len(s) - 1)
        out_scripts.append(build_gcx(dict(procs=fixed, script=bytes(s), font=gi['font'])))
    g1, g2 = out_scripts
    pad1 = (-len(g1)) % 4
    demo_off = i_sc + len(g1) + pad1
    chunk = bytes(head) + g1 + bytes(pad1) + g2
    chunk += bytes((-len(chunk)) % 4)
    return chunk, demo_off, stats


# ------------------------------------------------------------------ textures

def rebuild_dar(int_nd, usa_nd):
    ie, ue = dar_entries(int_nd), dar_entries(usa_nd)
    want = {strcode(n): n for n in ('abst_d_l', 'abst_d_r', 'abst_d_r1', 'abst_d_r2')}
    iby = {e[0]: e for e in ie}
    uby = {e[0]: e for e in ue}
    d_l, d_r = iby[strcode('abst_d_l')], iby[strcode('abst_d_r')]
    u_l, u_r1, u_r2 = uby[strcode('abst_d_l')], uby[strcode('abst_d_r1')], uby[strcode('abst_d_r2')]
    _s, _f, lpx, lpy, lcx, lcy, _n = struct.unpack_from('<7H', d_l[2], 74)
    _s, _f, rpx, rpy, rcx, rcy, _n = struct.unpack_from('<7H', d_r[2], 74)
    _s, _f, ulpx, ulpy, ulcx, ulcy, _n = struct.unpack_from('<7H', u_l[2], 74)
    assert (ulpx, ulpy, ulcx, ulcy) == (lpx, lpy, lcx, lcy), 'USA abst_d_l is placed elsewhere'
    used_cluts = set()
    for e in ie:
        _s, _f, px, py, cx, cy, _n = struct.unpack_from('<7H', e[2], 74)
        used_cluts.add((cx, cy))
    new_r2_clut = (960, 233)
    assert new_r2_clut not in used_cluts
    r1_payload, _o = set_pcxinfo(u_r1[2], rpx, rpy, rcx, rcy)                # (0,286) clut (848,233)
    r2_payload, _o = set_pcxinfo(u_r2[2], rpx + 20, rpy, new_r2_clut[0], new_r2_clut[1])   # 80 px = 20 words right of r1
    out = []
    for tid, ext, pl in ie:
        if tid == strcode('abst_d_l'):
            out.append((strcode('abst_d_l'), u_l[1], u_l[2]))
        elif tid == strcode('abst_d_r'):
            out.append((strcode('abst_d_r1'), u_r1[1], r1_payload))
            out.append((strcode('abst_d_r2'), u_r2[1], r2_payload))
        else:
            out.append((tid, ext, pl))
    blob = b''.join(struct.pack('<HhI', tid, ext, len(pl)) + pl for tid, ext, pl in out)
    check = dar_entries(blob)
    assert len(check) == len(ie) + 1 and all(len(e[2]) % 4 == 0 for e in check)
    print('DAR: %d -> %d entries; abst_d_l <- USA; abst_d_r -> abst_d_r1 @(%d,%d) clut (%d,%d) + abst_d_r2 @(%d,%d) clut (%d,%d)'
          % (len(ie), len(check), rpx, rpy, rcx, rcy, rpx + 20, rpy, new_r2_clut[0], new_r2_clut[1]))
    return blob


# ------------------------------------------------------------- verification

def verify_chunk(chunk, int_chunk, usa_chunk, demo_off):
    """re-parse both scripts, check every container, compare every page to USA"""
    def pages(buf, start):
        g, end = parse_gcx(buf, start)
        out = []
        for pid, body in g['procs'] + [(None, g['script'])]:
            # container self-check: the ARG must parse as a value list of exactly its length
            kids = []
            try:
                parse_values(body, 0, len(body), kids)
            except Bad as e:
                raise AssertionError('body of proc %s does not parse: %s' % (pid, e))
            for start_, size in blocks_in(body):
                pg = page_of(body, start_, size)
                if pg and pg[1] in (7, 8, 14) and all(k in pg[0] for k in 'elr'):
                    out.append(pg)
        return g, end, out
    g1, e1, p1 = pages(chunk, 236)
    g2, e2, p2 = pages(chunk, demo_off)
    assert e1 <= demo_off and e2 <= len(chunk)
    gi1, _ = parse_gcx(int_chunk, 236)
    gu1, _ = parse_gcx(usa_chunk, 236)
    assert g1['font'] == gi1['font'] and g2['font'] == parse_gcx(int_chunk, 57900)[0]['font'], 'fonts changed'
    _, _, ui = pages(int_chunk, 236); _, _, ui2 = pages(int_chunk, 57900)
    _, _, uu = pages(usa_chunk, 236); _, _, uu2 = pages(usa_chunk, 53008)
    new, old, usa = p1 + p2, ui + ui2, uu + uu2
    assert len(new) == len(old) == len(usa) == 122, (len(new), len(old), len(usa))
    for n, o, u in zip(new, old, usa):
        assert n[0] == o[0] == u[0] or (n[0] == o[0] and all(n[0][k] == u[0][k] for k in 'elr'))
        assert n[1] == u[1], 'count'
        assert n[2][0] == (o[2][0] if KEEP_PROMPT_CAPTION else u[2][0]), 'record 0'
        assert n[2][1:] == u[2][1:], 'lines differ from USA'
        # USA's lines are ASCII apart from four 0x80 0x27 pairs: the game font's
        # Latin apostrophe glyph, the same 0x80xx encoding the option chain uses.
        for r in n[2][1:]:
            i = 0
            while i < len(r):
                if r[i] == 0x80 and i + 1 < len(r) and 32 <= r[i + 1] < 127:
                    i += 2
                    continue
                assert 32 <= r[i] < 127 or r[i] == 0, 'unexpected byte %02X in a USA line' % r[i]
                i += 1
    # The location list: re-parse it out of the rebuilt script and compare it
    # record by record with the disc it was taken from, so a mis-stamped size
    # or a lost record cannot pass. Its u8 offset byte is checked implicitly:
    # `option_starts` reads it, and `location_block` requires the empty option
    # list it points at.
    assert location_block(g1['script']) is not None, 'the rebuilt script has no location list'
    built = location_records(g1['script'])
    source = location_records(parse_gcx(usa_chunk if USA_LOCATION_NAMES else int_chunk,
                                       236)[0]['script'])
    retail = location_records(gi1['script'])
    assert len(built) == len(source) == len(retail) == 31, (len(built), len(source))
    assert built == source, 'the location list does not equal its source'
    ichanged = sum(1 for a, b in zip(built, retail) if a != b)
    counts = Counter(n[1] for n in new)
    print('verified: 122 pages re-parsed; counts %s; every line record equals USA; record 0 %s'
          % (dict(counts), 'kept (Integral caption)' if KEEP_PROMPT_CAPTION else "USA's (empty)"))
    print('verified: the 31-record location list equals %s exactly; %d record(s)'
          ' differ from retail Integral'
          % ("USA's" if USA_LOCATION_NAMES else "Integral's own", ichanged))
    # the disc-change block
    for pid, body in g1['procs']:
        for s, z in blocks_in(body):
            optl, end = option_starts(body, s, z)
            if [l for l, p in optl] == ['a', 'e']:
                assert body[end - 1] == 0x00
                recs = records(body, dict(optl)['e'] + 3 + 3, end - 1)
                print('disc-change abstract: %d strings: %s' % (len(recs), [r[:-1].decode('latin-1') for r in recs]))


def verify_stage(stage):
    tags, payloads, offsets = portio.stage(stage)
    c = [(t[2], t[3]) for t in tags if t[1] == ord('c')]
    chunk = payloads[7]
    assert c[-1][1] == len(chunk)
    demo_off = c[4][1]
    assert demo_off % 4 == 0 and demo_off < len(chunk)
    g1, e1 = parse_gcx(chunk, c[3][1]); g2, e2 = parse_gcx(chunk, demo_off)
    assert e1 <= demo_off and e2 <= len(chunk)
    assert dar_entries(payloads[1])
    sb = payloads[0]
    words = struct.unpack_from('<8I', sb, 0)
    assert words[0] == 0x53c7 and words[2] == 0x566f and words[4] == 0x4974 and words[6] == 0x4975, 'overlay header ids'
    print('stage: %d sectors; sb %d, nd %d, chunk %d (demo.gcx @%d), sw %d, se %d; overlay entries %s'
          % (len(stage) // 2048, len(sb), len(payloads[1]), len(chunk), demo_off, len(payloads[8]), len(payloads[9]),
             ['%08X' % w for w in words[1::2]]))


def other_ppf_claims(disc_index, skip_name):
    """image offsets written by every other PPF in the mods folder for this disc"""
    claims = {}
    folder = os.path.join(MODS, str(disc_index))
    if not os.path.isdir(folder):
        return claims
    for name in sorted(os.listdir(folder)):
        if not name.lower().endswith('.ppf') or name == skip_name:
            continue
        for off, data in portio.read_ppf(os.path.join(folder, name)):
            for k in range(len(data)):
                claims[off + k] = name
    return claims


def main():
    deploy = '--deploy' in sys.argv
    overlay = OVERLAY
    if '--overlay' in sys.argv:
        overlay = sys.argv[sys.argv.index('--overlay') + 1]
    int_dir = open(INT_STAGE, 'rb').read()
    usa_dir = open(USA_STAGE, 'rb').read()
    int_tags, int_pay, _ = portio.stage(int_dir, 'abst')
    usa_tags, usa_pay, _ = portio.stage(usa_dir, 'abst')
    assert int_pay[8] == usa_pay[8] and int_pay[9] == usa_pay[9], 'sound files differ'
    chunk, demo_off, stats = rebuild_chunk(int_pay[7], usa_pay[7], int_tags, usa_tags)
    print('chunk: %d -> %d bytes (%+d); %d pages rewritten (%+d bytes), disc-change blocks %d,'
          ' location lists %d, skipped %s'
          % (len(int_pay[7]), len(chunk), len(chunk) - len(int_pay[7]), stats['pages'], stats['bytes'],
             stats['disc_change'], stats['locations'],
             Counter((c, tuple(o)) for c, o in stats['skipped']).most_common()))
    assert stats['locations'] == (1 if USA_LOCATION_NAMES else 0), \
        'the location list was not rewritten exactly once'
    verify_chunk(chunk, int_pay[7], usa_pay[7], demo_off)
    nd = rebuild_dar(int_pay[1], usa_pay[1])
    sb = open(overlay, 'rb').read()
    print('overlay: %s, %d bytes (retail %d)' % (overlay, len(sb), len(int_pay[0])))
    tags = [t.copy() for t in int_tags]
    tags[6][3] = demo_off                     # demo.gcx offset inside the cache section
    payloads = dict(int_pay)
    payloads[0], payloads[1], payloads[7] = sb, nd, chunk
    stage = portio.pack_stage(tags, payloads)
    verify_stage(stage)
    open(WORK + '/abst_en.bin', 'wb').write(stage)
    open(WORK + '/abst_chunk.bin', 'wb').write(chunk)
    open(WORK + '/abst_nd.bin', 'wb').write(nd)
    need = len(stage) // 2048
    for disc, base in enumerate(portio.INTEGRAL_IMAGES):
        image = Disc(DLC, base)
        try:
            name = 'INTEGRAL_disc%d_en_abst.ppf' % (disc + 1)
            ppf = portio.relocation(image, 'abst', stage, SLOT, DESC)
            path = WORK + '/' + name
            open(path, 'wb').write(ppf)
            recs = portio.read_ppf(path)
            claims = other_ppf_claims(disc, name)
            clash = sorted(set(claims[o + k] for o, d in recs for k in range(len(d)) if o + k in claims))
            assert not clash, 'disc %d: records overlap %s' % (disc + 1, clash)
            files = {n.upper(): (l, s) for n, l, s, d in image.walk() if not d}
            du_lba = files['/DUMMY3M.DAT;1'][0]
            sd_lba = files['/MGS/STAGE.DIR;1'][0]
            print('disc %d: %s: %d records, %d bytes; DUMMY3M slots %d..%d (LBA %d..); STAGE.DIR entry -> %d; no overlap with %d other PPF bytes'
                  % (disc + 1, name, len(recs), len(ppf), SLOT, SLOT + need - 1, du_lba + SLOT, du_lba + SLOT - sd_lba, len(claims)))
            if deploy:
                dst = os.path.join(MODS, str(disc), name)
                shutil.copyfile(path, dst)
                print('   deployed -> %s' % dst)
        finally:
            image.f.close()
    if not deploy:
        print('staged in %s (nothing deployed; use --deploy)' % WORK)


if __name__ == '__main__':
    main()
