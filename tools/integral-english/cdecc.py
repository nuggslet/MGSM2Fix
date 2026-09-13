"""EDC and ECC for raw CD-ROM Mode 2 Form 1 sectors.

Why this exists: a PPF for the collection only ever has to change the 2048-byte
payload of a sector, because the Master Collection's emulator reads the payload
and ignores the error-correction tail. A patch for a **real PlayStation disc
image** is not finished there. Changing a payload byte invalidates that
sector's EDC and its P/Q parity, and a strict emulator - or real hardware with a
marginal read - is entitled to notice. `rebuild.py --variant raw` therefore
recomputes the tail of every sector its records touch, and these are the sums.

Layout of a Mode 2 Form 1 sector (2352 bytes), which is what both games' discs
carry (`iso.Disc` detects the 24-byte header):

      0 ..   11   sync            00 FF FF FF FF FF FF FF FF FF FF 00
     12 ..   15   header          minute, second, frame (BCD), mode = 2
     16 ..   23   subheader       file, channel, submode, coding - twice
     24 .. 2071   user data       the 2048 bytes a PPF record addresses
   2072 .. 2075   EDC             over bytes 16..2071, little-endian
   2076 .. 2247   P parity        172 bytes
   2248 .. 2351   Q parity        104 bytes

Two details decide whether an implementation is right or merely plausible, and
both are settled by checking retail sectors rather than by reading a spec:

  * the EDC covers the SUBHEADER as well as the user data (16..2071), not the
    user data alone;
  * the ECC is computed with the four header bytes at 12..15 treated as ZERO.
    In Mode 2 the address is not protected, so a sector whose ECC was generated
    over its real header would fail on every disc ever pressed.

`py cdecc.py` runs the check that establishes both: it reads sectors straight
out of the collection's own images and compares computed against stored. That
same run is evidence about the images themselves - sectors that verify are real
raw sectors, not a re-wrapped payload, which is what the raw-disc variant
assumes when it computes offsets against them.

Algorithm after Neill Corlett's ECM (the reference implementation everyone
else's agrees with); the tables are built once at import.
"""

# --- tables -----------------------------------------------------------------
# ecc_f/ecc_b are the GF(2^8) multiply-by-x and its inverse lookup, with the
# 0x11D field polynomial; edc is a reflected CRC-32 with polynomial 0xD8018001.

_ECC_F = bytearray(256)
_ECC_B = bytearray(256)
_EDC_LUT = [0] * 256

for _i in range(256):
    _j = (_i << 1) ^ (0x11D if _i & 0x80 else 0)
    _ECC_F[_i] = _j & 0xFF
    _ECC_B[_i ^ (_j & 0xFF)] = _i
    _edc = _i
    for _ in range(8):
        _edc = (_edc >> 1) ^ (0xD8018001 if _edc & 1 else 0)
    _EDC_LUT[_i] = _edc

SECTOR = 2352
DATA = 24           # first byte of the 2048-byte payload
DATA_END = 2072     # one past it
EDC_AT = 2072
P_AT = 2076
Q_AT = 2248
TAIL = EDC_AT       # everything from here to the end of the sector is derived
TAIL_LEN = SECTOR - EDC_AT      # 280: EDC 4 + P 172 + Q 104


def edc(data, seed=0):
    """the CD EDC of `data` - a reflected CRC-32, polynomial 0xD8018001"""
    value = seed
    lut = _EDC_LUT
    for byte in data:
        value = (value >> 8) ^ lut[(value ^ byte) & 0xFF]
    return value


def _index_table(major_count, minor_count, major_mult, minor_inc, src_at):
    """Which bytes each parity symbol is made of.

    The pattern depends only on the four constants, never on the sector, so it
    is worked out once instead of per sector - which matters because a raw-disc
    build recomputes thousands of them. The Q pass reads further than the P
    pass: its window is 52 * 43 = 2236 bytes from byte 12, running to 2248, so
    it covers the P parity the previous pass just wrote. That is what makes it
    a product code - Q protects the data and P together - and it is why P has
    to be computed first."""
    size = major_count * minor_count
    table = []
    for major in range(major_count):
        index = (major >> 1) * major_mult + (major & 1)
        row = []
        for _ in range(minor_count):
            row.append(src_at + index)
            index += minor_inc
            if index >= size:
                index -= size
        table.append(tuple(row))
    return tuple(table)


_P_TABLE = _index_table(86, 24, 2, 86, 12)
_Q_TABLE = _index_table(52, 43, 86, 88, 12)


