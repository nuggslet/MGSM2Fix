#!/usr/bin/env python
"""Port the `title` stage's copy of the disc-swap messages (`en_menu3`).

RAW DISC ONLY - it must never go in `mods/`. The reason is the whole story
below, and it is not a fault in this file.

THE FOURTH COPY
---------------
The five disc-swap strings exist four times on each disc (README, "The
disc-swap text: four copies"). `en_menu2` ships the `demosel` and `change`
copies, `en_abst` the disc-change abstract's. This is the last one, in the
`title` stage, and it is the last text in the main game with a USA counterpart
that a raw-disc release would otherwise leave Japanese.

WHERE THEY LIVE, AND WHY THIS ONE IS DIFFERENT
----------------------------------------------
In `demosel` and `change` the strings are a standalone data chain. Here they
are **inline in the executable script body**: `07 <len> <payload>` STRING values
in the `-v` option of the title actor's `CMD 9906` (chara `0xCF79` =
`GV_StrCode("タイトル")` = CHARA_OPEN, `onoda/open/open.c`). So editing them can
change the script's length, and every container that carries a size over the
edit has to be re-stamped.

The actor is the engine's generic numbered-text module - the same one `abst.c`
and the VR movie captions use - and its init reads the option like this
(`open.c`, verbatim):

    if (GCL_GetOption('v')) work->fB10 = GCL_StrToInt(GCL_NextStr());
    for (n = 0; n < 24; n++) {
        work->unk[n].string = GCL_GetString(GCL_NextStr());
        work->unk[n].num = 0;
        Open_800C4500(work, n);          // allocate line n's 64x21 KCB
    }

**A fixed loop of 24**, so the record COUNT and ORDER must not change: index n
picks line n's screen position and colour out of `open_800C32B4[n]`. The block
holds 25 records - the five messages at indices 0, 1, 2, 3 and 5, `RADAR OFF`
at 4, and the rest of the title's own strings after them.

WHY IT CANNOT SHIP IN THE COLLECTION
------------------------------------
**The collection patches this exact block itself.** Its CD-ROM patch
`disc1_1822B55D_patch` is applied at image offset `0x1822B55D`, which is not
merely inside the block - it is the address of record 0's `07` header. The data
comes from `099/patch/disc1_1822B55D_patch_PS5.bin`, so the patch watch reports
it as "0 bytes" and its contents have never been visible.

Deploy this on top of that and the title stage dies on entry with a run of
`GCL:WRONG CODE` whose bytes are the English letters, then the process exits.
Seen 2026-08-28, again 2026-08-29, and reproduced exactly on 2026-09-07 with
this builder's length-preserving output: the same seventeen bytes every time,

    82 73 68 65 74 61 72 74 75 74 74 6f 6e 22 22 22 22

which read out of `Press the Start Button` and then off the end of the .gcx.
Note what was and was not tested: the crash is the **length-preserving** shape,
which is the one that leaves retail's layout completely intact - so the record
shape cannot be the fault. The shortened shape was never deployed; it moves the
records as well, so the collision applies to it at least as much.

**Our bytes are not what the interpreter was walking.** `GCL_GetNextValue` was
transcribed from `libgcl/parse.c` and run over this builder's output from every
start offset in the chunk: no start reproduces that sequence, and the trailing
`22 22 22 22` does not occur anywhere in the stage. The collection is writing
its own version of these five strings over the same bytes, with its own layout,
and the mixture is what desynchronises the walk. Nothing here can fix that from
the data side; on a raw PSX disc the collision does not exist.

If the collection's wording ever needs to be ours, the lever is
`SQHook::SetPatchFileBlacklist("disc1_1822B55D_patch")` behind an ini flag - the
same mechanism `BrightnessText` uses to take the brightness texture back - and
then this port would own the block. That is an ASI change, and it buys a screen
the collection cannot reach anyway, so it was not done (decided with the user,
2026-09-07).

WHAT IS RE-STAMPED, AND WHAT IS DELIBERATELY LEFT
-------------------------------------------------
Shortening a record shrinks the script, so three sizes have to follow it, and
`containers_over` is run per edited record to prove that list is complete:

    SCRIPT   BE32 before the body   - recomputed by `vrlib.Gcx.build`
    ARG      BE16 at body+1         - the script's top-level GCL_ARG
    COMMAND  BE16 at cmd+1          - the `0x9906` the records live in

The `-v` option's own u8 length is **left exactly as Integral wrote it**, which
is the same call `abst_build.py` makes for the mission log's `-i` option and the
same reason: it is an overflowed u8 (the option really spans 598 bytes and the
byte reads 86 = 598 & 0xFF), and nothing ever reads it, because
`GCL_GetOption` stops at its own letter and `v` is the **last** of the command's
eighteen options. That is checked, not assumed: `open.c` asks for exactly
a,b,c,d,e,f,g,h,j,k,l,m,o,p,r,s,v,x and the command carries exactly those
eighteen letters, so no scan ever advances past `v`. (A scan that did would land
mid-payload in **retail** too - Integral's own byte points at the middle of
record 2 - so the byte is meaningless in both games.)

THE DIAGNOSIS THIS REPLACES - CORRECTED 2026-09-07
--------------------------------------------------
The README recorded the cause as "the interpreter resumes parsing at the early
NUL", and prescribed shrinking four container sizes. Both halves were wrong, and
the checks that show it are cheap:

  * `GCL_GetNextValue` advances a STRING by its **length byte**, `p += *p + 1`,
    never by `strlen`. An early NUL inside a payload cannot desynchronise it.
  * The PPFs that sat in `mods/_disabled/` change **payload bytes only** - 150
    of them - and leave every record header, every length byte and every
    container size exactly as retail. They re-parse cleanly with `gclparse`, 25
    records at retail's offsets. There was no container fault to fix, and this
    builder's collection output is byte-identical to those files.

The prescription was written on 09-03 from the 08-28/29 logs and attached to an
artefact it had never been tested against. Worth keeping as a pattern: a
diagnosis carried forward onto a different artefact, and never re-checked.

NO TEXT IS MODIFIED
-------------------
Both shapes carry USA's sentences verbatim and whole. "Shortening" is about the
record slot, never the string: Integral's record 0 is 35 bytes and
`Insert DISC 1.` is 14 + NUL, so the raw shape sets the length byte to 15 and
gives the 20 bytes back to the script, where the collection shape left them as
spaces after the terminator. `verify` asserts the sentence is intact and that
nothing but a terminator and spaces follows it.

    py menu3.py                # the RAW-DISC PPFs into work/ (the deliverable)
    py menu3.py --collection   # the crashing shape, to reproduce the fault
    py menu3.py --deploy       # refuses, and says why
"""

