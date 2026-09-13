#!/usr/bin/env python
"""Parse a PPF3 the way Ketchup does, and refuse anything it would choke on.

    py ppfcheck.py <file.ppf> [more.ppf ...]
    py ppfcheck.py --deployed          # every PPF under the mods folder

Run this on anything before it goes near the game. A malformed header is
silent: the loader reads record offsets from the wrong place and writes 255-byte
blocks at garbage addresses until the game dies, filling the log at about a
gigabyte a minute. That happened once, on 2026-09-03, from a description field
of 60 bytes in a 50-byte slot - `ljust(50, b'\\x00')` pads but never truncates,
so every offset after it was 10 bytes out. The tools now assert their
description length; this checks the artifact itself.

Header is 60 bytes: b'PPF30', version byte, 50-byte description, then image
type, block check, undo and a spare byte. A block check adds 1024 bytes of the
original image before the first record, which Ketchup skips and this follows.
Records are u64 offset, u8 length, then that many bytes, to end of file.
"""
import glob, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from workdir import GAME

MODS = (GAME + '/mods') if GAME else None
# No disc image in the collection is anywhere near 1 GB, so an offset past this
# is not a plausible destination - it is a misparse.
OFFSET_CEILING = 0x40000000


def check(path):
    d = open(path, 'rb').read()
    problems = []
    if d[:5] != b'PPF30':
        return ['magic is %r, not PPF30' % d[:5]], 0, None, None
    desc = d[6:56].rstrip(b'\x00').decode('latin1', 'replace')
    if b'\x00' in d[6:56].rstrip(b'\x00'):
        problems.append('description contains an embedded NUL')

    if d[56]:
        problems.append('image type is %d, not 0 (BIN)' % d[56])
    if d[58]:
        problems.append('undo data is declared; Ketchup would misread the records')
    q = 60
    if d[57]:
        q = 60 + 1024
        if len(d) < q:
            return problems + ['block check declared but the file ends inside it'], 0, None, desc

    n, lo, hi = 0, None, 0
    while q + 9 <= len(d):
        off, ln = struct.unpack_from('<QB', d, q)
        if q + 9 + ln > len(d):
            problems.append('record %d at 0x%X claims %d bytes, past end of file' % (n, q, ln))
            break
        if off >= OFFSET_CEILING:
            problems.append('record %d writes at 0x%X, past any real disc image' % (n, off))
        if ln == 0:
            problems.append('record %d is zero length' % n)
        lo = off if lo is None else min(lo, off)
        hi = max(hi, off + ln)
        n += 1
        q += 9 + ln
    if q != len(d):
        problems.append('%d trailing bytes after the last record' % (len(d) - q))
    if n == 0:
        problems.append('no records')
    return problems, n, (lo, hi), desc


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    if args == ['--deployed']:
        if not MODS:
            raise SystemExit('Cannot find the Master Collection MGS1 directory; '
                              'run workdir.py to see what was searched, '
                              'or set INTEGRAL_ENGLISH_GAME.')
        paths = sorted(glob.glob(os.path.join(MODS, '**', '*.ppf'), recursive=True))
    else:
        paths = args
    if not paths:
        print('no PPFs found')
        return 2

    worst = 0
    for p in paths:
        problems, n, span, desc = check(p)
        name = os.path.relpath(p, MODS) if MODS and (p.startswith(MODS.replace('/', os.sep)) or \
            p.startswith(MODS)) else p
        if problems:
            worst = 1
            print('FAIL %s' % name)
            for m in problems[:6]:
                print('       %s' % m)
        else:
            print('ok   %-44s %5d records, 0x%09X..0x%09X  %r'
                  % (name, n, span[0], span[1], desc))
    print('\n%d file(s), %s' % (len(paths), 'all clean' if not worst else 'PROBLEMS ABOVE'))
    return worst


if __name__ == '__main__':
    sys.exit(main())