def _ecc_block(src, table, dest, at):
    f, b = _ECC_F, _ECC_B
    count = len(table)
    for major, row in enumerate(table):
        ecc_a = ecc_b = 0
        for where in row:
            temp = src[where]
            ecc_a ^= temp
            ecc_b ^= temp
            ecc_a = f[ecc_a]
        ecc_a = b[f[ecc_a] ^ ecc_b]
        dest[at + major] = ecc_a
        dest[at + major + count] = ecc_a ^ ecc_b


def form(sector):
    """1 or 2, from bit 5 of the subheader's submode byte.

    Both forms are "Mode 2" and only the subheader tells them apart. A Form 2
    sector carries 2324 bytes of user data, no parity at all, and an EDC at
    2348 that is allowed to be zero - it is what streamed audio and video ride
    in. Handing one to `tail()` would compute 280 bytes of Form 1 parity over
    what is really user data and write them into the middle of it, so `tail()`
    refuses instead. Both games' images carry such sectors: the check below
    finds them in USA's, at the very first data LBA."""
    assert len(sector) == SECTOR, len(sector)
    return 2 if sector[18] & 0x20 else 1


def tail(sector):
    """The 280 derived bytes (EDC + P + Q) for a whole 2352-byte Mode 2 Form 1
    sector, computed from its subheader and payload. The sector's own tail is
    ignored, so this is what it SHOULD hold."""
    assert len(sector) == SECTOR, len(sector)
    assert sector[15] == 2, 'not a Mode 2 sector (mode %d)' % sector[15]
    assert form(sector) == 1, 'Form 2 sector: it has no parity to compute'
    out = bytearray(sector)

    value = edc(out[16:DATA_END])
    out[EDC_AT:EDC_AT+4] = value.to_bytes(4, 'little')

    # The address is not covered by the ECC in Mode 2: zero it, compute, and
    # put it back. Getting this wrong is the classic way to produce parity that
    # looks reasonable and matches no disc in existence.
    address = bytes(out[12:16])
    out[12:16] = b'\0\0\0\0'
    _ecc_block(out, _P_TABLE, out, P_AT)      # data      -> P
    _ecc_block(out, _Q_TABLE, out, Q_AT)      # data + P  -> Q
    out[12:16] = address

    return bytes(out[TAIL:])


def fixed(sector):
    """the sector with its EDC/ECC tail recomputed"""
    return sector[:TAIL] + tail(sector)


def verify(sector):
    """True when the sector's stored tail already equals the computed one"""
    return sector[TAIL:] == tail(sector)


# --- the check that makes the above trustworthy -----------------------------

