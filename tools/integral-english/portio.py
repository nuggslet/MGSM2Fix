"""Shared, read-only disc access and deterministic PPF/stage serialization."""
from pathlib import Path
import hashlib
import struct

SECTOR = 2048
RAW = 2352
HEADER = 24
INTEGRAL_IMAGES = (0, 0x2AE54800)
USA_IMAGES = (0xF12F8000, 0x11B3E5800)
USA_VR_IMAGE = 0xD39B7000


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def pad(n):
    return (n + 2047) // 2048 * 2048


def entries(data):
    end = struct.unpack_from('<I', data)[0] + 12
    return {data[p:p+8].rstrip(b'\0').decode('ascii'):
            (struct.unpack_from('<I', data, p+8)[0], p+8)
            for p in range(4, end, 12) if data[p:p+8].rstrip(b'\0')}


def stage(data, name=None):
    base = entries(data)[name][0] * 2048 if name is not None else 0
    count = struct.unpack_from('<h', data, base+2)[0]
    tags, p = [], base+4
    while data[p+2]:
        tags.append(list(struct.unpack_from('<HBBi', data, p)))
        p += 8
    payloads, offsets, off = {}, {}, 2048
    for k, (_, mode, ext, size) in enumerate(tags):
        if mode == ord('c') and ext != 0xFF:
            continue
        assert size >= 0 and off + size <= count * 2048
        payloads[k] = data[base+off:base+off+size]
        offsets[k] = off
        off += pad(size)
    assert off == count * 2048, (name, off, count)
    return tags, payloads, offsets


def pack_stage(tags, payloads):
    tags = [t.copy() for t in tags]
    count = (2048 + sum(pad(len(v)) for v in payloads.values())) // 2048
    out = bytearray(2048)
    struct.pack_into('<BBh', out, 0, 1, 0, count)
    for k, t in enumerate(tags):
        if k in payloads:
            t[3] = len(payloads[k])
        struct.pack_into('<HBBi', out, 4+8*k, *t)
    for payload in payloads.values():
        out += payload + bytes(pad(len(payload))-len(payload))
    assert len(out) == count * 2048
    return bytes(out)


def records(data, start=0x1B8):
    result, p = [], start
    while p+2 < len(data) and data[p] == 7:
        length = data[p+1]
        assert length and p+2+length <= len(data) and data[p+1+length] == 0
        result.append(data[p+2:p+2+length])
        p += 2+length
    return result, p


def encode_records(items):
    assert all(0 < len(s) <= 255 and s[-1] == 0 for s in items)
    return b''.join(bytes((7, len(s))) + s for s in items)


def changed_runs(original, modified):
    assert len(original) == len(modified)
    p = 0
    while p < len(original):
        if original[p] == modified[p]:
            p += 1
            continue
        start = p
        while p < len(original) and original[p] != modified[p]:
            p += 1
        yield start, modified[start:p]


def image_offset(lba, offset):
    return (lba + offset // 2048) * 2352 + 24 + offset % 2048


def map_runs(lba, runs):
    for start, data in runs:
        p = 0
        while p < len(data):
            size = min(255, len(data)-p, 2048-(start+p) % 2048)
            yield image_offset(lba, start+p), data[p:p+size]
            p += size


BLOCKCHECK_AT = 0x9320
BLOCKCHECK_LEN = 1024


def blockcheck_of(path, base=0):
    """The 1024 bytes at 0x9320 of a disc image, which is what a PPF3's block
    check holds: an applier compares them and refuses a patch aimed at a
    different release. Ketchup skips the field, so this only matters for a raw
    disc image, where an ordinary PPF tool does the applying."""
    with open(path, 'rb') as handle:
        handle.seek(base + BLOCKCHECK_AT)
        block = handle.read(BLOCKCHECK_LEN)
    assert len(block) == BLOCKCHECK_LEN, path
    return block


def ppf(records, description, blockcheck=None):
    desc = description.encode('ascii')
    assert len(desc) <= 50
    out = bytearray(b'PPF30\x02' + desc.ljust(50, b'\0') + bytes(4))
    if blockcheck is not None:
        assert len(blockcheck) == BLOCKCHECK_LEN, len(blockcheck)
        out[57] = 1
        out += blockcheck
    for offset, data in records:
        for p in range(0, len(data), 255):
            chunk = data[p:p+255]
            assert chunk
            out += struct.pack('<QB', offset+p, len(chunk)) + chunk
    return bytes(out)


def add_blockcheck(data, block):
    """Put a block check into an already-built PPF without touching a record."""
    assert data[:6] == b'PPF30\x02' and len(block) == BLOCKCHECK_LEN
    assert not data[57], 'this PPF already carries a block check'
    return bytes(data[:57]) + b'\x01' + bytes(data[58:60]) + block + bytes(data[60:])


def read_ppf(path):
    data = Path(path).read_bytes()
    assert len(data) >= 60 and data[:6] == b'PPF30\x02'
    assert data[56] == 0 and data[58] == 0 and data[59] == 0, 'image type or undo data'
    # Byte 57 says a 1024-byte block check sits between the header and the first
    # record. Ketchup skips it the same way (ApplyPPF3 starts at 1084).
    p = 60
    if data[57]:
        p = 60 + BLOCKCHECK_LEN
        assert len(data) >= p, path
    result = []
    while p < len(data):
        offset, length = struct.unpack_from('<QB', data, p)
        p += 9
        assert length and p+length <= len(data)
        result.append((offset, data[p:p+length]))
        p += length
    return result


def patched_file(data, lba, patches):
    """Apply only records inside this ISO file; reject sector-tail writes."""
    out = bytearray(data)
    for path in patches:
        for offset, payload in read_ppf(path):
            sector, within = divmod(offset, 2352)
            relative = (sector-lba) * 2048 + within-24
            if sector < lba or sector >= lba + (len(data)+2047)//2048:
                continue
            assert 24 <= within and within+len(payload) <= 24+2048
            assert 0 <= relative <= len(data)-len(payload)
            out[relative:relative+len(payload)] = payload
    return bytes(out)


def relocation(disc, name, stage_data, slot, description):
    files = {n.upper(): (l, s) for n, l, s, d in disc.walk() if not d}
    sd_lba, sd_size = files['/MGS/STAGE.DIR;1']
    du_lba, du_size = files['/DUMMY3M.DAT;1']
    source = disc.read(sd_lba, sd_size)
    _, entry = entries(source)[name]
    assert len(stage_data) % 2048 == 0 and slot*2048+len(stage_data) <= du_size
    assert disc.read(du_lba+slot, len(stage_data)) == bytes(len(stage_data))
    writes = []
    for p in range(0, len(stage_data), 2048):
        page = stage_data[p:p+2048]
        start = len(page)-len(page.lstrip(b'\0'))
        end = len(page.rstrip(b'\0'))
        if start < end:
            writes.extend(map_runs(du_lba+slot, [(p+start, page[start:end])]))
    writes.append((image_offset(sd_lba, entry), struct.pack('<I', du_lba+slot-sd_lba)))
    return ppf(writes, description)
