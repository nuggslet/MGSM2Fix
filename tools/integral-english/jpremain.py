"""What Japanese is still on the three DEPLOYED Integral discs, and where?

Every other sweep here reads RETAIL, on purpose: `mainsweep.py` enumerates what
USA has in English so a gap cannot hide behind a patch that is already applied.
This one asks the opposite question - what does a player still meet? - and that
question can only be answered from the bytes the game actually loads.

    py jpremain.py

WHAT IT READS

Retail sectors out of the collection's container with every deployed PPF record
overlaid, and the STAGE.DIR entry followed afterwards: four families (`en_abst`,
`en_brf`, `en_option`, `en_preope`) relocate their stage into DUMMY3M and
repoint that entry, so reading the retail LBA would silently return the
unpatched stage. Patch the archive first, then read the entry, then follow it.

WHY THE CLASSIFIER IS NOT `game_text`'s FLAG

`audit_text.game_text` returns a `japanese` flag that means "this string still
contains game-encoded glyph codes", NOT "this string is Japanese" - and the
difference bit once. A ported English weapon description keeps Integral's own
bracket and button glyphs (`<9014>Socom Pistol<9015>|...#<901D>...`) and trips
the flag; counting on it says the item pool is 100% Japanese when it is 100%
English.

`game_text` already decodes the 0x80xx Latin bank to ASCII, so a code that
SURVIVES decoding is a kana, a kanji or a local-font glyph. Comparing what
survives against the Latin letters beside it separates the three cases:

    jp     codes outnumber Latin letters - it reads as Japanese
    mixed  both present, letters >= codes - either English carrying Integral's
           own quote/bracket glyphs, or an English label beside Japanese
    en     no codes at all

The `mixed` bucket is deliberately a bucket and not a verdict, because both of
those really occur: `<9A0E>Tokyo Game Show, Spring '98<9A0F>` is English in
Integral's typographic quotes, while `<9009><9A15><9A16><9060><812F><900B>NORMAL`
is Japanese with an English word in it. A first version of this counted every
string with three Latin letters as English and mislabelled twelve Japanese
strings on disc 1 that happen to contain FOXDIE, NORMAL or DISC 1.

WHAT IT DOES NOT COVER

The executables' own pools (`items.py`, `savemsg.py` know those addresses) and
texture lettering. `COVERAGE.md` carries those figures beside these.
"""
import glob, re, struct, sys, os
sys.path.insert(0, os.path.abspath('.'))
from collections import defaultdict
import portio, mainsweep, vr_sweep
from audit_text import game_text
from iso import Disc
from portio import INTEGRAL_IMAGES
from workdir import GAME, WORK
from vrlib import stage_gcx, int_disc

CODE = re.compile(r'<[0-9A-F]{4}>')

def classify(t):
    if t is None:
        return 'odd'
    codes = len(CODE.findall(t))
    bare = CODE.sub('', t).replace('|', ' ').replace('#', ' ')
    letters = sum(c.isalpha() and ord(c) < 128 for c in bare)
    if not codes:
        return 'en' if letters else 'blank'
    if codes > letters:
        return 'jp'
    return 'mixed' if letters else 'jp'

def report(label, items):
    tot = defaultdict(int)
    agg, samples = defaultdict(lambda: defaultdict(int)), {}
    for who, name, t in items:
        k = classify(t)
        tot[k] += 1
        agg[(who, name)][k] += 1
        if k in ('jp', 'mixed') and (who, name, k) not in samples:
            samples[(who, name, k)] = t
    print('=== %s ===' % label)
    print('  plain English %d | mixed %d | JAPANESE %d | blank/unparsed %d'
          % (tot['en'], tot['mixed'], tot['jp'], tot['blank'] + tot['odd']))
    rows = sorted(((v['jp'], v['mixed'], k) for k, v in agg.items() if v['jp'] or v['mixed']), reverse=True)
    print('  %-12s %-10s %4s %5s  %s' % ('owner', 'stage', 'jp', 'mixed', 'sample'))
    for jp, mx, (who, name) in rows:
        s = samples.get((who, name, 'jp')) or samples.get((who, name, 'mixed')) or ''
        print('  %-12s %-10s %4d %5d  %s' % (who, name, jp, mx, s[:42]))
    print('  TOTAL Japanese: %d   (mixed, English label beside Japanese: %d)' % (tot['jp'], tot['mixed']))
    print()
    return tot['jp'], tot['mixed']

grand = [0, 0]
for disc_ix, base in enumerate(INTEGRAL_IMAGES):
    pmap = {}
    for p in sorted(glob.glob('%s/mods/INTEGRAL/INTEGRAL/%d/*.ppf' % (GAME, disc_ix))):
        for off, blob in vr_sweep.ppf_records(p):
            for i, c in enumerate(blob):
                pmap[off + i] = c
    image = Disc(GAME + '/windata/dlc/dlc_japan.bin', base)
    items = []
    try:
        files = {n.upper(): (l, s) for n, l, s, d in image.walk() if not d}
        sd_lba, sd_size = files['/MGS/STAGE.DIR;1']
        def read(lba, size):
            d = bytearray(image.read(lba, size))
            for q in range(size):
                sec, w = divmod(q, 2048)
                off = (lba + sec) * 2352 + 24 + w
                if off in pmap:
                    d[q] = pmap[off]
            return bytes(d)
        sd = read(sd_lba, sd_size)
        for name, (rel, entry) in sorted(portio.entries(sd).items()):
            cnt = struct.unpack_from('<h', read(sd_lba + rel, 2048), 2)[0]
            for gcx in mainsweep.scripts(read(sd_lba + rel, cnt * 2048)):
                for who, raw in mainsweep.strings(gcx):
                    items.append((who, name, game_text(raw[:-1])[0]))
    finally:
        image.f.close()
    a, b = report('Integral disc %d, stage scripts' % (disc_ix + 1), items)
    grand[0] += a; grand[1] += b

# VR disc
isd = open(vr_sweep.INT_STAGE, 'rb').read()
usd = open(vr_sweep.USA_STAGE, 'rb').read()
disc = int_disc()
patches = []
for p in sorted(glob.glob(os.path.join(vr_sweep.MODS, '*.ppf'))):
    patches += vr_sweep.ppf_records(p)
items = []
for name in sorted(set(portio.entries(isd)) & set(portio.entries(usd))):
    try:
        gcx = stage_gcx(vr_sweep.deployed(isd, disc, name, patches))[4]
    except Exception:
        continue
    for who, b in vr_sweep.records(gcx):
        items.append((who, name, game_text(b)[0]))
a, b = report('Integral VR disc, stage scripts', items)
grand[0] += a; grand[1] += b
print('ALL THREE DISCS, stage scripts: %d Japanese, %d mixed' % tuple(grand))
