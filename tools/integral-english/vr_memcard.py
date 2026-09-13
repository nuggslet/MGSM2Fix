#!/usr/bin/env python
"""Port the USA VR Missions memory-card captions into Integral's `vrsave` and `selectvr` overlays.

Found 2026-09-11, from a CLEAR DATA screenshot: the VR disc's memory-card
module (`savemngr.c` - SAVE DATA / LOAD DATA / SAVE REPLAY DATA / LOAD REPLAY
DATA) is compiled into THREE stage overlays, each with its own copy of the 12
save and 12 load captions and the two prompts, exactly like the main game's
PHOTO ALBUM (`en_camsave`) and the VR `camera` stage (`vr_en_camsave`). The
executable's tables (`vr_en_savemsg`) are a fourth copy and were the only one
ported; the three in the overlays stayed Japanese. `vr_sweep.py` could not see
them: they are overlay pools, not GCL records.

    stage      Integral slots (overlay offset)     USA English slots      USA
    vrsave     save 0x8B8  load 0x8E8  prompts 0x918   0xA58  0xA88  0xAB8   English
    selectvr   save 0xD6B8 load 0xD6E8 prompts 0xD718  0xD978 0xD9A8 0xD9D8  English
    vrtitle    save 0x948  load 0x978  prompts 0x9A8   0x8D0 ...              JAPANESE

`vrtitle` is the CLEAR DATA screen, and USA's own `vrtitle` carries the
identical Japanese in its table - USA VR Missions draws セーブファイルがありません。
under NO FILE too - so by the standing rule it stays (README, "What stays
Japanese": "USA carries the identical Japanese"). The other two take USA's
text index for index.

Indices 0, 1 and 9 are empty in USA's tables (the "in progress" and
"completed" captions), so Integral's セーブ中です / セーブが完了しました /
ロード中です / ロードが完了しました stay, as `en_savemsg` decided for the main
game and `vr_camera` for the album; both prompts (上書きしてよろしいですか？ /
フォーマットしますか？) are empty in USA and stay. `FORMAT OK?` / `OVERWRITE OK?`
follow the prompts in the table and are English already.

Integral's pool for each module is the run of Japanese captions ending where
`FORMAT OK?` begins; the English plus the kept Japanese fit inside it, so each
stage is patched in place and keeps its sector count. USA's tables are
identified by their strings, never by position. Overlays load at a fixed
address: Integral VR 0x800C11A0, USA VR 0x800C4350.

usage: vr_memcard.py [--deploy]     (writes work/ always; the PPF to mods with --deploy)
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import struct, sys
import portio
from audit_text import game_text
from vrlib import INT_STAGE, USA_STAGE, int_disc, stage_lba, stage_bytes, inplace_records, write_ppf, deploy, WORK
import widths

PPF_NAME = 'INTEGRAL_vr_en_memcard.ppf'
DESC = 'MGS Integral VR-DISC: English memory card menus'
BASE_INT, BASE_USA = 0x800C11A0, 0x800C4350
N_CAP, N_PROMPT = 12, 2
KEEP = {1, 9}

# stage -> (Integral save, load, prompts, pool lo, pool hi), (USA save, load, prompts)
STAGES = {
    'vrsave':   ((0x8B8,  0x8E8,  0x918,  0x0FF4C, 0x10138), (0xA58,  0xA88,  0xAB8)),
    'selectvr': ((0xD6B8, 0xD6E8, 0xD718, 0x1F264, 0x1F450), (0xD978, 0xD9A8, 0xD9D8)),
}


def overlay(sd, name):
    tags, pay, offs = portio.stage(sd, name)
    assert tags[0][1] == ord('s'), 'first payload is not the overlay'
    return tags, pay, pay[0]


def slot(ov, base, off):
    w = struct.unpack_from('<I', ov, off)[0]
    if not (base <= w < base + len(ov)):
        raise ValueError('slot %X is not a pointer into the overlay: %08X' % (off, w))
    o = w - base
    return o, ov[o:ov.index(b'\0', o)]


def table(ov, base, off, n):
    return [slot(ov, base, off + 4*i) for i in range(n)]


def readable(s):
    t, jp = game_text(s)
    return t if t is not None else repr(s)


def port_stage(name, isd, usd):
    (I_SAVE, I_LOAD, I_PROMPTS, lo, hi), (U_SAVE, U_LOAD, U_PROMPTS) = STAGES[name]
    itags, ipay, iov = overlay(isd, name)
    utags, upay, uov = overlay(usd, name)
    I = dict(save=table(iov, BASE_INT, I_SAVE, N_CAP), load=table(iov, BASE_INT, I_LOAD, N_CAP),
             prompts=table(iov, BASE_INT, I_PROMPTS, N_PROMPT))
    U = dict(save=table(uov, BASE_USA, U_SAVE, N_CAP), load=table(uov, BASE_USA, U_LOAD, N_CAP),
             prompts=table(uov, BASE_USA, U_PROMPTS, N_PROMPT))
    # the tables are identified by content, not assumed
    assert U['save'][2][1] == b'Save failed.' and U['load'][4][1] == b'No save file.', name
    assert U['save'][10][1] == U['load'][10][1] == b'Now checking Memory Card.', name
    assert all(U['save'][i][1] == b'' and U['load'][i][1] == b'' for i in KEEP), name
    assert U['save'][0][1] == U['load'][0][1] == b'', name
    assert all(s == b'' for _, s in U['prompts']), (name, 'USA has prompt text; the keep decision needs revisiting')
    assert I['save'][0][1] == I['load'][0][1] == b'', name
    # Integral's captions all lie in the pool, and the pool ends at FORMAT OK?
    for t in ('save', 'load', 'prompts'):
        for i, (o, s) in enumerate(I[t]):
            if s == b'':
                continue
            assert lo <= o < hi, '%s %s[%d] at +%X lies outside the pool +%X..+%X' % (name, t, i, o, lo, hi)
    assert iov[hi:hi + 10] == b'FORMAT OK?', (name, iov[hi:hi + 10])
    # every Japanese string in the pool is referenced by one of the three tables, so nothing else reads it
    referenced = {o for t in I.values() for o, s in t if s}
    p = lo
    while p < hi:
        e = iov.index(b'\0', p)
        if e > p:
            assert p in referenced, '%s: pool string at +%X is not in any table' % (name, p)
        p = e + 1

    want = {}
    for t in ('save', 'load'):
        for i in range(N_CAP):
            if i == 0:
                want[(t, i)] = b''
            elif i in KEEP:
                want[(t, i)] = I[t][i][1]
            else:
                assert U[t][i][1] != b'', (name, t, i)
                want[(t, i)] = U[t][i][1]
    for i in range(N_PROMPT):
        want[('prompts', i)] = I['prompts'][i][1]

    widths.check_ceiling([s for s in want.values() if s], '%s pool' % name, widths.font(INT_STAGE))

    new = bytearray(iov)
    for a in range(lo, hi):
        new[a] = 0
    placed, cur = {b'': lo}, lo + 1
    for k in [k for k in want if want[k] != b'']:
        s = want[k]
        if s in placed:
            continue
        b = s + b'\0'
        assert cur + len(b) <= hi, '%s: pool overflow at %s' % (name, k)
        new[cur:cur + len(b)] = b
        placed[s] = cur
        cur += len(b)
    used = cur - lo
    bases = dict(save=I_SAVE, load=I_LOAD, prompts=I_PROMPTS)
    for (t, i), s in want.items():
        struct.pack_into('<I', new, bases[t] + 4*i, BASE_INT + placed[s])
    assert new[lo] == 0
    new = bytes(new)
    print('== %s' % name)
    for t, n in (('save', N_CAP), ('load', N_CAP), ('prompts', N_PROMPT)):
        got = table(new, BASE_INT, bases[t], n)
        for i in range(n):
            assert got[i][1] == want[(t, i)], (name, t, i, got[i][1], want[(t, i)])
            kept = want[(t, i)] and want[(t, i)] == I[t][i][1]
            print('  %-7s %2d %s%s' % (t, i, readable(want[(t, i)]), '  (kept)' if kept else ''))
    print('  pool: %d of %d bytes used' % (used, hi - lo))
    assert len(new) == len(iov)
    changed = [p for p in range(len(iov)) if new[p] != iov[p]]
    assert all(lo <= p < hi or I_SAVE <= p < I_SAVE + 4*N_CAP or I_LOAD <= p < I_LOAD + 4*N_CAP
               or I_PROMPTS <= p < I_PROMPTS + 4*N_PROMPT for p in changed), name
    payloads = dict(ipay)
    payloads[0] = new
    new_stage = portio.pack_stage(itags, payloads)
    old_stage = stage_bytes(isd, name)
    assert len(new_stage) == len(old_stage)
    assert portio.pack_stage(itags, ipay) == old_stage, 'repacking the untouched stage does not reproduce it'
    return old_stage, new_stage


def build():
    isd = open(INT_STAGE, 'rb').read()
    usd = open(USA_STAGE, 'rb').read()
    disc = int_disc()
    records = []
    for name in STAGES:
        old_stage, new_stage = port_stage(name, isd, usd)
        lba = stage_lba(disc, isd, name)
        recs = inplace_records(lba, old_stage, new_stage)
        print('  %s: %d records, %d bytes changed' % (name, len(recs), sum(len(d) for _, d in recs)))
        records += recs
    records.sort()
    return records


def main():
    records = build()
    data = write_ppf(_os.path.join(WORK, PPF_NAME), records, DESC)
    print('%s: %d records, %d bytes changed' % (PPF_NAME, len(records), sum(len(d) for _, d in records)))
    if '--deploy' in sys.argv:
        print('deployed', deploy(PPF_NAME, data))


if __name__ == '__main__':
    main()
