"""Print the FULL lines a glyph appears in, with everything else decoded.

    py glyphreview.py g291 g367 g460          the lines those shapes appear in
    py glyphreview.py --blank                 every shape with no character yet
    py glyphreview.py --char 室               every shape assigned that character

The transcription pass reads glyphs from a contact sheet with short excerpts
beside them. That settles most of them, and the ones it does not settle are
exactly the ones where 26 characters of context were not enough. This prints
the whole line instead, and by then the rest of the line is decoded, so the
sentence usually names the character outright - which is a far better test
than a second look at 144 pixels. It is also how a *wrong* answer gets caught:
a mis-read glyph turns a sentence into a non-word.
"""
import argparse
import collections
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import jptext
import radiomap
from workdir import WORK

CODE = re.compile(r'<([0-9A-F]{4})>')
GLYPH = 36


def sheet(path=None):
    """gid -> (occurrences, shape), and shape -> gid"""
    path = path or (WORK + '/glyphs-to-identify.tsv')
    rows, byshape = {}, {}
    with io.open(path, encoding='utf-8') as fh:
        head = fh.readline().rstrip('\n').split('\t')
        ii, oi, ci, si = (head.index('id'), head.index('occurrences'),
                          head.index('char'), head.index('shape_hex'))
        for line in fh:
            f = line.rstrip('\n').split('\t')
            if len(f) <= si:
                continue
            shape = bytes.fromhex(f[si])
            rows[f[ii]] = (int(f[oi]), f[ci].strip(), shape)
            byshape[shape] = f[ii]
    return rows, byshape


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ids', nargs='*')
    ap.add_argument('--blank', action='store_true', help='every unfilled shape')
    ap.add_argument('--char', help='every shape assigned this character')
    ap.add_argument('--lines', type=int, default=3, help='lines per shape')
    ap.add_argument('--disc', type=int, default=1)
    ap.add_argument('--verify', action='store_true',
                    help='every shape, one line each, compact - the read-through')
    ap.add_argument('--width', type=int, default=46,
                    help='verify: characters of context around the glyph')
    args = ap.parse_args()

    rows, byshape = sheet()
    want = set(args.ids)
    if args.blank:
        want |= {g for g, (_, c, _) in rows.items() if not c}
    if args.verify:
        want |= set(rows)
        args.lines = 1
    if args.char:
        want |= {g for g, (_, c, _) in rows.items() if c == args.char}
    if not want:
        ap.error('give some gN ids, --blank or --char')
    wanted_shapes = {rows[g][2]: g for g in want if g in rows}
    print('%d shape(s) wanted' % len(wanted_shapes))

    jptext.load_shape_table()
    m = radiomap.build(args.disc - 1, verbose=False)
    out = collections.defaultdict(list)
    for off, text in m['rows']:
        blob = radiomap.blob_of(m, off)
        if blob is None:
            continue
        hit = set()
        chars = []
        for part in re.split(r'(<[0-9A-F]{4}>)', text):
            mm = CODE.fullmatch(part or '')
            if not mm:
                chars.append((part or '').replace('#N', '/'))
                continue
            v = int(mm.group(1), 16) & ~0x6000
            if 0x9600 <= v < 0x9A00:
                i = radiomap.bank1_index(v) * GLYPH
                raw = blob[i:i + GLYPH]
                gid = wanted_shapes.get(raw)
                if gid is not None:
                    hit.add(gid)
                    chars.append('【%s】' % gid)
                    continue
                ch = jptext.char_for_shape(raw) if len(raw) == GLYPH else None
                chars.append(ch or '・')
            else:
                chars.append(jptext.glyph_char(v) or '・')
        if not hit:
            continue
        line = ''.join(chars)
        for gid in hit:
            if len(out[gid]) >= args.lines:
                continue
            shown = line
            if args.verify:
                # spell every other marked shape out, keep only the target
                # marked, and trim to a window around it
                one = re.sub(r'【(g\d+)】',
                             lambda mo, g=gid: ('<%s>' % rows[mo.group(1)][1]
                                                if mo.group(1) == g
                                                else rows[mo.group(1)][1] or '?'),
                             line)
                i = one.find('<%s>' % rows[gid][1])
                w = args.width
                shown = one[max(0, i - w):i + w + 3]
            if shown not in out[gid]:
                out[gid].append(shown)

    order = sorted(want, key=lambda g: -rows[g][0] if g in rows else 0)
    if args.verify:
        for gid in order:
            n, ch, _ = rows.get(gid, (0, '', b''))
            print('%-6s %s x%-6d %s' % (gid, ch or '?', n,
                                        (out.get(gid) or ['(no line)'])[0]))
        return 0
    for gid in order:
        n, ch, _ = rows.get(gid, (0, '', b''))
        print('\n%s  x%d  %s' % (gid, n, ('= %s' % ch) if ch else '(blank)'))
        for line in out.get(gid, []) or ['   (no line found)']:
            print('   %s' % line)
    return 0


if __name__ == '__main__':
    sys.exit(main())
