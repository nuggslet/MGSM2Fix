#!/usr/bin/env python
"""Port the VR MOVIE selection captions from USA's VR disc.

WHAT THE CAPTIONS ARE
---------------------
The one line under the thumbnail on EXTRA -> MOVIE. The *description* shown when
a clip opens is already English (it comes from `vr_en_missions`' window text);
this is the short caption beside the title, and it is still Japanese.

They live in `OPTION 't'` of the `movie` script's `CMD 9906` whose chara is
`0xFAA8` (the second id is `0x1D31`). Integral holds four records, USA six,
because **USA splits each TGS caption across two lines**:

    Integral                                  USA
    +125 36B  東京ゲームショウ'98春 出展映像A    +125 25B  Exhibition clip {"}A{"}for
                                              +140 33B  the Tokyo Game Show, Spring '98.
    +14B 36B  ...the same with B               +163 + +17E  likewise
    +171 23B  E3{(}97/6{)}...映像              +1A1 26B  Video clip from E3 (6/97)
    +18A  1B  (empty terminator)               +1BD  1B  (empty terminator)

Confirmed on screen 2026-09-06 from a USA VR shot beside an Integral one: USA
really does draw two lines, and its EXIT sits higher to make room (the user
confirmed the higher EXIT again on 2026-09-07; the "~19 display px" figure once
recorded here has no shot behind it in the screenshots folder and is only ~2
game px, so it is not a number to build on - measure USA's overlay instead).

The port itself is VERIFIED ON SCREEN 2026-09-07, all three clips. One cosmetic
item is open and deferred by the user: line 1's ink overlaps Integral's EXIT box
by two pixel rows, because Integral's caption face is taller than USA's at the
same table y. It is a chrome question - see README "The MOVIE selection
captions".

WHY THE ARITHMETIC IS THE VERIFICATION
--------------------------------------
Replacing each Integral record with USA's counterpart(s) changes the record
bytes by +24, +24 and +3 = **+51**, and Integral's `t` is 105 bytes against
USA's 156 - exactly 105 + 51. So a correct edit makes Integral's `t` payload
**byte-identical to USA's**, which `verify()` asserts. Everything outside `t`
stays Integral's, and every enclosing container is re-stamped by
`Gcx.build()` the way `vr_option.port_chain` does it.

THE LINE COUNT IS IN THE OVERLAY, AND IT IS ONE WORD
----------------------------------------------------
Solved 2026-09-07 (see the two_line_patch section below for the full map). The
caption actor uses the engine's generic numbered-text module - the same one
`abst.c` implements in the decomp, down to `work->field_51C[index].string` - and
it draws EVERY slot that is lit, every frame. The builder makes one 64x21 KCB
per `-t` record and starts them all invisible; `+D628` re-blanks them each
frame; `+D684 highlight(work, i)` lights caption i in colour 0x6739. So the
number of lines a clip shows is just how many slots get lit, and that is the
single place the two discs differ:

    Integral +F464   lw a0, 0x48(sp);  jal highlight;  move a1, s6
    USA      +F51C   lw a0, 0x48(sp);  jal highlight;  move a1, s0   (s0 = s6*2)
                     lw a0, 0x48(sp);  jal highlight;  ori  a1, s0, 1

s6 is the clip index in both. This port retargets that one `jal` at a 16-word
stub appended to the overlay, which calls retail's own highlight for clip*2 and
clip*2+1 - USA's two lines, by USA's own arithmetic.

WHAT THE TWO EARLIER GUESSES GOT WRONG
--------------------------------------
Both are recorded because each cost a build and a play test:

  * the *record count* alone (2026-09-06). Six records with retail's one
    highlight gives one line per clip, so clip A showed a fragment and the
    other clips someone else's line. Right data, wrong count of lit slots.
  * the *position table* (2026-09-07). Writing USA's y values moved the rows on
    screen exactly as predicted and still gave one line each - which was read as
    "the layout is not data-driven, so nothing here is data". Both halves were
    needed: the table places the lines, the code decides how many there are.

The mistake underneath both was an address. `Act`'s tail reads a global at
0x800A9580 and indexes a table with it, and that was read as `captions[clip]`,
which made `record = clip` look structural in the draw. The decomp names it:
`abst_sprt` indexes the same way with **GV_Clock**, the frame parity, and the
table is the ordering table per frame. The draw is handed an OT, not a string,
and it draws all lit slots - so nothing there ever limited a clip to one line.
The `f`/`m` options were never the lead either: `-m` is read by the unlock
*gate* at `+FF58`, and they stay Integral's, being GCL variable references that
would point Integral at slots its own scripts never write:

    OPTION 'f'   Integral  VAR 11 00 04 80      USA  VAR 11 00 04 82
    OPTION 'm'   Integral  VAR 12 00 04 82      USA  VAR 12 00 04 81

WHAT THIS SCRIPT WRITES
-----------------------
Two builds. The full one is now the shippable one:

  * `INTEGRAL_vr_en_movie.ppf` - all three captions in USA's English, USA's
    two-line layout for the two TGS clips: USA's six records, USA's position
    table and the two_lines stub.
  * `INTEGRAL_vr_en_movie_e3.ppf` - the historical subset, the E3 caption alone
    with retail's structure untouched. Kept because it needs no code change at
    all, so it is the fallback if the stub ever has to come out. The two files
    OVERLAP and must never both sit in `mods/`: Ketchup applies a folder in name
    order, so `..._movie_e3.ppf` would land last and overwrite part of the full
    build.

    py vr_movie.py              # stages both builds into work/, deploys neither
    py vr_movie.py --deploy     # ... and installs the FULL build, moving the
                                # e3-only file out of mods/ so it cannot win
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
import struct, sys

import portio
from optbright import glyph_widths, text_width
from vrlib import (INT_STAGE, USA_STAGE, int_disc, stage_lba, stage_bytes, stage_gcx,
                   parse_arg, emit_arg, Gcx, inplace_records, write_ppf, be16, CMD_CHARA,
                   walk_commands, deploy, MODS)

PPF_NAME = 'INTEGRAL_vr_en_movie.ppf'
DESC = 'MGS Integral VR-DISC: English movie captions'
assert len(DESC) <= 50
CAPTION_CHARA = 0xFAA8
LETTER = 't'
# Integral record index -> the USA record indices that replace it
PAIRING = {0: (0, 1), 1: (2, 3), 2: (4,)}
EXPECT_DELTA = 51

# THE E3-ONLY SUBSET - shipped 2026-09-06, superseded 2026-09-07, kept as the
# fallback. Integral's records are [A][B][E3][empty] and USA's E3 caption is a
# single record too, so swapping record 2's text leaves the record COUNT, ORDER
# and the overlay exactly as retail: correct under any clip->line mapping and
# with no code change at all. That is why it was shippable before the line count
# was understood, and why it is what to fall back to if the stub ever has to come
# out. It leaves both TGS captions Japanese.
SAFE_PAIRING = {2: (4,)}
SAFE_NAME = 'INTEGRAL_vr_en_movie_e3.ppf'

# --- the caption LINE POSITION table: half of the fix (the other half is the
# line COUNT, in two_line_patch below).
#
# helper2 (overlay +D170) places each caption from a 12-byte table at
# `0x800C9454` (overlay +82B4), entry per index, with exactly opt.c's model:
# `{int num; short x; short y; int color}`, num 0 = draw at (x, y), num 1 =
# centre on it. helper1 (+CFFC) reads the same entry's colour at offset 8. No
# other code references the table, so it belongs to this actor alone.
#
# Every function in this actor is logically identical to USA's but for data
# addresses and one immediate (the font VRAM column, 832 against 768), so the
# tables were the only difference in sight, and they differ in y alone: USA
# alternates two rows because each of its captions is two lines, Integral
# repeats one row. USA's table holds exactly these six entries and then other
# data; Integral's runs on (entry 6 y 196, entry 7 y 190).
#
#     entry        0    1    2    3    4    5
#     Integral   208  208  208  196  196  196
#     USA        196  208  196  208  196  208
#
# TESTED IN GAME 2026-09-07 with USA's y values written in - four halfwords, num
# (1), x (160) and the colour (0x6739) being identical already - and the write
# demonstrably landed: TGS B drew a row lower than the other two clips, exactly
# as [196, 208, 196, ...] predicts for record 1. Every clip still drew one line,
# because the count of LIT slots had not been touched yet. Both are needed: this
# table places the lines, the stub decides how many there are. Note what this
# does to the E3 caption - USA puts its single line on the upper row (entry 4,
# y 196) where Integral's sits at 208, so it moves up 12 px to USA's own
# placement.
#
# Applied to the full build only: with one record per clip retail's single row
# is already right.
POS_TABLE = 0x82B4                 # overlay offset of the table (RAM 0x800C9454)
# USA's copy sits 8 bytes earlier in its own overlay: helper2 loads it as
# 0x800D0000 - 0x39F4 = 0x800CC60C against Integral's 0x800D0000 - 0x6BAC, and
# USA's overlay base is 0x800C4350. Its table holds exactly six entries - the
# two-line pairs - where Integral's runs on past them.
USA_POS_TABLE = 0x82BC
POS_STRIDE = 12
POS_Y = [196, 208, 196, 208, 196, 208]      # USA's, entry for entry
POS_Y_RETAIL = [208, 208, 208, 196, 196, 196]
POS_NUM, POS_X, POS_COLOR = 1, 160, 0x6739


# --- THE TWO-LINE FIX: the caption count is in the overlay's selection code.
#
# Found 2026-09-07 by reading the actor instead of its data. The generic text
# module the caption actor uses (its first copy in this overlay, +CFFC..+D6A4)
# keeps up to 24 numbered lines, each with its own 64x21 KCB, its own position
# table entry and its own colour:
#
#   +FE44 the builder  GCL_GetOption('t') then GCL_NextStr/GCL_GetString until
#                      NULL, at most 24; per record it calls helper1 (allocate
#                      the KCB), helper2 (font_print_string into it, size and
#                      place it from the position table, mark the slot active)
#                      and +D5CC(t, i, 0) (font_set_color 0 = invisible), and
#                      stores the record count at work+0x45C.
#   +D2B4 the draw     emits four sprites for EVERY active slot, all of them,
#                      every frame - so any number of lines can be on screen.
#   +D628 per frame    font_set_color(box, 0) over all `count` slots: every
#                      caption starts each frame invisible.
#   +D684 highlight    +D5CC(t, i, 0x6739): light caption i. ONE call site.
#
# So the number of lines a clip shows is simply how many slots get lit, and
# that is the one place the two discs differ:
#
#   Integral +F464   lw a0, 0x48(sp);  jal highlight;  move a1, s6
#   USA      +F51C   sll s0, s6, 1
#                    lw a0, 0x48(sp);  jal highlight;  move a1, s0
#                    lw a0, 0x48(sp);  jal highlight;  ori  a1, s0, 1
#
# s6 is the clip index in both. USA lights records clip*2 and clip*2+1 - its
# two lines - where Integral lights record clip. Everything else in the actor is
# the same program: Act, the builder, the draw, helper1, helper2 and the
# position table all match instruction for instruction but for data addresses
# and the font VRAM column (832 against 768).
#
# USA's sequence is 14 words and Integral's block has 11, so the fix cannot be
# written in place. Instead the call site keeps its shape and only its target
# changes - one word - and a 16-word stub does the doubling:
#
#   two_lines(work, clip):  highlight(work, clip*2); highlight(work, clip*2+1)
#
# WHERE THE STUB LIVES, and why that is safe. It is appended to the overlay
# payload, at +1DFB8 (RAM 0x800DF158). The payload is 122,808 bytes and its
# sectors hold 122,880, so 64 bytes fit in padding the stage already carries:
# no payload moves, the stage keeps its 123 sectors and its LBA, and no
# collection patch is orphaned. The RAM is overlay space by construction - USA's
# own `movie` overlay is 9,576 bytes longer at the same load base (0x800C11A0),
# and Integral's `init` runs to 0x800EA1EC there.
#
# AND THE SIX SLOTS FIT BY RETAIL'S OWN ARITHMETIC. The module's slot array ends
# at work+0x2c+0x434+24*224 = work+0x1960, exactly where Act's two tpage
# primitives sit, so 24 lines is what the work struct is dimensioned for; the
# builder's own limit is 24. Their KCBs go to VRAM (832, 256+21*i) with the CLUT
# on the band's last row, an arena the allocator itself treats as x 832 (then
# 896), y 256..511. Six boxes reach y 382, and no texture in the stage's archive
# - identical in both discs - is anywhere near x 832: nothing there but these
# captions.
INT_BASE = 0x800C11A0
OV_CALL_SITE = 0xF464              # `jal highlight` in the clip-selection code
OV_HIGHLIGHT = 0x800CE824          # highlight(work, i): light caption i
JAL_HIGHLIGHT = 0x0C033A09         # the retail word at OV_CALL_SITE

# --- THE EXIT BOX: the room USA makes for a two-line caption.
#
# Asked and approved 2026-09-07, and it is an EXCEPTION to the standing default
# of keeping Integral's visuals: the user's rule 3 covers "the chrome that
# positions text", and here the chrome has to move or the text cannot be placed
# where USA places it. Everything about the box stays Integral's - its art, its
# colour, its size - only its y changes, to USA's own value.
#
# WHY IT IS NEEDED. Verified on screen 2026-09-07: with USA's caption rows the
# first line's ink runs game y 201-213 and Integral's EXIT box bottom border
# sits at 201-202, so they overlap by two rows. USA avoids it twice over - a
# shorter caption face (the user's observation; the README's "The white caption
# font differs between the two VR discs" keeps the face Integral's) and a higher
# EXIT box.
#
# WHERE THE POSITION LIVES. Each widget in this stage is built by
# `Init_Res(slot, 0, GV_StrCode(name), y)`, which centres the texture
# horizontally from its own header and offsets it by y from screen centre 120.
# Listing every call in both overlays gives 28 widgets whose y values are
# IDENTICAL between the discs except one:
#
#     sp_exit    Integral y +70 (screen 190)      USA y +66 (screen 186)
#
# and the measured retail box top border is exactly 190, which confirms the
# model. One immediate carries it - a scan of every addiu/ori/slti in either
# overlay finds precisely one instruction holding 70 (Integral) or 66 (USA).
#
# AND THE HIGHLIGHT MOVES WITH IT, which is what the user asked to be sure of.
# The widget is one object at `work+0xF0`, and both things that light it take
# that object as their anchor rather than a coordinate of their own:
# `+10198` attaches the `cur_l` cursor with `f(work+0xF0, strcode, 1, 1)` and
# `+F38C` sets its lit state with `f(work+0xF0, 0xFF, 1)`. Both call sites are
# byte-identical between the discs, as are all ten references to `work+0xF0`.
# So USA's whole widget - box, label and selection highlight - sits at 186
# because of this one immediate; nothing else could position the highlight, or
# USA's own would be wrong.
EXIT_Y_SITE = 0xF128               # addiu a3, zero, 70 - sp_exit's Init_Res y
EXIT_Y_USA_SITE = 0xF1D0           # the same instruction in USA's overlay
EXIT_Y_RETAIL, EXIT_Y_USA = 70, 66
addiu_a3 = lambda v: (0x09 << 26) | (0 << 21) | (7 << 16) | (v & 0xFFFF)

ZERO, A0, A1, SP, RA = 0, 4, 5, 29, 31
_I    = lambda op, rs, rt, imm: (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF)
addiu = lambda rt, rs, imm: _I(0x09, rs, rt, imm)
sw    = lambda rt, off: _I(0x2B, SP, rt, off)
lw    = lambda rt, off: _I(0x23, SP, rt, off)
sll   = lambda rd, rt, sa: (rt << 16) | (rd << 11) | (sa << 6)
ori   = lambda rt, rs, imm: _I(0x0D, rs, rt, imm)
jal   = lambda target: (3 << 26) | ((target >> 2) & 0x03FFFFFF)
jr    = lambda rs: (rs << 21) | 8
NOP   = 0


# The caption KCB is a 64-word rect, so `c_width = rect.w * 4 / 12` is 21 cells
# = 252 px and font_print_string wraps at `kcb->width - 12` = **240 px** - the
# same budget as the main game's option entries, and wrapping there is not
# cosmetic: the continuation lands on the CLUT row and writes past the buffer
# (README, "Wrap width" and the font gotchas). Measured against Integral's own
# font.res, USA's longest caption line is 201 px, so it fits with 39 px to
# spare; `max_width` is a byte, and 201 clears that ceiling too.
WRAP_PX = 240


def check_widths(texts):
    """every ported caption line, measured with Integral VR's own glyph advances"""
    W = glyph_widths(INT_STAGE)
    for t in texts:
        px = text_width(t, W)
        assert px <= WRAP_PX, ('%r is %d px and would wrap at %d - the continuation would land on the '
                               'CLUT row (README, "Wrap width")' % (t, px, WRAP_PX))
        print('    %3d px / %d  %r' % (px, WRAP_PX, t.decode('latin-1')))


