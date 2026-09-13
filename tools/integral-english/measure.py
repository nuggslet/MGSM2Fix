"""Measure the briefing labels from a screenshot: threshold the green text,
group rows into bands, report each band's x extent and cap height."""
import sys
from PIL import Image

def bands(path, x0, x1, y0, y1, minrun=6):
    im = Image.open(path).convert('RGB').crop((x0, y0, x1, y1))
    w, h = im.size
    px = im.load()
    rows = []
    for y in range(h):
        xs = [x for x in range(w)
              if (lambda r, g, b: g > 110 and g > r + 25 and g > b + 25)(*px[x, y])]
        rows.append(xs)
    out, cur = [], None
    for y in range(h):
        if len(rows[y]) >= 3:
            if cur is None: cur = [y, y, []]
            cur[1] = y; cur[2].extend(rows[y])
        else:
            if cur and cur[1] - cur[0] + 1 >= minrun: out.append(cur)
            cur = None
    if cur and cur[1] - cur[0] + 1 >= minrun: out.append(cur)
    return [(min(c[2]) + x0, max(c[2]) + x0, c[0] + y0, c[1] + y0) for c in out]

if __name__ == '__main__':
    path, x0, x1, y0, y1 = sys.argv[1], *map(int, sys.argv[2:6])
    for a, b, t, bo in bands(path, x0, x1, y0, y1):
        print('   x %4d..%-4d  w=%-4d   y %4d..%-4d  h=%-3d' % (a, b, b - a + 1, t, bo, bo - t + 1))
