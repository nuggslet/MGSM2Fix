#!/usr/bin/env python
"""Unlock the EXTRA -> MOVIE clips: a test aid, not part of the English port.

WHY THE MISSION UNLOCK DOES NOT DO THIS
---------------------------------------
`vr_unlock.py` patches three predicates in the **`selectvr`** overlay and so
governs the mission menu. The movies are gated separately, in the **`movie`**
overlay's own code, which is why they stayed `???` with the mission aid
deployed - and why USA's VR disc shows `???` there too with no patches at all.

THE GATE
--------
Found 2026-09-06 by disassembling around the caption command's `-m` read
(Integral movie overlay `+FFA0`):

    lw    s1, 6608(s1)       ; a progress count
    ori   v1, v1, 0x5556     ; v1 = 0x55555556
    mult  s1, v1             ; the standard signed divide-by-3 ...
    sra   v0, s1, 31
    mfhi  t0
    subu  v1, t0, v0         ; ... so v1 = count / 3      <-- PATCHED
    slti  v0, v1, 45         ; gate 1
    bne   v0, zero, ...
    slti  v0, v1, 75         ; gate 2   -> bits in work+30

`count / 3` against thresholds is exactly the scoring pattern `vr_unlock.py`
documents for the mission menu ("`count / 3` against 80 / 70 / 60 / 50"), and
the fix is the same one instruction: replace the `subu` that computes the score
with an immediate above every threshold, so both gates pass and every clip is
selectable.

    overlay +FFC4   01021823  subu v1, t0, v0   ->   24030100  addiu v1, zero, 0x100

WHY THIS IS SAFE TO REMOVE
--------------------------
It writes no progress: the score is recomputed from the count every time the
stage runs, and this only changes what the comparison sees. Delete the PPF and
the clips gate exactly as before. It patches the `movie` overlay, which
`vr_en_missions` does not touch (that patch only rewrites the script chunk), so
the two cannot interfere - checked, not assumed.

    py vr_unlock_movies.py            # stages the PPF
    py vr_unlock_movies.py --deploy   # and installs it
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
import struct

import portio
from vrlib import INT_STAGE, int_disc, stage_lba, stage_bytes, inplace_records, write_ppf

NAME = 'INTEGRAL_vr_unlock_movies.ppf'
DESC = 'MGS Integral VR-DISC: all EXTRA movies unlocked'
assert len(DESC) <= 50
MODS = 'mods/INTEGRAL/VR-DISK'

SCORE_AT = 0xFFC4                 # offset in the `movie` overlay
SUBU_V1_T0_V0 = 0x01021823        # subu v1, t0, v0     (v1 = count / 3)
MAX_SCORE = 0x24030100            # addiu v1, zero, 0x100  (over both thresholds)


def build():
    isd = open(INT_STAGE, 'rb').read()
    data = stage_bytes(isd, 'movie')
    tags, payloads, offsets = portio.stage(data)
    ov = [k for k, (tid, mode, ext, sz) in enumerate(tags)
          if mode == ord('s') and ext == ord('b')]
    assert len(ov) == 1, 'movie has %d overlays' % len(ov)
    p = offsets[ov[0]] + SCORE_AT
    new = bytearray(data)
    was = struct.unpack_from('<I', new, p)[0]
    assert was == SUBU_V1_T0_V0, \
        'movie overlay +%X holds %08X, expected %08X (the gate moved?)' % (SCORE_AT, was, SUBU_V1_T0_V0)
    struct.pack_into('<I', new, p, MAX_SCORE)
    print('movie overlay +%X (stage +%X): %08X -> %08X  the score gate sees a full score'
          % (SCORE_AT, p, was, MAX_SCORE))
    assert len(new) == len(data)
    # sanity: this must not collide with what vr_en_missions writes to this stage
    lba = stage_lba(int_disc(), isd, 'movie')
    recs = inplace_records(lba, data, bytes(new), merge_gap=0)
    assert len(recs) == 1 and len(recs[0][1]) == 4, 'expected one 4-byte record, got %r' % [(hex(o), len(d)) for o, d in recs]
    mine = set(range(recs[0][0], recs[0][0] + 4))
    d = _os.path.join(GAME, MODS)
    if _os.path.isdir(d):
        for f in sorted(_os.listdir(d)):
            if not f.endswith('.ppf') or f == NAME:
                continue
            for off, blob in portio.read_ppf(_os.path.join(d, f)):
                clash = mine & set(range(off, off + len(blob)))
                assert not clash, '%s already writes %s' % (f, sorted(clash))
        print('checked: no deployed PPF writes those 4 bytes')
    return recs


def main():
    recs = build()
    path = _os.path.join(WORK, NAME)
    blob = write_ppf(path, recs, DESC)
    print('%s: %d record, %d bytes' % (NAME, len(recs), sum(len(d) for _, d in recs)))
    if '--deploy' in _sys.argv:
        dst = _os.path.join(GAME, MODS, NAME)
        open(dst, 'wb').write(blob if isinstance(blob, bytes) else open(path, 'rb').read())
        print('deployed %s' % dst)
    else:
        print('\nnot deployed; re-run with --deploy. Delete the PPF to relock.')


if __name__ == '__main__':
    main()
