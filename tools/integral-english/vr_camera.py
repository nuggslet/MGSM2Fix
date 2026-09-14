#!/usr/bin/env python
"""Port the USA VR Missions ALBUM / SAVE PHOTO captions into Integral's VR `camera` overlay.

The VR disc's `camera` stage (PHOTO DATA / SAVE PHOTO / ALBUM) draws its
memory-card captions from pointer tables inside its overlay, exactly like the
main game's PHOTO ALBUM (`en_camsave`):

    Integral slots (overlay offset)           USA English slots
    0x608  save captions, 12 entries          0x7A8
    0x638  load captions, 12 entries          0x7D8
    0x668  上書きしてよろしいですか？           0x808  (empty in USA: kept)
    0x66C  フォーマットしますか？               0x80C  (empty in USA: kept)
    0x708  the photo save flow, 7 entries      0x8E4  Overwrite? / Now saving. /
           (overwrite? / saving / error /             Error occured while saving. /
           completed / card missing /                 Save completed. / Memory Card
           failed / failed)                           undetected. / Save failed. x2

USA's overlay carries all five languages (Spanish at 0x608 ... English at
0x7A8), which is why the slots differ; the English tables were identified by
their strings, never by position. Indices 1 and 9 of the save/load tables are
empty in USA (the "in progress" and "completed" captions) and stay Integral's,
as en_savemsg decided for the main game. `Now saving.` and `Save completed.`
ARE English in the photo flow table, so that flow - the one the user sees when
saving a photo - becomes English.

Overlays load at a fixed address: Integral VR 0x800C11A0 (the executable's
_bss_objend), USA VR 0x800C4350. The Japanese pool at +0xCB1C..+0xCD08 takes
the English plus the kept Japanese with room to spare, so the stage keeps its
38 sectors and is patched in place. `OVERWRITE OK?` / `FORMAT OK?` / `YES` /
`NO` / `COMPLETE` / `ERROR` / `MEMORY CARD 1/2` / `SAVE DATA` are already
English and untouched. The one Japanese record in the stage's script
(変更内容を上書き保存しますか？ under OVERWRITE OK?) has an empty USA record and
stays, by the same rule.

usage: vr_camera.py [--deploy]
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import struct, sys
import portio
from audit_text import game_text
from vrlib import INT_STAGE, USA_STAGE, int_disc, stage_lba, stage_bytes, inplace_records, write_ppf, deploy, WORK
import widths

PPF_NAME = 'INTEGRAL_vr_en_camsave.ppf'
DESC = 'MGS Integral VR-DISC: English photo album text'
BASE_INT, BASE_USA = 0x800C11A0, 0x800C4350
INT_SAVE, INT_LOAD, INT_PROMPTS, INT_PHOTO = 0x608, 0x638, 0x668, 0x708
USA_SAVE, USA_LOAD, USA_PROMPTS, USA_PHOTO = 0x7A8, 0x7D8, 0x808, 0x8E4
N_CAP, N_PROMPT, N_PHOTO = 12, 2, 7
POOL = (0x0CB1C, 0x0CD08)         # Integral's Japanese captions; OVERWRITE OK? / FORMAT OK? follow and stay
KEEP = {1, 9}


def overlay(sd):
    tags, pay, offs = portio.stage(sd, 'camera')
    assert tags[0][1] == ord('s')
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


def main():
    isd = open(INT_STAGE, 'rb').read()
    usd = open(USA_STAGE, 'rb').read()
    itags, ipay, iov = overlay(isd)
    utags, upay, uov = overlay(usd)
    I = dict(save=table(iov, BASE_INT, INT_SAVE, N_CAP), load=table(iov, BASE_INT, INT_LOAD, N_CAP),
             prompts=table(iov, BASE_INT, INT_PROMPTS, N_PROMPT), photo=table(iov, BASE_INT, INT_PHOTO, N_PHOTO))
    U = dict(save=table(uov, BASE_USA, USA_SAVE, N_CAP), load=table(uov, BASE_USA, USA_LOAD, N_CAP),
             prompts=table(uov, BASE_USA, USA_PROMPTS, N_PROMPT), photo=table(uov, BASE_USA, USA_PHOTO, N_PHOTO))
    # the tables are identified by content, not assumed
    assert U['save'][2][1] == b'Save failed.' and U['load'][4][1] == b'No save file.'
    assert U['save'][10][1] == U['load'][10][1] == b'Now checking Memory Card.'
    assert [s for _, s in U['photo']] == [b'Overwrite?', b'Now saving.', b'Error occured while saving.',
                                          b'Save completed.', b'Memory Card undetected.', b'Save failed.', b'Save failed.']
    assert all(U['save'][i][1] == b'' and U['load'][i][1] == b'' for i in KEEP) and U['save'][0][1] == U['load'][0][1] == b''
    assert all(s == b'' for _, s in U['prompts']), 'USA has prompt text; the keep decision needs revisiting'
    assert I['save'][0][1] == I['load'][0][1] == b''
    # Integral: the photo flow reuses save-table strings, plus two of its own outside the pool
    lo, hi = POOL
    for name in ('save', 'load', 'prompts'):
        for o, s in I[name]:
            assert lo <= o < hi, '%s string at +%X lies outside the pool' % (name, o)
    outside = [(o, s) for o, s in I['photo'] if not lo <= o < hi]
    print('photo-flow strings outside the pool (left in place, no longer referenced): %s' % ['+%X' % o for o, _ in outside])

    # choose every slot's text
    want = {}
    for name in ('save', 'load'):
        for i in range(N_CAP):
            io, istr = I[name][i]
            if i == 0:
                want[(name, i)] = b''
            elif i in KEEP:
                want[(name, i)] = istr
            else:
                assert U[name][i][1] != b'', (name, i)
                want[(name, i)] = U[name][i][1]
    for i in range(N_PROMPT):
        want[('prompts', i)] = I['prompts'][i][1]
    for i in range(N_PHOTO):
        want[('photo', i)] = U['photo'][i][1]

    # These are single drawn lines, so the one width limit that always holds
    # applies: kcb->max_width is a u8 read with lbu, and the renderer cannot
    # measure past 255 px. The per-window budget is deliberately not asserted
    # anywhere in this port - retail exceeds it - see widths.py.
    widths.check_ceiling([s for s in want.values() if s],
                         'camera pool', widths.font(INT_STAGE))

    new = bytearray(iov)
    for a in range(lo, hi):
        new[a] = 0
    # the shared empty string becomes the pool's first byte
    placed, cur = {b'': lo}, lo + 1
    empty_off = lo
    order = [k for k in want if want[k] != b'']
    # lay out unique strings in first-use order
    for k in order:
        s = want[k]
        if s in placed:
            continue
        b = s + b'\0'
        assert cur + len(b) <= hi, 'pool overflow at %s' % (k,)
        new[cur:cur+len(b)] = b
        placed[s] = cur
        cur += len(b)
    used = cur - lo
    bases = dict(save=INT_SAVE, load=INT_LOAD, prompts=INT_PROMPTS, photo=INT_PHOTO)
    for (name, i), s in want.items():
        struct.pack_into('<I', new, bases[name] + 4*i, BASE_INT + placed[s])
    # the empty string must still be an empty string
    assert new[empty_off] == 0
    new = bytes(new)
    # verify by reading every slot back
    for name, n in (('save', N_CAP), ('load', N_CAP), ('prompts', N_PROMPT), ('photo', N_PHOTO)):
        got = table(new, BASE_INT, bases[name], n)
        for i in range(n):
            assert got[i][1] == want[(name, i)], (name, i, got[i][1], want[(name, i)])
            print('  %-7s %2d %s%s' % (name, i, readable(want[(name, i)]), '  (kept)' if want[(name, i)] == I[name][i][1] and want[(name, i)] else ''))
    print('pool: %d of %d bytes used' % (used, hi - lo))
    assert len(new) == len(iov)
    # nothing outside the pool and the slot tables changed
    changed = [p for p in range(len(iov)) if new[p] != iov[p]]
    assert all(lo <= p < hi or INT_SAVE <= p < INT_SAVE + 4*N_CAP or INT_LOAD <= p < INT_LOAD + 4*N_CAP
               or INT_PROMPTS <= p < INT_PROMPTS + 8 or INT_PHOTO <= p < INT_PHOTO + 4*N_PHOTO for p in changed)
    payloads = dict(ipay)
    payloads[0] = new
    new_stage = portio.pack_stage(itags, payloads)
    old_stage = stage_bytes(isd, 'camera')
    assert len(new_stage) == len(old_stage)
    assert portio.pack_stage(itags, ipay) == old_stage, 'repacking the untouched stage does not reproduce it'
    lba = stage_lba(int_disc(), isd, 'camera')
    recs = inplace_records(lba, old_stage, new_stage)
    data = write_ppf(_os.path.join(WORK, PPF_NAME), recs, DESC)
    print('%s: %d records, %d bytes changed' % (PPF_NAME, len(recs), sum(len(d) for _, d in recs)))
    if '--deploy' in sys.argv:
        print('deployed', deploy(PPF_NAME, data))


if __name__ == '__main__':
    main()
