#!/usr/bin/env python
"""Port the VR-DISC title (EXTRA menu) and option-screen help lines.

Both are GCL STRING records inside one `chara` command of the stage script,
read by index by the overlay (the same shape as the main game's option chain):

vrtitle  `chara 0x5667` (vrtitle) option -t, 11 records. Records 2..5 are the
         EXTRA menu's help lines for EXIT / MOVIE / PHOTOGRAPHING / ALBUM and
         take USA's `Return to the title screen.` / `View the movie.` /
         `Take a picture.` / `See the album.`. Record 6 is the help line of
         Integral's fifth item, PocketStation (its texture 0x29A8 reads
         "PocketStation" where USA's reads "STAFF CREDIT"), so USA's `See the
         staff credits.` is NOT its translation: it stays Japanese, as do the
         PocketStation prompt and its はい/いいえ (records 7-9), which USA
         leaves empty.
option   `chara 0x976C` (opt) option -e, 31 records, index for index with USA:
         1 Sound setting. 2 Vibration setting. 3/12/26 Use directional buttons
         to test. 5 Key configuration setting. 6 Return to the title screen.
         Everything USA leaves empty (the colon 7, オン/オフ/ステレオ/モノラル 8-11,
         the unused brightness paragraph 13-16, the KEY CONFIG rows' lines
         17-25, the language rows 28-30) stays Integral's. The chain grows by
         49 bytes inside its 2048-byte sector: the stage keeps 73 sectors, so
         the option stage is NOT relocated and the collection's KEY CONFIG
         interception patch keeps landing where it always did.

usage: vr_menus.py [--deploy]
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import sys
from audit_text import game_text
import widths
from vrlib import (INT_STAGE, USA_STAGE, int_disc, stage_lba, stage_bytes, stage_gcx, repack_stage,
                   parse_arg, emit_arg, inplace_records, write_ppf, deploy, be16, WORK, CMD_CHARA, Gcx)

JOBS = {
    # stage:   (chara id, option letter, {Integral record index: USA record index}, PPF name, description)
    'vrtitle': (0x5667, 't', {2: 2, 3: 3, 4: 4, 5: 5}, 'INTEGRAL_vr_en_title.ppf', 'MGS Integral VR-DISC: English EXTRA menu text'),
}
# the option stage's help lines are the same shape (`chara 0x976C`, option -e, 31
# records, 1/2/3/5/6/12/26 from USA) but ship from vr_option.py together with the
# KEY CONFIG labels, in one PPF for that stage


def chain(gcx, chara, letter):
    """(body, block, the STRING values of that chara's option) from the script body"""
    body = gcx.script
    block = parse_arg(body)
    for c in block:
        if c.kind == 'COMMAND' and c.id == CMD_CHARA:
            a = c.args()
            if a and a[0].kind == 'STRID' and be16(body, a[0].pos+1) == chara:
                o = c.option(letter)
                assert o is not None, 'chara %04X has no -%s' % (chara, letter)
                return body, block, [v for v in o.values if v.kind == 'STRING']
    raise AssertionError('chara %04X not in the script' % chara)


def rec(body, v):
    return body[v.pos+2:v.end]


def readable(s):
    t, jp = game_text(s[:-1])
    return t if t is not None else repr(s)


def main():
    isd = open(INT_STAGE, 'rb').read()
    usd = open(USA_STAGE, 'rb').read()
    disc = int_disc()
    for name, (chara, letter, mapping, ppf_name, desc) in JOBS.items():
        idata, udata = stage_bytes(isd, name), stage_bytes(usd, name)
        itags, ipay, ici, ifiles, igcx = stage_gcx(idata)
        utags, upay, uci, ufiles, ugcx = stage_gcx(udata)
        ibody, iblock, irecs = chain(igcx, chara, letter)
        ubody, ublock, urecs = chain(ugcx, chara, letter)
        assert emit_arg(ibody, iblock, {}) == ibody
        print('== %s: Integral %d records, USA %d' % (name, len(irecs), len(urecs)))
        assert len(irecs) == len(urecs), 'record counts differ'
        replace = {}
        for i in range(len(irecs)):
            src = rec(ibody, irecs[i])
            if i in mapping:
                new = rec(ubody, urecs[mapping[i]])
                assert new != b'\0', 'USA record %d is empty' % mapping[i]
                assert game_text(new[:-1])[0] and not game_text(new[:-1])[1], 'USA record %d is not plain English' % i
                widths.check_ceiling([new[:-1]], '%s record %d' % (name, i),
                                     widths.font(INT_STAGE))
                replace[id(irecs[i])] = bytes((7, len(new))) + new
                print('  %2d %-46s <- %s' % (i, readable(src)[:46], readable(new)))
            else:
                urec = rec(ubody, urecs[i])
                print('  %2d %-46s    kept (USA: %s)' % (i, readable(src)[:46], readable(urec) if urec != b'\0' else 'empty'))
        newbody = emit_arg(ibody, iblock, replace)
        igcx.script = newbody
        new_gcx = igcx.build()
        # verify
        g2 = Gcx(new_gcx, 0)
        b2, _, r2 = chain(g2, chara, letter)
        for i in range(len(irecs)):
            exp = rec(ubody, urecs[mapping[i]]) if i in mapping else rec(ibody, irecs[i])
            assert rec(b2, r2[i]) == exp, i
        new_stage = repack_stage(idata, new_gcx)
        print('  script %d -> %d bytes; stage %d -> %d sectors' % (len(ibody), len(newbody), len(idata)//2048, len(new_stage)//2048))
        assert len(new_stage) == len(idata), '%s grew: relocation needed' % name
        lba = stage_lba(disc, isd, name)
        recs = inplace_records(lba, idata, new_stage)
        data = write_ppf(_os.path.join(WORK, ppf_name), recs, desc)
        print('  %s: %d records, %d bytes changed' % (ppf_name, len(recs), sum(len(d) for _, d in recs)))
        if '--deploy' in sys.argv:
            print('  deployed', deploy(ppf_name, data))


if __name__ == '__main__':
    main()
