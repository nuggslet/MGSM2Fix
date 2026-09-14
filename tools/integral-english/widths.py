"""How wide a ported line renders, and how wide it is allowed to be.

Wrapping in this engine is not cosmetic. `font_draw_string` breaks a line that
exceeds its buffer's width and draws the remainder 18 rows down, inside a
buffer that was allocated for exactly the lines the script declares - so the
continuation lands on the CLUT row and keeps writing past the end. The visible
result is doubled text and wrecked palettes; the real result is a freeze at the
next allocation. The main game hit this once, on the option screen.

Two of the VR builders already measured what they emit (`vr_movie.check_widths`
at 240 px, `vr_option`). This is that check for the rest, in one place.

WHAT THE BUDGET IS, read out of the decomp rather than guessed
(`koba/vr/vrwindow.c`, and `font_draw_string` in `font/font.c`):

    a window declares  -w x y w h
    w is rounded UP to a multiple of 4                 -> w_rect.w
    the text margin is  w_rect.w - 16                  -> m_rect.w
    the KCB rect is  m_rect.w / 4  VRAM words, so the
      buffer is  (m_rect.w / 4) * 4  pixels wide       -> kcb->width
    font_draw_string reserves 12 px unless FONT_NO_KINSOKU,
      and vrwindow passes flag 0                       -> minus 12

so a `-w 32 53 256 118` window gives 256 -> 240 -> 228 px of room.

There is also a ceiling no window can lift: `kcb->max_width` is one byte, read
with `lbu`, so **255 px** is the hard limit anywhere in the engine. Where a
family's own budget is not established, that ceiling still applies and is worth
asserting on its own.

Widths come from Integral's `font.res`, because Integral's font is what draws.
Measured 2026-09-07, its VR font and its main-game font have identical advances
for all 96 ASCII glyphs, which is why the main game's on-screen measurements
carry over.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from optbright import glyph_widths

PX_ZENKAKU = 12         # font_get_glyph_width returns 12 for anything non-hankaku
MAX_WIDTH_CEILING = 255  # kcb->max_width is a u8 loaded with lbu
KINSOKU_RESERVE = 12     # font_draw_string subtracts this unless FONT_NO_KINSOKU


_FONTS = {}


def font(stage_dir):
    """{ASCII code: advance in px} out of the stage dir's own font.res.

    Cached: `glyph_widths` reads the whole stage archive to find `font.res` by
    signature, and a stage archive is 72 MB. Calling this per record inside a
    loop is the obvious way to write the check and would otherwise re-read that
    file every time.
    """
    if stage_dir not in _FONTS:
        _FONTS[stage_dir] = glyph_widths(stage_dir)
    return _FONTS[stage_dir]


def width(record, W):
    """Rendered advance of one record's bytes.

    The scripts store 16-bit glyph codes, but a code in 0x8000..0x807F - the
    ASCII range - is written as a single byte below 0x80. Everything else is
    two bytes with a lead byte of 0x80 or more and costs a flat 12 px, script
    -local font glyphs (0x9A00 and up) included. Note the lead-byte test is
    `>= 0x80`, not `>= 0x81`: codes 0x8080..0x80FF exist and a one-off test
    would read their lead byte as an ASCII advance and silently under-measure.
    """
    px, i = 0, 0
    while i < len(record):
        if record[i] >= 0x80:
            px += PX_ZENKAKU
            i += 2
        else:
            px += W.get(record[i], 0)
            i += 1
    return px


def window_budget(w_option_values):
    """Pixels of room in a `vrwindow` whose -w option holds these four ints.

    Every step is from the decomp, and the rounding matters twice:

        w_rect.w = align4(w)                        vrwindow.c
        m_rect.w = w_rect.w - 16                    vrwindow.c
        rect.w   = m_rect.w / 4                     Vrwindow_800D7E54, VRAM words
        c_width  = (rect.w * 4) / 12                font_set_kcb, whole cells
        kcb.width= (c_skip + 12) * c_width          font_get_buffer_size, c_skip 0
        buf_width= kcb.width - 12                   font_draw_string, flag 0

    So the room is a whole number of 12-pixel cells, less one. A 256-wide
    window gives 20 cells, 240 px, 228 px of room.
    """
    w = int(w_option_values[2])
    if w & 3:
        w += 4 - (w % 4)                 # vrwindow.c rounds the width up
    words = (w - 16) // 4                # m_rect.w in 16-bit VRAM words
    cells = (words * 4) // 12            # font_set_kcb truncates to whole cells
    return 12 * cells - KINSOKU_RESERVE


def check(lines, budget, where, W):
    """Raise if any line is too wide. Returns the widest measurement."""
    worst = 0
    for line in lines:
        px = width(line, W)
        worst = max(worst, px)
        if px > budget:
            raise AssertionError(
                '%s: a line renders %d px with Integral\'s font, over the %d px'
                ' this window allows - it would wrap, and a wrap here writes'
                ' past the text buffer. Bytes: %s' % (where, px, budget, line[:48].hex()))
    return worst


LINE_BREAK = b'\x80\x7c'    # the '|' code; the item and weapon pools break on it


def split_lines(record):
    """One stored string may hold several drawn lines.

    The executable's item and weapon descriptions separate theirs with the
    glyph code 0x807C - a '|' - so measuring the stored bytes whole says a
    perfectly ordinary description is 434 px and fails a check it should pass.
    Window records carry no separator, so splitting them is a no-op.
    """
    return record.split(LINE_BREAK)


def check_ceiling(lines, where, W, ceiling=MAX_WIDTH_CEILING):
    """The budget that always applies, for text whose own window is not known."""
    worst = 0
    for stored in lines:
        for line in split_lines(stored):
            px = width(line, W)
            worst = max(worst, px)
            if px > ceiling:
                raise AssertionError(
                    '%s: a line renders %d px, over the %d px that kcb->max_width can'
                    ' even hold (it is a u8). Bytes: %s'
                    % (where, px, ceiling, line[:48].hex()))
    return worst
