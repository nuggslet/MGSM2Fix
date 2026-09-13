#!/usr/bin/env python
"""Measure the Option -> SCREEN brightness paragraph in one or two screenshots.

    py shotcmp_brightness.py <shot.jpg> [<other.jpg>]

Anchors on the green line and reports the text's first-line position in units
of its own line pitch, so two shots with different aspect ratios or border art
compare directly. Reference values, measured 2026-09-03:

    SwanStation USA           1.88 line-heights below the green line
    the port (Integral)       1.86
    the collection's USA      2.86 before [Patches] BrightnessText, 1.86 after

Also tests for the opaque notch the collection's re-centred texture painted
over the grey ramp: mean luminance INSIDE the paragraph's x span against a
strip to its LEFT, row by row between the green line and the text. A row where
the inside reads darker is the canvas painting over a brighter ramp band.

With two shots it also reports the delta between them, and a per-band pixel
difference over the whole screen below the header.
"""
import sys
from PIL import Image, ImageChops

OUT = (500, 800)        # left of the paragraph canvas (canvas starts ~825 at 3840 wide)
IN = (900, 2800)        # inside it


def lum(t):
    return 0.299 * t[0] + 0.587 * t[1] + 0.114 * t[2]


def measure(path, label):
    im = Image.open(path).convert('RGB')
    W, H = im.size
    p = im.load()
    sx = W / 3840.0
    out_x = (int(OUT[0] * sx), int(OUT[1] * sx))
    in_x = (int(IN[0] * sx), int(IN[1] * sx))

    best, gy = -1, None
    for y in range(H):
        s = sum(max(0, g - (r + b) // 2)
                for r, g, b in (p[x, y] for x in range(int(W * .30), int(W * .60), 8)))
        if s > best:
            best, gy = s, y

    rows = [y for y in range(gy + 4, int(H * .80))
            if sum(1 for x in range(in_x[0], in_x[1], 2) if lum(p[x, y]) > 120) > 6]
    lines, cur = [], [rows[0]]
    for y in rows[1:]:
        if y - cur[-1] <= 3:
            cur.append(y)
        else:
            lines.append((cur[0], cur[-1])); cur = [y]
    lines.append((cur[0], cur[-1]))
    tops = [L[0] for L in lines if L[1] - L[0] >= 8][:4]
    pitch = (tops[2] - tops[0]) / 2.0 if len(tops) >= 3 else float('nan')

    print('%-18s %dx%d  green line y=%d  lines at %s  pitch %.1f px'
          % (label, W, H, gy, tops, pitch))
    print('%-18s first line %.2f line-heights below the green line'
          % ('', (tops[0] - gy) / pitch))
    notch = []
    y = gy + 6
    while y < tops[0] - 2:
        a = sum(lum(p[x, y]) for x in range(*out_x, 4)) / len(range(*out_x, 4))
        b = sum(lum(p[x, y]) for x in range(*in_x, 4)) / len(range(*in_x, 4))
        if b - a < -1.5:
            notch.append((y, a, b))
        y += max(1, H // 240)
    if notch:
        print('%-18s NOTCH: %d row(s) read darker inside the canvas, e.g. y=%d %.2f -> %.2f'
              % ('', len(notch), notch[0][0], notch[0][1], notch[0][2]))
    else:
        print('%-18s no notch: no row inside the canvas reads darker than the strip beside it' % '')
    return im, gy, tops, pitch


def main():
    args = sys.argv[1:]
    if not args or len(args) > 2:
        print(__doc__)
        return 2
    a = measure(args[0], 'A')
    if len(args) == 2:
        print()
        b = measure(args[1], 'B')
        print()
        print('delta A-B: first line %d px = %.2f line pitches'
              % (a[2][0] - b[2][0], (a[2][0] - b[2][0]) / a[3]))
        if a[0].size == b[0].size:
            px = ImageChops.difference(a[0], b[0]).load()
            W, H = a[0].size
            print('pixel difference by band (max over sampled pixels):')
            for y0 in range(0, H, H // 9):
                y1 = min(y0 + H // 9, H)
                m = max(max(px[x, y]) for y in range(y0, y1, 3) for x in range(int(W * .125), int(W * .875), 3))
                print('   rows %4d..%-4d  max %3d' % (y0, y1, m))
    return 0


if __name__ == '__main__':
    sys.exit(main())
