"""Recompute the EDC/ECC tail of every sector a raw-disc patch set touches.

A PPF for the Master Collection only ever writes the 2048-byte payload of a
sector, because the collection's emulator reads the payload and never looks at
the error-correction tail behind it. A patch meant for a **real PlayStation disc
image** is not finished there: changing a payload byte invalidates that sector's
EDC and its P/Q parity, and the drive is entitled to act on that. So
`rebuild.py --variant raw` runs this pass and ships the corrected tails as one
more PPF per disc.

The whole pass rests on one invariant, checked per sector and not assumed:

    before applying anything, the sector as we believe it exists on the retail
    disc must verify against its own stored parity.

That is a 280-byte checksum over 2048 bytes, so it cannot pass by luck. It
catches the two ways this could go quietly wrong - a sector whose true content
we do not actually have, and an image that is not the disc we think it is.

It matters because the collection's images are not faithful everywhere. Every
executable's ISO extent is **zero-filled** (313 of 313 sectors on Integral disc
1, and so on) while keeping the parity of the sector it used to be. The retail
executables the build already requires are put back before the check, which is
exactly why the check is worth making: `int1.exe` reproduces all 313 stored
tails, so it is provably the file the disc was pressed with. DUMMY3M is
zero-filled too, but there it is honest - its stored parity is the parity OF a
blank payload - so relocated stages need no substitute and pass the same check.

The tails are absolute writes and do not depend on patch order, but they do
depend on every other patch in the set being applied: they are computed from
the final payload of the whole set. A raw disc therefore takes all of its PPFs
or none of them.
"""
import cdecc
from portio import read_ppf

SECTOR = cdecc.SECTOR


def writes_by_sector(paths):
    """{sector: {offset within payload: byte}} over every record in every PPF"""
    out = {}
    for path in paths:
        for offset, payload in read_ppf(path):
            sector, within = divmod(offset, SECTOR)
            assert cdecc.DATA <= within, (path, hex(offset))
            assert within + len(payload) <= cdecc.DATA_END, \
                'record crosses the payload boundary: %s at 0x%X' % (path, offset)
            out.setdefault(sector, {}).update(
                {within - cdecc.DATA + k: b for k, b in enumerate(payload)})
    return out


