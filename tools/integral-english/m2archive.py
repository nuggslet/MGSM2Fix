"""Read the collection's own data archive, and its CD-ROM patches.

Until 2026-09-07 the collection's "named file" patches were a hole in the
record. Its patch table has two kinds of entry: an *offset* patch, which passes
its bytes inline and which `SQHook::SetPatchWatch` can therefore log, and a
*named file* patch, which passes only a file name - the game reads the bytes
from its own archive later, so the watch reports it as "0 bytes" and its content
stayed invisible. Five of those land on the port's own text, including the one
that makes `en_menu3` raw-disc only, and every document here said the same
thing: what they contain is not known.

This reads them.

    py m2archive.py --list 099/patch          # what patches exist for Integral
    py m2archive.py --roms                    # the disc images and their bases
    py m2archive.py --extract 099/patch/disc1_1822B55D_patch_PS5.bin.m --out x.bin
    py m2archive.py --patches                 # every patch that touches Integral
                                              #   disc 1, decoded against retail

HOW THE ARCHIVE IS PUT TOGETHER

`windata/alldata.bin` (and `windata/dlc/dlc_japan.bin`) is a flat blob; the
index beside it, `alldata.psb.m`, is an **MDF**: the four bytes `mdf\\0`, a u32
inflated size, then a zlib stream that has been XOR-obfuscated. The key is
derived from the file's own name:

    md5("25G/xpvTbsb+6" + basename.lower())        -> 16 bytes
    four little-endian u32 -> MT19937 init_by_array
    the first 16 outputs, little-endian            -> a 64-byte keystream
    body[i] ^= keystream[i % 64]

The literal `25G/xpvTbsb+6` sits in `METAL GEAR SOLID.exe` at file offset
0x75ECE0, immediately before the names `patchdata.psb.m`, `alldata.psb.m` and
`alldata.bin`. **Lowercasing the name matters** and is easy to miss, because
every `.psb.m` name is already lowercase - it only shows up on entries like
`disc1_1822B55D_patch_PS5.bin.m`, which must be hashed as
`disc1_1822b55d_patch_ps5.bin.m`.

The inflated index is a **PSB v3**: a compressed name trie plus a value tree,
whose `file_info` maps 4,926 paths to `[offset, size]` in the blob. Entries
under `099/patch/` are themselves MDFs wrapping a *stored* (uncompressed)
deflate block, so a patch payload is plain bytes behind one layer of
obfuscation - and its adler32 checks, which is what makes an extraction certain
rather than plausible.

WHAT THIS ESTABLISHED

  * The index names the disc images and their offsets, and every one of the six
    image bases this port found the hard way, by scanning for volume
    descriptors, is confirmed exactly (`--roms`).
  * The four disc-swap patches do not translate anything. They **blank the line
    that tells the player to open the disc tray** and reword the prompt - the
    same kind of change as dropping the brightness screen's O-button line, and
    for the same reason: there is no tray. See `--patches` and the README.
  * The platform suffixes (`_NX`, `_PS`, `_PS5`, `_XBOX`, `_STEAM`, `_PS_SWAP`)
    differ only in button-glyph codes inside the same strings.

Nothing here writes to the game. No extracted game data belongs in this
repository.
"""
import argparse
import hashlib
import os
import random
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from workdir import GAME

KEY = '25G/xpvTbsb+6'
KEYSTREAM_LEN = 64


# ----------------------------------------------------------------- MDF