import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
from pathlib import Path
import struct, sys

import portio, gclparse
from vrlib import Gcx, be16, be32
from iso import Disc
from portio import INTEGRAL_IMAGES

DESC = 'MGS Integral: title screen disc messages'
assert len(DESC) <= 50
STAGE = 'title'
CHARA_OPEN = 0xCF79            # GV_StrCode("タイトル"), onoda/open/open.c
LETTER = 'v'
OPEN_LOOP = 24                 # open.c reads exactly 24 records
PRE = 4                        # the BE32 script length gclparse expects before the body
CHANGE, DEMOSEL = 0x04650A1F, 0x04702FED

# record index in the title block -> (the disc's own Japanese, by source) English.
# The Japanese is not hardcoded: it is read from the `change`/`demosel` chains
# this disc already carries, so a mismatch means the block moved and the build
# stops rather than writing English over the wrong record.
PORT = {
    0: ('demosel', 0, 0, b'Insert DISC 1.'),
    1: ('demosel', 1, 0, b'after inserting DISC 1.'),
    2: ('change',  1, 0, b'Press the Start Button'),
    3: ('change',  3, 0, b'Now Checking...'),
    # Record 4 is `RADAR OFF`, and it is here only because this builder shifts
    # the records around it. `en_menu` writes it for the collection build; in the
    # raw variant menu2.py skips it (see the comment there) and this builder owns
    # it, so the two never write the same bytes with different layouts. It has no
    # counterpart in the change/demosel chains, hence None for the cross-check.
    # Caught 2026-09-07 by rebuild.py's packaged-set overlap check.
    4: (None,      0, 0, b'RADAR OFF'),
    5: ('change',  5, 2, b'The correct DISC was not inserted.'),
}


def gcx_of(stage_dir, name=STAGE):
    """-> (tags, payloads, chunk key, Gcx) for a stage's scenerio.gcx"""
    tags, pay, offs = portio.stage(stage_dir, name)
    ci = next(k for k, t in enumerate(tags) if t[2] == 0xFF)
    g = [t[3] for t in tags if t[2] == ord('g')]
    assert len(g) == 1, 'expected one scenerio.gcx tag, got %d' % len(g)
    return tags, pay, ci, Gcx(pay[ci], g[0])


def parsed(script):
    """gclparse wants the BE32 length that sits before the body, so parse a
    prefixed copy; every offset it returns is then script-relative + 4."""
    blob = struct.pack('>I', len(script)) + bytes(script)
    root, slen = gclparse.parse_script(blob, PRE)
    assert slen == len(script)
    return blob, root


