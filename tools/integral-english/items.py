#!/usr/bin/env python
"""Port MGS1 (USA) English item/weapon descriptions into MGS Integral disc 1 & 2.

Reads the two boot executables, repacks the English strings into Integral's
Japanese text arenas, rewrites the two pointer tables, and emits PPF3 patches
addressed at the raw (2352-byte sector) CD images for Ketchup / MGSM2Fix.
"""
import struct, os, sys
from workdir import WORK

TADDR   = 0x80010000
HDR     = 0x800          # PS-EXE header; file 0x800 == RAM 0x80010000

# --- donor: MGS1 (USA), SLUS_005.94 / SLUS_007.76 (identical on both discs)
US_ITEM_TAB, US_WEAP_TAB = 0x800A0B38, 0x800A0D24
US_OUTLIER               = 0x0913A0          # "Mine Detector / HARD or EXTREME"
# --- target: MGS Integral, SLPM_862.47 / SLPM_862.48 (identical on both discs)
IN_ITEM_TAB, IN_WEAP_TAB = 0x8009E3E4, 0x8009E5CC
IN_OUTLIER               = 0x08EC4C
IN_OUTLIER_LUI           = 0x02BEB0          # lui  reg,hi16
IN_OUTLIER_ADDIU         = 0x02BEB4          # addiu reg,reg,lo16
N_ITEM, N_WEAP           = 26, 10            # the item table's last two entries are the frozen Ration/Ketchup pair
ARENA_A = (0x0016AC, 0x001E74)               # item description pool
ARENA_B = (0x001EE8, 0x002304)               # weapon description pool

# The inventory's side column shows an ABBREVIATION, and those are a different
# set of strings from the descriptions above - a block of NUL-terminated names
# on an 8-byte stride, items 23 down to 0, already Latin on both discs. The port
# leaves the whole block alone because it is already English and already
# identical... except for one entry.
#
#   Integral  0x9BCC0  'SCARF\0\0\0'
#   USA       0x9E448  'HANDKER\0'
#
# Integral's own Japanese for that item calls it ハンカチ - a handkerchief - and
# its own description, which this patch replaces with USA's, says so in both
# languages. So its short label disagreed with its own text while USA's agreed
# with both. **Changed on the user's instruction, 2026-09-07** (NextSteps §5.9:
# Integral's own English is outside the no-translation rule as written, so it is
# asked for rather than assumed).
#
# It is USA's `HANDKER` that goes in, not the full word: the slot is 8 bytes, so
# seven characters is the ceiling, and abbreviating "Handkerchief" any other way
# would be inventing text rather than porting it. Seven is exactly what USA
# ships, so the string lands in the existing slot and nothing moves.
#
# Two entries of that block are NOT in it: Cold Medicine and Diazepam are
# 8 characters, one too many for a slot that must also hold a terminator, so
# both games keep those two names elsewhere. And Integral's weapon block has one
# name USA has no equivalent for at all, `MP 5 SD`.
IN_SHORTNAME = 0x09BCC0                      # 8-byte slot, item 22's abbreviation
US_SHORTNAME = 0x09E448
SHORTNAME_LEN = 8

# Code in menu/item.c and menu/weapon.c that edits the descriptions in place,
# with byte offsets laid out for the Japanese strings (found 2026-09-05 from the
# user's screenshots and Ketchup's audit lines):
#  - menu_item_printDescription: itemDescription[46] = GM_CardFlag + '0' - the
#    card level digit. USA's string has its '1' at 45 and USA's own exe stores
#    at 45 (sb $v0, 0x2d($a1) at 0x8003D9E8); Integral's stores at 46 and
#    overwrote the space: "level 17security".
#  - menu_weapon_printDescription: bytes 0x70..0x72 of the SOCOM description
#    become "d0 03 00" or "90 b6 91" for the suppressor line. USA's 83-byte
#    string ends long before 0x70 and USA's identical code writes into padding;
#    in the repacked pool those bytes were the Mine Detector text ("Cannot be
#    used in" -> "Cannot be<90b6><91..>"). Made the no-ops they are in USA.
IN_CARD_LEVEL_SB  = 0x8003B690                # sb $v0, 0x2e($a0) -> 0x2d
IN_SOCOM_SB       = (0x8003E070, 0x8003E07C, 0x8003E088, 0x8003E094, 0x8003E0A0, 0x8003E0B0)
IN_SOCOM_SB_WORDS = (0xA0620070, 0xA0620071, 0xA0400072, 0xA0620070, 0xA0620071, 0xA0620072)

