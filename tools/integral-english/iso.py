import struct, sys

SECTOR = 2352

class Disc:
    def __init__(self, path, base=0):
        self.f = open(path, 'rb')
        self.base = base
        self.f.seek(0, 2); self.size = self.f.tell()
        self.nsec = self.size // SECTOR
        # detect header size: mode2 form1 = 24, mode1 = 16
        self.f.seek(base + 16 * SECTOR)
        raw = self.f.read(SECTOR)
        self.hdr = 24 if raw[24:24+6] == b'\x01CD001' else 16
        assert raw[self.hdr:self.hdr+6] == b'\x01CD001', raw[12:40]

    def sector(self, lba, n=1):
        out = bytearray()
        for i in range(n):
            self.f.seek(self.base + (lba + i) * SECTOR + self.hdr)
            out += self.f.read(2048)
        return bytes(out)

    def offset(self, lba, off=0):
        """byte offset within the raw image of a logical byte in the file data"""
        return (lba + off // 2048) * SECTOR + self.hdr + (off % 2048)

    def read(self, lba, size):
        n = (size + 2047) // 2048
        return self.sector(lba, n)[:size]

    def pvd(self):
        d = self.sector(16)
        return dict(volid=d[40:72].decode('latin1').strip(),
                    root=d[156:156+34])

    def walk(self, dirrec=None, path='', out=None):
        if out is None: out = []
        if dirrec is None:
            dirrec = self.pvd()['root']
        lba = struct.unpack_from('<I', dirrec, 2)[0]
        size = struct.unpack_from('<I', dirrec, 10)[0]
        data = self.read(lba, size)
        i = 0
        while i < len(data):
            L = data[i]
            if L == 0:
                i = (i // 2048 + 1) * 2048
                continue
            rec = data[i:i+L]
            flen = rec[32]
            name = rec[33:33+flen].decode('latin1')
            flags = rec[25]
            clba = struct.unpack_from('<I', rec, 2)[0]
            csize = struct.unpack_from('<I', rec, 10)[0]
            if flen == 1 and name in ('\x00', '\x01'):
                pass
            elif flags & 2:
                out.append((path + '/' + name, clba, csize, True))
                self.walk(rec, path + '/' + name, out)
            else:
                out.append((path + '/' + name, clba, csize, False))
            i += L
        return out

if __name__ == '__main__':
    for p in sys.argv[1:]:
        d = Disc(p)
        print('==== %s  sectors=%d hdr=%d volid=%r' % (p, d.nsec, d.hdr, d.pvd()['volid']))
        for name, lba, size, isdir in d.walk():
            print('%-40s lba=%7d size=%10d off=0x%09X %s' % (name, lba, size, d.offset(lba), 'DIR' if isdir else ''))
