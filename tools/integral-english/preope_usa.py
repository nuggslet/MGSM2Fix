"""Build Previous Operations from retail Integral and USA, without a baseline.

USA's exact lines and pages: MG1 90 lines/13 pages, MG2 133 lines/19 pages.
Each seven-line page gets an eighth empty slot for Integral's renderer. MG1
replaces the retail MG1 chain; the original MG2 chain remains (unread), while
USA's MG2 text is appended as the NUL-string blob read by the modified overlay.

    py preope_usa.py             # stages both PPFs in WORK, never deploys
    py preope_usa.py --deploy    # explicit deployment after validation
"""
from pathlib import Path
import argparse
import struct
from workdir import WORK, GAME, DECOMP
from portio import (stage, pack_stage, records, encode_records, relocation,
                    INTEGRAL_IMAGES)
from iso import Disc
from gclparse import parse_script, containers_over, be16, be32

MG2_RECAP_OFFSET = 22042


def paginate(lines):
    out = []
    for p in range(0, len(lines), 7):
        page = lines[p:p+7]
        out.extend(page + [b'\0']*(8-len(page)))
    return out


def build_stage():
    work = Path(WORK)
    tags, payloads, _ = stage((work/'int1_stage.dir').read_bytes(), 'preope')
    _, usa, _ = stage((work/'usa1_stage.dir').read_bytes(), 'preope')
    donor, _ = records(usa[6])
    assert b'mercenary who was feared' in b''.join(donor), 'not USA source text'
    mg1, mg2 = paginate(donor[4:94]), paginate(donor[95:228])
    assert len(mg1) == 104 and len(mg2) == 152
    original = payloads[6]
    old, end = records(original)
    assert len(old) == 191
    updated = old[:4] + mg1 + old[76:]
    chain = encode_records(updated)
    delta = len(chain)-(end-0x1B8)
    script = bytearray(original[:0x1B8]+chain+original[end:])
    root, size = parse_script(original, 0x172)
    for node in containers_over(root, 0x1B8, end):
        fmt, read = ('>I', be32) if node.size_bits == 32 else ('>H', be16)
        struct.pack_into(fmt, script, node.size_at, read(original, node.size_at)+delta)
    assert len(script)-0x1B8 == MG2_RECAP_OFFSET, 'overlay/blob offset mismatch'
    script += b''.join(mg2)
    tree, newsize = parse_script(script, 0x172)
    got, _ = records(script)
    assert newsize == size+delta and tree.kids[0].end == 0x172+newsize
    assert got == updated
    assert bytes(script[0x1B8+MG2_RECAP_OFFSET:]).split(b'\0')[:-1] == [s[:-1] for s in mg2]
    payloads[6] = bytes(script)
    payloads[0] = (Path(DECOMP)/'obj/preope.bin').read_bytes()
    result = pack_stage(tags, payloads)
    assert len(result) == 90*2048
    assert stage(result)[1] == payloads
    print('preope: retail inputs, USA 13/19 pages, 223 chain records, 152 blob slots verified')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--deploy', action='store_true')
    args = parser.parse_args()
    work = Path(WORK)
    built = build_stage()
    (work/'preope_usa.bin').write_bytes(built)
    from ppfcheck import check
    for disc, base in enumerate(INTEGRAL_IMAGES):
        image = Disc(Path(GAME)/'windata/dlc/dlc_japan.bin', base)
        try:
            blob = relocation(image, 'preope', built, 0,
                              'MGS Integral: English Previous Operations')
        finally:
            image.f.close()
        name = 'INTEGRAL_disc%d_en_preope.ppf' % (disc+1)
        path = work/name
        path.write_bytes(blob)
        assert not check(path)[0]
        if args.deploy:
            (Path(GAME)/'mods/INTEGRAL/INTEGRAL'/str(disc)/name).write_bytes(blob)
        print(path)


if __name__ == '__main__':
    main()