def find_records(script):
    """-> (command node, option node, [(offset, payload)], list end), all offsets
    script-relative, for the CHARA_OPEN command's `-v` option"""
    blob, root = parsed(script)
    cmd = None
    for n, _d in gclparse.walk_tree(root):
        if n.kind == 'COMMAND' and be16(blob, n.pos + 3) == 0x9906:
            ids = [k for k in n.kids if k.kind == 'STRID']
            if ids and be16(blob, ids[0].pos + 1) == CHARA_OPEN:
                cmd = n
                break
    assert cmd is not None, 'no CMD 9906 for chara %04X' % CHARA_OPEN
    opts = [k for k in cmd.kids if k.kind == 'OPTION']
    assert opts and chr(blob[opts[-1].pos + 1]) == LETTER,         "the last option is not '%s' - a GetOption scan could pass it" % LETTER
    opt = opts[-1]
    p = opt.pos + 3
    assert blob[p] == 0x0A, 'the -v option does not start with an INT'
    p += 5
    recs = []
    while p < cmd.end and blob[p] == 0x07:
        L = blob[p + 1]
        assert L and blob[p + 1 + L] == 0, 'record at 0x%X is not NUL-terminated' % p
        recs.append((p - PRE, bytes(blob[p + 2:p + 2 + L])))
        p += 2 + L
    assert blob[p] == 0x00, 'the record list does not end with GCL_END'
    return cmd, opt, recs, p - PRE


def build(stage_dir, shorten):
    tags, pay, ci, g = gcx_of(stage_dir)
    script = bytearray(g.script)
    cmd, opt, recs, listend = find_records(script)
    assert len(recs) >= OPEN_LOOP, 'only %d records; open.c reads %d' % (len(recs), OPEN_LOOP)
    src = {'change': portio.records(stage_dir, CHANGE)[0],
           'demosel': portio.records(stage_dir, DEMOSEL)[0]}

    # 1. every size that is READ over the edits, found per record because the
    #    messages do not all share one container (the README's own warning)
    _blob, root = parsed(script)
    need = {}
    for i in PORT:
        off, payload = recs[i]
        for n in gclparse.containers_over(root, off + PRE, off + PRE + 2 + len(payload)):
            need[n.size_at] = (n.kind, n.size_bits)
    for at in sorted(need):
        kind, bits = need[at]
        print('    encloses the edits: %-7s %2d-bit size at script 0x%X%s'
              % (kind, bits, at - PRE, '  (left alone, see below)' if kind == 'OPTION' else ''))
    assert set(k for k, _b in need.values()) == {'SCRIPT', 'ARG', 'COMMAND', 'OPTION'}, need
    # The OPTION is the `-v` u8 and it is deliberately NOT re-stamped. It already
    # cannot describe its own contents: the option really spans 598 bytes and the
    # byte reads 86 = 598 & 0xFF, so retail's own value points into the middle of
    # record 2. Nothing reads it (open.c asks for exactly the eighteen letters the
    # command carries, so every GetOption scan stops at its own letter and never
    # advances past the last one), and leaving it keeps the byte identical to
    # retail - the safest value if some unknown reader ever does look. This is the
    # same call abst_build.py makes for the mission log's `-i` option.
    vpos = opt.pos - PRE                     # the option opcode, script-relative
    assert need[opt.pos + 2] == ('OPTION', 8), need
    true_span = listend - vpos - 2           # up to the record list's GCL_END
    assert script[vpos + 2] == true_span & 0xFF and true_span > 0xFF, \
        'the -v length byte (%d) is not the overflowed truncation of %d that this assumes' \
        % (script[vpos + 2], true_span)
    print('    -v spans %d bytes and its u8 reads %d = %d & 0xFF - overflowed, and unread'
          % (true_span, script[vpos + 2], true_span))

    # 2. the new record bytes. Two shapes, and the only difference is where the
    #    slack goes - the English sentence itself is identical in both, verbatim
    #    and whole. Nothing about the text is shortened; the RECORD is.
    out, delta, report = bytearray(), 0, []
    for i, (off, payload) in enumerate(recs):
        if i in PORT:
            where, j, skip, english = PORT[i]
            if where is not None:
                want = src[where][j][skip:-1]
                assert payload[skip:-1] == want, \
                    'record %d is not %s[%d] - the block moved (have %r)' % (i, where, j, payload[:16])
            assert len(english) + 1 <= len(payload), 'English is longer than the slot'
            if shorten:
                new = english + b'\x00'
            else:
                # keep the record's own length: English, terminator, spaces.
                # menu2.py's recipe for the demosel/change copies, which ship.
                cap = len(payload) - 1
                slack = cap - len(english) - 1
                term = bytes(2 if slack % 2 else 1)
                new = english + term + b' ' * (cap - len(english) - len(term)) + b'\x00'
                assert len(new) == len(payload), (len(new), len(payload))
            report.append((i, len(payload), len(new), english))
            delta += len(new) - len(payload)
        else:
            new = payload
        out += bytes((7, len(new))) + new
    assert shorten or delta == 0, 'the length-preserving build changed the script size'
    script[recs[0][0]:listend] = out          # the bytearray shrinks with it

    # 3. re-stamp the BE16 sizes; the SCRIPT's own BE32 is Gcx.build's job
    for at in sorted(need):
        kind, bits = need[at]
        if kind in ('SCRIPT', 'OPTION'):     # Gcx.build recomputes one, the other is dead
            continue
        assert bits == 16, '%s carries a %d-bit size' % (kind, bits)
        struct.pack_into('>H', script, at - PRE, be16(script, at - PRE) + delta)

    # 4. verify by re-parsing, which is self-checking by construction
    vbyte_before = script[vpos + 2]
    g.script = bytes(script)
    _blob2, root2 = parsed(g.script)
    cmd2, opt2, recs2, _end2 = find_records(g.script)
    assert len(recs2) == len(recs), 'record count changed: %d -> %d' % (len(recs), len(recs2))
    for i, (_off, payload) in enumerate(recs2):
        if i in PORT:
            english = PORT[i][3]
            assert payload.startswith(english) and payload[-1] == 0, \
                'record %d is not the English' % i
            # whatever follows the sentence is terminator and spaces only, and
            # the game stops rendering at the first NUL either way
            assert set(payload[len(english):]) <= {0, 0x20}, \
                'record %d has something other than a terminator after the text' % i
            assert shorten == (len(payload) == len(english) + 1), \
                'record %d has the wrong shape for this variant' % i
        else:
            assert payload == recs[i][1], 'record %d changed and should not have' % i
    assert opt2.pos == opt.pos, 'the -v option moved'
    assert g.script[opt2.pos - PRE + 2] == vbyte_before, 'the -v length byte changed'
    return tags, pay, ci, g, delta, report, vbyte_before