class Substitutes:
    """True payload for extents the collection hollowed out - the executables.

    Add one per extent with the file's first LBA and its real bytes; `payload`
    then answers for any sector inside it.
    """

    def __init__(self):
        self.spans = []

    def add(self, lba, data, name=''):
        self.spans.append((lba, lba + (len(data) + 2047) // 2048, data, name))

    def payload(self, sector):
        for start, end, data, _ in self.spans:
            if start <= sector < end:
                at = (sector - start) * 2048
                return data[at:at+2048].ljust(2048, b'\0')
        return None

    def name(self, sector):
        for start, end, _, name in self.spans:
            if start <= sector < end:
                return name
        return None


def tails(image_path, base, paths, substitutes=None, report=None):
    """PPF records carrying the corrected EDC/ECC of every touched sector.

    `image_path`/`base` locate the disc image the offsets address; `paths` are
    the PPFs of the complete set for that disc.
    """
    substitutes = substitutes or Substitutes()
    changed = writes_by_sector(paths)
    records, substituted, blank = [], 0, 0

    with open(image_path, 'rb') as handle:
        for sector in sorted(changed):
            handle.seek(base + sector * SECTOR)
            raw = bytearray(handle.read(SECTOR))
            assert len(raw) == SECTOR, 'sector %d is past the end of the image' % sector
            assert raw[:12] == b'\x00' + b'\xff'*10 + b'\x00', \
                'sector %d is not a raw 2352-byte sector' % sector
            assert raw[15] == 2 and cdecc.form(raw) == 1, \
                'sector %d is not Mode 2 Form 1; a patch must not land here' % sector

            replacement = substitutes.payload(sector)
            if replacement is not None:
                raw[cdecc.DATA:cdecc.DATA_END] = replacement
                substituted += 1
            elif raw[cdecc.DATA:cdecc.DATA_END] == bytes(2048):
                blank += 1

            # The invariant. If this fails, the bytes we think are on the retail
            # disc at this sector are not the bytes that produced its parity, so
            # any tail computed here would be fiction.
            if not cdecc.verify(bytes(raw)):
                where = substitutes.name(sector) or 'no substitute'
                raise AssertionError(
                    'sector %d does not verify against its own stored parity before'
                    ' patching (%s). Its true payload is unknown, so its EDC/ECC'
                    ' cannot be recomputed.' % (sector, where))

            for within, byte in changed[sector].items():
                raw[cdecc.DATA + within] = byte

            tail = cdecc.tail(bytes(raw))
            at = sector * SECTOR + cdecc.TAIL
            for p in range(0, len(tail), 255):
                records.append((at + p, tail[p:p+255]))

    if report is not None:
        report.update(sectors=len(changed), substituted_sectors=substituted,
                      blank_sectors=blank, tail_bytes=len(changed) * cdecc.TAIL_LEN)
    return records


def all_writes(paths):
    """{sector: {offset within the sector: byte}} - tails included this time"""
    out = {}
    for path in paths:
        for offset, payload in read_ppf(path):
            sector, within = divmod(offset, SECTOR)
            out.setdefault(sector, {}).update(
                {within + k: b for k, b in enumerate(payload)})
    return out


def verify_set(image_path, base, paths, substitutes=None):
    """Apply a finished patch set in memory and check the result.

    The end-to-end proof that a raw build is sound: every sector any patch
    touched must verify against the parity the set itself wrote. It reads the
    image; it never writes one. Returns (checked, failures).
    """
    substitutes = substitutes or Substitutes()
    changed = all_writes(paths)
    failures = []
    with open(image_path, 'rb') as handle:
        for sector, writes in sorted(changed.items()):
            handle.seek(base + sector * SECTOR)
            raw = bytearray(handle.read(SECTOR))
            replacement = substitutes.payload(sector)
            if replacement is not None:
                raw[cdecc.DATA:cdecc.DATA_END] = replacement
            for within, byte in writes.items():
                raw[within] = byte
            if not cdecc.verify(bytes(raw)):
                failures.append(sector)
    return len(changed), failures


def _main(argv):
    import argparse
    import glob
    from iso import Disc
    from workdir import GAME, WORK

    parser = argparse.ArgumentParser(description=verify_set.__doc__)
    parser.add_argument('package', help="a rebuild.py --variant raw output's package/ directory")
    parser.add_argument('--game', default=GAME)
    parser.add_argument('--work', default=WORK)
    args = parser.parse_args(argv)

    container = args.game + '/windata/dlc/dlc_japan.bin'
    sets = [('Integral disc 1', 'mods/INTEGRAL/INTEGRAL/0', 0, 'int1.exe', '/MGS/SLPM_862.47;1'),
            ('Integral disc 2', 'mods/INTEGRAL/INTEGRAL/1', 0x2AE54800, 'int2.exe', '/MGS/SLPM_862.48;1'),
            ('Integral VR',     'mods/INTEGRAL/VR-DISK',    0x57592000, 'vrint.exe', '/MGS/SLPM_862.49;1')]
    bad = 0
    for label, folder, base, exe, iso_path in sets:
        paths = sorted(glob.glob(args.package.rstrip('/\\') + '/' + folder + '/*.ppf'))
        if not paths:
            print('%-16s no PPFs at %s' % (label, folder))
            bad += 1
            continue
        disc = Disc(container, base)
        try:
            lba, _ = next((l, s) for n, l, s, d in disc.walk()
                          if not d and n.upper() == iso_path)
        finally:
            disc.f.close()
        substitutes = Substitutes()
        substitutes.add(lba, open(args.work + '/' + exe, 'rb').read(), exe)
        checked, failures = verify_set(container, base, paths, substitutes)
        print('%-16s %2d PPF(s), %5d touched sector(s): %s'
              % (label, len(paths), checked,
                 'all verify' if not failures else
                 '%d FAIL (first %s)' % (len(failures), failures[:4])))
        bad += len(failures)
    print('RAW SET', 'VERIFIED' if not bad else 'FAILED')
    return 0 if not bad else 1


if __name__ == '__main__':
    import sys
    sys.exit(_main(sys.argv[1:]))
