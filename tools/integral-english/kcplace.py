"""Allocate VRAM and CLUT slots for USA's eight KEY CONFIG labels inside
Integral's option stage.

Integral's own eight slots are freed first (they are being replaced), then each
USA texture is placed largest-first. Constraints, per the sc_text work:
  * one tpage: a 4bpp texture must satisfy (px % 64) * 4 + w <= 256
                                       and  py % 256 + h <= 256
  * VRAM is 1024 x 512 sixteen-bit words; a 4bpp row of w pixels is w/4 words
  * the display area and every other texture must not be touched
CLUTs are 16 entries -> 16 words on one row, at a 16-word-aligned x.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK
from optscan import stage, parse, geo, strcode

LAB = ['key_sykan', 'key_button', 'key_syukan', 'key_buki', 'key_reverse',
       'key_normal', 'key_action', 'key_hohuku']
# the two framebuffers: 320x240 16bpp each, at (0,0) and (0,256) -> 320 words wide
FB = [(0, 0, 320, 240), (0, 256, 320, 240)]

def words(g): return (g['w'] * g['bpp'] + 15) // 16

def inventory(sd):
    sect, tags, F, pay = stage(sd, 'option')
    e, _ = parse(pay[1])
    ids = {strcode(n) & 0xFFFFFFFF: n for n in LAB}
    out = []
    for x in e:
        try: g = geo(x[3])
        except Exception: continue
        out.append((x[0], ids.get(x[0]), g, x[3]))
    return out

def build_maps(inv, free_names):
    occ = set(); clut = {}
    for x0, y0, w, h in FB:
        for y in range(y0, y0 + h):
            for x in range(x0, x0 + w): occ.add((x, y))
    for tid, name, g, _b in inv:
        if name in free_names: continue
        for y in range(g['py'], g['py'] + g['h']):
            for x in range(g['px'], g['px'] + words(g)): occ.add((x, y))
        clut[(g['cx'], g['cy'])] = name or '0x%X' % tid
    return occ, clut

def fits(occ, px, py, wd, h):
    # UVs are 8-bit and SetPacketTexture computes u1 = off_x + w, v1 = off_y + h
    # (DG_SetTexture: off_x = (px % 64) * 4, off_y = py % 256, tex->w = w - 1).
    # Either reaching 256 wraps to 0 and the quad then samples the whole page,
    # which renders as garbage - so both must stay <= 255.
    if (px % 64) * 4 + wd * 4 > 255: return False
    if py % 256 + h > 255: return False
    if px + wd > 1024 or py + h > 512: return False
    for y in range(py, py + h):
        for x in range(px, px + wd):
            if (x, y) in occ: return False
    return True

# The option screen's font KCBs occupy x 768..960, y 256..340 (opt.c: rect.x
# 768/832/896, w 64, h 21 stacked, CLUTs at y 276) - never place there.
FONT = (768, 256, 1024, 344)

def in_font(px, py, wd, h):
    fx0, fy0, fx1, fy1 = FONT
    return not (px + wd <= fx0 or px >= fx1 or py + h <= fy0 or py >= fy1)

def place(occ, wd, h, prefer=None):
    if prefer and fits(occ, prefer[0], prefer[1], wd, h) and not in_font(prefer[0], prefer[1], wd, h):
        return prefer
    # prefer the band the option stage already keeps these labels in, then the
    # rest of the texture area, and only then the gap between the framebuffers
    for lo, hi in ((460, 512), (344, 460), (240, 256), (0, 240)):
        for py in range(lo, hi):
            for px in range(0, 1024 - wd):
                if fits(occ, px, py, wd, h) and not in_font(px, py, wd, h): return (px, py)
    return None

def take(occ, px, py, wd, h):
    for y in range(py, py + h):
        for x in range(px, px + wd): occ.add((x, y))

def clut_slot(occ, clut, nc):
    need = 16
    for lo, hi in ((233, 245), (245, 256), (460, 512), (344, 460)):
        for cy in range(lo, hi):
            for cx in range(0, 1024 - need, 16):
                if (cx, cy) in clut: continue
                if in_font(cx, cy, need, 1): continue
                if any((x, cy) in occ for x in range(cx, cx + need)): continue
                return (cx, cy)
    return None

# Two labels are PADDED to their quad's width rather than having the quad
# changed (the overlay has no room): allocate for the padded width.
PAD_TO = {}          # not needed: every rectangle now equals its art (USA's own)

def allocate():
    I = inventory(WORK + '/int1_stage.dir'); U = inventory(WORK + '/usa1_stage.dir')
    ug = {n: dict(g) for _t, n, g, _b in U if n}
    for n, w in PAD_TO.items(): ug[n]['w'] = w
    ig = {n: g for _t, n, g, _b in I if n}
    occ, clut = build_maps(I, set(LAB))
    out = {}
    for n in LAB:                                       # LAB is ordered largest-first
        g = ug[n]; wd = words(g)
        pos = place(occ, wd, g['h'], prefer=(ig[n]['px'], ig[n]['py']))
        assert pos, 'no VRAM for %s (%dx%d)' % (n, g['w'], g['h'])
        take(occ, pos[0], pos[1], wd, g['h'])
        cs = clut_slot(occ, clut, g['nc'])
        assert cs, 'no CLUT slot for %s' % n
        clut[cs] = n
        out[n] = {'vram': pos, 'clut': cs, 'w': g['w'], 'h': g['h'],
                  'words': wd, 'nc': g['nc'],
                  'was_vram': (ig[n]['px'], ig[n]['py']), 'was_clut': (ig[n]['cx'], ig[n]['cy'])}
    return out

if __name__ == '__main__':
    a = allocate()
    print('%-12s %-9s %-14s %-14s %-14s %s' % ('label', 'USA size', 'VRAM', "(Integral's)", 'CLUT', 'reused slot?'))
    for n in ['key_button','key_sykan','key_syukan','key_normal','key_reverse','key_action','key_buki','key_hohuku']:
        d = a[n]
        print('%-12s %-9s (%4d,%4d)   (%4d,%4d)     (%4d,%4d)   %s'
              % (n, '%dx%d' % (d['w'], d['h']), d['vram'][0], d['vram'][1],
                 d['was_vram'][0], d['was_vram'][1], d['clut'][0], d['clut'][1],
                 'yes' if d['vram'] == d['was_vram'] else 'moved'))