def _selfcheck(argv):
    import os, sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from workdir import GAME

    images = [
        ('Integral disc 1', GAME + '/windata/dlc/dlc_japan.bin', 0),
        ('Integral disc 2', GAME + '/windata/dlc/dlc_japan.bin', 0x2AE54800),
        ('Integral VR',     GAME + '/windata/dlc/dlc_japan.bin', 0x57592000),
        ('USA disc 1',      GAME + '/windata/alldata.bin',       0xF12F8000),
        ('USA VR',          GAME + '/windata/alldata.bin',       0xD39B7000),
    ]
    # Spread across the whole image: the volume descriptor, the low data area,
    # and deep into the payload where the stages live.
    lbas = [16, 17, 23, 24, 100, 1000, 5000, 20000, 60000, 100000, 130000, 136654]

    bad = 0
    for name, path, base in images:
        if not os.path.exists(path):
            print('%-16s MISSING %s' % (name, path))
            bad += 1
            continue
        size = os.path.getsize(path)
        with open(path, 'rb') as f:
            checked = failed = form2 = hollow = 0
            modes = set()
            for lba in lbas:
                at = base + lba * SECTOR
                if at + SECTOR > size:
                    continue
                f.seek(at)
                raw = f.read(SECTOR)
                if raw[:12] != b'\x00' + b'\xff'*10 + b'\x00':
                    continue            # not a raw sector; nothing to check
                modes.add(raw[15])
                if raw[15] != 2:
                    continue
                if form(raw) == 2:
                    form2 += 1          # no parity to check; skipped on purpose
                    continue
                if raw[DATA:DATA_END] == bytes(2048) and not verify(raw):
                    # The collection zero-fills the ISO extents it never reads -
                    # every executable, and DUMMY3M, which is blank on the disc
                    # too. It leaves the sector's ORIGINAL parity in place, so a
                    # zeroed payload with a tail that does not match it is the
                    # signature of that, not a fault. It is also what lets the
                    # executable check below reconstruct the real sector.
                    hollow += 1
                    continue
                checked += 1
                if not verify(raw):
                    failed += 1
                    if failed == 1:
                        print('  %s lba %d: stored %s computed %s' % (
                            name, lba, raw[TAIL:TAIL+8].hex(), tail(raw)[:8].hex()))
            print('%-16s %3d Form 1 sector(s) verified, %d mismatch(es),'
                  ' %d Form 2 and %d zero-filled skipped, mode(s) %s'
                  % (name, checked, failed, form2, hollow, sorted(modes) or '-'))
            bad += failed
            if not checked:
                print('%-16s NOTHING CHECKED - the image is not raw 2352-byte sectors'
                      % name)
                bad += 1

    # The strong check, and the one that matters for the raw-disc variant.
    #
    # An executable's ISO extent is zero-filled by the collection but keeps the
    # parity of the sector it used to be. Put the separately supplied retail
    # executable back into those sectors and the parity has to come out equal to
    # what is stored - a 280-byte sum over 2048 bytes, 313 times. If it does,
    # three things are settled at once: these routines are right, that file IS
    # the executable the disc was pressed with, and the image around it carries
    # genuine retail sector metadata rather than something re-authored. That is
    # the assumption the raw-disc build rests on, measured instead of hoped.
    print()
    from iso import Disc
    exes = [
        ('Integral disc 1', GAME + '/windata/dlc/dlc_japan.bin', 0,
         '/MGS/SLPM_862.47;1', 'int1.exe'),
        ('Integral disc 2', GAME + '/windata/dlc/dlc_japan.bin', 0x2AE54800,
         '/MGS/SLPM_862.48;1', 'int2.exe'),
        ('Integral VR',     GAME + '/windata/dlc/dlc_japan.bin', 0x57592000,
         '/MGS/SLPM_862.49;1', 'vrint.exe'),
        ('USA disc 1',      GAME + '/windata/alldata.bin',       0xF12F8000,
         '/MGS/SLUS_005.94;1', 'us1.exe'),
    ]
    from workdir import WORK
    for name, path, base, iso_path, exe_name in exes:
        exe_path = os.path.join(WORK, exe_name)
        if not (os.path.exists(path) and os.path.exists(exe_path)):
            print('%-16s executable check skipped (missing input)' % name)
            continue
        disc = Disc(path, base)
        try:
            lba, size = next((l, s) for n, l, s, d in disc.walk()
                             if not d and n.upper() == iso_path)
        finally:
            disc.f.close()
        exe = open(exe_path, 'rb').read()
        if len(exe) != size:
            print('%-16s %s is %d bytes, the extent is %d - not the same file'
                  % (name, exe_name, len(exe), size))
            continue
        same = differ = 0
        with open(path, 'rb') as f:
            for i in range((size + 2047) // 2048):
                f.seek(base + (lba + i) * SECTOR)
                raw = bytearray(f.read(SECTOR))
                raw[DATA:DATA_END] = exe[i*2048:(i+1)*2048].ljust(2048, b'\0')
                if tail(bytes(raw)) == bytes(raw[TAIL:]):
                    same += 1
                else:
                    differ += 1
        verdict = ('reproduces the disc exactly' if not differ else
                   'DOES NOT match this image')
        print('%-16s %-10s %4d/%d sector tails reproduced - %s'
              % (name, exe_name, same, same + differ, verdict))
        # Integral is what the raw variant patches, so only Integral is fatal.
        # USA is a text donor whose executable is known not to match the
        # collection's SLUS image (see the README); it is reported, not failed.
        if differ and name.startswith('Integral'):
            bad += 1

    print()
    # A change to the payload must change the tail, or the whole exercise is
    # pointless: prove the sums actually depend on the data.
    with open(images[0][1], 'rb') as f:
        f.seek(16 * SECTOR)
        raw = bytearray(f.read(SECTOR))
    before = tail(bytes(raw))
    raw[DATA] ^= 0xFF
    after = tail(bytes(raw))
    print('payload-sensitive:', 'yes' if before != after else 'NO - BROKEN')
    if before == after:
        bad += 1

    print('SELF-CHECK', 'PASSED' if not bad else 'FAILED (%d)' % bad)
    return 0 if not bad else 1


if __name__ == '__main__':
    import sys
    sys.exit(_selfcheck(sys.argv[1:]))
