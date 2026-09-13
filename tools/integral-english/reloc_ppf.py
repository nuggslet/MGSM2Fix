#!/usr/bin/env python
"""Emit a PPF that parks a rebuilt stage in DUMMY3M.DAT and repoints its
STAGE.DIR entry.  DUMMY3M is 27 MB of zero padding that the executable's
file-name table does not mention, so nothing the game reads is overwritten.

    py reloc_ppf.py <stage> <stage.bin> <dummy3m sector> <disc.bin> <out.ppf>

STAGE_TABLE.offset is relative to STAGE.DIR's own LBA (get_stage_pos returns
dir->offset + fs_table_header.pos), so the value written is position independent.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
import struct, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from iso import Disc

stage_name, stage_path, slot, disc_p, out_p = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4], sys.argv[5]

desc = ('MGS Integral: English %s' % stage_name).encode('latin1')
# PPF3's description field is exactly 50 bytes, and `ljust` only pads - it never
# truncates. A longer string shifts every record offset, and the loader then
# writes at garbage addresses until the game dies. Cost 306 MB of log to find.
assert len(desc) <= 50, 'PPF3 description field is 50 bytes'
stage = open(stage_path, 'rb').read()
disc = Disc(disc_p)
files = {n.upper(): (l, s) for n, l, s, d in disc.walk() if not d}
sd_lba = files['/MGS/STAGE.DIR;1'][0]
du_lba, du_size = files['/DUMMY3M.DAT;1']
need = len(stage) // 2048
assert need * 2048 == len(stage)
assert slot + need <= du_size // 2048, 'stage does not fit in DUMMY3M.DAT'
offset = (du_lba + slot) - sd_lba
print('%s: %s -> DUMMY3M sector %d (%d sectors), STAGE.DIR entry offset %d'
      % (os.path.basename(disc_p), stage_name, slot, need, offset))

f = open(disc_p, 'rb')
def img(lba, within): return lba * 2352 + disc.hdr + within
writes = []
for s in range(need):
    page = stage[s*2048:(s+1)*2048]
    f.seek(img(du_lba + slot + s, 0))
    assert f.read(2048) == bytes(2048), 'DUMMY3M sector %d is not blank' % (slot + s)
    lo, hi = 0, 2048
    while lo < hi and not page[lo]: lo += 1
    while hi > lo and not page[hi-1]: hi -= 1
    if lo < hi: writes.append((img(du_lba + slot + s, lo), page[lo:hi]))

sd = open(WORK + '/int1_stage.dir', 'rb').read()
hsz = struct.unpack('<I', sd[:4])[0]
ent = None
for p in range(4, hsz + 12, 12):
    if sd[p:p+8].rstrip(b'\x00') == stage_name.encode(): ent = p; break
assert ent is not None, 'stage %r not in STAGE.DIR' % stage_name
old = struct.unpack('<I', sd[ent+8:ent+12])[0]
f.seek(img(sd_lba + ent // 2048, ent % 2048 + 8))
assert f.read(4) == struct.pack('<I', old), 'STAGE.DIR entry does not match retail'
print('   entry at file offset %d: sector %d -> %d' % (ent, old, offset))
writes.append((img(sd_lba + ent // 2048, ent % 2048 + 8), struct.pack('<I', offset)))

out = bytearray(b'PPF30' + bytes([2])
                + desc.ljust(50, b'\x00') + bytes(4))
n = 0
for off, data in writes:
    for k in range(0, len(data), 255):
        c = data[k:k+255]
        out += struct.pack('<Q', off + k) + bytes([len(c)]) + c
        n += 1
os.makedirs(os.path.dirname(out_p) or '.', exist_ok=True)
open(out_p, 'wb').write(bytes(out))
print('   -> %s  (%d records, %d bytes)' % (out_p, n, len(out)))
