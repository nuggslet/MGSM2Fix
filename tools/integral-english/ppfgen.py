#!/usr/bin/env python
"""Emit a PPF3 for a modified STAGE.DIR against a retail Integral disc image.

usage: ppfgen.py <base.dir> <new.dir> <disc.bin> <out.ppf> "<description>"
The base image must hold whatever is already on the disc at the changed offsets;
every run is verified against the real image before a record is written.
"""
import struct, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from iso import Disc

base_p, new_p, disc_p, out_p, desc = sys.argv[1:6]
orig = open(base_p, 'rb').read()
new  = open(new_p, 'rb').read()
assert len(orig) == len(new), 'STAGE.DIR size changed'

disc = Disc(disc_p)
lba = None
for name, l, size, isdir in disc.walk():
    if name.upper().endswith('STAGE.DIR;1'):
        lba, fsize = l, size; break
assert lba is not None, 'STAGE.DIR not found in %s' % disc_p

def image_off(fo):
    return (lba + fo // 2048) * 2352 + disc.hdr + (fo % 2048)

runs, i = [], 0
while i < len(orig):
    if new[i] != orig[i]:
        j = i
        while j < len(orig) and new[j] != orig[j]: j += 1
        runs.append((i, new[i:j])); i = j
    else: i += 1

# split at 2048-byte page boundaries: a PPF record may not span sectors
recs = []
for fo, data in runs:
    p = 0
    while p < len(data):
        end = ((fo + p) // 2048 + 1) * 2048
        n = min(len(data) - p, end - (fo + p))
        recs.append((fo + p, data[p:p+n])); p += n

f = open(disc_p, 'rb'); bad = 0
for fo, data in recs:
    f.seek(image_off(fo))
    if f.read(len(data)) != orig[fo:fo+len(data)]: bad += 1
print('%s: STAGE.DIR lba=%d  %d runs / %d records / %d bytes  verify=%s'
      % (os.path.basename(disc_p), lba, len(runs), len(recs),
         sum(len(r[1]) for r in recs), 'ALL MATCH' if bad == 0 else '%d MISMATCH' % bad))
if bad: sys.exit(1)

out = bytearray(b'PPF30' + bytes([2]) + desc.encode()[:50].ljust(50, b'\x00') + bytes([0,0,0,0]))
assert len(out) == 60
n = 0
for fo, data in recs:
    for k in range(0, len(data), 255):
        c = data[k:k+255]
        out += struct.pack('<Q', image_off(fo) + k) + bytes([len(c)]) + c
        n += 1
os.makedirs(os.path.dirname(out_p) or '.', exist_ok=True)
open(out_p, 'wb').write(bytes(out))
print('   -> %s  (%d PPF records, %d bytes)' % (out_p, n, len(out)))