# Ketchup disk descriptors: image offset of RAM 0x80010000 for each disc
INTEGRAL_BASE = {0: 0x131D2238, 1: 0x0EB38078}
SECTOR_DATA, SECTOR_RAW = 0x800, 0x930

ram  = lambda fo: TADDR + fo - HDR
fofs = lambda a:  a - TADDR + HDR

def cstr(d, fo):
    e = d.find(b'\x00', fo)
    return d[fo:e + 1]

def read_table(d, tab, n):
    base = fofs(tab)
    return [struct.unpack_from('<I', d, base + i * 4)[0] for i in range(n)]

def image_offset(base, fo):
    k = fo - HDR
    return base + (k // SECTOR_DATA) * SECTOR_RAW + (k % SECTOR_DATA)

def ppf3(records, desc):
    out = bytearray()
    out += b'PPF30'
    out += bytes([2])                                    # method: PPF3.0
    out += desc.encode('ascii')[:50].ljust(50, b'\x00')  # description
    out += bytes([0, 0, 0, 0])                           # imagetype, blockcheck, undo, dizfile
    assert len(out) == 60
    for off, data in records:
        for i in range(0, len(data), 255):
            chunk = data[i:i + 255]
            out += struct.pack('<Q', off + i) + bytes([len(chunk)]) + chunk
    return bytes(out)

def main():
    us  = open(os.path.join(WORK, 'us1.exe'), 'rb').read()
    ino = open(os.path.join(WORK, 'int1.exe'), 'rb').read()
    new = bytearray(ino)

    us_items = [cstr(us, fofs(a)) for a in read_table(us, US_ITEM_TAB, N_ITEM)]
    us_weaps = [cstr(us, fofs(a)) for a in read_table(us, US_WEAP_TAB, N_WEAP)]
    outlier  = cstr(us, US_OUTLIER)

    in_items = read_table(ino, IN_ITEM_TAB, N_ITEM)
    in_weaps = read_table(ino, IN_WEAP_TAB, N_WEAP)

    # lay strings out in the same order they appear in the original image
    def pack(arena, strings, order, extra=()):
        lo, hi = arena
        for a in range(lo, hi):
            new[a] = 0
        cur = lo
        placed = {}
        for k in order:
            s = strings[k]
            if cur + len(s) > hi:
                raise SystemExit('arena 0x%06X overflow at entry %d' % (lo, k))
            new[cur:cur + len(s)] = s
            placed[k] = ram(cur)
            cur = (cur + len(s) + 3) & ~3
        tail = []
        for s in extra:
            if cur + len(s) > hi:
                raise SystemExit('arena 0x%06X overflow on extra string' % lo)
            new[cur:cur + len(s)] = s
            tail.append(ram(cur))
            cur = (cur + len(s) + 3) & ~3
        return placed, tail, hi - cur

    order_i = sorted(range(N_ITEM), key=lambda i: in_items[i])
    order_w = sorted(range(N_WEAP), key=lambda i: in_weaps[i])
    new_i, _,  slack_a = pack(ARENA_A, us_items, order_i)
    new_w, tail, slack_b = pack(ARENA_B, us_weaps, order_w, extra=(outlier,))
    out_ram = tail[0]

    for i in range(N_ITEM):
        struct.pack_into('<I', new, fofs(IN_ITEM_TAB) + i * 4, new_i[i])
    for i in range(N_WEAP):
        struct.pack_into('<I', new, fofs(IN_WEAP_TAB) + i * 4, new_w[i])

    # repoint the HARD/EXTREME mine-detector message (lui/addiu pair)
    lui   = struct.unpack_from('<I', new, IN_OUTLIER_LUI)[0]
    addiu = struct.unpack_from('<I', new, IN_OUTLIER_ADDIU)[0]
    assert lui >> 26 == 0x0F and addiu >> 26 == 0x09
    lo16 = out_ram & 0xFFFF
    hi16 = ((out_ram >> 16) + (1 if lo16 & 0x8000 else 0)) & 0xFFFF
    struct.pack_into('<I', new, IN_OUTLIER_LUI,   (lui   & 0xFFFF0000) | hi16)
    struct.pack_into('<I', new, IN_OUTLIER_ADDIU, (addiu & 0xFFFF0000) | lo16)

    # the ID Card's level digit goes where USA's string has it
    w = struct.unpack_from('<I', new, fofs(IN_CARD_LEVEL_SB))[0]
    assert w == 0xA082002E, 'card level store is not the expected sb: %08X' % w
    assert us_items[17][45:47] == b'1 ', us_items[17]
    struct.pack_into('<I', new, fofs(IN_CARD_LEVEL_SB), 0xA082002D)

    # the SOCOM suppressor rewrite becomes the no-op it is in USA
    for a, expect in zip(IN_SOCOM_SB, IN_SOCOM_SB_WORDS):
        w = struct.unpack_from('<I', new, fofs(a))[0]
        assert w == expect, 'SOCOM store at %08X is not the expected sb: %08X' % (a, w)
        struct.pack_into('<I', new, fofs(a), 0)          # nop
    assert len(us_weaps[0]) < 0x70, 'USA SOCOM description reaches the suppressor offsets'

    # item 22's side-column abbreviation: USA's, in Integral's own slot
    was  = bytes(ino[IN_SHORTNAME:IN_SHORTNAME + SHORTNAME_LEN])
    want = bytes(us[US_SHORTNAME:US_SHORTNAME + SHORTNAME_LEN])
    assert was == b'SCARF\0\0\0', 'Integral short name is not SCARF: %r' % was
    assert want == b'HANDKER\0', 'USA short name is not HANDKER: %r' % want
    assert want.count(b'\0') >= 1 and want[-1] == 0, 'USA short name is not terminated in its slot'
    new[IN_SHORTNAME:IN_SHORTNAME + SHORTNAME_LEN] = want
    print('item 22 abbreviation: %r -> %r' % (was.rstrip(b'\0'), want.rstrip(b'\0')))

    open(os.path.join(WORK, 'int1_en.exe'), 'wb').write(new)
    print('arena A slack: %d bytes   arena B slack: %d bytes' % (slack_a, slack_b))
    print('HARD/EXTREME message relocated to %08X' % out_ram)

    # ---- every byte of the regions the port owns, changed or not.
    # The collection's own RAM patches rewrite this pool (two ~2.8 KB blocks at
    # 0x8001101C and 0x8001108C) before Ketchup's pass, and Ketchup only writes
    # the bytes a PPF names: a byte the English happened to share with retail
    # kept the collection's value. That was the SOCOM line break (0x800119DC,
    # retail 0x80 of "80 23", ours 0x80 of "80 7c") drawn as a katakana glyph
    # on 2026-09-05. Owning whole regions also lets Ketchup::Audit see them.
    regions = [ARENA_A, ARENA_B,
               (fofs(IN_ITEM_TAB), fofs(IN_ITEM_TAB) + 4 * N_ITEM),
               (fofs(IN_WEAP_TAB), fofs(IN_WEAP_TAB) + 4 * N_WEAP),
               (IN_OUTLIER_LUI, IN_OUTLIER_LUI + 4), (IN_OUTLIER_ADDIU, IN_OUTLIER_ADDIU + 4),
               (fofs(IN_CARD_LEVEL_SB), fofs(IN_CARD_LEVEL_SB) + 4),
               (IN_SHORTNAME, IN_SHORTNAME + SHORTNAME_LEN)]
    regions += [(fofs(a), fofs(a) + 4) for a in IN_SOCOM_SB]
    regions.sort()
    for (a, b), (c, d) in zip(regions, regions[1:]):
        assert b <= c, 'owned regions overlap'
    owned = bytearray(len(ino))
    for a, b in regions:
        owned[a:b] = b'\x01' * (b - a)
    assert not any(new[k] != ino[k] and not owned[k] for k in range(len(ino))), 'bytes changed outside the owned regions'
    runs = [(a, bytes(new[a:b])) for a, b in regions]
    print('%d owned regions, %d bytes (%d differ from retail)'
          % (len(runs), sum(len(r[1]) for r in runs), sum(1 for k in range(len(ino)) if owned[k] and new[k] != ino[k])))

    # ---- split runs at 2048-byte data-page boundaries, map to image offsets
    for disc, base in INTEGRAL_BASE.items():
        recs = []
        for fo, data in runs:
            p = 0
            while p < len(data):
                page_end = ((fo + p - HDR) // SECTOR_DATA + 1) * SECTOR_DATA + HDR
                n = min(len(data) - p, page_end - (fo + p))
                recs.append((image_offset(base, fo + p), data[p:p + n]))
                p += n
        blob = ppf3(recs, 'MGS Integral: English item text (disc %d)' % (disc + 1))
        name = os.path.join(WORK, 'INTEGRAL_disc%d_en_items.ppf' % (disc + 1))
        os.makedirs(os.path.dirname(name), exist_ok=True)
        open(name, 'wb').write(blob)
        print('%s: %d records, %d bytes' % (name, len(recs), len(blob)))

if __name__ == '__main__':
    main()