def two_line_stub():
    """two_lines(a0 = work, a1 = clip): light caption clip*2, then clip*2+1.

    a0 and a1 do not survive a call, so `work` goes to the frame's argument-save
    area and `clip` to 0x10(sp); `ra` is restored only after the second `jal`,
    which writes it. The two nops are R3000 hazard slots - a load's result is
    not readable in the next instruction, and `jr ra` needs one after `lw ra`."""
    return [
        addiu(SP, SP, -0x18),      # addiu sp, sp, -0x18
        sw(RA, 0x14),              # sw    ra, 0x14(sp)
        sw(A1, 0x10),              # sw    a1, 0x10(sp)     ; clip
        sll(A1, A1, 1),            # sll   a1, a1, 1        ; clip*2
        jal(OV_HIGHLIGHT),         # jal   highlight
        sw(A0, 0x00),              # sw    a0, 0(sp)        ; delay slot: work
        lw(A0, 0x00),              # lw    a0, 0(sp)
        lw(A1, 0x10),              # lw    a1, 0x10(sp)
        NOP,                       # nop                    ; load delay
        sll(A1, A1, 1),            # sll   a1, a1, 1
        jal(OV_HIGHLIGHT),         # jal   highlight
        ori(A1, A1, 1),            # ori   a1, a1, 1        ; delay slot: clip*2+1
        lw(RA, 0x14),              # lw    ra, 0x14(sp)
        NOP,                       # nop                    ; load delay
        jr(RA),                    # jr    ra
        addiu(SP, SP, 0x18),       # addiu sp, sp, 0x18
    ]


