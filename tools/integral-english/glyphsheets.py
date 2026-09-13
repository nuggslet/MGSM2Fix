"""Render every unidentified game glyph for transcription, with its contexts.

`jptext.py` reads 87% of the Japanese on the disc. The rest is bank 1, whose
glyphs are drawn in a font Konami made: template matching against a modern
outline font scores 47% top-1 and against Shinonome's native 12-dot bitmaps
only 5.9%, so there is no reference font to look them up in. What is left is
recognition, and a person - or a multimodal model - reading the glyphs is the
tool that fits.

    py glyphsheets.py                 -> work/glyphs-to-identify.{pdf,tsv}
                                         + work/glyphpages/pageNN.{png,txt}
    py glyphsheets.py --per-page 96

WHAT IS ON A SHEET, AND WHAT IS BESIDE IT

Each cell is one glyph at 5x with its ID beneath. **The companion `.txt` for
each page is the more important half**: for every glyph on that page it prints
real lines from the game with the glyph marked 【 】 and everything already
readable spelled out, so the character is fixed by the sentence rather than by
squinting at 144 pixels. At 12x12 線/緑, 鏡/鎌 and 間/問 are the same picture;
in a sentence they are not. Read the image and the text together.

The 78 glyphs already named from the stage archives are **left in the sheets on
purpose**, with the answers in `work/glyph-answers.tsv`. Transcribe them like
any other and score yourself: that is a measured error rate on this exact font
at this exact size, from a labelled set, which is the only honest way to say
how good the rest of the pass is.

WHY THIS NEEDS NO TABLE OFFSETS

Bank 1 is a per-block table, so `code -> shape` differs between blocks - but
`shape -> character` is global, proven by the 78 stage-archive shapes turning
up byte-identical inside `RADIO.DAT`. So a glyph named once here is named
everywhere it is reused, and none of this depends on locating a block's table.

Shapes are ordered by how often they occur **in the text** - which needed
`radiomap.py` to get right; the earlier ordering was by presence in a font
blob, over a fragment map that was wrong for 93% of strings. The real
distribution is steep: the top 100 shapes are 63% of the bank-1 text, the top
300 are 91%, and the tail to 1,200 is the last 9%.
"""
import argparse
import collections
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image, ImageDraw

import dumpjp
import jptext
import radiomap
from workdir import WORK

N = 12
GLYPH = 36
CODE = re.compile(r'<([0-9A-F]{4})>')


def draw_glyph(b, scale):
    im = Image.new('L', (N, N))
    for y in range(N):
        bits = int.from_bytes(b[y*3:y*3+3], 'big')
        for x in range(N):
            im.putpixel((x, y), ((bits >> (22 - 2*x)) & 3) * 85)
    return im.point(lambda v: 255 - v).resize((N*scale, N*scale), Image.NEAREST)


def stage_named():
    """shape -> character, for the bank-1 glyphs already read off the archives"""
    blobs = dumpjp.stage_blobs(0)
    out = {}
    for (stg, code), ch in jptext.BANK1.items():
        blob = blobs.get(stg)
        if blob is None:
            continue
        raw = blob[((code & ~0x6000) - 0x9A01)*GLYPH:][:GLYPH]
        if len(raw) == GLYPH:
            out[raw] = ch
    return out


