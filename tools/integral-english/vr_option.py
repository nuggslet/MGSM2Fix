#!/usr/bin/env python
"""Port the USA VR Missions option screen into Integral's VR-DISC `option` stage.

One PPF for the stage, INTEGRAL_vr_en_option.ppf, patched IN PLACE (73 sectors
before and after), so the collection's KEY CONFIG interception patch keeps
landing on the stage the game reads (README "The collection's KEY CONFIG
interception": relocating the main game's option stage orphaned it).

Three things change:

1. The help-line chain (`chara 0x976C`, option -e, 31 records, index for
   index with USA): 1 Sound setting. 2 Vibration setting. 3/12/26 Use
   directional buttons to test. 5 Key configuration setting. 6 Return to the
   title screen. Everything USA leaves empty stays Integral's (the colon,
   オン/オフ/ステレオ/モノラル, the unused brightness paragraph 13-16, the KEY
   CONFIG rows' lines 17-25, the language rows 28-30). +49 bytes, inside the
   chunk's sector.

2. The eight KEY CONFIG label textures become USA's (key_action 32x8,
   key_buki 44x7, key_hohuku 28x8, key_syukan 88x10, key_button 88x13,
   key_sykan 112x13, key_normal 40x10, key_reverse 44x6), placed in free VRAM
   and CLUT slots of Integral's layout (kcplace.py's rules: one texture page,
   8-bit UVs, clear of the framebuffers and the font KCBs). They are 824 bytes
   bigger than Integral's and the DAR had 484 bytes of slack, so every texture
   in the DAR is re-encoded losslessly (pcx4: the same RLE, one stream per row
   for 4bpp, one stream per image for 8bpp, palettes untouched) - that frees
   enough to stay at 59 sectors. Pixels and palettes are verified equal.

3. The overlay's geometry, by binary patch, since the VR option overlay is not
   decompiled (vr_kcgeom.py reads both overlays; kcquads/kcrects did the same
   for the main game):
   * the per-button-type rectangle function (the authority for key_action /
     key_buki / key_hohuku / key_syukan) is USA's, copied over Integral's:
     Integral's slot is 748 bytes, USA's function 616, its two `j` retargeted,
     the rest nops. Both set the same label colours (checked). key_syukan's x
     is then moved +11 px, the decision of 2026-09-03 for the main game: the
     same key_back art puts Integral's connector curve 11 px right of USA's.
   * the Init_Res quads of key_button / key_sykan / key_reverse / key_normal
     get USA's values. Where Integral's compiler shared one register between
     two labels (button/sykan x1, reverse/normal y0 and y1) the redundant
     stores of orient (32(sp), already 0) and abe (28(sp), already 1) and the
     jal delay slots are rewritten to load and store the second value.
   * abe (blend) becomes 1 on the four labels Integral drew opaque: USA's
     palettes have no black, so an opaque quad would show a box.

usage: vr_option.py [--deploy]
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import struct, sys
import portio, pcx4, kcplace
import vr_kcgeom as K
from optscan import parse, geo, strcode
from audit_text import game_text
from vrlib import (INT_STAGE, USA_STAGE, int_disc, stage_lba, stage_bytes, stage_gcx, repack_stage,
                   parse_arg, emit_arg, inplace_records, write_ppf, deploy, be16, WORK, CMD_CHARA, Gcx)

PPF_NAME = 'INTEGRAL_vr_en_option.ppf'
DESC = 'MGS Integral VR-DISC: English option text'
CHAIN = {1: 1, 2: 2, 5: 5, 6: 6, 12: 12, 26: 26}
# Record 3 is NOT ported, and this is the main game's decision of 2026-09-02
# repeated: USA holds `Use directional buttons to test.` at 3, 12 and 26, but
# Integral draws 3 at the row-LABEL position and 12/26 at the sentence position,
# so giving 3 USA's sentence draws a second copy over the first (seen in game
# 2026-09-06).  Integral's own record 3 is 振動テスト - the row's name plus a
# sentence USA's line already covers - so it is blanked, exactly as
# `optlabel2.py` does for the main game.
BLANK = {3: b' '}
SYUKAN_SHIFT = 11                 # game px, the 2026-09-03 decision for the same art
SLOT_SECTORS = 59                 # the DAR's sectors in the retail stage
RUN_CAP = 63                      # pcx4's real maximum run; pays for the 4-alignment pad

# --- Init_Res call-site rewrites in Integral's overlay: (offset, old word, new word)
def addiu(rt, rs, imm): return (9 << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF)
def sw(rt, imm, rs=29): return (0x2B << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF)
A1, A3, V0, T0, S0, S1, S2, ZERO = 5, 7, 2, 8, 16, 17, 18, 0
CALL_SITE_PATCHES = [
    # key_button (-149,-70,-61,-58) -> (-148,-70,-60,-57)
    (0x50B0, addiu(A3, ZERO, -149), addiu(A3, ZERO, -148)),
    (0x50B8, addiu(S0, ZERO, -61), addiu(S0, ZERO, -60)),
    (0x50C0, addiu(V0, ZERO, -58), addiu(V0, ZERO, -57)),
    # key_sykan (-149,38,-61,50) -> (-148,38,-36,51); x1 was button's s0
    (0x5108, addiu(A3, ZERO, -149), addiu(A3, ZERO, -148)),
    (0x5110, addiu(V0, ZERO, 50), addiu(V0, ZERO, 51)),
    (0x5118, sw(S0, 20), addiu(T0, ZERO, -36)),
    (0x5128, sw(ZERO, 32), sw(T0, 20)),                 # delay slot; orient stays 0 from the button call
    # key_reverse (29,41,93,47) -> (40,42,84,48)
    (0x515C, addiu(A3, ZERO, 29), addiu(A3, ZERO, 40)),
    (0x5160, addiu(S2, ZERO, 41), addiu(S2, ZERO, 42)),
    (0x5164, addiu(V0, ZERO, 93), addiu(V0, ZERO, 84)),
    (0x5168, addiu(S0, ZERO, 47), addiu(S0, ZERO, 48)),
    # key_normal (-35,41,17,47) -> (-18,39,22,49); y0/y1 were reverse's s2/s0
    (0x51BC, addiu(A3, ZERO, -35), addiu(A3, ZERO, -18)),
    (0x51C0, addiu(V0, ZERO, 17), addiu(V0, ZERO, 22)),
    (0x51C4, sw(S2, 16), addiu(S2, S2, -3)),
    (0x51CC, sw(S0, 24), addiu(S0, S0, 1)),
    (0x51D0, sw(S1, 28), sw(S0, 24)),                   # abe stays 1 from the reverse call
    (0x51D8, sw(ZERO, 32), sw(S2, 16)),                 # delay slot; orient stays 0
    # --- the SELECTION HIGHLIGHT behind the key_sykan row, found on screen
    # 2026-09-07 with the collection's patches off (the first look at Integral's
    # own VR KEY CONFIG). The row's label is now USA's 112-wide `first person
    # view`, but the lit box behind it was still retail's 88, so the hexagon's
    # right chevron hung outside the glow. Measured off paired shots: USA's glow
    # ends at game x 125, ours at 101 - 24 px short, exactly 112 - 88.
    #
    # The box is not an Init_Res quad. Integral draws it with hardcoded
    # arguments, `glow(work, x, y, w, h, 255, ...)` at 0x800C23E4, and the
    # key_sykan row has two such call sites (the two selection states). Only the
    # x and the width move, to the label quad's own -148 and 112; the height
    # stays Integral's 12 against USA's 13-tall art, which is Integral's own and
    # not what was wrong. The key_button row's pair (y -70) is left alone - that
    # label is 88 wide on both discs.
    #
    # The height goes with it: USA's art is 13 rows to Integral's 12, and the
    # main game's opt.c already writes `option_800C449C(work, -148, 38, 112, 13,
    # 255, 1)` at both of its equivalent sites - the same helper, the same
    # arguments, ported in 2026-09-03. Measured on the shots: USA's glow is 17
    # rows to our 16 before this. So all three arguments become USA's, and the
    # box matches the label it frames.
    (0x2234, addiu(A1, ZERO, -149), addiu(A1, ZERO, -148)),
    (0x223C, addiu(A3, ZERO, 88), addiu(A3, ZERO, 112)),
    (0x2240, addiu(V0, ZERO, 12), addiu(V0, ZERO, 13)),
    (0x2424, addiu(A1, ZERO, -149), addiu(A1, ZERO, -148)),
    (0x242C, addiu(A3, ZERO, 88), addiu(A3, ZERO, 112)),
    (0x2430, addiu(V0, ZERO, 12), addiu(V0, ZERO, 13)),
    # abe = 1 on key_action / key_buki / key_hohuku / key_syukan
    (0x5418, sw(ZERO, 28), sw(S1, 28)),
    (0x5470, sw(ZERO, 28), sw(S1, 28)),
    (0x54C8, sw(ZERO, 28), sw(S1, 28)),
    (0x551C, sw(ZERO, 28), sw(S1, 28)),
]
# --- the per-state colour switch: stop lighting Integral's colon and value on
# the rows that now carry USA's English help line (2026-09-06, from the shots).
# See this module's docstring for the whole switch; the reset loop at +F44 sets
# every entry to colour 0, so an entry the switch skips is invisible - the same
# lever the main game used to unlight entry 27 (README, "The sc_text texture
# port"). No text changes: Integral's records 7, 8-11 and 27 stay as they are.
JAL_HELPER = 0x0C0306B5           # jal +934, the colour helper
J_1218     = 0x080308EE           # j +1218, the shared helper call in the tail
J_1220     = 0x080308F0           # j +1220, the instruction after it
LIT_PATCHES = [
    # SOUND {1, 7, 10|11} -> {1}
    (0x1004, JAL_HELPER, 0),          # entry 7, the colon
    (0x102C, J_1218, J_1220),         # value 10 (ステレオ)
    (0x1034, J_1218, J_1220),         # value 11 (モノラル)
    # VIBRATION {2, 7, 8|9} -> {2}
    (0x1054, JAL_HELPER, 0),          # entry 7, the colon
    (0x107C, J_1218, J_1220),         # value 8 (オン)
    (0x1084, J_1218, J_1220),         # value 9 (オフ)
    # VIBRATION TEST {3, 27, 12|26} -> {3 (blank), 12|26}
    (0x10A4, JAL_HELPER, 0),          # entry 27, the colon; the sentence stays lit
]

# --- the KCB position table: give every ported help line USA's own placement.
# opt.c: num 0 = draw at (x, y); num 1 = centre on it (x - max_width/2,
# y - max_height/2). Retail Integral VR is num 0 with an x hand-set for each
# JAPANESE string's width, so an English line of a different width sits
# off-centre (seen in game 2026-09-06); USA VR is {1, 160, 196} for all 31.
# This is the same substitution the main game's table already carries
# ({1, 160, 200} for its four ported lines) - see opt.c's own comment.
POS_TABLE = 0x10                  # overlay offset; 31 x {int num; short x; short y; int color}
POS_STRIDE = 12
USA_POS = (1, 160, 196)           # USA VR's entry, identical for every index
# every record the port replaces with USA English, and retail's value we expect
POS_PATCHES = {
    1:  (0,  76, 190),            # Sound setting.
    2:  (0, 100, 190),            # Vibration setting.
    3:  (0,  42, 190),            # (blanked, but keep it consistent with USA)
    5:  (0,  88, 190),            # Key configuration setting.
    6:  (0,  94, 190),            # Return to the title screen.
    12: (0, 122, 190),            # Use directional buttons to test.
    26: (0, 122, 190),            # Use directional buttons to test.
}

EXPECT_QUADS = {                  # USA's Init_Res quads (x0, y0, x1, y1, abe, orient)
    'key_button': (-148, -70, -60, -57, 1, 0), 'key_sykan': (-148, 38, -36, 51, 1, 0),
    'key_reverse': (40, 42, 84, 48, 1, 0), 'key_normal': (-18, 39, 22, 49, 1, 0),
    'key_action': (74, -18, 138, -11, 1, 0), 'key_buki': (-136, -18, -92, -11, 1, 0),
    'key_hohuku': (-136, 2, -84, 9, 1, 0), 'key_syukan': (78, -39, 138, -32, 1, 0),
}


def overlay_patch(iov, uov):
    new = bytearray(iov)
    # 1. the per-type function: USA's code over Integral's slot
    istart, iend = K.find_type_function(iov), K.function_end(iov, iov and K.find_type_function(iov))
    ustart, uend = K.find_type_function(uov), K.function_end(uov, K.find_type_function(uov))
    code = bytearray(uov[ustart:uend])
    assert len(code) <= iend - istart, 'USA function (%d) does not fit Integral slot (%d)' % (len(code), iend - istart)
    shift = istart - ustart
    for i in range(0, len(code), 4):
        w = struct.unpack_from('<I', code, i)[0]
        op = w >> 26
        assert op != 3, 'jal inside the per-type function'
        if op == 2:
            target = (K.USA_BASE & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            off = target - K.USA_BASE
            assert ustart <= off < uend, 'j out of the function at +%X' % i
            noff = off + shift
            nt = K.INT_BASE + noff
            struct.pack_into('<I', code, i, (2 << 26) | ((nt >> 2) & 0x3FFFFFF))
    # key_syukan +11: its x0 = 49 and x1 = 137 are loaded in type 1's block and in
    # the common tail (types 0 and 2); no other rectangle uses either value, and
    # the type_rects check below proves every path ends at the shifted label
    x0s = [i for i in range(0, len(code), 4) if struct.unpack_from("<I", code, i)[0] & 0xFFE0FFFF == addiu(0, ZERO, 49)]
    x1s = [i for i in range(0, len(code), 4) if struct.unpack_from("<I", code, i)[0] & 0xFFE0FFFF == addiu(0, ZERO, 137)]
    assert len(x0s) == 2 and len(x1s) == 2, (x0s, x1s)
    for lst, v in ((x0s, 49 + SYUKAN_SHIFT), (x1s, 137 + SYUKAN_SHIFT)):
        for i in lst:
            w = struct.unpack_from('<I', code, i)[0]
            struct.pack_into('<I', code, i, (w & 0xFFFF0000) | (v & 0xFFFF))
    new[istart:istart+len(code)] = code
    for i in range(istart + len(code), iend, 4):
        struct.pack_into('<I', new, i, 0)              # nop the leftover
    # 2. the Init_Res call sites
    for off, old, repl in CALL_SITE_PATCHES:
        got = struct.unpack_from('<I', new, off)[0]
        assert got == old, 'overlay +%X holds %08X, expected %08X' % (off, got, old)
        struct.pack_into('<I', new, off, repl)
    # 3. the colour switch: unlight the colon and the values where USA's English
    # help line now says it all
    for off, old, repl in LIT_PATCHES:
        got = struct.unpack_from('<I', new, off)[0]
        assert got == old, 'overlay +%X holds %08X, expected %08X (colour switch moved?)' % (off, got, old)
        struct.pack_into('<I', new, off, repl)
    # 4. the position table: USA's placement for every ported line
    for k, expect in sorted(POS_PATCHES.items()):
        off = POS_TABLE + POS_STRIDE * k
        got = struct.unpack_from('<ihh', new, off)
        assert got == expect, 'position entry %d is %s, expected %s (table moved?)' % (k, got, expect)
        struct.pack_into('<ihh', new, off, *USA_POS)
        # the USA overlay must actually say this, so the values follow the disc
        assert struct.unpack_from('<ihh', uov, off) == USA_POS, 'USA entry %d is not %s' % (k, USA_POS)
    new = bytes(new)
    # verify against the simulators
    q = K.label_quads(new, K.INT_BASE)
    for lab, exp in EXPECT_QUADS.items():
        assert q.get(lab) == exp, (lab, q.get(lab), exp)
    for t in (0, 1, 2):
        ip, ic = K.type_rects(new, istart, t, K.INT_BASE)
        up, uc = K.type_rects(uov, ustart, t, K.USA_BASE)
        assert ic == uc, 'label colours differ for type %d' % t
        for poly in up:
            want = K.rect_of(up[poly])
            if poly == 13:
                want = (want[0] + SYUKAN_SHIFT, want[1], want[2] + SYUKAN_SHIFT, want[3])
            assert K.rect_of(ip[poly]) == want, (t, poly, K.rect_of(ip[poly]), want)
    return new, (istart, iend, ustart, uend)


# --- the DAR

def dar_entries(payload):
    ents, rest = parse(payload)
    return [list(e) for e in ents], rest


def build_dar(ipay, upay, alloc):
    """Integral's DAR with USA's eight labels at their allocated VRAM/CLUT slots,
    every texture re-encoded losslessly"""
    ients, irest = dar_entries(ipay)
    uents, urest = dar_entries(upay)
    umap = {e[0]: e for e in uents}
    ids = {strcode(n) & 0xFFFF: n for n in K.LABELS}
    out = bytearray()
    report = []
    for tid, ext, size, blob in ients:
        name = ids.get(tid)
        if name:
            src = bytearray(umap[tid][3])
            a = alloc[name]
            struct.pack_into('<HHHH', src, 78, a['vram'][0], a['vram'][1], a['clut'][0], a['clut'][1])
            src = bytes(src)
        else:
            src = blob
        g = geo(src)
        if g['bpp'] == 4:
            w, h, pal, rows = pcx4.decode(src)
            enc = pcx4.encode(src, w, h, pal, rows, maxrun=RUN_CAP)
            w2, h2, pal2, rows2 = pcx4.decode(enc)
            assert (w2, h2, pal2, rows2) == (w, h, pal, rows)
        else:
            w, h, px, tail = pcx4.decode8(src)
            enc = pcx4.encode8(src, px, tail, maxrun=RUN_CAP)
            assert pcx4.decode8(enc) == (w, h, px, tail)
        assert enc[:128] == src[:128], 'header changed'
        # Every DAR entry's size must be a multiple of 4: an entry header is
        # {u16 id, s16 ext, u32 size} immediately after the previous payload, so
        # an odd size leaves the next header - and its u32 - misaligned. Retail
        # Integral and USA both hold this for all 51/57 entries; our first build
        # left 39 sizes odd, 41 starts misaligned, and the option stage crashed
        # the CPU at `load option` (2026-09-06). The pad sits inside `size` and
        # PcxInflate stops on its own byte count, so it is never decoded.
        enc = bytes(enc) + bytes(-len(enc) % 4)
        assert len(enc) % 4 == 0
        out += struct.pack('<HhI', tid, ext, len(enc)) + enc
        report.append((tid, name, size, len(enc)))
    out += irest
    return bytes(out), report


def allocate(int_sd_path, usa_sd_path):
    I = kcplace.inventory(int_sd_path)
    U = kcplace.inventory(usa_sd_path)
    ug = {n: dict(g) for _t, n, g, _b in U if n}
    ig = {n: g for _t, n, g, _b in I if n}
    assert set(ug) == set(ig) == set(K.LABELS)
    occ, clut = kcplace.build_maps(I, set(K.LABELS))
    out = {}
    for n in kcplace.LAB:                                   # largest first
        g = ug[n]
        wd = kcplace.words(g)
        pos = kcplace.place(occ, wd, g['h'], prefer=(ig[n]['px'], ig[n]['py']))
        assert pos, 'no VRAM for %s' % n
        kcplace.take(occ, pos[0], pos[1], wd, g['h'])
        cs = kcplace.clut_slot(occ, clut, g['nc'])
        assert cs, 'no CLUT slot for %s' % n
        clut[cs] = n
        out[n] = dict(vram=pos, clut=cs, w=g['w'], h=g['h'], was=(ig[n]['px'], ig[n]['py']), was_clut=(ig[n]['cx'], ig[n]['cy']))
    return out


# --- the chain

def chain(gcx, chara=0x976C, letter='e'):
    body = gcx.script
    block = parse_arg(body)
    for c in block:
        if c.kind == 'COMMAND' and c.id == CMD_CHARA:
            a = c.args()
            if a and a[0].kind == 'STRID' and be16(body, a[0].pos+1) == chara:
                o = c.option(letter)
                return body, block, [v for v in o.values if v.kind == 'STRING']
    raise AssertionError('option chain not found')


def port_chain(igcx, ugcx):
    ibody, iblock, irecs = chain(igcx)
    ubody, ublock, urecs = chain(ugcx)
    assert len(irecs) == len(urecs) == 31
    assert emit_arg(ibody, iblock, {}) == ibody
    replace = {}
    for i, j in CHAIN.items():
        new = ubody[urecs[j].pos+2:urecs[j].end]
        t, jp = game_text(new[:-1])
        assert t and not jp and new != b'\0'
        replace[id(irecs[i])] = bytes((7, len(new))) + new
        print('  chain %2d <- %s' % (i, t))
    for i, text in BLANK.items():
        new = text + b'\0'
        replace[id(irecs[i])] = bytes((7, len(new))) + new
        print('  chain %2d <- (blank, the main game\'s 2026-09-02 decision)' % i)
    igcx.script = emit_arg(ibody, iblock, replace)
    new_gcx = igcx.build()
    b2, _, r2 = chain(Gcx(new_gcx, 0))
    for i in range(31):
        if i in CHAIN:
            exp = ubody[urecs[CHAIN[i]].pos+2:urecs[CHAIN[i]].end]
        elif i in BLANK:
            exp = BLANK[i] + b'\0'
        else:
            exp = ibody[irecs[i].pos+2:irecs[i].end]
        assert b2[r2[i].pos+2:r2[i].end] == exp
    return new_gcx


def main():
    isd = open(INT_STAGE, 'rb').read()
    usd = open(USA_STAGE, 'rb').read()
    idata, udata = stage_bytes(isd, 'option'), stage_bytes(usd, 'option')
    itags, ipay, ici, ifiles, igcx = stage_gcx(idata)
    utags, upay, uci, ufiles, ugcx = stage_gcx(udata)
    assert itags[0][1] == ord('s') and itags[1][1] == ord('n')
    # 1. overlay
    new_ov, spans = overlay_patch(ipay[0], upay[0])
    print('overlay: per-type function Integral +%X..+%X <- USA +%X..+%X; %d call-site words rewritten; quads and rectangles verified' % (spans + (len(CALL_SITE_PATCHES),)))
    # 2. DAR
    alloc = allocate(INT_STAGE, USA_STAGE)
    for n, a in alloc.items():
        print('  %-12s %3dx%-3d VRAM %-11s CLUT %-11s (Integral had VRAM %s CLUT %s)' % (n, a['w'], a['h'], a['vram'], a['clut'], a['was'], a['was_clut']))
    new_dar, report = build_dar(ipay[1], upay[1], alloc)
    before = sum(8 + s for _, _, s, _ in report)
    print('DAR: %d entries, %d -> %d bytes (slot %d); labels: %s' % (
        len(report), len(ipay[1]), len(new_dar), SLOT_SECTORS * 2048,
        ', '.join('%s %d->%d' % (n, s, e) for _, n, s, e in report if n)))
    assert len(new_dar) <= SLOT_SECTORS * 2048, 'DAR does not fit its sectors'
    # 3. chain
    new_gcx = port_chain(igcx, ugcx)
    # 4. the stage
    payloads = dict(ipay)
    payloads[0] = new_ov
    payloads[1] = new_dar
    payloads[ici] = ipay[ici][:igcx.start] + new_gcx
    payloads[ici] += bytes(-len(payloads[ici]) % 4)
    new_stage = portio.pack_stage(itags, payloads)
    assert len(new_stage) == len(idata), 'stage changed size: %d -> %d' % (len(idata), len(new_stage))
    # read it back the way the loader does
    tags2, pay2, off2 = portio.stage(new_stage)
    assert pay2[0] == new_ov and pay2[1] == new_dar
    ents2, _ = parse(pay2[1])
    for tid, ext, size, blob in ents2:
        g = geo(blob)
        if g['bpp'] == 4:
            w, h, pal, rows = pcx4.decode(blob)
            assert (g['px'] % 64) * 4 + w <= 255 and g['py'] % 256 + h <= 255, 'UV wrap on %04X' % tid
    lba = stage_lba(int_disc(), isd, 'option')
    recs = inplace_records(lba, idata, new_stage)
    data = write_ppf(_os.path.join(WORK, PPF_NAME), recs, DESC)
    open(_os.path.join(WORK, 'vr_option_stage.bin'), 'wb').write(new_stage)
    print('%s: %d records, %d bytes' % (PPF_NAME, len(recs), sum(len(d) for _, d in recs)))
    if '--deploy' in sys.argv:
        print('deployed', deploy(PPF_NAME, data))


if __name__ == '__main__':
    main()