def exit_patch(ov, uov):
    """Integral's EXIT box at USA's y, so a two-line caption has room"""
    ov = bytearray(ov)
    got = struct.unpack_from('<I', ov, EXIT_Y_SITE)[0]
    assert got == addiu_a3(EXIT_Y_RETAIL), ('overlay +%X holds %08X, not `addiu a3, zero, %d` - the '
                                           'sp_exit init moved' % (EXIT_Y_SITE, got, EXIT_Y_RETAIL))
    # the value follows the disc: USA's overlay must actually say 66
    usa = struct.unpack_from('<I', uov, EXIT_Y_USA_SITE)[0]
    assert usa == addiu_a3(EXIT_Y_USA), ('USA overlay +%X holds %08X, not `addiu a3, zero, %d`'
                                         % (EXIT_Y_USA_SITE, usa, EXIT_Y_USA))
    struct.pack_into('<I', ov, EXIT_Y_SITE, addiu_a3(EXIT_Y_USA))
    print('  EXIT box: sp_exit y %+d -> %+d (screen %d -> %d), USA\'s own value; the cur_l highlight '
          'and the lit state both anchor on the same object at work+0xF0'
          % (EXIT_Y_RETAIL, EXIT_Y_USA, 120 + EXIT_Y_RETAIL, 120 + EXIT_Y_USA))
    return bytes(ov)