def keystream(name):
    """The 64-byte XOR keystream for one archive member, from its own name."""
    digest = hashlib.md5((KEY + name.lower()).encode('ascii')).digest()
    # CPython seeds Mersenne Twister with init_by_array over the integer's
    # 32-bit words, which is exactly what the game does with the four words of
    # the MD5. Nothing here depends on Python's random behaviour beyond that.
    engine = random.Random(int.from_bytes(digest, 'little'))
    return b''.join(engine.getrandbits(32).to_bytes(4, 'little')
                    for _ in range(KEYSTREAM_LEN // 4))


def unmdf(blob, name):
    """`mdf\\0` + u32 size + obfuscated zlib  ->  the inflated bytes."""
    assert blob[:4] == b'mdf\0', 'not an MDF: %r' % blob[:4]
    want = struct.unpack_from('<I', blob, 4)[0]
    key = keystream(name)
    body = bytes(b ^ key[i % KEYSTREAM_LEN] for i, b in enumerate(blob[8:]))
    out = zlib.decompress(body)          # raises on a bad adler32
    assert len(out) == want, (len(out), want)
    return out


# ----------------------------------------------------------------- PSB

def _array(d, p):
    """A PSB integer array: a type byte giving the count's width, the count,
    a type byte giving each entry's width, then the entries."""
    n = d[p] - 12
    p += 1
    count = int.from_bytes(d[p:p+n], 'little')
    p += n
    m = d[p] - 12
    p += 1
    return ([int.from_bytes(d[p+m*i:p+m*i+m], 'little') for i in range(count)],
            p + m * count)


class Psb:
    """Enough of PSB v3 to read an archive index: names, strings, value tree."""

    def __init__(self, data):
        assert data[:4] == b'PSB\0', data[:4]
        self.d = data
        self.version = struct.unpack_from('<H', data, 4)[0]
        head = struct.unpack_from('<8I', data, 8)
        self.off_names, self.off_strings, self.off_strdata = head[1], head[2], head[3]
        self.off_entries = head[7]
        p = self.off_names
        self.n_offsets, p = _array(data, p)
        self.n_tree, p = _array(data, p)
        self.n_start, p = _array(data, p)
        assert p == self.off_entries, (hex(p), hex(self.off_entries))
        self.s_offsets, _ = _array(data, self.off_strings)

    def name(self, index):
        """Walk the compressed trie backwards from a leaf to the root."""
        node = self.n_tree[self.n_start[index]]
        out = bytearray()
        while node:
            parent = self.n_tree[node]
            out.append(node - self.n_offsets[parent])
            node = parent
        return bytes(reversed(out)).decode('utf-8', 'replace')

    def string(self, index):
        at = self.off_strdata + self.s_offsets[index]
        return self.d[at:self.d.index(b'\0', at)].decode('utf-8', 'replace')

    def value(self, p):
        d = self.d
        t = d[p]
        p += 1
        if t <= 1:
            return None, p
        if t == 2:
            return False, p
        if t == 3:
            return True, p
        if 4 <= t <= 12:
            n = t - 4
            return int.from_bytes(d[p:p+n], 'little'), p + n
        if 13 <= t <= 20:
            return _array(d, p - 1)
        if 21 <= t <= 24:
            n = t - 20
            return self.string(int.from_bytes(d[p:p+n], 'little')), p + n
        if 25 <= t <= 28:
            n = t - 24
            return '<resource %d>' % int.from_bytes(d[p:p+n], 'little'), p + n
        if t == 29:
            return 0.0, p
        if t == 30:
            return struct.unpack_from('<f', d, p)[0], p + 4
        if t == 31:
            return struct.unpack_from('<d', d, p)[0], p + 8
        if t == 32:
            offsets, q = _array(d, p)
            return [self.value(q + o)[0] for o in offsets], q
        if t == 33:
            names, q = _array(d, p)
            offsets, q = _array(d, q)
            return {self.name(n): self.value(q + o)[0]
                    for n, o in zip(names, offsets)}, q
        raise ValueError('unknown PSB type %d at 0x%X' % (t, p - 1))

    def root(self):
        return self.value(self.off_entries)[0]


class Archive:
    """One `<name>.bin` blob and the `<name>.psb.m` index beside it."""

    def __init__(self, blob_path):
        self.path = blob_path
        index_path = blob_path[:-4] + '.psb.m'
        raw = open(index_path, 'rb').read()
        self.index = Psb(unmdf(raw, os.path.basename(index_path)))
        self.files = self.index.root()['file_info']

    def read(self, name):
        """The stored bytes of one member, still MDF-wrapped."""
        offset, size = self.files[name]
        with open(self.path, 'rb') as handle:
            handle.seek(offset)
            return handle.read(size)

    def extract(self, name):
        """The member's real contents."""
        return unmdf(self.read(name), os.path.basename(name))


def archives():
    return [os.path.join(GAME, 'windata/alldata.bin'),
            os.path.join(GAME, 'windata/dlc/dlc_japan.bin'),
            os.path.join(GAME, 'windata/dlc/dlc_europe.bin')]


# ----------------------------------------------------------------- reporting

def show_patches(prefix='099/patch/disc1_'):
    """Decode every Integral disc-1 patch and say what it changes.

    A patch's name carries its target: `disc1_1822B55D_patch_PS5` writes at
    image offset 0x1822B55D of Integral disc 1. Comparing its payload against
    the retail bytes there is what turns a blob into a finding.
    """
    from audit_text import game_text
    from portio import records
    archive = Archive(os.path.join(GAME, 'windata/alldata.bin'))
    image = os.path.join(GAME, 'windata/dlc/dlc_japan.bin')     # Integral disc 1 at base 0
    names = sorted(n for n in archive.files if n.startswith(prefix))
    print('%d patch member(s) under %s\n' % (len(names), prefix))
    with open(image, 'rb') as disc:
        for name in names:
            stem = os.path.basename(name).split('_patch')[0]
            try:
                offset = int(stem.split('_')[1], 16)
            except (IndexError, ValueError):
                continue
            payload = archive.extract(name)
            disc.seek(offset)
            retail = disc.read(len(payload))
            differ = sum(1 for a, b in zip(retail, payload) if a != b)
            print('%-46s image 0x%08X  %4d bytes  %3d differ'
                  % (os.path.basename(name), offset, len(payload), differ))
            if not differ:
                print('     (writes exactly what is already there)')
                continue
            try:
                theirs, _ = records(payload, 0)
                mine, _ = records(retail, 0)
            except AssertionError:
                theirs = mine = []
            if not theirs:
                # Most of the 101 are not text blocks and do not start on an
                # 07 record header; say so rather than print nothing.
                print('     (not an 07-record block at offset 0 - %d of %d bytes differ)'
                      % (differ, len(payload)))
                continue
            for i, (a, b) in enumerate(zip(mine, theirs)):
                if a == b:
                    continue
                ta = (game_text(a[:-1])[0] or '<undecodable>')[:52]
                tb = (game_text(b[:-1])[0] or '<undecodable>')[:52]
                print('     rec%-2d retail %r' % (i, ta))
                print('           theirs %r' % (tb,))
            print()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--list', metavar='PREFIX', nargs='?', const='',
                        help='list members whose path starts with PREFIX')
    parser.add_argument('--roms', action='store_true',
                        help='the disc images and the offsets this port addresses')
    parser.add_argument('--extract', metavar='PATH')
    parser.add_argument('--out', metavar='FILE')
    parser.add_argument('--patches', action='store_true',
                        help='decode the Integral disc-1 CD-ROM patches against retail')
    args = parser.parse_args()

    if args.patches:
        show_patches()
        return 0

    if args.roms:
        for path in archives():
            if not os.path.exists(path):
                continue
            archive = Archive(path)
            print('== %s' % path)
            for name in sorted(archive.files):
                if '/roms/' in name:
                    offset, size = archive.files[name]
                    print('   %-46s 0x%09X  %12d bytes' % (name, offset, size))
        return 0

    if args.list is not None:
        for path in archives():
            if not os.path.exists(path):
                continue
            archive = Archive(path)
            hits = sorted(n for n in archive.files if n.startswith(args.list))
            print('== %s  (%d of %d members)' % (path, len(hits), len(archive.files)))
            for name in hits:
                offset, size = archive.files[name]
                print('   %-56s 0x%09X %9d' % (name, offset, size))
        return 0

    if args.extract:
        for path in archives():
            if not os.path.exists(path):
                continue
            archive = Archive(path)
            if args.extract in archive.files:
                data = archive.extract(args.extract)
                if args.out:
                    open(args.out, 'wb').write(data)
                    print('%d bytes -> %s' % (len(data), args.out))
                else:
                    print('%d bytes, first 64: %s' % (len(data), data[:64].hex()))
                return 0
        print('not found in any archive: %s' % args.extract)
        return 1

    parser.print_help()
    return 2


if __name__ == '__main__':
    sys.exit(main())
