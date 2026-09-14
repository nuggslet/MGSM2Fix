"""Dump EVERY untranslated Japanese string on all three Integral discs.

`COVERAGE.md` itemised the stage archives and the executable pools, and
`discaudit.py` measured the other files by volume - but until this existed there
was no single line-by-line list of the Japanese that is actually on a disc. This
writes one.

    py jplist.py                    -> work/japanese-inventory.tsv + a summary
    py jplist.py --min 12           only runs of 12+ kana/kanji (less noise)
    py jplist.py --render 40        also draw the 40 longest runs as PNGs

The output is a TSV, one row per string: disc, source file, byte offset, byte
length, how many glyph slots, how many of those are kana/kanji, and the string
in `game_text`'s `<xxxx>` code form. It is written to `work/` and NOT committed:
the commentary alone is millions of glyph slots, so the file runs to tens of MB.
The repository keeps the counts and the method; the list is regenerated.

WHAT COUNTS AS A JAPANESE STRING HERE

`game_text` already decodes the 0x80xx Latin bank to ASCII, so what identifies
Japanese is a run of pairs whose lead byte is in a kana or kanji bank:

    kana / kanji   0x81 0x82 0x96          <- a run needs at least --min of these
    interior       0x80 0x90 0x9A 0xC1 0xC2 0xD0

The interior leads are Latin letters, spaces, punctuation, button glyphs and the
text control codes, all of which appear *inside* Japanese strings - the
controller-port line in `s07b` embeds `<8031>` for its "1", and every codec line
carries `d0 03`. Allowing them inside a run but not counting them toward the
threshold is what keeps `MP 5 SD` (Latin name, Japanese body) in and keeps binary
out. That distinction was learned twice: once when a lead-byte range test called
31.7% of `RADIO.DAT` Japanese, and once when a three-Latin-letters test called
twelve Japanese strings English because they contain FOXDIE or NORMAL.

WHAT IT READS

Deployed bytes for the stage archives and executables - retail sectors with every
deployed PPF overlaid, and the relocated STAGE.DIR entry followed, exactly as
`jpremain.py` does - and retail bytes for `RADIO.DAT`, `DEMO.DAT` and `VOX.DAT`,
which no patch touches. `BRF.DAT` and `FACE.DAT` are scanned too and come out
empty; if they ever do not, something has changed.

AND EVERY RAW-FILE RUN IS CHECKED AGAINST THE USA DISC

A run that also appears in the USA disc's copy of the same file is dropped.
That single test is what makes the list mean "untranslated Japanese" rather
than "bytes in the text banks": it removes all of `BRF.DAT`'s 56 matches and
all of `FACE.DAT`'s, which are image data, and it would also remove text USA
left Japanese as well - a different category from Integral-exclusive content.

ONE DEFINITIONAL EDGE, AND WHY `jpremain.py` DISAGREES

`CORE` is the kana banks and the main kanji bank. It deliberately excludes
`0x9Axx`, which holds glyphs too but is also where Integral keeps its
typographic quotes - counting it would call `<9A0E>Tokyo Game Show<9A0F>
Japanese. The cost is that a string built only from `0x9Axx` glyphs, like the
`cmd 4AD9` location titles, is not listed here.

So the two tools give different counts for the stage archives - 98 rows here
against `jpremain.py`'s 153 - and **`jpremain.py` is the authority there**: it
works on complete parsed records and can weigh glyphs against Latin letters,
which is the better test when there is no binary to guard against. This
tool's value is the raw files, where nothing else looks at all.

None of this Japanese has a USA counterpart, so none of it is porting work under
the standing rule - it is what a translation project would start from. See
`COVERAGE.md`, "The file-level blind spot, and what it hid".
"""
import argparse
import glob
import os
import re
import struct
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import mainsweep
import portio
import vr_sweep
from audit_text import game_text
from iso import Disc
from portio import INTEGRAL_IMAGES, USA_IMAGES
from vrlib import int_disc, stage_gcx
from workdir import GAME, WORK

CONTAINER = GAME + '/windata/dlc/dlc_japan.bin'
USA_CONTAINER = GAME + '/windata/alldata.bin'
CORE = b'\x81\x82\x96'                      # kana and kanji banks
INTERIOR = b'\x80\x90\x9a\xc1\xc2\xd0'      # Latin, punctuation, buttons, controls
RUN = re.compile(b'(?:[' + re.escape(CORE + INTERIOR) + b'][\x00-\xff])+')
CHUNK = 8 * 1024 * 1024


