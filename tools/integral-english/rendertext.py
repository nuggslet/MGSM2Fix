"""Read the game's own Japanese by drawing it with the game's own font.

The scripts and executables do not store Shift-JIS. They store **font indices**,
so there is no table anywhere that turns a Japanese string into characters -
`audit_text.game_text` can only print the raw codes, which is why this project's
notes are full of things like `<822F><8253><820B><8221>`. The one way to read
such a string is the way the game does: look the glyphs up and draw them.

    py rendertext.py --item 22                 an item description, Integral
    py rendertext.py --item 22 --exe us1.exe   the same slot on the USA disc
    py rendertext.py --hex 822f8253820b8221    any run of codes
    py rendertext.py --weapon 3

It writes a PNG and prints where. Read it with your eyes; nothing here claims to
translate anything, and nothing should.

HOW THE LOOKUP WORKS (font/font.c, `get_zen_font_data` and its caller)

    code &= ~0x6000                       style flags are not part of the glyph
    code < 0x8200   -> code - 0x8101
    code < 0x8300   -> code - 0x81AE
    code < 0x9600   -> (code - 0x8F71 - code/256) + 0xA9
    code < 0x9A00   -> ((code - 0x956B) - code/256) | 0x1000     (bank 1)
    glyph = zendata[idx >> 12] + ((idx & 0xFFF) - 1) * 36

Each glyph is 12x12 at 2 bits per pixel, 36 bytes, MSB first. Two details were
settled by rendering a word whose reading was already known rather than by
reasoning: the bit order (MSB-first; LSB-first draws every glyph mirrored) and
that against `font.res` as it sits in the stage archive the bank begins one
glyph earlier than the `- 1` above implies. Get either wrong and the output is
still plausible-looking Japanese, just the wrong Japanese - which is exactly why
it was checked against a known word.

`font.res` lives in the `init` stage and is found by signature, the same way
`optbright.glyph_widths` finds the width table.
"""
import argparse
import os
import struct
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from workdir import WORK

SIG = struct.pack('>II', 392, 2306)
ZENDATA = 2306
GLYPH = 36
TADDR, HDR = 0x80010000, 0x800
ITEM_TAB = {'int1.exe': 0x8009E3E4, 'int2.exe': 0x8009E3E4, 'us1.exe': 0x800A0B38, 'us2.exe': 0x800A0B38}
WEAP_TAB = {'int1.exe': 0x8009E5CC, 'int2.exe': 0x8009E5CC, 'us1.exe': 0x800A0D24, 'us2.exe': 0x800A0D24}


def zen_index(code):
    code &= ~0x6000
    if code < 0x8200:
        return code - 0x8101
    if code < 0x8300:
        return code - 0x81AE
    if code < 0x9600:
        return (code - 0x8F71 - (code // 256)) + 0xA9
    if code < 0x9A00:
        return ((code - 0x956B) - (code // 256)) | 0x1000
    return 0


def font_res(stage_dir):
    data = open(stage_dir, 'rb').read()
    at = data.find(SIG)
    assert at >= 0, 'font.res not found in %s' % stage_dir
    return data, at


def glyph(data, at, code):
    idx = zen_index(code)
    bank, n = idx >> 12, idx & 0xFFF
    if bank or not n:
        return None                      # bank 1 lives elsewhere; not located
    base = at + ZENDATA + n * GLYPH
    raw = data[base:base + GLYPH]
    if len(raw) < GLYPH:
        return None
    im = Image.new('L', (12, 12))
    for y in range(12):
        bits = int.from_bytes(raw[y*3:y*3+3], 'big')
        for x in range(12):
            im.putpixel((x, y), ((bits >> (22 - 2*x)) & 3) * 85)
    return im


def codes(raw):
    out, p = [], 0
    while p < len(raw):
        if raw[p] < 0x80:
            out.append(('ascii', raw[p])); p += 1
        else:
            out.append(('zen', (raw[p] << 8) | raw[p+1])); p += 2
    return out


def render(raw, stage_dir, path, scale=6):
    data, at = font_res(stage_dir)
    cs = codes(raw)
    im = Image.new('L', (12 * len(cs), 12), 0)
    unknown = 0
    for i, (kind, c) in enumerate(cs):
        if kind == 'ascii':
            continue                     # ASCII is readable already; left blank
        g = glyph(data, at, c)
        if g is None:
            unknown += 1
        else:
            im.paste(g, (i * 12, 0))
    im.resize((im.width * scale, im.height * scale), Image.NEAREST).save(path)
    return len(cs), unknown


def string_at(exe_name, table, index):
    data = open(os.path.join(WORK, exe_name), 'rb').read()
    ptr = struct.unpack_from('<I', data, table[exe_name] - TADDR + HDR + index * 4)[0]
    at = ptr - TADDR + HDR
    return data[at:data.index(b'\0', at)]


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--exe', default='int1.exe')
    p.add_argument('--stage', default=None, help='stage dir holding font.res (default: matches --exe)')
    p.add_argument('--item', type=int)
    p.add_argument('--weapon', type=int)
    p.add_argument('--hex')
    p.add_argument('--out', default='rendertext.png')
    args = p.parse_args()

    if args.hex:
        raw = bytes.fromhex(args.hex)
        where = 'hex'
    elif args.item is not None:
        raw = string_at(args.exe, ITEM_TAB, args.item)
        where = 'item %d' % args.item
    elif args.weapon is not None:
        raw = string_at(args.exe, WEAP_TAB, args.weapon)
        where = 'weapon %d' % args.weapon
    else:
        p.print_help()
        return 2

    stage = args.stage or os.path.join(
        WORK, ('usa1_stage.dir' if args.exe.startswith('us') else 'int1_stage.dir'))
    n, unknown = render(raw, stage, args.out)
    from audit_text import game_text
    print('%s of %s: %d bytes, %d glyph slot(s)%s' %
          (where, args.exe, len(raw), n,
           ', %d not in the located bank' % unknown if unknown else ''))
    print('codes : %s' % (game_text(raw)[0] or raw.hex()))
    print('drawn : %s' % os.path.abspath(args.out))
    return 0


if __name__ == '__main__':
    sys.exit(main())
