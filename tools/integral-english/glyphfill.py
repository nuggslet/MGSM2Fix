"""Merge a glyph transcription into work/glyphs-to-identify.tsv, and score it.

    py glyphfill.py work/transcription.txt
    py glyphfill.py --score            just report, change nothing

The transcription file is `gN <tab or space> character` per line, blanks and
`#` comments ignored. Merging is idempotent, so a page can be redone.

WHY IT SCORES ITSELF

78 of the shapes on the sheets were already identified from the stage archives
(`jptext.BANK1`), and `glyphsheets.py` writes those answers to
`work/glyph-answers.tsv` **without putting them on the sheet in any way that
marks them**. Transcribing them blind and comparing is a measured error rate on
this font at this size, from a labelled set - the only honest way to say how
good the rest of a pass is. Two caveats, both worth stating rather than hiding:
the answers also appear spelled out in the page `.txt` contexts, so a reader
who works from context alone is not being tested; and 78 is a small sample.

It also applies a check that costs nothing and catches a whole class of error:
**bank 1 never reuses a bank-0 bitmap** - all 1,200 shapes were compared
against every glyph in `font.res` and not one matched - so a character that
bank 0 already has (the 255 kanji of `GLYPH_90`, the kana, the punctuation)
cannot be the answer to a bank-1 glyph. Assigning one means a misread.
"""
import argparse
import collections
import io
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import jptext
from workdir import WORK


def read_transcription(path):
    out = {}
    with io.open(path, encoding='utf-8') as fh:
        for line in fh:
            line = line.split('#')[0].strip()
            if not line:
                continue
            f = line.replace('\t', ' ').split()
            if len(f) < 2 or not f[0].startswith('g'):
                continue
            out[f[0]] = '' if f[1] == '-' else f[1]     # `-` clears a wrong one
    return out


def bank0_chars():
    ch = set(jptext.GLYPH_90.values()) | set(jptext.GLYPH_91.values())
    ch |= {chr(0x3041 + i) for i in range(0x53)}
    ch |= {chr(0x30A1 + i) for i in range(0x56)}
    return ch


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('files', nargs='*', default=[WORK + '/transcription.txt'])
    ap.add_argument('--tsv', default=WORK + '/glyphs-to-identify.tsv')
    ap.add_argument('--answers', default=WORK + '/glyph-answers.tsv')
    ap.add_argument('--score', action='store_true', help='report only')
    ap.add_argument('--publish', nargs='?', const='bank1-glyphs.tsv', metavar='PATH',
                    help='write the committed table: the same rows with shape_hex'
                         ' replaced by jptext.shape_key, so no game data ships')
    args = ap.parse_args()

    new = {}
    for p in args.files:
        if os.path.exists(p):
            new.update(read_transcription(p))
    print('%d transcribed glyph(s) read' % len(new))

    with io.open(args.tsv, encoding='utf-8') as fh:
        head = fh.readline().rstrip('\n').split('\t')
        rows = [l.rstrip('\n').split('\t') for l in fh]
    ci, ii = head.index('char'), head.index('id')
    si = head.index('shape_hex') if 'shape_hex' in head else -1
    filled = 0
    for r in rows:
        if r[ii] in new:
            r[ci] = new[r[ii]]
            filled += 1
    if not args.score:
        with io.open(args.tsv, 'w', encoding='utf-8', newline='') as fh:
            fh.write('\t'.join(head) + '\n')
            for r in rows:
                fh.write('\t'.join(r) + '\n')
    have = sum(1 for r in rows if r[ci].strip())
    print('%d of %d rows now have a character (%.1f%%)'
          % (have, len(rows), 100.0 * have / len(rows)))

    # coverage of the actual text, weighted by how often each shape occurs
    oi = head.index('occurrences')
    tot = sum(int(r[oi]) for r in rows)
    got = sum(int(r[oi]) for r in rows if r[ci].strip())
    print('that is %.2f%% of the bank-1 glyph instances' % (100.0 * got / tot))
    print('bank-1 is 12.8%% of the glyph codes in japanese-inventory.tsv, so '
          'coverage OF THE INVENTORY is %.2f%%' % (87.2 + 12.8 * got / tot))
    print('  (that is over the inventory, which holds ~85% of the commentary\'s'
          ' glyphs.\n   The export no longer uses it for RADIO.DAT - radiotext.py'
          ' walks the game\'s\n   own records. NextSteps.md \xa719.)')

    bad = [(r[ii], r[ci]) for r in rows if r[ci].strip() in bank0_chars()]
    if bad:
        print('\nWARNING: %d assignment(s) name a character bank 0 already has, '
              'which bank 1 never repeats:' % len(bad))
        for gid, ch in bad[:20]:
            print('   %s -> %s' % (gid, ch))

    dup = collections.Counter(r[ci] for r in rows if r[ci].strip())
    rep = [(c, n) for c, n in dup.items() if n > 1]
    if rep:
        print('\n%d character(s) assigned to more than one shape (possible, '
              'but check): %s' % (len(rep), ' '.join('%s x%d' % t for t in sorted(rep))))

    if os.path.exists(args.answers):
        with io.open(args.answers, encoding='utf-8') as fh:
            fh.readline()
            key = {}
            for l in fh:
                f = l.rstrip('\n').split('\t')
                if len(f) >= 2:
                    key[f[0]] = f[1]
        by = {r[ii]: r[ci].strip() for r in rows}
        done = [(g, c) for g, c in key.items() if by.get(g)]
        right = [(g, c) for g, c in done if by[g] == c]
        if done:
            print('\nscored against %d known answers: %d right, %d wrong (%.1f%%)'
                  % (len(done), len(right), len(done) - len(right),
                     100.0 * len(right) / len(done)))
            for g, c in sorted(done):
                if by[g] != c:
                    print('   %-6s read %s, is %s' % (g, by[g], c))
    if args.publish:
        # The committed table carries the identification, not the bitmap:
        # a glyph bitmap is game data and none goes in this repository
        # (CREDITS.md). jptext loads either spelling, so lookups are
        # unaffected - what is lost is the ability to RENDER a glyph from
        # the committed copy, which is why work/glyphs-to-identify.tsv keeps
        # shape_hex and glyphreview.py reads that one.
        out, seen = [], {}
        for r in rows:
            r = list(r)
            k = jptext.shape_key(bytes.fromhex(r[si]))
            if k in seen:
                print('ABORT: %s and %s hash alike' % (seen[k], r[ii]))
                return 1
            seen[k] = r[ii]
            r[si] = k
            out.append(r)
        hd = list(head)
        hd[si] = 'shape_id'
        with io.open(args.publish, 'w', encoding='utf-8', newline='') as fh:
            fh.write('\t'.join(hd) + '\n')
            for r in out:
                fh.write('\t'.join(r) + '\n')
        print('%s: %d shape(s), no bitmaps' % (args.publish, len(out)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
