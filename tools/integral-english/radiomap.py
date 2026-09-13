"""Map RADIO.DAT: where every fragment starts, and where its bank-1 font is.

    py radiomap.py                 the map, and the checks that prove it
    py radiomap.py --disc 2
    py radiomap.py --shapes        also write work/radio_shapes.pkl (shape -> uses)

WHY THIS EXISTS

Bank-1 glyph codes are indices into a font blob that a RADIO.DAT fragment
carries at its own end, so reading the Japanese needs, for every string, the
fragment it belongs to. The first attempt at that scanned every 2048-byte
boundary, accepted any sector whose header arithmetic looked sane, and gave
each string to "the nearest candidate behind it that covers it". It reported
100.000% of strings attributed and was wrong for 93% of them: the text lookups
produced **91,834 distinct 12x12 bitmaps**, where a Japanese font has a couple
of thousand, and only 9.8% of them had the blank twelfth row that every real
glyph in this font has. "Every string got attributed" was never a measure of
whether the attribution was right - it is a measure of whether the rule
terminates.

Two things fix it, and both come from reading the game instead of the bytes.

**1. Parse the script.** `menu_gcl_exec_block_800478B4` (menu/radiomes.c) says a
fragment's script is not GCL - it is a record list of its own:

    frag+0..1    two bytes the parser skips
    frag+2..5    BE32 face code: FACE.DAT sector = v & 0xFFFFFF, (v >> 24) * 2048 bytes
    frag+6..7    BE16
    frag+8       script: [0] marker, [1..2] BE16 totalSize, records from [3]
    record       FF <code> <BE16 size> <payload>, next record at +size+2, 0 ends
    font blob    frag + 9 + totalSize   (= script + totalSize + 1, what the
                 function returns and where font_set_font_addr(1, ...) points)

Walking those records is self-checking: a sector that is not a fragment
desynchronises within a record or two. 597 of 5,468 sectors survive it, the
first ending at exactly 0x1B1 - the value that was proved independently by
finding the one place in 11 MB where the glyph for 本 is followed by 出.

**2. Get the answer key.** Fragments are named by "radio codes", packed by
`sub_80047D70`:

    startSector = code & 0xFFFF
    Japanese    = (code >> 24) sectors at startSector
    English     = ((code >> 16) & 0xFF) sectors at startSector + (code >> 24)

and the codes are arguments to the GCL `radio` command (id 0x24E1,
`GV_StrCode("radio")`, game/script.c) in the stage scripts. 192 of them are
recoverable that way. **All 192 Japanese starts and all 192 English starts are
among the 597 the parse found, and no declared extent is overrun.** That is
what makes the parse trustworthy rather than merely plausible.

The 169 fragments no code names are real too - the codes for them live
somewhere this does not look - and they are kept, minus one class:

**A nested block has the same header as a fragment.** `radio_if_80047514` hands
`menu_gcl_exec_block_800478B4` the block inside an IF, so a nested block that
happens to land 8 bytes after a sector boundary parses exactly like a fragment.
Dropping every candidate that lies inside another candidate's script region
removes all 28 such cases that the radio codes independently prove false, and
none of the 384 they prove true.

WHAT THE RESULT MEASURES

553 fragments; the text lookups then produce 1,200 distinct bitmaps, 100% of them
with the blank twelfth row. Re-run that after any change here - it is a far
better measure than the share of strings that got *an* answer, which is what
the broken predecessor reported.

**But it is not sufficient, and do not treat it as the check.** Both numbers
are aggregates over 125 fragments, and they cannot see ONE fragment's base go
bad: a base wrong by a whole number of glyphs still slices on glyph
boundaries, so every bitmap it reads is a real glyph with a real blank twelfth
row. Valid bitmaps, wrong characters, both numbers unmoved.

    py radiotext.py --check      per fragment, and it catches that
    py radiotext.py --selftest   slips bases on purpose to prove --check works

Run both after any change here. NextSteps.md §21 records the two
plausible-looking per-fragment metrics that were measured and thrown away
before the working one, so nobody rebuilds them.
"""
import argparse
import bisect
import collections
import io
import os
import pickle
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import abst_build
import gclparse
import portio
from iso import Disc
from portio import INTEGRAL_IMAGES
from vrlib import chunk_index
from workdir import GAME, WORK

CONTAINER = GAME + '/windata/dlc/dlc_japan.bin'
CODE = re.compile(r'<([0-9A-F]{4})>')
GLYPH, SECTOR = 36, 2048
RADIO_CMD = 0x24E1                      # GV_StrCode("radio"), game/script.c

