"""Measure submenu rows relative to the vertical divider line to its left."""
import sys
from PIL import Image
def green(p): return p[1] > 150 and p[1] > p[0] + 30 and p[1] > p[2] + 30
def analyse(path):
    im = Image.open(path).convert('RGB'); px = im.load(); W, H = im.size
    # the divider: the column in 1900..2200 that is green for the most rows
    best = (0, None)
    for x in range(1900, 2200):
        n = sum(1 for y in range(560, 1800) if green(px[x, y]))
        if n > best[0]: best = (n, x)
    cx = best[1]
    ys = [y for y in range(500, 1900) if green(px[cx, y])]
    dtop, dbot = ys[0], ys[-1]
    # rows to the right of the divider
    x0 = cx + 14
    rows = []
    for y in range(500, 1900):
        xs = [x for x in range(x0, 3400) if green(px[x, y])]
        rows.append(xs)
    out = []; cur = None
    for i, y in enumerate(range(500, 1900)):
        if len(rows[i]) >= 8:
            if cur is None: cur = [y, y, []]
            cur[1] = y; cur[2].extend(rows[i])
        else:
            if cur and cur[1] - cur[0] + 1 >= 4: out.append(cur)
            cur = None
    if cur and cur[1] - cur[0] + 1 >= 4: out.append(cur)
    return cx, dtop, dbot, [(c[0], c[1], min(c[2]), max(c[2])) for c in out if max(c[2]) - min(c[2]) > 60]
for path in sys.argv[1:]:
    cx, dtop, dbot, bands = analyse(path)
    print('%s' % path.split('/')[-1])
    print('   divider x=%d  y %d..%d  (len %d)' % (cx, dtop, dbot, dbot - dtop + 1))
    for t, b, a, c in bands:
        print('     row top %4d  = divider%+5d    h %-3d  x %4d..%-4d w %d'
              % (t, t - dtop, b - t + 1, a, c, c - a + 1))
