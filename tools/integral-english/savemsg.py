#!/usr/bin/env python
"""Port MGS1 (USA)'s memory-card messages into MGS Integral's executable.

`menu/datasave.c` keeps two 12-entry caption tables, `saveCaptions_8009EB4C`
and `loadCaptions_8009EB7C`, indexed by the low byte of the save/load request
code (`captions[(unsigned char)dword_800ABB58]`).  The request codes are the
same in both games, so the index means the same state in both, and the port is
index for index:

    idx  Integral                                USA
      0  ""                                      ""
      1  セーブが完了しました。 (Save completed.)  ""          <- kept: USA shows nothing
      2  セーブできませんでした。                  Save failed.
      3  エラーが発生しました。                    Error occured while saving.
      4  空きブロックがたりません。                No empty block.
         (load table: セーブファイルがありません。 No save file.)
      5  メモリーカードが初期化されていません。    Memory Card is not formated.
      6  セーブしました。                          Data saved.
      7  フォーマットに失敗しました。              Formating failed.
      8  メモリーカードがさされていません。        Memory Card undetected.
      9  セーブ中です。 (Saving.)                 ""          <- kept
     10  メモリーカードをチェックしています。      Now checking Memory Card.
     11  フォーマットしています。                  Now formating Memory Card.

Where USA has no text (1 and 9, and their load-table twins) Integral's Japanese
stays: those are Integral-only messages, and the rule is verbatim USA text where
USA has it, Integral's own text otherwise.  Everything else in the module is
already English in Integral (SAVING..., LOAD DATA, NO FILE, YES/NO, OVERWRITE
OK?, FORMAT OK?, EZ/NM/HD/EX) or Integral-only Japanese drawn alongside the
English (the 上書きしますか？ prompt), and is left alone.

Mechanism: same as the item descriptions (mkpatch.py).  The 17 Japanese strings
sit in one pool in .rodata; the English (262 bytes) plus the four kept Japanese
strings (76) fit in it (435), so the pool is repacked in place and the two
pointer tables rewritten.  The PPF addresses the executable's sectors on each
disc image; Ketchup mirrors executable writes into RAM (the Master Collection
never re-reads the executable from disc).  Both Integral discs carry the same
executable bytes for this module.

usage: savemsg.py [--deploy]     (writes work/ always; PPFs to mods with --deploy)
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
import os, struct, sys

TADDR, HDR = 0x80010000, 0x800
US_EXE, IN_EXE = WORK + '/us1.exe', WORK + '/int1.exe'
IN_SAVE_TAB, IN_LOAD_TAB = 0x8009EB4C, 0x8009EB7C
IN_POOL = (0x80011F18, 0x800120CB)          # 17 Japanese strings, NUL-terminated, contiguous
N = 12
# USA: the tables are found from the strings they must contain, never assumed
US_LOAD_ANCHOR = (4, b'No save file.')       # load table index 4
US_SAVE_ANCHOR = (6, b'Data saved.')         # save table index 6
KEEP = {1, 9}                                # USA draws nothing here; Integral's text stays
INTEGRAL_BASE = {0: 0x131D2238, 1: 0x0EB38078}   # image offset of RAM 0x80010000 per disc (Ketchup)
SECTOR_DATA, SECTOR_RAW = 0x800, 0x930
MODS = os.path.join(GAME, 'mods/INTEGRAL/INTEGRAL')
NAMES = {0: 'INTEGRAL_disc1_en_savemsg.ppf', 1: 'INTEGRAL_disc2_en_savemsg.ppf'}

ram = lambda fo: TADDR + fo - HDR
fofs = lambda a: a - TADDR + HDR


def cstr(d, a):
    o = fofs(a); return d[o:d.index(b'\x00', o)]


def table(d, a, n=N):
    return [struct.unpack_from('<I', d, fofs(a) + 4*i)[0] for i in range(n)]


def find_us_table(us, anchor):
    idx, text = anchor
    saddr = ram(us.index(text + b'\x00'))
    hits = [fo for fo in range(0, len(us) - 4, 4) if struct.unpack_from('<I', us, fo)[0] == saddr]
    assert len(hits) == 1, 'anchor %r referenced %d times' % (text, len(hits))
    return ram(hits[0] - 4*idx)


def image_offset(base, fo):
    k = fo - HDR
    return base + (k // SECTOR_DATA) * SECTOR_RAW + (k % SECTOR_DATA)


def ppf3(records, desc):
    out = bytearray(b'PPF30' + bytes([2]) + desc.encode('ascii')[:50].ljust(50, b'\x00') + bytes(4))
    for off, data in records:
        for i in range(0, len(data), 255):
            c = data[i:i+255]; out += struct.pack('<Q', off + i) + bytes([len(c)]) + c
    return bytes(out)


def main():
    deploy = '--deploy' in sys.argv
    us = open(US_EXE, 'rb').read(); ino = open(IN_EXE, 'rb').read(); new = bytearray(ino)

    us_save = table(us, find_us_table(us, US_SAVE_ANCHOR)); us_load = table(us, find_us_table(us, US_LOAD_ANCHOR))
    in_save = table(ino, IN_SAVE_TAB); in_load = table(ino, IN_LOAD_TAB)
    U = [[cstr(us, p) for p in t] for t in (us_save, us_load)]
    I = [[cstr(ino, p) for p in t] for t in (in_save, in_load)]
    # the state alignment, checked rather than trusted
    assert U[0][0] == U[1][0] == b'' and I[0][0] == I[1][0] == b''
    assert all(U[t][i] == b'' for t in (0, 1) for i in KEEP), 'USA has text where Integral-only text was assumed'
    assert all(U[t][i] != b'' for t in (0, 1) for i in range(2, 12) if i not in KEEP), 'USA missing text at a ported index'
    assert U[1][4] == b'No save file.' and U[0][6] == b'Data saved.' and U[0][2] == b'Save failed.' and U[1][2] == b'Load failed.'
    assert U[0][10] == U[1][10] == b'Now checking Memory Card.' and U[0][11] == U[1][11] == b'Now formating Memory Card.'

    # pool: everything the tables point at, except the shared empty string
    lo, hi = IN_POOL
    empty = in_save[0]
    assert all(lo <= p < hi or p == empty for p in in_save + in_load), 'a caption lies outside the pool'
    for a in range(fofs(lo), fofs(hi)): new[a] = 0

    # choose each entry's text, pool identical strings once, lay them out in table order
    want = [[None]*N, [None]*N]
    for t in (0, 1):
        for i in range(N):
            if in_save[i] == empty and t == 0 or in_load[i] == empty and t == 1:
                want[t][i] = None
            elif i in KEEP:
                want[t][i] = I[t][i]
            else:
                want[t][i] = U[t][i]
    placed, cur = {}, fofs(lo)
    newtabs = [[empty]*N, [empty]*N]
    for t in (0, 1):
        for i in range(N):
            s = want[t][i]
            if s is None: continue
            if s not in placed:
                b = s + b'\x00'
                assert cur + len(b) <= fofs(hi), 'pool overflow at table %d entry %d' % (t, i)
                new[cur:cur+len(b)] = b; placed[s] = ram(cur); cur += len(b)
            newtabs[t][i] = placed[s]
    used = cur - fofs(lo)
    for t, tab in ((0, IN_SAVE_TAB), (1, IN_LOAD_TAB)):
        for i in range(N): struct.pack_into('<I', new, fofs(tab) + 4*i, newtabs[t][i])

    # report
    print('pool %d of %d bytes used; %d strings' % (used, fofs(hi) - fofs(lo), len(placed)))
    for t, name in ((0, 'save'), (1, 'load')):
        print('== %s table' % name)
        for i in range(N):
            s = cstr(bytes(new), newtabs[t][i])
            tag = 'kept (Integral only)' if i in KEEP else ('' if s == b'' else 'USA')
            print('  %2d  %-32s %s' % (i, s.decode('latin1') if all(0x20 <= c < 0x7F for c in s) else '<%d Japanese bytes>' % len(s), tag))

    # verify: re-read the patched image the way the game does
    chk = bytes(new)
    for t, tab in ((0, IN_SAVE_TAB), (1, IN_LOAD_TAB)):
        got = [cstr(chk, p) for p in table(chk, tab)]
        for i in range(N):
            exp = b'' if newtabs[t][i] == empty else want[t][i]
            assert got[i] == exp, (t, i, got[i], exp)
    assert len(chk) == len(ino) and all(chk[k] == ino[k] for k in range(len(ino)) if not (fofs(lo) <= k < fofs(hi) or fofs(IN_SAVE_TAB) <= k < fofs(IN_LOAD_TAB) + 4*N)), 'bytes changed outside the pool and tables'

    # emit every byte of the pool and of both tables, changed or not, addressed
    # at each disc's executable sectors. Until 2026-09-05 only differing runs
    # were emitted; the collection's own RAM patches rewrite six 14-byte spans
    # inside this pool before Ketchup's pass, and Ketchup writes only the bytes a
    # PPF names, so a byte the English shared with retail could keep the
    # collection's value (that is exactly how the SOCOM description broke, see
    # items.py). Owning the whole pool also puts every byte under Ketchup::Audit.
    runs = [[fofs(lo), fofs(hi)], [fofs(IN_SAVE_TAB), fofs(IN_LOAD_TAB) + 4*N]]
    assert not any(chk[k] != ino[k] for k in range(len(ino)) if not any(a <= k < b for a, b in runs)), 'bytes changed outside the pool and tables'
    os.makedirs(WORK, exist_ok=True)
    open(WORK + '/int1_savemsg.exe', 'wb').write(chk)
    # A run may cross a 2048-byte payload boundary; the image has 304 bytes of
    # sector tail there, and Ketchup drops any byte whose in-sector position is
    # >= 2048, so every record must stay inside one sector's payload.
    def records(base):
        out = []
        for a, b in runs:
            while a < b:
                e = min(b, a + SECTOR_DATA - ((a - HDR) % SECTOR_DATA))
                out.append((image_offset(base, a), chk[a:e])); a = e
        return out
    for disc, base in INTEGRAL_BASE.items():
        recs = records(base)
        for off, data in recs:      # Ketchup's own rule, replayed
            k = off - base
            assert k % SECTOR_RAW + len(data) <= SECTOR_DATA, 'record crosses a sector tail'
        blob = ppf3(recs, 'MGS Integral: English memory card messages')
        p = WORK + '/%s' % NAMES[disc]; open(p, 'wb').write(blob)
        print('disc %d: %d runs -> %d records, %d bytes -> %s' % (disc + 1, len(runs), len(recs), sum(len(d) for _, d in recs), p))
        if deploy:
            d = os.path.join(MODS, str(disc), NAMES[disc]); open(d, 'wb').write(blob); print('   deployed %s' % d)
    if not deploy: print('\nNOT DEPLOYED. Re-run with --deploy to install.')


if __name__ == '__main__':
    main()
