"""Every file on every disc: size, Integral-vs-USA delta, and does it hold text?

Written 2026-09-09, after a question from outside the project revealed that
every sweep here had only ever read ONE file. `mainsweep.py`, `vr_sweep.py`,
`jpsweep.py`, `audit_text.py` and `jpremain.py` all walk `STAGE.DIR` (plus the
executables); nothing had looked inside `RADIO.DAT`, `DEMO.DAT`, `VOX.DAT`,
`BRF.DAT`, `FACE.DAT` or `ZMOVIE.STR`. Codec dialogue is not in `STAGE.DIR` at
all - `menu/radiomes.c` loads it from `RADIO.DAT` by sector - so no GCL sweep
could ever have reached it, and 6.5 MB of Integral-exclusive Japanese developer
commentary sat unnoticed behind a coverage claim that did not mention which file
it was about.

    py discaudit.py

The diagnostic that matters is the **size delta against the USA disc**, because
that is what points at content one release has and the other does not:
`RADIO.DAT` is +9.4 MB on Integral, and that is the commentary. `BRF.DAT` and
`FACE.DAT` come out clean by the same measure, which is worth having measured
rather than assumed.

The text probe is deliberately crude - dialogue-length ASCII runs, and runs of
kana-bank font-index pairs - because these files are mostly binary and the point
is to rank files for a closer look, not to classify strings. `COVERAGE.md`, "The
file-level blind spot, and what it hid", carries the findings; anything this
flags should be read with `rendertext.py`, since none of the Japanese here is
Shift-JIS.
"""
import re, sys, os
sys.path.insert(0, os.path.abspath('.'))
from iso import Disc
from portio import INTEGRAL_IMAGES, USA_IMAGES, USA_VR_IMAGE
from workdir import GAME

JP = GAME + '/windata/dlc/dlc_japan.bin'
ALL = GAME + '/windata/alldata.bin'
EN_PAT = re.compile(rb'[A-Za-z][ -~]{15,}')
JP_PAT = re.compile(rb'(?:[\x81\x82][\x00-\xff]){8,}')
SAMPLE = 24 * 1024 * 1024          # probe at most this much of a huge file

def listing(container, base):
    im = Disc(container, base)
    try:
        out = {}
        for n, l, s, isdir in im.walk():
            if isdir:
                continue
            out[n.upper()] = (l, s)
        return out, im
    except Exception as e:
        try: im.f.close()
        except Exception: pass
        return None, None

def probe(im, lba, size):
    n = min(size, SAMPLE)
    d = im.read(lba, n)
    en = EN_PAT.findall(d)
    jp = JP_PAT.findall(d)
    return len(en), sum(len(x) for x in en), len(jp), sum(len(x) for x in jp), n

sets = [('Integral d1', JP, INTEGRAL_IMAGES[0]), ('Integral d2', JP, INTEGRAL_IMAGES[1]),
        ('Integral VR', JP, 0x57592000), ('USA d1', ALL, USA_IMAGES[0]),
        ('USA d2', ALL, USA_IMAGES[1]), ('USA VR', ALL, USA_VR_IMAGE)]
tables = {}
for tag, c, b in sets:
    f, im = listing(c, b)
    if f is None:
        print('%-12s could not be opened at 0x%X' % (tag, b)); continue
    tables[tag] = (f, im)

names = sorted({n for f, _ in tables.values() for n in f})
print('%-22s %14s %14s %10s' % ('file', 'Integral d1', 'USA d1', 'delta'))
for n in names:
    i = tables.get('Integral d1', ({}, None))[0].get(n)
    u = tables.get('USA d1', ({}, None))[0].get(n)
    si = i[1] if i else 0
    su = u[1] if u else 0
    print('%-22s %14d %14d %10s' % (n, si, su, ('%+d' % (si - su)) if i and u else '-'))

print()
print('%-12s %-22s %10s  %7s %9s  %7s %9s' % ('disc', 'file', 'probed', 'EN runs', 'EN bytes', 'JP runs', 'JP bytes'))
for tag in ('Integral d1', 'USA d1', 'Integral VR'):
    if tag not in tables: continue
    f, im = tables[tag]
    for n in sorted(f, key=lambda k: -f[k][1]):
        lba, size = f[n]
        if size < 4096 or 'DUMMY' in n:
            continue
        try:
            ec, eb, jc, jb, pn = probe(im, lba, size)
        except Exception as e:
            print('%-12s %-22s  probe failed: %s' % (tag, n, e)); continue
        print('%-12s %-22s %10d  %7d %9d  %7d %9d' % (tag, n, pn, ec, eb, jc, jb))
for f, im in tables.values():
    try: im.f.close()
    except Exception: pass