def rebuild(stage_dir, shorten):
    tags, pay, ci, g, delta, report, vbyte = build(stage_dir, shorten)
    for i, old, new, english in report:
        print('    record %2d  %3d -> %3d bytes  %r' % (i, old, new, english.decode()))
    print('    script %+d bytes; -v length byte left at %d (overflowed u8, never read)' % (delta, vbyte))
    payloads = dict(pay)
    chunk = pay[ci][:g.start] + g.build()
    chunk += bytes(-len(chunk) % 4)
    payloads[ci] = chunk
    new_stage = portio.pack_stage(tags, payloads)
    base = portio.entries(stage_dir)[STAGE][0] * 2048
    old_len = struct.unpack_from('<h', stage_dir, base + 2)[0] * 2048
    assert len(new_stage) == old_len, 'stage changed size: %d -> %d' % (old_len, len(new_stage))
    out = bytearray(stage_dir)
    out[base:base + len(new_stage)] = new_stage
    return bytes(out)


def main():
    work = Path(WORK)
    # The collection build exists only to reproduce the crash; it is never
    # written unless it is asked for by name, and it has no deploy path at all.
    collides = '--collection' in sys.argv
    shorten = not collides
    suffix = '_raw' if shorten else '_collides'
    if '--deploy' in sys.argv:
        raise SystemExit(
            'menu3 does not deploy. The collection patches this very block itself\n'
            '(disc1_1822B55D_patch, at the address of our first record), and the two\n'
            'layouts do not mix: the title stage dies with a GCL:WRONG CODE run.\n'
            'Proven on screen 2026-08-28/29 and again 2026-09-07. This port is for a\n'
            'raw PSX disc, where no such patch exists. See the docstring.')
    print('variant: %s\n' % ('RAW DISC - records shortened, containers re-stamped'
                             if shorten else
                             'COLLECTION - the crashing shape, kept only to reproduce it'))
    for disc, image_base in enumerate(INTEGRAL_IMAGES):
        name = 'int%d_stage.dir' % (disc + 1)
        print('--- disc %d (%s) ---' % (disc + 1, name))
        original = (work / name).read_bytes()
        modified = rebuild(original, shorten)
        image = Disc(Path(GAME) / 'windata/dlc/dlc_japan.bin', image_base)
        try:
            lba = next(l for n, l, s, d in image.walk() if n.upper() == '/MGS/STAGE.DIR;1')
        finally:
            image.f.close()
        runs = list(portio.changed_runs(original, modified))
        data = portio.ppf(portio.map_runs(lba, runs), DESC)
        out = work / ('INTEGRAL_disc%d_en_menu3%s.ppf' % (disc + 1, suffix))
        out.write_bytes(data)
        print('    %s: %d changed run(s), %d bytes' % (out.name, len(runs), sum(len(r[1]) for r in runs)))
    if shorten:
        print('\nStaged for the raw-disc variant (NextSteps 5.4). Do not put these in\n'
              'mods/: they are for an image the collection does not patch.')


if __name__ == '__main__':
    main()