def two_line_patch(ov):
    """the overlay with the stub appended and the call site pointed at it"""
    ov = bytearray(ov)
    got = struct.unpack_from('<I', ov, OV_CALL_SITE)[0]
    assert got == JAL_HIGHLIGHT, ('overlay +%X holds %08X, not `jal %08X` - the selection code moved'
                                  % (OV_CALL_SITE, got, OV_HIGHLIGHT))
    assert len(ov) % 4 == 0, 'overlay length %d is not 4-aligned' % len(ov)
    stub, at = two_line_stub(), len(ov)
    room = portio.pad(len(ov)) - len(ov)
    assert 4 * len(stub) <= room, ('the stub needs %d bytes and the payload has %d of sector padding'
                                   % (4 * len(stub), room))
    ov += b''.join(struct.pack('<I', w) for w in stub)
    struct.pack_into('<I', ov, OV_CALL_SITE, jal(INT_BASE + at))
    print('  overlay: two_lines stub at +%X (RAM %08X), %d bytes of %d spare; +%X now `jal %08X`'
          % (at, INT_BASE + at, 4 * len(stub), room, OV_CALL_SITE, INT_BASE + at))
    return bytes(ov), at


def captions(gcx):
    """(script bytes, parsed block, the OPTION 't' and its STRING records) of the
    CMD 9906 whose chara is CAPTION_CHARA. The command is NOT top level - it sits
    inside an enclosing command's `-e` option, wrapped in an ARG - so this walks
    the tree with vrlib.walk_commands rather than scanning `parse_arg`'s result."""
    body = gcx.script
    block = parse_arg(body)
    for c, _lang, _path in walk_commands(body, block):
        if c.kind == 'COMMAND' and c.id == CMD_CHARA:
            a = c.args()
            if a and a[0].kind == 'STRID' and be16(body, a[0].pos + 1) == CAPTION_CHARA:
                o = c.option(LETTER)
                assert o is not None, "the caption command has no -%s option" % LETTER
                return body, block, o, [v for v in o.values if v.kind == 'STRING']
    raise AssertionError('caption command (chara %04X) not found' % CAPTION_CHARA)