RDCODE = {0: 'NULL', 1: 'TALK', 2: 'VOICE', 3: 'ANIM', 4: 'ADD_CONTACT',
          5: 'MEMSAVE', 6: 'SOUND', 7: 'PROMPT', 8: 'VARSAVE', 0x10: 'IF',
          0x11: 'ELSE', 0x12: 'ELSEIF', 0x20: 'SWITCH', 0x21: 'CASE',
          0x22: 'DEFAULT', 0x30: 'RANDSWITCH', 0x31: 'RANDCASE',
          0x40: 'EVAL', 0x80: 'SCRIPT'}


def bank1_index(code):
    """glyph index for a bank-1 code, per font.c's `get_zen_font_data`.

    `zen_index` (rendertext.py) is `((code - 0x956B) - code/256) | 0x1000`, so
    the index drops one per 256-page: `code - 0x9601` is right inside `0x96xx`
    and one too high from `0x97xx` on, because each page's `00` entry is not a
    glyph. The inventory only ever contained `0x96xx` codes, so the error was
    invisible until the record walk started reading `0x97xx` - where it named
    every glyph one position too far along.
    """
    return (code - 0x956B) - (code >> 8)


def read_radio(disc_ix):
    """RADIO.DAT and FACE.DAT's sector count off one of the collection's ISOs"""
    image = Disc(CONTAINER, INTEGRAL_IMAGES[disc_ix])
    try:
        files = {n.upper(): (l, s) for n, l, s, dd in image.walk() if not dd}
        return (image.read(*files['/MGS/RADIO.DAT;1']),
                files['/MGS/FACE.DAT;1'][1] // SECTOR)
    finally:
        image.f.close()


def walk_block(d, s, limit):
    """the record list of the block whose header is at `s` -> (records, end).

    Raises ValueError the moment anything does not fit, which is what makes
    this usable as a test for "is this a fragment".
    """
    total = (d[s + 1] << 8) | d[s + 2]
    end = s + total + 1
    if not (s + 4 < end <= limit):
        raise ValueError('total %d out of range' % total)
    recs, p = [], s + 3
    while p < end:
        if d[p] == 0:
            break
        if d[p] != 0xFF:
            raise ValueError('marker %02X at 0x%X' % (d[p], p))
        code = d[p + 1]
        if code not in RDCODE:
            raise ValueError('record code %02X at 0x%X' % (code, p))
        size = (d[p + 2] << 8) | d[p + 3]
        if size < 2 or p + size + 2 > end:
            raise ValueError('record size %d at 0x%X' % (size, p))
        recs.append((p, code, size))
        p += size + 2
    if not recs:
        raise ValueError('no records')
    return recs, end


def fragments(radio):
    """every sector start whose script parses -> font base. Includes nested
    blocks that landed on a sector boundary; `drop_nested` removes those."""
    out = {}
    for frag in range(0, len(radio) - 16, SECTOR):
        try:
            _, end = walk_block(radio, frag + 8, len(radio))
        except ValueError:
            continue
        out[frag] = end
    return out


def drop_nested(frags):
    """a candidate inside another's script region is an IF/SWITCH body"""
    starts = sorted(frags)
    nested = set()
    for i, f in enumerate(starts):
        j = i + 1
        while j < len(starts) and starts[j] < frags[f]:
            nested.add(starts[j])
            j += 1
    return {f: b for f, b in frags.items() if f not in nested}, nested


def radio_codes(disc_ix):
    """(startSector, jpSectors, enSectors) -> the stages that name it.

    The codes are INT arguments of the GCL `radio` command's -b/-o/-c options.
    gclparse leaves an OPTION's payload unparsed, so it is parsed here.
    """
    found = collections.defaultdict(set)
    sd = open('%s/int%d_stage.dir' % (WORK, disc_ix + 1), 'rb').read()
    for name in portio.entries(sd):
        try:
            tags, payloads, _ = portio.stage(sd, name)
            chunk = payloads[chunk_index(tags)]
        except Exception:
            continue
        for t in tags:
            if t[1] != ord('c') or t[2] != ord('g'):
                continue
            try:
                g, _ = abst_build.parse_gcx(chunk, t[3])
            except Exception:
                continue
            for blob in [g['script']] + [b for _, b in g['procs']]:
                root = gclparse.Node('ROOT', 0, len(blob))
                try:
                    gclparse.parse_values(blob, 0, len(blob), root.kids)
                except gclparse.Bad:
                    continue
                for n, _ in gclparse.walk_tree(root):
                    if n.kind != 'COMMAND':
                        continue
                    if ((blob[n.pos + 3] << 8) | blob[n.pos + 4]) != RADIO_CMD:
                        continue
                    for k in n.kids:
                        if k.kind != 'OPTION' or chr(blob[k.pos + 1]) not in 'boc':
                            continue
                        kids = []
                        try:
                            gclparse.parse_values(blob, k.pos + 3, k.end, kids)
                        except gclparse.Bad:
                            continue
                        for v in kids:
                            if v.kind != 'INT':
                                continue
                            u = struct.unpack_from('>I', blob, v.pos + 1)[0]
                            sec, njp, nen = u & 0xFFFF, u >> 24, (u >> 16) & 0xFF
                            if njp and sec + njp + nen <= len(chunk):
                                found[(sec, njp, nen)].add(name)
    return found


def strings(disc_ix, source='RADIO.DAT'):
    """(offset, coded text) for one source of one disc, from the inventory"""
    want = 'disc%d' % (disc_ix + 1)
    out = []
    with io.open(WORK + '/japanese-inventory.tsv', encoding='utf-8') as fh:
        fh.readline()
        for line in fh:
            f = line.rstrip('\n').split('\t')
            if len(f) >= 7 and f[0] == want and f[1] == source:
                out.append((int(f[2], 16), f[6]))
    out.sort()
    return out


def shape_table(radio, frags, rows):
    """(shape -> instances, offset -> (font base, limit), stats).

    The limit is the next fragment's start: a font blob may not be read past
    it. The measure of a good map is how FEW distinct shapes come out, and how
    many have the blank last row - not how many strings got an answer.
    """
    starts = sorted(frags)
    roffs = [r[0] for r in rows]
    count = collections.Counter()
    base_of = {}
    stats = collections.Counter()
    for i, f in enumerate(starts):
        base = frags[f]
        nxt = starts[i + 1] if i + 1 < len(starts) else len(radio)
        for off, text in rows[bisect.bisect_left(roffs, f + 8):
                              bisect.bisect_left(roffs, base)]:
            base_of[off] = (base, nxt)
            for c in CODE.findall(text):
                v = int(c, 16) & ~0x6000
                if not (0x9600 <= v < 0x9A00):
                    continue
                a = base + bank1_index(v) * GLYPH
                if a + GLYPH > nxt:
                    stats['over-run'] += 1
                    continue
                count[radio[a:a + GLYPH]] += 1
    stats['unattributed'] = len(rows) - len(base_of)
    return count, base_of, stats


def blob_of(m, off):
    """the bank-1 font blob that applies to the string at `off`, or None"""
    at = m['base_of'].get(off)
    if at is None:
        return None
    base, limit = at
    return m['radio'][base:min(limit, base + GLYPH * 256)]


def build(disc_ix, verbose=True):
    radio, face_sectors = read_radio(disc_ix)
    raw = fragments(radio)
    frags, nested = drop_nested(raw)
    codes = radio_codes(disc_ix)
    jp = {s * SECTOR: n * SECTOR for s, n, _ in codes}
    en = {(s + n) * SECTOR: m * SECTOR for s, n, m in codes if m}
    rows = strings(disc_ix)
    count, base_of, stats = shape_table(radio, frags, rows)
    if verbose:
        blank = sum(n for s, n in count.items() if s[33:] == b'\0\0\0')
        tot = sum(count.values()) or 1
        print('disc%d  RADIO.DAT %d bytes, %d sectors'
              % (disc_ix + 1, len(radio), len(radio) // SECTOR))
        print('  candidates that parse      %5d' % len(raw))
        print('  nested blocks dropped      %5d' % len(nested))
        print('  fragments                  %5d' % len(frags))
        print('  radio codes recovered      %5d  (%d JP starts, %d EN starts)'
              % (len(codes), len(jp), len(en)))
        print('  ... JP starts that parse   %5d / %d' %
              (sum(1 for s in jp if s in frags), len(jp)))
        print('  ... EN starts that parse   %5d / %d' %
              (sum(1 for s in en if s in frags), len(en)))
        bad = sum(1 for s, n in jp.items() if s in frags and frags[s] > s + n)
        print('  ... declared extents overrun %3d' % bad)
        print('  strings                    %5d  (unattributed %d)'
              % (len(rows), stats['unattributed']))
        print('  glyph instances            %5d  (over-run %d)'
              % (tot, stats['over-run']))
        print('  DISTINCT shapes            %5d' % len(count))
        print('  blank twelfth row          %5.1f%%' % (100.0 * blank / tot))
    return dict(radio=radio, frags=frags, nested=nested, codes=codes,
                jp=jp, en=en, rows=rows, count=count, base_of=base_of)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--disc', type=int, default=1, choices=(1, 2))
    ap.add_argument('--shapes', action='store_true',
                    help='write work/radio_shapes.pkl and work/radio_map.pkl')
    args = ap.parse_args()
    m = build(args.disc - 1)
    if args.shapes:
        pickle.dump(m['count'], open(WORK + '/radio_shapes.pkl', 'wb'))
        pickle.dump({'frags': m['frags'], 'base_of': m['base_of'],
                     'jp': m['jp'], 'en': m['en']},
                    open(WORK + '/radio_map.pkl', 'wb'))
        print('-> %s/radio_shapes.pkl, radio_map.pkl' % WORK)
    return 0


if __name__ == '__main__':
    sys.exit(main())
