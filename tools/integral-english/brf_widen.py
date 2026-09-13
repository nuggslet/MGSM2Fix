"""Widen the briefing menu labels to USA proportions.

Two edits that must land together:
  * the draw quad, hardcoded as immediates in GetResources
    (asm/overlays/brf/brf_800C99C0.s) - patched in the overlay image
  * the texture, swapped to USA's artwork at its native size, given VRAM room

VRAM units are ceil(w * bpp / 16); the archive mixes 4bpp and 8bpp textures.
A texture must also fit inside one 64-unit texture page.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
import struct, json, sys
sys.path.insert(0, '.')

BASEADDR = 0x800C3208

def pad(x, a=2048): return (x + a - 1) // a * a

def ents(d):
    h = struct.unpack('<I', d[:4])[0]; o = []
    for p in range(4, h + 12, 12):
        n = d[p:p+8].rstrip(b'\x00')
        if n: o.append((n.decode('latin1', 'replace'), struct.unpack('<I', d[p+8:p+12])[0]))
    return o

def stage(path, name='brf'):
    d = open(path, 'rb').read(); base = dict(ents(d))[name] * 2048
    ver, pdb, sect = struct.unpack('<BBh', d[base:base+4])
    tags = []; p = base + 4
    while True:
        tid, mode, ext, sz = struct.unpack('<HBBi', d[p:p+8])
        if mode == 0: break
        tags.append([tid, mode, ext, sz]); p += 8
    FILE = [k for k, t in enumerate(tags) if not (chr(t[1]) == 'c' and chr(t[2]) in 'klhg')]
    off, pay = 2048, {}
    for k in FILE:
        pay[k] = d[base+off:base+off+tags[k][3]]; off += pad(tags[k][3])
    assert off == sect * 2048, 'layout mismatch'
    return sect, tags, FILE, pay

def parse(b):
    out = []; p = 0
    while p + 8 <= len(b):
        tid, ext, size = struct.unpack('<HhI', b[p:p+8])
        if size <= 0 or p + 8 + size > len(b): break
        out.append([tid, ext, size, b[p+8:p+8+size]]); p += 8 + size
    return out, b[p:]

def geo(b):
    bpp = b[3] * b[65]
    maxx, maxy = struct.unpack('<HH', b[8:12]); minx, miny = struct.unpack('<HH', b[4:8])
    st, fl, px, py, cx, cy, nc = struct.unpack('<7H', b[74:88])
    assert st == 12345
    return dict(w=maxx-minx+1, h=maxy-miny+1, bpp=bpp, px=px, py=py, cx=cx, cy=cy, nc=nc, fl=fl)

def units(w, bpp): return (w * bpp + 15) // 16

def ufits(px, w, bpp):
    """DG_SetTexture stores off_x = (px % 64) * (16 / bpp) TEXELS and
    tex->w = w - 1, and brf_800C983C sets poly->u1 = off_x + tex->w + 1 into a
    u_char.  So the limit is off_x + w <= 255; over that the U wraps and the
    quad samples across the page as vertical stripes."""
    return (px % 64) * (16 // bpp) + w <= 255

def vfits(py, h):
    """Same on the other axis: off_y = py % 256, tex->h = h - 1, and
    poly->v2 = off_y + tex->h + 1.  This also keeps a texture inside one
    256-row tpage half, which the tpage field cannot express otherwise."""
    return (py % 256) + h <= 255

def strcode(s):
    i = 0
    for ch in s.encode(): i = (((i << 5) | (i >> 11)) & 0xFFFF); i = (i + ch) & 0xFFFF
    return i

si, ti, Fi, pi = stage(WORK + '/int1_stage.dir')
su, tu, Fu, pu = stage(WORK + '/usa1_stage.dir')
ni = [k for k, t in enumerate(ti) if chr(t[1]) + chr(t[2]) == 'nd'][0]
nu = [k for k, t in enumerate(tu) if chr(t[1]) + chr(t[2]) == 'nd'][0]
ei, taili = parse(pi[ni]); eu, _ = parse(pu[nu])
U = {e[0]: e[3] for e in eu}
quads = json.load(open(WORK + '/brf_quads_all.json'))

# Horizontal: move the whole submenu group to USA's absolute position.
#
# The FILE column already measures pixel-identical to USA, so both games share
# the actor origin - which means Integral's submenu really does sit 20 game px
# right of USA's, and USA's xl values are the right target rather than an
# offset from Integral's.  Moving the rule with the labels is what makes this
# work; moving the labels alone either broke the label-to-rule gap or pushed
# `the terrorists' armament` off the screen edge.
#
# With xl at USA's values nothing overflows (family A 10 + 128 = 138, family B
# 29 + 120 = 149, against the 160 edge), so the earlier clamp is gone.
LINE_DELTA = 0
RIGHT_LIMIT = 158

# Two pairs share one xr immediate, so the narrower member gets the wider one's
# quad - and since the selection highlight follows the quad, its highlight runs
# far past its text (br_s06, Dr. Naomi, was 112 wide against USA's 52).  USA
# gives each its own xr.  Un-sharing needs one spare slot per block and there is
# none - except that the rewritten positioner now overwrites all four poly y
# values every frame, which makes the GetResources yt/yb *dead* for every
# br_sNN.  Their loads are the spare slots.  All four rows are unconditional,
# so the positioner always runs and the y really is always replaced.
UNSHARE = ['br_s02', 'br_s06', 'br_s09', 'br_s11']

def unshare_patches(xl_of, w_of):
    def li(rt, v): return 0x24000000 | (rt << 16) | (v & 0xFFFF)
    def sw(rt, o): return 0xAFA00000 | (rt << 16) | o
    T0, S1, S4, S5 = 8, 17, 20, 21
    x02 = xl_of('br_s02') + w_of('br_s02'); x06 = xl_of('br_s06') + w_of('br_s06')
    x09 = xl_of('br_s09') + w_of('br_s09'); x11 = xl_of('br_s11') + w_of('br_s11')
    return [
        # br_s02: its own xr in t0, and s5 left holding br_s06's xr for later
        (0x800C9EF8,
         [0x2408FFD1, 0x24150056, 0xAFA80010, 0x2408FFDE, 0xAFB50014, 0xAFA80018, 0xAFB1001C],
         [li(T0, x02), li(S5, x06), sw(T0, 16), sw(T0, 20), sw(T0, 24), sw(S1, 28), 0],
         'br_s02 xr=%d, leaves s5=%d for br_s06' % (x02, x06)),
        # br_s06 reads that s5 unchanged; br_s09 just needs its own immediate
        (0x800CA134, [0x2414007A], [li(S4, x09)], 'br_s09 xr=%d' % x09),
        # br_s11 no longer reads s4
        (0x800CA1C8,
         [0x2408FFE5, 0xAFA80010, 0x2408FFF2, 0xAFB40014, 0xAFA80018, 0xAFB1001C],
         [li(T0, x11), sw(T0, 16), sw(T0, 20), sw(T0, 24), sw(S1, 28), 0],
         'br_s11 xr=%d' % x11),
    ]
S00_X_ADDRS = (0x800C7674, 0x800C76A4)      # br_s00's animated x0 and x1 base
S00_X_OLD, S00_X_NEW = 26, 10               # to USA's family-A xl

# brf_800C6930 is NOT the selection highlight (the glow follows the label quad
# and already matches USA).  It sets two untextured polys per flag-gated item -
# (27,28) br_s01, (29,30) br_s03, (31,32) br_s05, (33,34) br_s10, (35,36) br_s13,
# (37,38) br_s15 - drawn only while that item's flag == 1, at that item's row y.
# Integral: box [y-4, y+10], bar [y+10, y+11].  USA (800C910C) takes a 5th arg K:
# bar [y+3, y+4], box [y-K, y+3], K = s4 (10) except br_s03's site (s4+8 = 18)
# and br_s13's (14).  K is derived here from the box poly index in a2, using
# the twelve dead x-copy instructions as slots.  Not visible in a save with none
# of those flags set, so unverified in game; the previous HILITE patch here
# (box [y, y+5]) was a misreading of this function as the selection highlight.
FRAME_ADDR = 0x800C6930
FRAME_OLD = [0x24840780, 0x00051080, 0x00451021, 0x000210C0, 0x00441021, 0x24E5000A,
             0x84480008, 0x84490020, 0x24E3000B, 0xA445000A, 0xA4450012, 0xA443001A,
             0xA4430022, 0xA4480008, 0xA4490010, 0xA4480018, 0xA4490020, 0x00061080,
             0x00461021, 0x000210C0, 0x00441021, 0x84480008, 0x84490020, 0x24E7FFFC,
             0xA447000A, 0xA4470012, 0xA445001A, 0xA4450022, 0xA4480008, 0xA4490010,
             0xA4480018, 0x03E00008, 0xA4490020]
FRAME_NEW = [0x24840780,   # addiu a0, a0, 1920
             0x00051080,   # sll   v0, a1, 2
             0x00451021,   # addu  v0, v0, a1
             0x000210C0,   # sll   v0, v0, 3
             0x00441021,   # addu  v0, v0, a0       poly[idx1]
             0x24E50003,   # addiu a1, a3, 3        bar top    (USA)
             0x24E30004,   # addiu v1, a3, 4        bar bottom (USA)
             0xA445000A,   # sh    a1, 10(v0)
             0xA4450012,   # sh    a1, 18(v0)
             0xA443001A,   # sh    v1, 26(v0)
             0xA4430022,   # sh    v1, 34(v0)
             0x03004021,   # addu  t0, t8, zero     K = t8 (10, or 7 with 4-5 member items)
             0x24C9FFE2,   # addiu t1, a2, -30
             0x15200002,   # bne   t1, zero, +2     a2 != 30 -> skip
             0x24C9FFDC,   # addiu t1, a2, -36      (delay slot, always)
             0x27080008,   # addiu t0, t8, 8        br_s03's frame: K = t8 + 8
             0x15200002,   # bne   t1, zero, +2     a2 != 36 -> skip
             0x00061080,   # sll   v0, a2, 2        (delay slot, needed anyway)
             0x2408000E,   # addiu t0, zero, 14     br_s13's frame
             0x00461021,   # addu  v0, v0, a2
             0x000210C0,   # sll   v0, v0, 3
             0x00441021,   # addu  v0, v0, a0       poly[idx2]
             0x00E83823,   # subu  a3, a3, t0       box top = y - K
             0xA447000A,   # sh    a3, 10(v0)
             0xA4470012,   # sh    a3, 18(v0)
             0xA445001A,   # sh    a1, 26(v0)       box bottom = y + 3
             0xA4450022,   # sh    a1, 34(v0)
             0x03E00008,   # jr    ra
             # --- 800C69A0: member-layout helper, entered by jal from the member block.
             # In: a0 = item count n. Out: t9 = row advance for br_s03..s06 (USA's s6):
             # 20 with three items, 17 with more. Lives in the frame function's dead
             # tail; the block has no room for these four words itself.
             0x28830004,   # slti  v1, a0, 4        v1 = (n < 4)
             0x00031040,   # sll   v0, v1, 1
             0x00431021,   # addu  v0, v0, v1       3 * v1
             0x03E00008,   # jr    ra
             0x24590011]   # addiu t9, v0, 17       (delay) t9 = 17 + 3 * (n < 4)
assert len(FRAME_OLD) == len(FRAME_NEW) == 33

# Where each submenu's list starts.  Both builds centre the list on the total
# height of its items, but from different arithmetic; measured against USA's
# shots the rows sat 4 high (outline), 1 low (member) and 12 high (detailed)
# once the per-label `above` was accounted for.  USA's formulas:
#   outline   s0 = -41 - (20n - 15) / 2          Integral had (20n - 7)
#   member    s0 = -21 - (20n -  5) / 2          Integral had (20n - 7); this is
#             USA's 3-item branch - with 4 or 5 items USA switches to a 17-row
#             advance and (17n - 2) / 2, which Integral's fixed-immediate
#             advances cannot follow, so that state still differs (documented)
#   detailed  s0 =   9 - (16n + 10d - 11) / 2    d = two-line items (1, or 2 with
#             br_s13); Integral had ~((17n - 4) / 2).  Rewritten in place using
#             the block's three nops.
START_Y = [(0x800C6EEC, -7, -15, 'outline start: (20n-15)/2, USA')]
# The outline block's signed halving is redundant (20n-15 > 0); its two spare
# words seed t8 = 10, USA's s4: the frame polys' K, which the member block
# lowers to 7 when it takes the 17-row branch.
OUTLINE_T8_ADDR = 0x800C6EF0
OUTLINE_T8_OLD = [0x00041FC2, 0x00831821, 0x00031843]   # srl v1,a0,31 / addu v1,a0,v1 / sra v1,v1,1
OUTLINE_T8_NEW = [0x00041843, 0x2418000A, 0x00000000]   # sra v1,a0,1  / addiu t8,zero,10 / nop
# USA's member block has two branches: three items -> advance 20 (br_s02 30),
# start -21 - (20n-5)/2; four or five -> advance 17 (br_s02 27, i.e. 17|10),
# start -21 - (17n-2)/2, and the frame K base drops from 10 to 7. Integral's
# advances were fixed immediates, so the block is rewritten: the advance lives
# in t9 (the helper above computes it), the K base in t8, and the start is
# (t9*(n-1) + 15) / 2, which is 20n-5 or 17n-2 in one expression. t8/t9 are
# safe across the block: the only calls are our positioner (a0,a1,v0,v1) and
# frame function (t0,t1,a1,a3,v0,v1). Same 44 words, two nops to spare.
MEMBER_ADDR = 0x800C6FB0
MEMBER_OLD = [0x8E420088, 0x24130001, 0x14530002, 0x24040003, 0x24040004, 0x8E420090,
              0x00000000, 0x14530003, 0x00041080, 0x24840001, 0x00041080, 0x00441021,
              0x00021080, 0x2444FFF9, 0x000417C2, 0x00821021, 0x00021043, 0x2403FFEB,
              0x00628023, 0x02402021, 0x2405000B, 0x02003021, 0x24070014, 0x2408FFE8,
              0xA6230622, 0xA623062A, 0x2403FFEC, 0xA6230632, 0xA623063A, 0x24C3FFFC,   # (RULE already applied)
              0x2442FFF0, 0xA6280620, 0xA6340628, 0xA6280630, 0xA6340638, 0xA6360648,
              0xA623064A, 0xA6350650, 0xA6230652, 0xA6360658, 0xA622065A, 0xA6350660,
              0x0C031A6D, 0xA6220662]
MEMBER_NEW = [0x8E420088,   # lw    v0, 136(s2)     br_s03 flag
              0x24130001,   # addiu s3, zero, 1
              0x14530002,   # bne   v0, s3, +2
              0x24040003,   # addiu a0, zero, 3     (delay) n = 3
              0x24040004,   # addiu a0, zero, 4
              0x8E420090,   # lw    v0, 144(s2)     br_s05 flag
              0x2408FFE0,   # addiu t0, zero, -32   p39 connector LEFT end: USA's (Integral -24), see CONNECTOR_LEFT
              0x14530002,   # bne   v0, s3, +2
              0x2405000B,   # addiu a1, zero, 11    (delay) br_s02's poly
              0x24840001,   # addiu a0, a0, 1
              0x0C031A68,   # jal   800C69A0        t9 = 20 or 17
              0x2489FFFF,   # addiu t1, a0, -1      (delay) n - 1
              0x2738FFF6,   # addiu t8, t9, -10     frame K base: 10 or 7
              0x03290018,   # mult  t9, t1
              0x00001012,   # mflo  v0
              0x2442000F,   # addiu v0, v0, 15      20n-5 | 17n-2
              0x00021043,   # sra   v0, v0, 1       v = half
              0x2403FFEB,   # addiu v1, zero, -21
              0x00628023,   # subu  s0, v1, v0      s0 = -21 - v
              0x02402021,   # addu  a0, s2, zero
              0x02003021,   # addu  a2, s0, zero
              0x3727000A,   # ori   a3, t9, 10      br_s02 advance: 30 or 27
              0xA6230622,   # sh    v1, 1570(s1)    connector y0 = -21
              0xA623062A,   # sh    v1, 1578(s1)
              0x2403FFEC,   # addiu v1, zero, -20
              0xA6230632,   # sh    v1, 1586(s1)
              0xA623063A,   # sh    v1, 1594(s1)
              0x24C3FFFC,   # addiu v1, a2, -4      rule top = s0 - 4
              0x2442FFF0,   # addiu v0, v0, -16     rule bottom = v - 16
              0xA6280620,   # sh    t0, 1568(s1)    connector x0
              0xA6340628,   # sh    s4, 1576(s1)    connector x1
              0xA6280630,   # sh    t0, 1584(s1)
              0xA6340638,   # sh    s4, 1592(s1)
              0xA6360648,   # sh    s6, 1608(s1)    rule x0
              0xA623064A,   # sh    v1, 1610(s1)    rule y0
              0xA6350650,   # sh    s5, 1616(s1)
              0xA6230652,   # sh    v1, 1618(s1)
              0xA6360658,   # sh    s6, 1624(s1)
              0xA622065A,   # sh    v0, 1626(s1)    rule y2
              0xA6350660,   # sh    s5, 1632(s1)
              0xA6220662,   # sh    v0, 1634(s1)
              0x00000000,
              0x0C031A6D,   # jal   800C69B4        position br_s02
              0x00000000]
assert len(MEMBER_OLD) == len(MEMBER_NEW) == 44
# The detailed submenu's horizontal connector (poly 41) ends at the rule in USA
# (`sh zero` into x1/x3); Integral stores fp = 20 and the connector runs 20 px
# past the rule as a faint additive line - one row +24 brighter from x 160 on,
# (faint only on the collection: its renderer samples br_line1's dark row 23
# into the one-pixel quad; hardware and SwanStation draw the bright row - README)
# 25 screenshot pixels over threshold. fp is br_s00's advance reused as an x,
# like s4 was for polys 25/39.
CONNECTOR_END = [(0x800C7180, 0xA63E0678, 0xA6200678, 'poly 41 x1: fp (20) -> 0'),
                 (0x800C718C, 0xA63E0688, 0xA6200688, 'poly 41 x3: fp (20) -> 0')]
# br_s03..s06 take their advance from t9 instead of the immediate 20
MEMBER_ADV = [(0x800C7094, 0x24070014, 0x03203821, 'br_s03'), (0x800C70AC, 0x24070014, 0x03203821, 'br_s04'),
              (0x800C70E4, 0x24070014, 0x03203821, 'br_s05'), (0x800C70FC, 0x24070014, 0x03203821, 'br_s06')]
# The L-shaped connectors GetResources sets up for the six flag-gated, indented
# items are TWO textured br_line2 quads each (polys 27-38): the bar (Integral
# 30..44, USA 14..22) and the drop (29..33, USA 13..17; the texture's line sits
# one texel in, so it draws at 30 / 14). Both move to USA's. Measured before
# the fix: Integral's drops at screen x 190, USA's at 174; the first pass
# patched only the bars and changed nothing visible.
CONNECTOR_X = [(0x800CA3EC, 30, 14), (0x800CA480, 30, 14), (0x800CA510, 30, 14), (0x800CA5A0, 30, 14),
               (0x800CA630, 30, 14), (0x800CA6C0, 30, 14), (0x800CA3F0, 44, 22),
               (0x800CA438, 29, 13), (0x800CA4C8, 29, 13), (0x800CA558, 29, 13), (0x800CA5E8, 29, 13),
               (0x800CA678, 29, 13), (0x800CA708, 29, 13),
               (0x800CA43C, 33, 17), (0x800CA4CC, 33, 17), (0x800CA55C, 33, 17), (0x800CA5EC, 33, 17),
               (0x800CA67C, 33, 17), (0x800CA70C, 33, 17)]
# br_s01 (time limit) has an animated reveal like br_s00's: x0 = xl, x1 = xl +
# w*step/6. Integral's w is 84, which divides by 6, so the compiler folded it to
# 14*step and hardcoded xl 46 twice; USA's is 52, so it multiplies by the 1/6
# reciprocal that a0 still holds from the br_s00 block (loaded at 800C7648, and
# nothing between writes a0). Rebuilt as 52*step/6 + 29 in the same words (the handler's closing jr ra stays):
# the block's four y-normalising stores (y0->y1, y3->y2) are dead because the
# row positioner writes all four y's every frame, and the compiler's negative-
# quotient fixup is unnecessary for a non-negative numerator.
S01_ADDR = 0x800C76BC
S01_OLD = [0x000510C0, 0x00451023, 0x00024040, 0xA1630036, 0xA1630047, 0xA1630048,
           0x2403002E, 0x84E9019A, 0x84EA01B2, 0x2502002E, 0xA4E30198, 0xA4E201A0,
           0xA4E301A8, 0xA4E201B0, 0xA4E9019A, 0xA4E901A2, 0xA4EA01AA, 0x03E00008, 0xA4EA01B2]
S01_NEW = [0x000510C0,   # sll   v0, a1, 3        8*step
           0x00451021,   # addu  v0, v0, a1       9*step
           0x00024080,   # sll   t0, v0, 2        36*step
           0xA1630036,   # sb    v1, 54(t3)       (kept: reveal state bytes)
           0xA1630047,   # sb    v1, 71(t3)
           0xA1630048,   # sb    v1, 72(t3)
           0x2403001D,   # addiu v1, zero, 29     x0 = USA's xl
           0x00051100,   # sll   v0, a1, 4        16*step
           0x00481021,   # addu  v0, v0, t0       52*step
           0x00440018,   # mult  v0, a0           * 0x2AAAAAAB
           0xA4E30198,   # sh    v1, 408(a3)      x0
           0xA4E301A8,   # sh    v1, 424(a3)      x2
           0x00004010,   # mfhi  t0               52*step/6
           0x2502001D,   # addiu v0, t0, 29       x1 = 29 + width
           0xA4E201A0,   # sh    v0, 416(a3)      x1
           0xA4E201B0,   # sh    v0, 432(a3)      x3
           0x00000000,
           0x03E00008,   # jr    ra               (unchanged: the block ends the handler)
           0x00000000]   # (delay slot: was `sh t2, 434(a3)`, t2 is no longer loaded)
assert len(S01_OLD) == len(S01_NEW) == 19
DETAIL_ADDR = 0x800C7100
DETAIL_OLD = [0x8E4200A4, 0x00000000, 0x14530002, 0x24040006, 0x24040007, 0x8E4200B0,
              0x00000000, 0x14530002, 0x00000000, 0x24840001, 0x8E4200B8, 0x00000000,
              0x14530003, 0x00041100, 0x24840001, 0x00041100, 0x00441021, 0x2444FFFC,
              0x000417C2, 0x00821021, 0x00021043, 0x00028027]
DETAIL_NEW = [0x8E4200A4,   # lw    v0, 164(s2)     br_s10 flag
              0x2405FFFF,   # addiu a1, zero, -1    10*1 - 11
              0x14530002,   # bne   v0, s3, +2
              0x24040006,   # addiu a0, zero, 6
              0x24040007,   # addiu a0, zero, 7
              0x8E4200B0,   # lw    v0, 176(s2)     br_s13 flag
              0x2409FFE5,   # addiu t1, zero, -27   p41 connector LEFT end, USA's - in the load-delay slot (does not read v0)
              0x14530003,   # bne   v0, s3, +3
              0x00000000,
              0x24840001,   # addiu a0, a0, 1
              0x24050009,   # addiu a1, zero, 9     10*2 - 11
              0x8E4200B8,   # lw    v0, 184(s2)     br_s15 flag
              0x00000000,
              0x14530002,   # bne   v0, s3, +2
              0x00041100,   # sll   v0, a0, 4       (delay slot)
              0x24840001,   # addiu a0, a0, 1
              0x00041100,   # sll   v0, a0, 4       16n
              0x00451021,   # addu  v0, v0, a1      16n + 10d - 11
              0x00021043,   # sra   v0, v0, 1       / 2  (always positive here)
              0x00028027,   # nor   s0, zero, v0    -v0 - 1
              0x2610000A,   # addiu s0, s0, 10      = 9 - v0
              0x24180006]   # addiu t8, zero, 6     USA's s4 = 6 here: the detailed drops' K (member left 7 or 10)
assert len(DETAIL_OLD) == len(DETAIL_NEW) == 22
# The operation-outline submenu's vertical rule.  Integral draws it centred,
# top = (-41 - v1) - 2 and bottom = v1 - 38 with v1 = (20n - 7)/2, so its length
# is 2*v1 + 5 = 20n - 2 - the 20 being Integral's original row advance.  For the
# one drawn item that is 17 game px; USA measures 12.6.  The bottom constant is
# independent of the row start (s0 = -41 - v1), so shortening it moves the rule
# without moving any text: 2*v1 + 0 = 12 for n = 1.
# The submenu group's x: the vertical rules (polys 26/40/42) take theirs from
# s6/s5, and the horizontal connectors (polys 25/39/41) run from the FILE column
# to the rule, taking their right end from s4.  Shift the rule and that right
# end by -20 so they land on USA's.  The connectors' left ends were believed to
# anchor to the FILE column and were left at Integral's values until 2026-09-10;
# CONNECTOR_LEFT below is the correction.
GROUP_DX = -20
RULE_X = [(0x800C6F28, 19, 19 + GROUP_DX, 'vertical rule left  (s6, polys 26/40/42)'),
          (0x800C6F38, 23, 23 + GROUP_DX, 'vertical rule right (s5, polys 26/40/42)')]
# s4 is `addu s4, a3, zero` - it reuses br_s00's advance 20 as an x coordinate,
# so it needs a real load, not an edited immediate.  a3 must keep its 20.
RULE_S4 = (0x800C6F18, 0x00E0A021, 0x24140000 | ((20 + GROUP_DX) & 0xFFFF),
           'connector right end (s4, polys 25/39/41)')

# The layout block is not the last word on x: every rule and connector is
# rewritten by an animated reveal (`x = 11p - K`) that runs from the jump table
# at 800C74C0.  For the outline submenu that block is still active once settled,
# so its constants won and the rule stayed at game x 20 - inside the text -
# while the others followed s6/s5 to 0.  Shift the animated constants by the
# same -20.  The connectors' LEFT ends are a separate matter - see CONNECTOR_LEFT
# below; only their animated right ends move here.
ANIM_X = [(0x800C73A0, -46, -66, 'p25 connector right end'),
          (0x800C7424, -24, -44, 'p39 connector right end'),
          (0x800C745C, -46, -66, 'p41 connector right end'),
          (0x800C74FC, -47, -67, 'p26 outline rule  x0/x2'),
          (0x800C7508, -43, -63, 'p26 outline rule  x1/x3'),
          (0x800C7580, -25, -45, 'p40 member rule   x0/x2'),
          (0x800C7584, -21, -41, 'p40 member rule   x1/x3'),
          (0x800C75B8, -47, -67, 'p42 detailed rule x0/x2'),
          (0x800C75C4, -43, -63, 'p42 detailed rule x1/x3')]

# With each submenu's v (half-height) now USA's, the rule constants can simply
# be USA's own: top = s0 - 4 (Integral: s0 - 2), bottoms v - 36 / v - 16 /
# v + 14.  Lengths were already equal (12.8 / 62.8 / 102.8 game px measured in
# both); only the tops moved with the rows.
RULE = [(0x800C6F34, -2, -4, 'operation-outline rule top'),
        (0x800C6F3C, -38, -36, 'operation-outline rule bottom'),
        (0x800C7024, -2, -4, 'operation-member rule top'),
        (0x800C7028, -18, -16, 'operation-member rule bottom'),
        (0x800C7174, -2, -4, 'detailed-information rule top'),
        (0x800C7178, 2, 14, 'detailed-information rule bottom')]

# The three horizontal connectors' LEFT ends (polys 25 / 39 / 41: the line from
# the selected FILE button to the submenu's rule).  Integral starts them at -46,
# -24 and -46; USA at -39, -32 and -27.  Until 2026-09-10 the port kept
# Integral's, on the belief that they "anchor to the FILE column" - but the FILE
# column's boxes are USA's now (br_f00..03 at USA's widths), so the outline and
# detailed lines ran 7 px INTO their box and the member line stopped 8 px short
# of its.  Found by the user on SwanStation and in MC alike; the 26 shot pairs
# that signed the family off compared game x 150-320, and the left ends sit at
# 114-136.  Each end has two writers: the layout block and the reveal animation
# (`x = 11p - K`), and both take USA's value.  p39's layout value lives in
# MEMBER_NEW[6]; p41's layout store shared s7 with p25 (both -46) and now takes
# t1, loaded with -27 in DETAIL_NEW's load-delay slot (t1 is free there: the
# member block's mult is done and the frame function that clobbers t0/t1 is
# only called after the stores).
CONNECTOR_LEFT = [(0x800C6F14, -46, -39, 'p25 outline, layout (s7)'),
                  (0x800C7390, -46, -39, 'p25 outline, reveal'),
                  (0x800C73F0, -24, -32, 'p39 member, reveal'),
                  (0x800C744C, -46, -27, 'p41 detailed, reveal')]
CONNECTOR_LEFT_P41 = [(0x800C717C, 0xA6370670, 0xA6290670, 'p41 x0: sh s7 -> sh t1 (-27)'),
                      (0x800C7184, 0xA6370680, 0xA6290680, 'p41 x2: sh s7 -> sh t1 (-27)')]

HILITE = [(0x800C6944, 10,  5, 'bar top / box bottom'),
          (0x800C6950, 11,  6, 'bar bottom'),
          (0x800C698C, -4,  0, 'box top')]

def xl_patches(int_ovl, usa_ovl):
    from quadscan import scan
    U = {n: xl for a, n, xl, yt, xr, yb, sa in scan(usa_ovl, 0x800C5968, 0x800CC1D0) if n}
    out = {}
    for a, n, xl, yt, xr, yb, sa in scan(int_ovl, BASEADDR, 0x800C983C):
        if not n or not n.startswith('br_s') or n == 'br_s00': continue
        if xl is None or sa is None or U.get(n) is None: continue
        new = U[n] + LINE_DELTA
        if new != xl: out[n] = (sa, xl, new)
    return out
WIDEN = {strcode(n): n for n in quads}
for t in WIDEN: assert t in U, 'missing %s in USA archive' % WIDEN[t]
I0 = {e[0]: e[3] for e in ei}

# ---- target quads -----------------------------------------------------------
# b_select.c: brf_800C983C does setXY4(poly, xl, yt, xr, yb) with the UVs
# spanning the whole texture, so the texture is *stretched* to the quad.  The
# rendered size is therefore the quad's, and a texture only draws at its true
# size when its canvas equals the quad.  Several labels share an immediate
# (same store address), so a group gets one quad sized to its largest member
# and the smaller members are padded out to match.
# Only xr is ever patched.  The quad height is 13 for every br_sNN - retail's
# yb immediates all give exactly 13, and brf_800C69B4 forces 13 at runtime from
# 16 call sites regardless - and 17 for the FILE labels.  Measured against USA:
# the art must be PADDED into that height, never stretched to it, or the label
# renders 13/art_h too tall (the terrorists' armament came out 1.88x).
# brf_800C69B4 drew every br_sNN into a quad of ONE hardcoded height
# (`addiu v1, a2, 13`).  The selection highlight follows that quad, so a fixed
# height gives a fixed highlight - USA's varies because it sizes each box to its
# own label (`above + below + 5` = the texture height, for all 16).
#
# The exact per-label height is already in the poly and the positioner never
# touches it: brf_800C983C sets v0 = tex->off_y and v2 = tex->off_y + tex->h + 1,
# and tex->h is height-1, so **v2 - v0 is the texture height**.  Reading it needs
# four instructions, and the function has them: its x normalisation is entirely
# redundant (setXY4 already leaves x2 == x0 and x1 == x3), so the two `lh` and
# the two `sh` that copy x are dead, as is the nop.  Unlike the poly's y, the
# UVs are stable across frames, so this cannot accumulate.
ROW_H_ADDR = 0x800C69C8
ROW_H_OLD = [0x84440008,   # lh   a0,  8(v0)      x0   <- redundant
             0x84450020,   # lh   a1, 32(v0)      x3   <- redundant
             0x24C3000D,   # addiu v1, a2, 13     the fixed height
             0xA446000A,   # sh   a2, 10(v0)      y0
             0xA4460012,   # sh   a2, 18(v0)      y1
             0xA443001A,   # sh   v1, 26(v0)      y2
             0xA4430022,   # sh   v1, 34(v0)      y3
             0xA4440008,   # sh   a0,  8(v0)      x0 = x0  <- redundant
             0xA4450010,   # sh   a1, 16(v0)      x1 = x3  <- redundant
             0xA4440018,   # sh   a0, 24(v0)      x2 = x0  <- redundant
             0xA4450020]   # sh   a1, 32(v0)      x3 = x3  <- redundant
ROW_H_NEW = [0x9044001D,   # lbu  a0, 29(v0)      v2
             0x9045000D,   # lbu  a1, 13(v0)      v0  (= py % 256)
             0x00000000,   # nop                  LOAD DELAY: a1 is not readable yet
             0x00851823,   # subu v1, a0, a1      v1 = texture height
             0x30A50007,   # andi a1, a1, 7       above = py % 8  (see USA_ABOVE)
             0x00C52023,   # subu a0, a2, a1      top    = y - above
             0x00831821,   # addu v1, a0, v1      bottom = top + height
             0xA444000A,   # sh   a0, 10(v0)      y0
             0xA4440012,   # sh   a0, 18(v0)      y1
             0xA443001A,   # sh   v1, 26(v0)      y2
             0xA4430022]   # sh   v1, 34(v0)      y3
# The nop is the whole 2026-09-10 fix. The R3000 delivers a load one
# instruction late: the instruction right after `lbu a1` still sees the OLD
# a1, which here is the caller's poly index (9..24), so the height came out
# as v2 - idx (100 rows and more) and every label was stretched into a column
# of sampled VRAM. The Master Collection's emulator does not model the delay,
# so the arithmetic ran as written there and the 26 shot pairs it was checked
# against could never show it; SwanStation and hardware do model it. Retail's
# eleven words never read a register in the slot after loading it, and neither
# does any other block this port rewrites - `hazards.py` checks that on every
# build. The x normalisation retail spent four words on is still dropped:
# setXY4 leaves x2 == x0 and x1 == x3 and nothing else writes these x's except
# the br_s00 / br_s01 reveals, which write all four consistently.
# br_s00 has no stored quad: its right edge is animated as x1 = 52n/6 + 26,
# with the 52 baked into a shift/add chain at 800C7658.  Rebuilding the chain
# as 100n (using $at as scratch, same five slots) gives it USA's 100 px width.
S00_ADDR = 0x800C7658
S00_OLD = [0x00051040, 0x00451021, 0x00021080, 0x00451021, 0x00021080]
S00_NEW = [0x00050940, 0x00051180, 0x00411021, 0x00050880, 0x00411021]
# Row advance: read USA's own constants, never guessed.  Rows are laid out by
# accumulating the positioner's return value y + h, where h is its 4th argument.
# USA reworked that function - it takes the box extents as extra stack arguments
# where Integral hardcodes 13 - but the call structure is identical, so USA's
# advances transfer directly.  Both tables are extracted by simulating the
# registers over each overlay (see rowargs.py), so this follows the discs.
INT_FN, USA_BASE, USA_FN = 0x800C69B4, 0x800C5968, 0x800C918C

# br_s03..br_s06 take their advance from USA's s6, which is CONDITIONAL - 20 by
# default (800C96C8) and 17 only on one branch (800C97DC).  Extracting the 17
# and hardcoding it made the operation-member rows 3 game px tighter than USA:
# measured Roy Campbell -> Dr. Naomi at 17.28 game px against USA's 19.89, and
# 19.89 is the 20 path, which is exactly what Integral already had.  So leave
# those four alone.  (br_s02 is `ori a3, s6, 10`, i.e. 27 or 30; its measured
# gap is 27.1, so its patch to 27 stays.)
CONDITIONAL_ADV = {11, 12, 13, 14, 15}      # br_s02..br_s06: set by the member block rewrite (t9)
# br_s02 is `ori a3, s6, 10`, so it follows the same conditional s6: 27 when
# s6 = 17, 30 when s6 = 20.  USA renders the 20 path here (its br_s03..s06 gaps
# measure 20), so br_s02 is 20 | 10 = 30, not the 27 the other branch gives.
ADV_OVERRIDE = {}                           # (br_s02's 30/27 is `ori a3, t9, 10` in the rewrite)

def advance_patches(int_ovl, usa_ovl):
    from rowargs import run_bytes
    usa = {idx: adv for _, idx, adv, _, _ in run_bytes(usa_ovl, USA_BASE, USA_FN)}
    W = list(struct.unpack('<%dI' % (len(int_ovl)//4), int_ovl[:len(int_ovl)//4*4]))
    def at(a): return W[(a - BASEADDR)//4]
    out = []
    for a, idx, adv, _, _ in run_bytes(int_ovl, BASEADDR, INT_FN):
        if idx in CONDITIONAL_ADV: continue     # USA's value here is state-dependent
        want = ADV_OVERRIDE.get(idx, usa.get(idx))
        if want is None or want == adv: continue
        for x in [a+4] + list(range(a, a-0x80, -4)):        # incl. the delay slot
            w = at(x); op = w >> 26
            if op == 9 and ((w >> 16) & 31) == 7: out.append((x, adv, want, idx)); break
            if op == 0 and (w & 0x3F) == 0x21 and ((w >> 11) & 31) == 7:
                out.append((x, adv, want, idx)); break
    return out      # the reason for unanimous approval
from rowargs import run_bytes as _rb
USA_ADV = {}
for _a, _i, _adv, _ab, _be in _rb(pu[0], USA_BASE, USA_FN):
    if _i is not None and 9 <= _i <= 24: USA_ADV[strcode('br_s%02d' % (_i - 9))] = _adv
assert len(USA_ADV) == 16 and all(v for v in USA_ADV.values()), 'USA advances incomplete'
# USA's positioner draws [y - above, y + below + 5]; `above` (2-4) is its 5th
# argument.  Ours reads it back as the texture's VRAM row mod 8, so placement
# below must put each br_sNN at py % 8 == above.
USA_ABOVE = {}
for _a, _i, _adv, _ab, _be in _rb(pu[0], USA_BASE, USA_FN):
    if _i is not None and 9 <= _i <= 24: USA_ABOVE[strcode('br_s%02d' % (_i - 9))] = _ab
assert len(USA_ABOVE) == 16 and all(0 <= v <= 7 for v in USA_ABOVE.values()), USA_ABOVE
def row_ok(tid, py): return tid not in USA_ABOVE or py % 8 == USA_ABOVE[tid]

XL = dict(xl_patches(pi[0], pu[0]))
# nothing may cross the right edge now that xl is USA's
from quadscan import scan as _scan
for _a, _n, _xl, _yt, _xr, _yb, _sa in _scan(pi[0], BASEADDR, 0x800C983C):
    if not _n or not _n.startswith('br_s') or _n not in XL: continue
    _new = XL[_n][2]; _w = geo(U[strcode(_n)])['w']
    assert _new + _w <= RIGHT_LIMIT, '%s would end at %d' % (_n, _new + _w)
for n, (addr, old, new) in XL.items():
    if n in quads: quads[n]['xl'] = [new, addr]     # xr is derived from this

gid = {}
for n, g in quads.items():
    if n not in UNSHARE: gid.setdefault(g['xr'][1], []).append(n)
newimm = {addr: max(quads[n]['xl'][0] + geo(U[strcode(n)])['w'] for n in members)
          for addr, members in gid.items()}
target = {}
for n, g in quads.items():
    if n.startswith('br_f'):
        h = 17
    else:
        h = geo(U[strcode(n)])['h']       # the quad now IS the texture height
    # un-shared labels get their own xr, so the quad is exactly USA's width
    w = geo(U[strcode(n)])['w'] if n in UNSHARE else newimm[g['xr'][1]] - g['xl'][0]
    target[strcode(n)] = (w, h)
# br_s00's right edge is computed at runtime (x1 = t0 + 26, an animated reveal),
# so it has no patchable quad and keeps Integral's slot.
ALL = ['br_s%02d' % i for i in range(16)] + ['br_f%02d' % i for i in range(4)]
for n in ALL:
    t = strcode(n)
    assert t in U and t in I0, 'missing %s' % n
    WIDEN[t] = n
    if t not in target:
        g = geo(I0[t])
        # br_s00 too: its height comes from the same per-label rule
        # br_s00's animated width is patched to USA's below
        w = geo(U[t])['w'] if n == 'br_s00' else g['w']
        target[t] = (w, 17 if n.startswith('br_f') else geo(U[t])['h'])
assert len(target) == 20, 'expected all 20 labels, got %d' % len(target)
json.dump({hex(k): list(v) for k, v in target.items()}, open(WORK + '/brf_target.json', 'w'))
json.dump({hex(k): v for k, v in newimm.items()}, open(WORK + '/brf_imm.json', 'w'))

# ---- VRAM occupancy from everything that is NOT being widened ---------------
grid = bytearray(1024 * 512)
def mark(px, py, uw, h):
    for y in range(py, min(512, py + h)):
        row = y * 1024
        for x in range(px, min(1024, px + uw)): grid[row + x] = 1
def busy(px, py, uw, h):
    if px + uw > 1024 or py + h > 512: return True
    for y in range(py, py + h):
        row = y * 1024
        if any(grid[row + x] for x in range(px, px + uw)): return True
    return False

for e in ei:
    g = geo(e[3])
    if e[0] not in WIDEN: mark(g['px'], g['py'], units(g['w'], g['bpp']), g['h'])
    if g['cy'] < 512: mark(g['cx'], g['cy'], 16, 1)

# ---- place each widened label ----------------------------------------------
place = {}
def tgt(t):
    w, h = target[t]; return w, h, units(w, geo(U[t])['bpp'])
for tid in sorted(WIDEN, key=lambda t: -tgt(t)[2]):
    ig = geo(I0[tid]); tw, th, need = tgt(tid)
    bpp = geo(U[tid])['bpp']
    if ufits(ig['px'], tw, bpp) and vfits(ig['py'], th) and row_ok(tid, ig['py']) and not busy(ig['px'], ig['py'], need, th):
        place[tid] = (ig['px'], ig['py'])
    else:
        # DO NOT move these to rows 256..511. It was tried on 2026-09-10 and
        # is strictly worse: the relocated labels themselves render as
        # garbage while the three that stay in place are fine, so this code
        # path cannot address the lower half of VRAM - the tpage field
        # selects one 256-row half and the briefing assumes the top one.
        # `vfits` permits y >= 256 and that permission is wrong here.
        #
        # The band this search does pick, x 896..958 at y 1..156, is not
        # innocent either: the labels drawn from it are correct, but the
        # briefing's right column is not, and removing en_brf entirely brings
        # the column back. Something the port cannot see owns those bytes.
        # `busy()` models only the `nd` payload's 51 textures, and the
        # content the briefing pages in through brf_800CAC7C() is not in the
        # stage at all - it comes from BRF.DAT by sector. See NextSteps 24.
        found = None
        for page in (896, 960, 832, 768, 704, 640, 576, 512):
            for ny in range(0, 512 - th):
                for nx in range(page, page + 64 - need + 1):
                    if ufits(nx, tw, bpp) and vfits(ny, th) and row_ok(tid, ny) and not busy(nx, ny, need, th):
                        found = (nx, ny); break
                if found: break
            if found: break
        assert found, 'no VRAM room for %s (%d units x %d rows)' % (WIDEN[tid], need, th)
        place[tid] = found
    mark(place[tid][0], place[tid][1], need, th)

print('%-8s %-9s %-14s %-11s %-11s %s' % ('label', 'where', 'VRAM', 'quad', 'USA art', 'fit'))
for tid in sorted(place, key=lambda t: WIDEN[t]):
    ig, ug = geo(I0[tid]), geo(U[tid]); tw, th, _ = tgt(tid)
    tag = 'in place' if place[tid] == (ig['px'], ig['py']) else 'moved'
    fit = 'exact' if (ug['w'], ug['h']) == (tw, th) else (
          'pad %+d,%+d' % (tw - ug['w'], th - ug['h']) if ug['w'] <= tw and ug['h'] <= th
          else 'SCALE to fit')
    print('  %-8s %-9s (%3d,%3d)     %3dx%-3d    %3dx%-3d    %s'
          % (WIDEN[tid], tag, place[tid][0], place[tid][1], tw, th, ug['w'], ug['h'], fit))
json.dump({hex(k): list(v) for k, v in place.items()}, open(WORK + '/brf_widen.json', 'w'))
print('placed %d widened labels' % len(place))