def core_count(run):
    return sum(1 for i in range(0, len(run) - 1, 2) if run[i] in CORE)


def looks_like_prose(run):
    """reject compressed/image data that happens to sit in the text banks.

    `BRF.DAT` is the case that forced this: 56 runs matched there, every one
    of them a single code repeated (`<8283><8283><8283>...`), and all 56 exist
    on the USA disc too - it is image data, not Japanese. Real prose uses many
    distinct glyphs and repeats none of them heavily.
    """
    codes = [run[i:i + 2] for i in range(0, len(run) - 1, 2) if run[i] in CORE]
    if len(codes) < 4:
        return True
    distinct = len(set(codes))
    commonest = max(codes.count(c) for c in set(codes))
    return distinct >= 4 and commonest <= len(codes) * 0.5


def runs_in(data, minimum, base=0):
    """(offset, bytes) for every prose-looking run with `minimum`+ kana/kanji.

    Used on RAW files, where a match is a guess about bytes. Parsed records out
    of a stage archive do not need either test and do not get them."""
    for m in RUN.finditer(data):
        run = m.group()
        if core_count(run) >= minimum and looks_like_prose(run):
            yield base + m.start(), run


def rows_from_blob(disc, source, data, minimum, base=0):
    out = []
    for off, run in runs_in(data, minimum, base):
        text, _ = game_text(run)
        out.append((disc, source, off, len(run), len(run) // 2, core_count(run), text or ''))
    return out


def deployed_stage_rows(disc_ix, minimum):
    """the stage archives, as the game loads them (patches applied, relocation followed)"""
    pmap = {}
    for p in sorted(glob.glob('%s/mods/INTEGRAL/INTEGRAL/%d/*.ppf' % (GAME, disc_ix))):
        for off, blob in vr_sweep.ppf_records(p):
            for i, c in enumerate(blob):
                pmap[off + i] = c
    image = Disc(CONTAINER, INTEGRAL_IMAGES[disc_ix])
    rows = []
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
            count = struct.unpack_from('<h', read(sd_lba + rel, 2048), 2)[0]
            data = read(sd_lba + rel, count * 2048)
            for gcx in mainsweep.scripts(data):
                for who, raw in mainsweep.strings(gcx):
                    payload = raw[:-1]
                    if not core_count(payload):
                        continue
                    text, _ = game_text(payload)
                    rows.append(('disc%d' % (disc_ix + 1), 'STAGE.DIR/%s/%s' % (name, who),
                                 0, len(payload), len(payload) // 2,
                                 core_count(payload), text or ''))
        # the executable, in the same deployed form
        exe, lba, size = next((n, l, s) for n, l, s in
                              [(n, l, s) for n, (l, s) in files.items()] if 'SLPM' in n)
        data = read(lba, size)
        rows += rows_from_blob('disc%d' % (disc_ix + 1), os.path.basename(exe), data, minimum)
    finally:
        image.f.close()
    return rows


def usa_run_set(disc_ix, want, minimum):
    """the same runs on the USA disc's copy of the same file.

    A run that is on both discs is not untranslated Japanese: it is either
    binary that happens to sit in the text banks (all of `BRF.DAT`'s) or text
    USA left Japanese too, which is a different category from Integral-only
    content. Subtracting this is what makes the list mean what it says.
    """
    try:
        image = Disc(USA_CONTAINER, USA_IMAGES[disc_ix])
    except Exception:
        return None
    try:
        files = {n.upper(): (l, s) for n, l, s, d in image.walk() if not d}
        if want not in files:
            return set()
        lba, size = files[want]
        out, done = set(), 0
        while done < size:
            n = min(CHUNK, size - done)
            data = image.read(lba + done // 2048, n)
            for _, run in runs_in(data, minimum):
                out.add(bytes(run))
            done += n
        return out
    finally:
        image.f.close()


def raw_file_rows(disc_ix, want, minimum):
    """RADIO.DAT and friends: no patch touches them, so retail bytes are what ship"""
    image = Disc(CONTAINER, INTEGRAL_IMAGES[disc_ix])
    rows = []
    try:
        files = {n.upper(): (l, s) for n, l, s, d in image.walk() if not d}
        if want not in files:
            return rows
        lba, size = files[want]
        usa = usa_run_set(disc_ix, want, minimum)
        done = 0
        while done < size:
            n = min(CHUNK, size - done)
            data = image.read(lba + done // 2048, n)
            for off, run in runs_in(data, minimum, base=done):
                if usa is not None and bytes(run) in usa:
                    continue                     # also on the USA disc: not ours
                text, _ = game_text(run)
                rows.append(('disc%d' % (disc_ix + 1), want.split('/')[-1].rstrip(';1'),
                             off, len(run), len(run) // 2, core_count(run), text or ''))
            done += n
    finally:
        image.f.close()
    return rows


def vr_rows(minimum):
    isd = open(vr_sweep.INT_STAGE, 'rb').read()
    usd = open(vr_sweep.USA_STAGE, 'rb').read()
    disc = int_disc()
    patches = []
    for p in sorted(glob.glob(os.path.join(vr_sweep.MODS, '*.ppf'))):
        patches += vr_sweep.ppf_records(p)
    rows = []
    for name in sorted(set(portio.entries(isd)) & set(portio.entries(usd))):
        try:
            gcx = stage_gcx(vr_sweep.deployed(isd, disc, name, patches))[4]
        except Exception:
            continue
        for who, b in vr_sweep.records(gcx):
            if not core_count(b):
                continue
            text, _ = game_text(b)
            rows.append(('vr', 'STAGE.DIR/%s/%s' % (name, who), 0, len(b),
                         len(b) // 2, core_count(b), text or ''))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--min', type=int, default=6,
                    help='minimum kana/kanji for a run in a RAW file (default 6);'
                         ' parsed stage records are listed whatever their length')
    ap.add_argument('--render', type=int, default=0,
                    help='also draw the N longest runs as PNGs under work/jplist/')
    ap.add_argument('--out', default=WORK + '/japanese-inventory.tsv')
    args = ap.parse_args()

    rows = []
    for disc_ix in (0, 1):
        print('disc %d: stage archives and the executable...' % (disc_ix + 1), flush=True)
        rows += deployed_stage_rows(disc_ix, args.min)
        for want in ('/MGS/RADIO.DAT;1', '/MGS/DEMO.DAT;1', '/MGS/VOX.DAT;1',
                     '/MGS/BRF.DAT;1', '/MGS/FACE.DAT;1'):
            print('  %s...' % want, flush=True)
            rows += raw_file_rows(disc_ix, want, args.min)
    print('VR disc: stage archives...', flush=True)
    rows += vr_rows(args.min)

    with open(args.out, 'w', encoding='utf-8', newline='') as fh:
        fh.write('disc\tsource\toffset\tbytes\tglyphs\tkana_kanji\ttext\n')
        for r in rows:
            fh.write('%s\t%s\t0x%X\t%d\t%d\t%d\t%s\n' % r)
    print()
    print('%d row(s) -> %s' % (len(rows), args.out))

    by = defaultdict(lambda: [0, 0])
    for disc, source, off, nbytes, glyphs, core, text in rows:
        key = (disc, source.split('/')[0])
        by[key][0] += 1
        by[key][1] += core
    print()
    print('%-6s %-16s %10s %14s' % ('disc', 'source', 'strings', 'kana/kanji'))
    for (disc, source), (n, core) in sorted(by.items()):
        print('%-6s %-16s %10d %14d' % (disc, source, n, core))
    print('%-6s %-16s %10d %14d' % ('TOTAL', '', len(rows), sum(v[1] for v in by.values())))

    if args.render:
        import subprocess
        outdir = WORK + '/jplist'
        os.makedirs(outdir, exist_ok=True)
        longest = sorted(rows, key=lambda r: -r[5])[:args.render]
        for i, r in enumerate(longest):
            raw = ''.join(re.findall(r'<([0-9A-F]{4})>', r[6]))
            if not raw:
                continue
            subprocess.run([sys.executable, os.path.join(os.path.dirname(__file__), 'rendertext.py'),
                            '--hex', raw, '--out', '%s/%03d_%s.png' % (outdir, i, r[1].replace('/', '_'))],
                           capture_output=True)
        print('rendered %d run(s) into %s' % (len(longest), outdir))
    return 0


if __name__ == '__main__':
    sys.exit(main())
