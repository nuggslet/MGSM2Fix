#!/usr/bin/env python
"""Port the USA VR Missions executable's English into Integral's VR-DISC executable.

Two families, one PPF each, both addressed at the VR disc's executable sectors
(Ketchup mirrors executable writes into RAM; image offset of RAM 0x80010000 is
0x000865F8 on this disc):

  INTEGRAL_vr_en_items.ppf    item and weapon descriptions (menu/item.c,
                              menu/weapon.c: `itm_descriptions`, `wpn_descriptions`)
  INTEGRAL_vr_en_savemsg.ppf  the memory-card captions (menu/datasave.c:
                              `saveCaptions`, `loadCaptions`)

Donor: SLUS_009.57. It is a five-language build: every table exists once per
language behind a table of tables (items 0x8009F0DC, weapons 0x8009F300, both
with English first), and GCL variable 0x11 selects. The English tables are read
directly; nothing is inferred from the pools' order.

Index for index, checked rather than trusted:

  items    Integral 26 entries = USA's 24 + the frozen Ration/Ketchup pair
           (entries 24, 25; `frozenItemsDescriptions`). USA has no frozen
           pair, so those two stay Japanese; unreachable in VR anyway.
  weapons  10 entries in both. Integral's MP5 SD description (a separate
           pointer, 0x800A9220) has no USA counterpart: kept.
  mine     `mineDetectorUnusable` (HARD/EXTREME) is a .data array with no
           USA text: kept (VR has no difficulty level).
  captions the same 12+12 request codes as the main game; USA leaves 1 and 9
           empty (save/load "in progress" and "completed"), so Integral's
           セーブ中です / セーブが完了しました / ロード中です / ロードが完了しました stay,
           exactly as en_savemsg decided. The third small table after
           loadCaptions (上書きしてよろしいですか？ / フォーマットしますか？ /
           FORMAT OK?) has no English in USA: kept.

Every byte of each repacked pool is written (padding included), so a
collection RAM patch landing at a retail-equal byte cannot survive (the SOCOM
line-break lesson of 2026-09-05).

usage: vr_exe.py [--deploy]      (writes work/ always; PPFs to mods with --deploy)
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import struct, sys
from audit_text import game_text
from vrlib import INT_EXE, USA_EXE, WORK, exe_image_offset, ram, fofs, HDR, write_ppf, deploy

# Integral SLPM_862.49 (decomp build, obj_vr/asm.map)
IN_ITEM_TAB, IN_WEAP_TAB = 0x8009C11C, 0x8009C304
IN_FROZEN_TAB = 0x8009C17C
IN_MP5_PTR = 0x800A9220
IN_SAVE_TAB, IN_LOAD_TAB = 0x8009C884, 0x8009C8B4
N_ITEM, N_WEAP, N_CAP = 26, 10, 12
ITEM_ARENA = (0x80010EA0, 0x80011668)     # 26 item descriptions, then 4 bytes of zero
WEAP_ARENA = (0x800116DC, 0x80011B60)     # 10 weapon descriptions and the MP5's; debug strings follow
CAP_ARENA = (0x80011F0C, 0x800120C0)      # the 17 captions; the prompt table's strings follow
# USA SLUS_009.57
US_ITEM_TABS, US_WEAP_TABS = 0x8009F0DC, 0x8009F300   # tables of per-language tables
US_SAVE_TAB, US_LOAD_TAB = 0x8009FA0C, 0x8009FA3C     # found from 'No save file.' / 'Now checking Memory Card.'
US_N_ITEM = 24
KEEP_CAP = {1, 9}


def cstr(d, a):
    o = fofs(a)
    return d[o:d.index(b'\0', o)]


def table(d, a, n):
    return [struct.unpack_from('<I', d, fofs(a) + 4*i)[0] for i in range(n)]


def readable(s):
    t, jp = game_text(s)
    return t if t is not None else repr(s)


def records_for(new, orig):
    """PPF records for every byte that differs, split at 2048-byte payload boundaries"""
    out = []
    p = 0
    while p < len(new):
        if new[p] == orig[p]:
            p += 1
            continue
        q = p
        while q < len(new) and new[q] != orig[q]:
            q += 1
        s = p
        while s < q:
            e = min(q, (s - HDR) // 0x800 * 0x800 + 0x800 + HDR)
            out.append((exe_image_offset(s), bytes(new[s:e])))
            s = e
        p = q
    return out


def pack_pool(new, arena, strings_in_order):
    """zero the arena, lay the strings out 4-aligned in the given order; -> addresses

    No width check here, on purpose. `widths.check_ceiling` is applied to the
    VR menus and the mission windows, where one stored string is one drawn
    line. These pools are not like that: a description holds several lines with
    an embedded separator, and while the English ones break on 0x807C ('|'),
    RETAIL Integral's own Japanese entries in this very arena measure 540 px
    unsplit - so they must break on something else that has not been
    established. Asserting a ceiling here would fail on bytes the game ships
    and works with, which makes it a wrong check rather than a strict one.
    Establishing the Japanese break code is what would make this checkable.
    """
    lo, hi = arena
    for a in range(fofs(lo), fofs(hi)):
        new[a] = 0
    cur, placed = fofs(lo), []
    for s in strings_in_order:
        assert s[-1:] != b'\0'
        b = s + b'\0'
        assert cur + len(b) <= fofs(hi), 'arena %08X overflow' % lo
        new[cur:cur+len(b)] = b
        placed.append(ram(cur))
        cur = (cur + len(b) + 3) & ~3
    return placed, fofs(hi) - cur


def build_items(us, ino):
    new = bytearray(ino)
    us_tabs = table(us, US_ITEM_TABS, 5)
    us_items = [cstr(us, p) for p in table(us, us_tabs[0], US_N_ITEM)]
    def titled(s, name):                      # `B0 14` opens the 《name》 header: 0x9014 with a style flag
        return (s[0] & 0x9F, s[1]) == (0x90, 0x14) and s[2:2+len(name)] == name
    assert titled(us_items[0], b'Cigarettes') and titled(us_items[23], b'Suppressor'), 'USA item table 0 is not English'
    us_wtabs = table(us, US_WEAP_TABS, 5)
    us_weaps = [cstr(us, p) for p in table(us, us_wtabs[0], N_WEAP)]
    assert titled(us_weaps[0], b'Socom Pistol') and titled(us_weaps[9], b'PSG1'), 'USA weapon table 0 is not English'
    in_items = table(ino, IN_ITEM_TAB, N_ITEM)
    in_weaps = table(ino, IN_WEAP_TAB, N_WEAP)
    in_frozen = table(ino, IN_FROZEN_TAB, 2)
    assert in_frozen == in_items[24:26], 'frozen pair is not the item table tail'
    mp5 = struct.unpack_from('<I', ino, fofs(IN_MP5_PTR))[0]
    lo, hi = ITEM_ARENA
    assert all(lo <= p < hi for p in in_items), 'item pointer outside the arena'
    lo2, hi2 = WEAP_ARENA
    assert all(lo2 <= p < hi2 for p in in_weaps) and lo2 <= mp5 < hi2, 'weapon pointer outside the arena'
    # every byte of both arenas must be a string the tables reach (no strangers)
    for arena, ptrs in ((ITEM_ARENA, in_items), (WEAP_ARENA, in_weaps + [mp5])):
        covered = bytearray(arena[1] - arena[0])
        for p in ptrs:
            s = cstr(ino, p)
            for k in range(p, p + len(s) + 1):
                covered[k - arena[0]] = 1
        strangers = [k for k in range(len(covered)) if not covered[k] and ino[fofs(arena[0]) + k]]
        assert not strangers, 'arena %08X has bytes no table reaches: %s' % (arena[0], ['%08X' % (arena[0]+k) for k in strangers[:8]])
    # items: USA's 24 in Integral's slots 0..23 (same order: checked on the anchors above and on the names below)
    names_ok = [(b'Cardboard box A', 2), (b'Mine Detector', 19), (b'Rope', 21)]
    for n, i in names_ok:
        assert us_items[i][2:2+len(n)] == n, (n, i, us_items[i][:30])
    want_items = us_items + [cstr(ino, in_items[24]), cstr(ino, in_items[25])]
    order = sorted(range(N_ITEM), key=lambda i: in_items[i])       # keep the original layout order
    placed, slack_a = pack_pool(new, ITEM_ARENA, [want_items[i] for i in order])
    addr_items = dict(zip(order, placed))
    for i in range(N_ITEM):
        struct.pack_into('<I', new, fofs(IN_ITEM_TAB) + 4*i, addr_items[i])
    struct.pack_into('<I', new, fofs(IN_FROZEN_TAB), addr_items[24])
    struct.pack_into('<I', new, fofs(IN_FROZEN_TAB) + 4, addr_items[25])
    # weapons: USA's 10 plus Integral's MP5 text
    want_weaps = us_weaps + [cstr(ino, mp5)]
    worder = sorted(range(N_WEAP + 1), key=lambda i: (in_weaps + [mp5])[i])
    placed, slack_b = pack_pool(new, WEAP_ARENA, [want_weaps[i] for i in worder])
    addr_w = dict(zip(worder, placed))
    for i in range(N_WEAP):
        struct.pack_into('<I', new, fofs(IN_WEAP_TAB) + 4*i, addr_w[i])
    struct.pack_into('<I', new, fofs(IN_MP5_PTR), addr_w[N_WEAP])
    # verify by reading back the way the game does
    chk = bytes(new)
    for i in range(N_ITEM):
        assert cstr(chk, table(chk, IN_ITEM_TAB, N_ITEM)[i]) == want_items[i]
    for i in range(N_WEAP):
        assert cstr(chk, table(chk, IN_WEAP_TAB, N_WEAP)[i]) == want_weaps[i]
    assert cstr(chk, struct.unpack_from('<I', chk, fofs(IN_MP5_PTR))[0]) == want_weaps[N_WEAP]
    assert table(chk, IN_FROZEN_TAB, 2) == table(chk, IN_ITEM_TAB, N_ITEM)[24:26]
    print('items: arena slack %d bytes; weapons: arena slack %d bytes' % (slack_a, slack_b))
    for i in range(N_ITEM):
        print('  item %2d %s' % (i, readable(want_items[i])[:78]))
    for i in range(N_WEAP + 1):
        print('  weap %2d %s' % (i, readable(want_weaps[i])[:78]))
    return chk


def build_captions(us, ino):
    new = bytearray(ino)
    us_save = [cstr(us, p) for p in table(us, US_SAVE_TAB, N_CAP)]
    us_load = [cstr(us, p) for p in table(us, US_LOAD_TAB, N_CAP)]
    assert us_load[4] == b'No save file.' and us_save[6] == b'Data saved.' and us_save[2] == b'Save failed.'
    assert us_save[10] == us_load[10] == b'Now checking Memory Card.'
    assert all(us_save[i] == b'' and us_load[i] == b'' for i in KEEP_CAP), 'USA has text where Integral-only text was assumed'
    assert us_save[0] == us_load[0] == b''
    in_save = table(ino, IN_SAVE_TAB, N_CAP)
    in_load = table(ino, IN_LOAD_TAB, N_CAP)
    empty = in_save[0]
    assert in_load[0] == empty
    lo, hi = CAP_ARENA
    assert all(lo <= p < hi or p == empty for p in in_save + in_load), 'a caption lies outside the arena'
    covered = bytearray(hi - lo)
    for p in in_save + in_load:
        if p == empty:
            continue
        s = cstr(ino, p)
        for k in range(p, p + len(s) + 1):
            covered[k - lo] = 1
    strangers = [k for k in range(len(covered)) if not covered[k] and ino[fofs(lo) + k]]
    assert not strangers, 'caption arena has bytes no table reaches'
    want = [[None]*N_CAP, [None]*N_CAP]
    src = ((in_save, us_save), (in_load, us_load))
    for t, (itab, utab) in enumerate(src):
        for i in range(N_CAP):
            if itab[i] == empty:
                continue
            want[t][i] = cstr(ino, itab[i]) if i in KEEP_CAP else utab[i]
            assert want[t][i] != b''
    # unique strings, laid out in the order the originals appear
    uniq = []
    for t in (0, 1):
        for i in range(N_CAP):
            s = want[t][i]
            if s is not None and s not in uniq:
                uniq.append(s)
    placed, slack = pack_pool(new, CAP_ARENA, uniq)
    addr = dict(zip(uniq, placed))
    for t, tab in ((0, IN_SAVE_TAB), (1, IN_LOAD_TAB)):
        for i in range(N_CAP):
            s = want[t][i]
            struct.pack_into('<I', new, fofs(tab) + 4*i, empty if s is None else addr[s])
    chk = bytes(new)
    for t, tab in ((0, IN_SAVE_TAB), (1, IN_LOAD_TAB)):
        got = [cstr(chk, p) for p in table(chk, tab, N_CAP)]
        for i in range(N_CAP):
            exp = b'' if want[t][i] is None else want[t][i]
            assert got[i] == exp, (t, i, got[i], exp)
    print('captions: arena slack %d bytes, %d strings' % (slack, len(uniq)))
    for t, name in ((0, 'save'), (1, 'load')):
        for i in range(N_CAP):
            s = want[t][i]
            print('  %s %2d %s%s' % (name, i, '' if s is None else readable(s), '  (kept: USA empty)' if i in KEEP_CAP else ''))
    return chk


def main():
    us = open(USA_EXE, 'rb').read()
    ino = open(INT_EXE, 'rb').read()
    assert len(ino) == 630784 and len(us) == 643072
    assert ino[0x800:0x80B] == b'SLPM_862.49' and us[0x800:0x80B] == b'SLUS_009.57'
    out = {}
    for name, builder, desc in (('INTEGRAL_vr_en_items.ppf', build_items, 'MGS Integral VR-DISC: English item text'),
                                ('INTEGRAL_vr_en_savemsg.ppf', build_captions, 'MGS Integral VR-DISC: English memory card text')):
        new = builder(us, ino)
        recs = records_for(new, ino)
        data = write_ppf(_os.path.join(WORK, name), recs, desc)
        print('%s: %d records, %d bytes changed, %d bytes PPF' % (name, len(recs), sum(len(d) for _, d in recs), len(data)))
        out[name] = data
        open(_os.path.join(WORK, name.replace('.ppf', '.exe')), 'wb').write(new)
    if '--deploy' in sys.argv:
        for name, data in out.items():
            print('deployed', deploy(name, data))


if __name__ == '__main__':
    main()
