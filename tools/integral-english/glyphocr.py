"""Identify the game's 12x12 glyph bitmaps by matching them against a real font.

`jptext.py` reads 87% of the Japanese on the disc: kana by arithmetic and the
`0x90` kanji bank by hand transcription. The rest is bank 1, and bank 1 cannot
be transcribed by hand - it is a **per-block** table, up to 255 glyphs in each
of ~1,900 `RADIO.DAT` conversation blocks, 541,920 uses in all. Locating a
block's table (see `COVERAGE.md`) yields bitmaps; something still has to say
which character each bitmap is. This does that, by rendering candidates from a
Japanese system font and scoring them against the game's pixels.

    py glyphocr.py --validate         score it against the 238 glyphs already known
    py glyphocr.py --bank 90          identify one bank and print the table
    py glyphocr.py --table out.json   write code -> character for every bank-0 glyph

WHY THIS IS CHECKABLE RATHER THAN HOPEFUL

The `0x90` bank was transcribed by eye and independently verified (consecutive
codes spell 地雷探知機, 精神安定剤, 風邪薬). That makes it a **labelled test
set of 238 kanji** drawn from the exact same font, so the recogniser's accuracy
is a measurement, not a guess - and any disagreement is worth looking at from
both sides, since the hand transcription can be wrong too.

HOW THE MATCH WORKS

The game stores 12x12 at 2 bits per pixel. MS Gothic carries hand-tuned embedded
bitmaps at small sizes, which is as close to a 1998 Japanese console font as a
modern system gets, so candidates are rasterised at several sizes and offsets
and the best alignment for each candidate is kept. Score is normalised
correlation over the 144 pixels after both sides are centred on their own ink;
centring matters because the game's glyphs sit tight in the cell and a
rasteriser does not.

The candidate universe is every two-byte character `cp932` can encode - the
character set a Japanese PlayStation game could actually have shipped - which is
about 7,000 including kana, punctuation and both kanji levels.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image, ImageDraw, ImageFont

import rendertext
from workdir import WORK

FONTS = [r'C:\Windows\Fonts\msgothic.ttc', r'C:\Windows\Fonts\meiryo.ttc',
         r'C:\Windows\Fonts\YuGothM.ttc']
RENDER_PX = 96          # rasterise big, then area-average down to 12x12
N = 12


def candidates(level1_only=True):
    """characters a 1998 Japanese console game could ship.

    Restricted to JIS level 1 by default (Shift-JIS 0x889F..0x9872) plus kana and
    punctuation. The first attempt used every cp932 two-byte code - 9,205
    characters - and the rare, ink-heavy ones (囂 麕 朧 髓) won almost every
    match, because at 12x12 a dense kanji is a blob and a blob correlates with
    everything. Narrowing the universe is half the accuracy."""
    out = {}
    ranges = [(0x8140, 0x879F)]                      # punctuation, kana, symbols
    ranges.append((0x889F, 0x9872) if level1_only else (0x889F, 0xEAA4))
    for lo, hi in ranges:
        for w in range(lo, hi + 1):
            lead, trail = w >> 8, w & 0xFF
            if trail < 0x40 or trail == 0x7F or trail > 0xFC:
                continue
            try:
                ch = bytes((lead, trail)).decode('cp932')
            except Exception:
                continue
            if len(ch) == 1 and not ch.isspace():
                out[ch] = None
    return list(out)


def grey12(im):
    """a PIL image -> 12x12 grey tuple, cropped to ink and area-averaged.

    Downsampling a large rasterisation is what makes a modern outline font
    comparable to a 12x12 hand-tuned bitmap: rendering straight at 12px gives
    grey mush with no structure left to match."""
    box = im.getbbox()
    if box is None:
        return None
    im = im.crop(box)
    w, h = im.size
    side = max(w, h)
    square = Image.new('L', (side, side), 0)
    square.paste(im, ((side - w) // 2, (side - h) // 2))
    small = square.resize((N, N), Image.BOX)
    return tuple(small.getdata())


def zncc(a, b):
    """zero-mean normalised correlation: immune to how much ink each side has,
    which is exactly the bias that made the first attempt pick dense kanji.
    """
    n = len(a)
    ma, mb = sum(a) / n, sum(b) / n
    da = [x - ma for x in a]
    db = [y - mb for y in b]
    va = sum(x * x for x in da) ** 0.5
    vb = sum(y * y for y in db) ** 0.5
    if va == 0 or vb == 0:
        return 0.0
    return sum(x * y for x, y in zip(da, db)) / (va * vb)


def build_reference(level1_only=True):
    """character -> its 12x12 grey forms, one per font"""
    ref = {}
    chars = candidates(level1_only)
    for path in FONTS:
        if not os.path.exists(path):
            continue
        try:
            font = ImageFont.truetype(path, RENDER_PX)
        except Exception:
            continue
        for ch in chars:
            im = Image.new('L', (RENDER_PX * 2, RENDER_PX * 2), 0)
            ImageDraw.Draw(im).text((RENDER_PX // 2, RENDER_PX // 4), ch,
                                    fill=255, font=font)
            g = grey12(im)
            if g is not None:
                ref.setdefault(ch, []).append(g)
    return ref


def identify(bitmap, ref, topn=1):
    best = []
    for ch, forms in ref.items():
        best.append((max(zncc(bitmap, f) for f in forms), ch))
    best.sort(reverse=True)
    return best[:topn]

def game_glyphs(bank):
    """code -> normalised bitmap for every drawable glyph in a bank"""
    data, at = rendertext.font_res(WORK + '/int1_stage.dir')
    out = {}
    for code in range(int(bank, 16) << 8, (int(bank, 16) << 8) + 0x100):
        try:
            im = rendertext.glyph(data, at, code)
        except Exception:
            im = None
        if im is None:
            continue
        bm = grey12(im)
        if bm is not None:
            out[code] = bm
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--validate', action='store_true')
    ap.add_argument('--bank', default='90')
    ap.add_argument('--table')
    ap.add_argument('--topn', type=int, default=3)
    args = ap.parse_args()

    print('building the reference set...', flush=True)
    ref = build_reference()
    print('%d candidate character(s), %d bitmap form(s)'
          % (len(ref), sum(len(v) for v in ref.values())), flush=True)

    if args.validate:
        import jptext
        known = jptext.GLYPH_90
        glyphs = game_glyphs('90')
        hits = miss = 0
        wrong = []
        for code, bm in sorted(glyphs.items()):
            want = known.get(code)
            if want is None or len(want) != 1 or want in ' 、。，々ー〜‥（）「」』％＆／…×○':
                continue
            got = identify(bm, ref, args.topn)
            names = [c for _, c in got]
            if want == names[0]:
                hits += 1
            elif want in names:
                hits += 1
                wrong.append((code, want, names, 'in top %d' % args.topn))
            else:
                miss += 1
                wrong.append((code, want, names, 'MISS'))
        total = hits + miss
        print()
        print('validated against the hand-transcribed 0x90 kanji:')
        print('  %d of %d agree within top %d  (%.1f%%)' % (hits, total, args.topn, 100.0 * hits / total))
        print()
        for code, want, names, how in wrong[:40]:
            print('  %04X hand=%s ocr=%s  %s' % (code, want, ' '.join(names), how))
        return 0

    glyphs = game_glyphs(args.bank)
    table = {}
    for code, bm in sorted(glyphs.items()):
        got = identify(bm, ref, 1)
        table['%04X' % code] = got[0][1]
        print('%04X %s  %.3f' % (code, got[0][1], got[0][0]))
    if args.table:
        with open(args.table, 'w', encoding='utf-8') as fh:
            json.dump(table, fh, ensure_ascii=False, indent=1)
        print('-> %s' % args.table)
    return 0


if __name__ == '__main__':
    sys.exit(main())