def font_remap(igcx, ugcx):
    """USA local-font code -> the code for the same glyph in Integral's font.

    Codes at or above 0x9A00 index a font stored INSIDE the script (36-byte
    12x12 2bpp glyphs, `index = code - 0x9A00`), and the two discs' fonts hold
    different glyphs, so copying a USA record verbatim renders mojibake - the
    README's own warning, and it happened: `Exhibition clip {q01}A{q02}for` came
    out on screen as `Exhibition clip 年A月for`, because USA's typographic quotes
    at indices 1 and 2 are 年 and 月 in Integral's font.

    No font surgery is needed here, because `vr_en_missions` has already merged
    USA's two quote glyphs into this stage's font: they sit at Integral indices
    14 and 15 (codes 9A0E / 9A0F). This matches glyph BITMAPS rather than
    trusting that, so it stays correct if the merge ever changes.
    """
    I = [igcx.font[k * 36:(k + 1) * 36] for k in range(len(igcx.font) // 36)]
    U = [ugcx.font[k * 36:(k + 1) * 36] for k in range(len(ugcx.font) // 36)]
    out = {}
    for j, g in enumerate(U):
        hits = [k for k, h in enumerate(I) if h == g]
        assert hits, ('USA local glyph %d (code %04X) is not in Integral\'s font; it would have to be '
                      'appended and the font grown' % (j + 1, 0x9A00 + j + 1))
        out[0x9A00 + j + 1] = 0x9A00 + hits[0] + 1
    print('local-font remap: %s' % ', '.join('%04X->%04X' % kv for kv in sorted(out.items())))
    return out


def apply_remap(rec, remap):
    """rewrite a record's local-font codes; `rec` is a whole 07 <len> <bytes> record"""
    body = bytearray(rec[2:])
    i, n = 0, 0
    while i + 1 < len(body):
        code = (body[i] << 8) | body[i + 1]
        # style flags live in 0x6000 (font.c: code &= ~0x6000); keep them
        bare = code & ~0x6000
        if bare in remap:
            new = remap[bare] | (code & 0x6000)
            body[i], body[i + 1] = new >> 8, new & 0xFF
            n += 1
            i += 2
            continue
        i += 2 if body[i] >= 0x80 else 1
    return bytes(rec[:2]) + bytes(body), n


def composite(isd):
    """The `movie` stage as the game actually sees it: retail plus every DEPLOYED
    VR PPF's writes to it.

    `vr_en_missions` already rebuilds this stage - it is where the clips'
    English descriptions and the merged script-local font come from - so a
    caption PPF built from retail would overwrite all of that. Caught 2026-09-06
    by a byte-overlap check between the deployed PPFs: a retail-based build
    clashed with `vr_en_missions` over all 704 of its bytes. Same trap as
    `menu.ppf`'s chain records on the main game (README, "The sc_text texture
    port"): build from the composite, and the emitted records then carry only
    the caption delta.
    """
    handover = _os.path.join(WORK, 'vr_movie_base.bin')
    # Refused, not degraded. This patch owns the whole `movie` stage, so
    # building it on retail would quietly ship a stage with the clips' English
    # descriptions reverted to Japanese - and it would look like a normal build.
    assert _os.path.exists(handover), (
        'missing %s.\n'
        'vr_windows.py ports the `movie` stage and hands it here rather than\n'
        'writing its own records for it, so that one patch owns the stage.\n'
        'Run `py vr_windows.py --build` first.' % handover)
    base = open(handover, 'rb').read()
    assert len(base) == len(stage_bytes(isd, 'movie')), 'handover stage changed size'
    print('composite base: %s (%d bytes) from vr_windows' % (handover, len(base)))
    return base


def build(pairing=PAIRING, expect_delta=EXPECT_DELTA, check_usa=True):
    isd, usd = open(INT_STAGE, 'rb').read(), open(USA_STAGE, 'rb').read()
    idata = composite(isd)
    itags, ipay, ici, ifiles, igcx = stage_gcx(idata)
    utags, upay, uci, ufiles, ugcx = stage_gcx(stage_bytes(usd, 'movie'))
    ibody, iblock, iopt, irecs = captions(igcx)
    ubody, ublock, uopt, urecs = captions(ugcx)
    assert len(irecs) == 4 and len(urecs) == 6, (len(irecs), len(urecs))
    print('caption option -%s: Integral %d records / %d bytes, USA %d records / %d bytes'
          % (LETTER, len(irecs), iopt.u8, len(urecs), uopt.u8))

    remap = font_remap(igcx, ugcx)
    replace, delta, lines = {}, 0, []
    for i, sources in sorted(pairing.items()):
        blob = b''
        for j in sources:
            rec = ubody[urecs[j].pos:urecs[j].end]
            assert rec[0] == 7 and rec[1] == len(rec) - 2, 'USA record %d is malformed' % j
            rec, nfix = apply_remap(rec, remap)
            assert rec[1] == len(rec) - 2, 'remap changed the length of record %d' % j
            blob += rec
            txt = rec[2:-1]
            lines.append(txt)
            print('  record %d <- USA %d (%2d B, %d glyph code%s remapped) %r'
                  % (i, j, len(txt), nfix, '' if nfix == 1 else 's',
                     ''.join(chr(b) if 32 <= b < 127 else '#' for b in txt)[:44]))
        old = ibody[irecs[i].pos:irecs[i].end]
        replace[id(irecs[i])] = blob
        delta += len(blob) - len(old)
    assert delta == expect_delta, 'record delta is %+d, expected %+d' % (delta, expect_delta)
    print('  line widths, measured with Integral VR font.res:')
    check_widths([t for t in lines if t])

    igcx.script = emit_arg(ibody, iblock, replace)
    new_gcx = igcx.build()
    payloads = dict(ipay)
    payloads[ici] = ipay[ici][:igcx.start] + new_gcx
    payloads[ici] += bytes(-len(payloads[ici]) % 4)
    # the OVERLAY (payload 0), for the full port only: with one record per clip
    # the retail single row and the retail single highlight are both correct.
    if len(pairing.get(0, ())) > 1:
        ov = bytearray(payloads[0])
        _t, upay0, _o = portio.stage(stage_bytes(usd, 'movie'))
        changed = 0
        for k, y in enumerate(POS_Y):
            off = POS_TABLE + POS_STRIDE * k
            num, x, oy, col = struct.unpack_from('<ihhi', ov, off)
            assert (num, x, oy, col & 0xFFFFFFFF) == (POS_NUM, POS_X, POS_Y_RETAIL[k], POS_COLOR),                 'position entry %d is %r, not retail (table moved?)' % (k, (num, x, oy, hex(col & 0xFFFFFFFF)))
            struct.pack_into('<h', ov, off + 6, y)
            changed += (oy != y)
        print('  line positions: %d of %d y values set to the USA layout %s' % (changed, len(POS_Y), POS_Y))
        # the line COUNT: light both of a clip's records, the way USA does
        payloads[0], stub_at = two_line_patch(bytes(ov))
        # and the room for the second line: USA's own EXIT y
        payloads[0] = exit_patch(payloads[0], upay[0])
    stage = portio.pack_stage(itags, payloads)
    assert len(stage) == len(idata), 'stage changed size: %d -> %d' % (len(idata), len(stage))
    verify(stage, ibody, irecs, ubody, uopt, urecs, pairing, check_usa, remap, ipay[0], upay[0])
    return idata, stage


def verify(stage, ibody, irecs, ubody, uopt, urecs, pairing, check_usa, remap, base_ov, uov):
    """Every ported record must equal USA's byte for byte, every record the
    pairing does not name must still equal Integral's, and the script must
    round-trip. For the full port the whole -t payload equals USA's."""
    tags2, pay2, ci, files, gcx = stage_gcx(stage)
    body, block, opt, recs = captions(gcx)
    want_n = len(irecs) + sum(len(v) - 1 for v in pairing.values())
    assert len(recs) == want_n, 'rebuilt option has %d records, want %d' % (len(recs), want_n)
    k = 0
    for i in range(len(irecs)):
        if i in pairing:
            for j in pairing[i]:
                want, _n = apply_remap(ubody[urecs[j].pos:urecs[j].end], remap)
                assert body[recs[k].pos:recs[k].end] == want, \
                    'record %d is not USA record %d (after the local-font remap)' % (k, j)
                k += 1
        else:
            assert body[recs[k].pos:recs[k].end] == ibody[irecs[i].pos:irecs[i].end], \
                'record %d should still be Integral\'s' % k
            k += 1
    if check_usa:
        got, wanted = body[opt.pos:opt.end], ubody[uopt.pos:uopt.end]
        # identical in LENGTH and in every byte except the remapped local-font
        # codes, which must differ - USA's quote indices are other glyphs in
        # Integral's font (see font_remap)
        assert len(got) == len(wanted), 'the -t option is %d bytes, USA has %d' % (len(got), len(wanted))
        differ = [k for k in range(len(got)) if got[k] != wanted[k]]
        print('verified: -t matches USA in length (%d bytes, %d records), differing in %d byte(s) - the '
              'remapped glyph codes; script round-trips; stage stays %d bytes'
              % (len(wanted), len(recs), len(differ), len(stage)))
    else:
        print('verified: %d record(s) taken from USA, the rest still Integral\'s; %d records total; '
              'script round-trips; stage stays %d bytes'
              % (sum(len(v) for v in pairing.values()), len(recs), len(stage)))
    assert emit_arg(body, block, {}) == body, 'the rebuilt script does not round-trip'
    verify_overlay(pay2[0], base_ov, uov, full=len(pairing.get(0, ())) > 1)


def verify_overlay(ov, base_ov, uov, full):
    """The overlay must be retail's, plus - for the full port only - USA's y
    values in the position table, the stub, and the one retargeted call."""
    if not full:
        assert ov == base_ov, 'the overlay changed in a build that should not touch it'
        return
    stub = b''.join(struct.pack('<I', w) for w in two_line_stub())
    at = len(base_ov)
    assert len(ov) == at + len(stub), 'overlay is %d bytes, expected %d' % (len(ov), at + len(stub))
    assert ov[at:] == stub, 'the appended stub is not what two_line_stub() emits'
    assert portio.pad(len(ov)) == portio.pad(len(base_ov)), 'the overlay outgrew its sectors'
    assert struct.unpack_from('<I', ov, OV_CALL_SITE)[0] == jal(INT_BASE + at),         'the call site does not point at the stub'
    # the only other differences are the four position-table y values
    diff = sorted({o // 4 * 4 for o in range(at) if ov[o] != base_ov[o]})
    want = sorted({OV_CALL_SITE, EXIT_Y_SITE} | {POS_TABLE + POS_STRIDE * k + 4 for k in range(len(POS_Y))
                                                 if POS_Y[k] != POS_Y_RETAIL[k]})
    assert diff == want, 'the overlay changed at %s, expected %s' % (
        ['+%X' % o for o in diff], ['+%X' % o for o in want])
    # and every ported placement must be what USA's own overlay says
    for k, y in enumerate(POS_Y):
        off = POS_TABLE + POS_STRIDE * k
        assert struct.unpack_from('<ihhi', ov, off)[:3] == (POS_NUM, POS_X, y), 'entry %d' % k
        assert struct.unpack_from('<ihhi', uov, USA_POS_TABLE + POS_STRIDE * k)[:3] == (POS_NUM, POS_X, y),             'USA entry %d is not {%d, %d, %d}' % (k, POS_NUM, POS_X, y)
    # the stub calls the retail highlight twice and nothing else
    jals = [w for w in two_line_stub() if (w >> 26) == 3]
    assert jals == [jal(OV_HIGHLIGHT)] * 2, 'the stub does not call highlight twice'
    # the EXIT box is at USA's y, and USA's overlay says so
    assert struct.unpack_from('<I', ov, EXIT_Y_SITE)[0] == addiu_a3(EXIT_Y_USA), 'the EXIT y is not USA value'
    assert struct.unpack_from('<I', uov, EXIT_Y_USA_SITE)[0] == addiu_a3(EXIT_Y_USA), 'USA EXIT y moved'
    print('verified: EXIT box y is USA\'s %+d (screen %d), from one immediate at +%X'
          % (EXIT_Y_USA, 120 + EXIT_Y_USA, EXIT_Y_SITE))
    print('verified: overlay +%X -> two_lines at +%X (%d bytes in sector padding), highlight(clip*2) and '
          '(clip*2+1); the position table matches USA on both discs; nothing else in the overlay moved'
          % (OV_CALL_SITE, at, len(stub)))


def emit(name, pairing, expect_delta, check_usa, note):
    print('\n--- %s ---' % name)
    base, stage = build(pairing, expect_delta, check_usa)
    isd = open(INT_STAGE, 'rb').read()
    lba = stage_lba(int_disc(), isd, 'movie')
    # `base` is the COMPOSITE (retail + vr_en_missions), so the diff is only the
    # caption delta. merge_gap=0 keeps each record to the bytes that actually
    # change: with the default 64 they span unchanged bytes too and overlap
    # vr_en_missions' records by ~750 bytes, which would leave the result
    # depending on which PPF Ketchup applied last. Exact records leave overlap
    # only where this patch deliberately overrides a byte vr_en_missions wrote,
    # and there it must win - Ketchup loads a folder in name order and
    # INTEGRAL_vr_en_missions.ppf sorts before INTEGRAL_vr_en_movie.ppf.
    # (Cleanest long-term fix: fold the captions into vr_windows.py so one patch
    # owns the stage. Not done blind - regenerating vr_en_missions would rewrite
    # 3.3 MB of verified output.)
    # Against RETAIL, not against the composite. This patch is the `movie`
    # stage's only owner (vr_windows.HANDOVER), so its records must carry every
    # byte that differs from the disc - the mission-window port included. Built
    # against the composite instead, the two patches would each write part of
    # the stage and Ketchup's file-name order would decide the result.
    recs = inplace_records(lba, stage_bytes(open(INT_STAGE, 'rb').read(), 'movie'),
                           stage, merge_gap=0)
    write_ppf(_os.path.join(WORK, name), recs, DESC)
    print('%s: %d records, %d bytes  %s' % (name, len(recs), sum(len(d) for _, d in recs), note))
    return stage


def main():
    isd, usd = open(INT_STAGE, 'rb').read(), open(USA_STAGE, 'rb').read()
    _i, _p, _c, _f, ig = stage_gcx(composite(isd))
    _i2, _p2, _c2, _f2, ug = stage_gcx(stage_bytes(usd, 'movie'))
    ib, _b, _o, ir = captions(ig)
    ub, _b2, _o2, ur = captions(ug)
    # the fallback: the E3 caption alone, structure and overlay untouched
    safe_delta = (ur[4].end - ur[4].pos) - (ir[2].end - ir[2].pos)
    s = emit(SAFE_NAME, SAFE_PAIRING, safe_delta, False, '<- FALLBACK, no code change')
    open(_os.path.join(WORK, 'vr_movie_e3_stage.bin'), 'wb').write(s)
    # the full port: both TGS captions become USA's two lines
    s = emit(PPF_NAME, PAIRING, EXPECT_DELTA, True, '<- FULL, the one to ship')
    open(_os.path.join(WORK, 'vr_movie_stage.bin'), 'wb').write(s)
    print("""
Two builds, and the FULL one is the port:

  %s
      All three captions in USA's English, the two TGS ones as USA's two lines:
      USA's six records, USA's position table, and one retargeted `jal` plus a
      16-word stub that lights caption clip*2 and clip*2+1 the way USA does.

  %s
      The E3 caption alone, retail's record count, order and overlay - the
      2026-09-06 build, kept as the fallback if the stub ever has to come out.
      It leaves both TGS captions Japanese.

They overlap, so exactly ONE of them may sit in mods/INTEGRAL/VR-DISK/ -
Ketchup applies a folder in name order and ..._movie_e3.ppf sorts last.""" % (PPF_NAME, SAFE_NAME))
    if '--deploy' in sys.argv:
        print()
        stale = _os.path.join(MODS, SAFE_NAME)
        if _os.path.exists(stale):
            parked = _os.path.join(WORK, SAFE_NAME + '.was-deployed')
            _os.replace(stale, parked)
            print('moved the e3-only patch out of mods/: %s' % parked)
        print('deployed', deploy(PPF_NAME, open(_os.path.join(WORK, PPF_NAME), 'rb').read()))
        print('run `py ppfcheck.py --deployed` now')


if __name__ == '__main__':
    main()
