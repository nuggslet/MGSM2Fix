#!/usr/bin/env python
"""Is any VR-disc text still Japanese where USA has English?

Reconstructs each Integral VR stage as the game will see it — the retail stage
with every deployed PPF in `mods/INTEGRAL/VR-DISK/` applied — then groups every
game-encoded GCL STRING record by the command that owns it, and shows the same
tally for USA's VR Missions.

Two things make a naive comparison useless, and this tool avoids both:

  * **The disc image has raw 2352-byte sectors.** A PPF offset is
    `(lba + off // 2048) * 2352 + 24 + off % 2048` (`portio.image_offset`), so
    reconstructing a stage from its PPFs means inverting that, not dividing by
    2048. Getting this wrong silently applies records at nonsense offsets and
    makes a finished port look unported.
  * **Positional comparison does not work.** The two discs do not lay their
    stages out alike — USA carries five language copies of each window and
    distributes missions across stages differently — which is why the port
    matches windows by content. So this compares *totals per owning command*,
    never record N against record N.

A command with Japanese on Integral and English on USA is text to look at. One
that is game-encoded on both is usually text neither version translated, or
text drawn with the script-local font: codes at or above 0x9A00 index a font
inside the script and `audit_text.game_text` cannot decode them, so English
containing a typographic quote reads as Japanese here. The report separates
those.

Result on 2026-09-06, over all 105 stages: Integral 222 game-encoded records
against 10 809 English; USA's own disc 940 against 10 344. Of Integral's 192
inside `vrwindow`, 181 are English with local-font glyphs; the 11 that are
genuinely Japanese are `vrsave`'s one debug window and `vrtitle`'s four, which
USA carries in the identical Japanese.

usage: vr_sweep.py [--samples]
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import glob
import re
import struct
from collections import defaultdict
import portio
from audit_text import game_text
from vrlib import (INT_STAGE, USA_STAGE, stage_bytes, stage_gcx, stage_lba, int_disc,
                   parse_arg, walk_commands, MODS, be16, CMD_CHARA)

LOCAL_FONT = re.compile(r'<9[A-F][0-9A-F]{2}>')
ANY_CODE = re.compile(r'<[0-9A-F]{4}>')


def ppf_records(path):
    d = open(path, 'rb').read()
    out, q = [], 60
    while q + 9 <= len(d):
        off, ln = struct.unpack_from('<QB', d, q)
        out.append((off, d[q + 9:q + 9 + ln]))
        q += 9 + ln
    return out


def deployed(sd, disc, name, patches):
    """the stage as the game sees it: retail bytes plus every PPF record that
    lands in it, inverting portio.image_offset's 2352-byte sector geometry"""
    data = bytearray(stage_bytes(sd, name))
    lba = stage_lba(disc, sd, name)
    for off, blob in patches:
        sector, within = divmod(off - 24, 2352)
        if within >= 2048:
            continue                       # a sector's ECC tail, never stage data
        p = (sector - lba) * 2048 + within
        if 0 <= p < len(data):
            data[p:p + len(blob)] = blob[:len(data) - p]
    return bytes(data)


def owner(body, cmd):
    if cmd.id == CMD_CHARA:
        a = cmd.args()
        if a and a[0].kind == 'STRID':
            return 'chara %04X' % be16(body, a[0].pos + 1)
    return 'cmd   %04X' % cmd.id


def records(gcx):
    """(owner, decoded text or None, raw bytes) for every non-empty STRING"""
    for body in [gcx.script] + [b for _, b in gcx.procs]:
        try:
            block = parse_arg(body)
        except Exception:
            continue
        for c, lang, path in walk_commands(body, block):
            for v in c.values:
                kids = ([v] if v.kind == 'STRING'
                        else [x for x in v.values if x.kind == 'STRING'] if v.kind == 'OPTION'
                        else [])
                for sv in kids:
                    rec = body[sv.pos + 2:sv.end]
                    b = rec[:-1] if rec.endswith(b'\0') else rec
                    if b:
                        yield owner(body, c), b


def tally(gcx, agg, name):
    for who, b in records(gcx):
        t, jp = game_text(b)
        e = agg[who]
        if t is None or jp:
            e[0] += 1
            e[3].add(name)
            # English with local-font glyphs: strip them and see if plain text is left
            if t is not None:
                bare = LOCAL_FONT.sub('', t)
                if bare.strip() and not ANY_CODE.search(bare):
                    e[4] += 1
            if e[2] is None:
                e[2] = t if t is not None else repr(b)[:60]
        else:
            e[1] += 1


def main():
    isd = open(INT_STAGE, 'rb').read()
    usd = open(USA_STAGE, 'rb').read()
    disc = int_disc()
    patches = []
    for p in sorted(glob.glob(_os.path.join(MODS, '*.ppf'))):
        patches += ppf_records(p)
    print('%d PPF records from %s' % (len(patches), MODS))

    def blank():
        return [0, 0, None, set(), 0]
    I, U = defaultdict(blank), defaultdict(blank)
    names = sorted(set(portio.entries(isd)) & set(portio.entries(usd)))
    for name in names:
        for agg, get in ((I, lambda n: deployed(isd, disc, n, patches)),
                         (U, lambda n: stage_bytes(usd, n))):
            try:
                gcx = stage_gcx(get(name))[4]
            except Exception:
                continue
            tally(gcx, agg, name)

    print('%-12s %6s %6s %6s | %6s %6s   stages' % (
        'owner', 'INT jp', 'local', 'INT en', 'USA jp', 'USA en'))
    for k in sorted(I, key=lambda k: -I[k][0]):
        ijp, ien, sample, stages, ilocal = I[k]
        if not ijp:
            continue
        ujp, uen = U.get(k, blank())[:2]
        note = '  <-- USA has English in this command' if uen and not ujp else ''
        print('%-12s %6d %6d %6d | %6d %6d   %s%s' % (
            k, ijp, ilocal, ien, ujp, uen,
            ', '.join(sorted(stages)[:5]) + ('...' if len(stages) > 5 else ''), note))
        if '--samples' in _sys.argv:
            print('             e.g. %s' % (sample or '')[:70])
    print()
    print('Integral: %d game-encoded records (%d of them English with local-font glyphs), %d plain English, over %d stages'
          % (sum(v[0] for v in I.values()), sum(v[4] for v in I.values()),
             sum(v[1] for v in I.values()), len(names)))
    print('USA:      %d game-encoded, %d plain English'
          % (sum(v[0] for v in U.values()), sum(v[1] for v in U.values())))


if __name__ == '__main__':
    main()
