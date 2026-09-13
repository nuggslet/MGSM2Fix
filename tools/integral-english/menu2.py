"""Rebuild en_menu and en_menu2 from retail, preserving shipped payloads.

Recovered from menufinal.py/menu2.py on 2026-09-04, with explicit inputs and no
deployment. The `title` copy of the disc-swap text is NOT built here: it lives
inline in that stage's script and needs container arithmetic, so it has its own
builder, `menu3.py`. It is raw-disc only - the collection patches the same block
and the two layouts do not mix (README, "Why `en_menu3` is raw-disc only").
"""
from pathlib import Path
from workdir import WORK, GAME, RAW
from iso import Disc
from portio import records, changed_runs, map_runs, ppf, INTEGRAL_IMAGES

CHANGE, DEMOSEL, TITLE = 0x04650A1F, 0x04702FED, 0x0455A9B5


def slots(data, base):
    items, _ = records(data, base)
    p = base
    for s in items:
        yield p, s
        p += 2+len(s)


def build(original):
    first = bytearray(original)
    # TITLE index 4 is `RADAR OFF`, in the same block as the title's disc-swap
    # copy. For a RAW disc `menu3.py` rewrites that whole block and shifts every
    # record in it, so en_menu must not also write this one at retail's offset -
    # the two would land on the same bytes with different layouts. menu3 ports it
    # instead. (Caught 2026-09-07 by rebuild.py's packaged-set overlap check, on
    # the first raw build.)
    edits = [(0x035251B8, 4, b'screen brightness setup'),
             (0x035251B8, 5, b'key configuration setup')]
    if not RAW:
        edits.insert(0, (TITLE, 4, b'RADAR OFF'))
    for base, index, text in edits:
        off, value = list(slots(original, base))[index]
        assert len(text) <= len(value)-1
        assert len(text)+1 <= len(value)-1
        first[off+2:off+1+len(value)] = (text+b'\0').ljust(len(value)-1, b' ')
    ca, ds = records(original, CHANGE)[0], records(original, DEMOSEL)[0]
    signatures = {
        'A2': (0, ca[0][:-1], b'Insert DISC 2.'),
        'C': (0, ca[1][:-1], b'Press the Start Button'),
        'B2': (0, ca[2][:-1], b'after inserting DISC 2.'),
        'D': (0, ca[3][:-1], b'Now Checking...'),
        'E': (2, ca[5][2:-1], b'The correct DISC was not inserted.'),
        'A1': (0, ds[0][:-1], b'Insert DISC 1.'),
        'B1': (0, ds[1][:-1], b'after inserting DISC 1.'),
        'N1': (0, ds[6][:-1], b'Meryl'),
        'N2': (0, ds[7][:-1], b'Otacon'),
        'N3': (0, ds[8][:-1], b'Red Meril'),
        'N4': (0, ds[9][:-1], b'Red Otacon'),
    }
    second = bytearray(first)
    for base, names in ((DEMOSEL, ('N3','N4','N1','N2','A1','B1','C','D','E')),
                        (CHANGE, ('A2','B2','C','D','E')),
                        (0x04650C44, ('A2','B2','C','D','E'))):
        found = set()
        for off, value in slots(original, base):
            for name in names:
                skip, sig, english = signatures[name]
                if value[skip:-1] != sig:
                    continue
                cap = len(value)-1
                assert len(english)+1 <= cap
                padding = cap-len(english)-1
                term = bytes(2 if padding % 2 else 1)
                second[off+2:off+1+len(value)] = english+term+b' '*(cap-len(english)-len(term))
                found.add(name)
                break
        assert found == set(names), (hex(base), set(names)-found)
    assert len(first) == len(second) == len(original)
    return bytes(first), bytes(second)


def main():
    work = Path(WORK)
    for disc, base in enumerate(INTEGRAL_IMAGES):
        original = (work/('int%d_stage.dir' % (disc+1))).read_bytes()
        first, second = build(original)
        image = Disc(Path(GAME)/'windata/dlc/dlc_japan.bin', base)
        try:
            lba = next(l for n,l,s,d in image.walk() if n.upper() == '/MGS/STAGE.DIR;1')
        finally:
            image.f.close()
        for suffix, old, new in (('menu', original, first), ('menu2', first, second)):
            result = ppf(map_runs(lba, changed_runs(old,new)), 'MGS Integral: English menu text')
            name = work/('INTEGRAL_disc%d_en_%s.ppf' % (disc+1,suffix))
            name.write_bytes(result)
            print(name)


if __name__ == '__main__':
    main()
