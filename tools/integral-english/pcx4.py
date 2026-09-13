"""Encode/decode the 4-plane 1bpp RLE PCX the MGS1 texture loader expects
(source/libdg/loader.c, PcxInflate4; PCX_RLE_CODE = 0xC0)."""
import struct

RLE = 0xC0

def decode(blob):
    """-> (w, h, palette[16] RGB tuples, indices[h][w])"""
    maxx, maxy = struct.unpack('<HH', blob[8:12])
    minx, miny = struct.unpack('<HH', blob[4:8])
    w, h = maxx - minx + 1, maxy - miny + 1
    nplanes = blob[65]
    stride  = struct.unpack('<H', blob[66:68])[0]
    pal = [tuple(blob[16 + 3*i: 19 + 3*i]) for i in range(16)]
    p = 128
    rows = []
    for _ in range(h):
        # PcxInflate4 decodes stride * nplanes bytes as ONE run-length stream
        # per row; runs may cross plane boundaries.
        line = bytearray()
        want = stride * nplanes
        while len(line) < want:
            c = blob[p]; p += 1
            if c <= RLE: line.append(c)
            else:
                n = c - RLE; d = blob[p]; p += 1
                line.extend([d]*n)
        planes = [line[i*stride:(i+1)*stride] for i in range(nplanes)]
        row = []
        for x in range(w):
            byte, bit = x >> 3, 7 - (x & 7)
            v = 0
            for pl in range(nplanes):
                v |= ((planes[pl][byte] >> bit) & 1) << pl
            row.append(v)
        rows.append(row)
    return w, h, pal, rows

def _rle(data, maxrun=62):
    """RLE the stream. `code = RLE + run`, so a run may be up to 63 bytes and
    PcxInflate4/8 decode that; the original call site here used 62 and every
    shipped patch was built with it, so 62 stays the default and a caller that
    wants the last byte back passes maxrun=63 explicitly."""
    assert 1 <= maxrun <= 63, maxrun
    out = bytearray(); i = 0; n = len(data)
    while i < n:
        v = data[i]; run = 1
        while i + run < n and data[i+run] == v and run < maxrun: run += 1
        if run > 1 or v > RLE:
            out.append(RLE + run); out.append(v)
        else:
            out.append(v)
        i += run
    return out

def encode(template, w, h, pal, rows, maxrun=62):
    """Rebuild a texture using `template`'s 128-byte header, with new size/pixels.
    px/py/cx/cy in the PCXINFO block at offset 74 are preserved from template."""
    hdr = bytearray(template[:128])
    struct.pack_into('<HH', hdr, 4, 0, 0)          # min_x, min_y
    struct.pack_into('<HH', hdr, 8, w-1, h-1)      # max_x, max_y
    nplanes = 4
    stride = (w + 7)//8
    if stride & 1: stride += 1                     # PCX rows are word aligned
    hdr[65] = nplanes
    struct.pack_into('<H', hdr, 66, stride)
    for i, c in enumerate(pal[:16]):
        hdr[16+3*i:19+3*i] = bytes(c)
    body = bytearray()
    for y in range(h):
        planes = [bytearray(stride) for _ in range(nplanes)]
        for x in range(w):
            v = rows[y][x]
            byte, bit = x >> 3, 7 - (x & 7)
            for pl in range(nplanes):
                if (v >> pl) & 1: planes[pl][byte] |= 1 << bit
        body += _rle(b''.join(bytes(p) for p in planes), maxrun)
    return bytes(hdr) + bytes(body)


# ------------------------------------------------------------------ 8bpp (PcxInflate8)
# An 8bpp texture (PCXINFO flag bit 0) is one RLE stream over w*h bytes followed by
# the standard PCX 256-colour palette block (0x0C marker + 768 bytes), which the
# loader reads from `PcxInflate8(...) + 1`.

def decode8(blob):
    """-> (w, h, pixels[w*h], tail bytes after the RLE data)"""
    maxx, maxy = struct.unpack('<HH', blob[8:12])
    minx, miny = struct.unpack('<HH', blob[4:8])
    w, h = maxx - minx + 1, maxy - miny + 1
    size = w * h
    out = bytearray(); p = 128
    while len(out) < size:
        c = blob[p]; p += 1
        if c <= RLE: out.append(c)
        else:
            n = c - RLE; d = blob[p]; p += 1
            out.extend([d]*n)
    return w, h, bytes(out), blob[p:]


def encode8(template, pixels, tail, maxrun=62):
    """the same texture with its RLE stream re-emitted (header and palette block kept)"""
    return bytes(template[:128]) + bytes(_rle(pixels, maxrun)) + bytes(tail)