def contexts(m, known, want, per_shape=3, width=13):
    """shape -> up to N decoded excerpts, the shape itself marked 【 】.

    `known` is shape -> char for everything already identified; anything else
    bank-1 prints as ・ so a line stays the right length and the reader can see
    how much of it is settled.
    """
    out = collections.defaultdict(list)
    seen = collections.Counter()
    for off, text in m['rows']:
        blob = radiomap.blob_of(m, off)
        if blob is None:
            continue
        parts = re.split(r'(<[0-9A-F]{4}>)', text)
        chars, shapes = [], []
        for part in parts:
            mm = CODE.fullmatch(part or '')
            if not mm:
                for ch in (part or ''):
                    if ch not in '\r\n\t':
                        chars.append(ch)
                        shapes.append(None)
                continue
            v = int(mm.group(1), 16) & ~0x6000
            if 0x9600 <= v < 0x9A00:
                i = radiomap.bank1_index(v) * GLYPH
                raw = blob[i:i + GLYPH]
                chars.append(known.get(raw, '・'))
                shapes.append(raw)
            else:
                chars.append(jptext.glyph_char(v) or '・')
                shapes.append(None)
        for i, raw in enumerate(shapes):
            if raw is None or raw not in want or seen[raw] >= per_shape:
                continue
            lo, hi = max(0, i - width), min(len(chars), i + width + 1)
            out[raw].append('%s【%s】%s' % (''.join(chars[lo:i]),
                                           chars[i] if chars[i] != '・' else '?',
                                           ''.join(chars[i+1:hi])))
            seen[raw] += 1
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--per-page', type=int, default=150)
    ap.add_argument('--cols', type=int, default=15)
    ap.add_argument('--scale', type=int, default=5)
    ap.add_argument('--pdf', default=WORK + '/glyphs-to-identify.pdf')
    ap.add_argument('--tsv', default=WORK + '/glyphs-to-identify.tsv')
    ap.add_argument('--answers', default=WORK + '/glyph-answers.tsv')
    args = ap.parse_args()

    m = radiomap.build(0, verbose=False)
    m2 = radiomap.build(1, verbose=False)
    count = collections.Counter(m['count'])
    count.update(m2['count'])
    named = stage_named()
    order = [s for _, s in sorted(((-n, s) for s, n in count.items()),
                                  key=lambda t: (t[0], t[1]))]
    print('%d distinct shapes in the bank-1 text; %d already named from the '
          'stage archives (%.1f%% of the instances)'
          % (len(order), sum(1 for s in order if s in named),
             100.0 * sum(count[s] for s in order if s in named)
             / sum(count.values())))

    ctx = contexts(m, named, set(order))
    print('contexts collected for %d of %d shapes' % (len(ctx), len(order)))

    SC = args.scale
    CW, CH = N*SC + 16, N*SC + 22
    pages, rows = [], []
    outdir = os.path.dirname(args.pdf) + '/glyphpages'
    os.makedirs(outdir, exist_ok=True)
    for pno, start in enumerate(range(0, len(order), args.per_page), 1):
        chunk = order[start:start + args.per_page]
        nrow = (len(chunk) + args.cols - 1) // args.cols
        im = Image.new('L', (args.cols*CW, nrow*CH), 255)
        d = ImageDraw.Draw(im)
        txt = ['page %d: glyphs g%d-g%d' % (pno, start, start + len(chunk) - 1), '']
        for i, shape in enumerate(chunk):
            gid = start + i
            x, y = (i % args.cols)*CW, (i // args.cols)*CH
            im.paste(draw_glyph(shape, SC), (x + 8, y + 4))
            d.text((x + 8, y + N*SC + 7), 'g%d' % gid, fill=0)
            d.rectangle([x, y, x + CW - 2, y + CH - 2], outline=200)
            rows.append((gid, count[shape], shape))
            txt.append('g%-5d x%-6d %s' % (gid, count[shape],
                                           '  |  '.join(ctx.get(shape, []))))
        pages.append(im)
        im.save('%s/page%02d.png' % (outdir, pno))
        io.open('%s/page%02d.txt' % (outdir, pno), 'w',
                encoding='utf-8').write('\n'.join(txt) + '\n')
    pages[0].save(args.pdf, save_all=True, append_images=pages[1:], resolution=150.0)
    print('%d page(s) of %dx%d -> %s and %s/pageNN.{png,txt}'
          % (len(pages), pages[0].width, pages[0].height, args.pdf, outdir))

    with io.open(args.tsv, 'w', encoding='utf-8', newline='') as fh:
        fh.write('id\toccurrences\tchar\tshape_hex\n')
        for gid, n, shape in rows:
            fh.write('g%d\t%d\t\t%s\n' % (gid, n, shape.hex()))
    with io.open(args.answers, 'w', encoding='utf-8', newline='') as fh:
        fh.write('id\tchar\tshape_hex\n')
        for gid, n, shape in rows:
            if shape in named:
                fh.write('g%d\t%s\t%s\n' % (gid, named[shape], shape.hex()))
    print('-> %s  (fill in the char column)' % args.tsv)
    print('-> %s  (%d known answers, for scoring the pass)'
          % (args.answers, sum(1 for _, _, s in rows if s in named)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
