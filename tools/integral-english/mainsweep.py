"""Is any main-disc UI text still Japanese where the USA release has English?

The VR disc has had this since 2026-09-06 (`vr_sweep.py`) and it is what lets
that disc's coverage be stated as a measurement rather than a hope. Discs 1 and
2 had only `audit_text.py`, which inventories *candidates* by framing and
explicitly cannot say whether a candidate has an English counterpart - which is
why `COVERAGE.md` still lists about 160 unclassified strings. This closes that
gap the same way `vr_sweep` did.

THE METHOD, and why it is stronger than counting strings

Every string lives inside a GCL command, and a command is identified by what it
spawns - `chara 0x9906` is the generic numbered-text module, `chara 0xCF79` the
title actor, and so on. Tallying strings *by owner* on both discs at once turns
a pile of bytes into a comparison:

    owner X:  Integral 40 Japanese, 0 English | USA 0 Japanese, 41 English

is a porting target. Whereas

    owner Y:  Integral 12 Japanese, 0 English | USA 12 Japanese, 0 English

is text USA never translated either, and

    owner Z:  Integral 0 Japanese, 30 English | USA 0 Japanese, 30 English

is already done. No heuristic about what "looks Japanese" has to be trusted:
the two discs are read the same way and compared with each other.

WHAT IT READS

Retail bytes on both sides, from the stage archives the build already extracts
(`int1_stage.dir` / `int2_stage.dir`, and `usa1_stage.dir` / `usa2_stage.dir` -
the REAL USA discs, not `us1_stage.dir`, which the port established is not the
USA build). Retail rather than the deployed state on purpose: the point is to
enumerate everything USA has in English, and then subtract what the port
already covers, so a gap cannot hide behind a patch that is already there. The
stages the port owns are listed in PORTED below and reported separately.

    py mainsweep.py                  disc 1
    py mainsweep.py --disc 2
    py mainsweep.py --samples        show an example string per owner

A main-game stage differs from a VR one in a way that matters here: its cache
section can hold SEVERAL `.gcx` scripts and `scenerio.gcx` need not be last, so
this walks every one rather than using `vrlib.stage_gcx`, which asserts the VR
layout (that assertion is right for the VR disc and is what lets a rebuilt
script grow there).
"""
import argparse
import difflib
import os as _os
import re
import sys as _sys
from collections import defaultdict

_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))

import portio
from audit_text import game_text
from vrlib import Gcx, cache_files, chunk_index, parse_arg, walk_commands, be16
from workdir import WORK

CMD_CHARA = 0x9906

# Stages the port already owns, so a residual Japanese string in them is
# accounted for rather than a finding. Each is named after the patch family.
PORTED = {
    'option':  'en_option',
    'preope':  'en_preope',
    'brf':     'en_brf',
    'abst':    'en_abst',
    'camera':  'en_camsave',
    'menu':    'en_menu',
    'title':   'en_menu / en_menu3 (raw only)',
    'change':  'en_menu2',
    'demosel': 'en_menu2',
    # `s07br` is Integral-only, so it never enters this sweep's shared-name
    # universe at all; it is listed because `en_pad2` ports it too.
    's07b':    'en_pad2',
    's07br':   'en_pad2',
}


def scripts(stage_data):
    """every GCL script in a stage's cache section, not merely the last one"""
    tags, payloads, _ = portio.stage(stage_data)
    try:
        ci = chunk_index(tags)
        files = cache_files(tags)
    except (IndexError, KeyError):
        return
    for ext, _tid, start, _end in files:
        if ext != 'g':
            continue
        try:
            yield Gcx(payloads[ci], start)
        except Exception:
            continue                     # not a script we can walk; counted as skipped


def owner_of(body, cmd):
    if cmd.id == CMD_CHARA:
        args = cmd.args()
        if args and args[0].kind == 'STRID':
            return 'chara %04X' % be16(body, args[0].pos + 1)
    return 'cmd   %04X' % cmd.id


def strings(gcx):
    """(owner, raw bytes) for every non-empty STRING in the script and its procs"""
    for body in [gcx.script] + [b for _, b in gcx.procs]:
        try:
            block = parse_arg(body)
        except Exception:
            continue
        for cmd, _lang, _path in walk_commands(body, block):
            who = owner_of(body, cmd)
            for value in cmd.values:
                pool = ([value] if value.kind == 'STRING'
                        else [v for v in value.values if v.kind == 'STRING']
                        if value.kind == 'OPTION' else [])
                for sv in pool:
                    raw = body[sv.pos+2:sv.end]
                    if len(raw) > 1:
                        yield who, raw


def tally(stage_data, into, name):
    for gcx in scripts(stage_data):
        for who, raw in strings(gcx):
            text, japanese = game_text(raw[:-1])
            entry = into[who]
            if text is None:
                entry['odd'] += 1
            elif japanese:
                entry['jp'] += 1
                entry['stages'].add(name)
                entry.setdefault('sample', text)
            else:
                entry['en'] += 1


def blank():
    return defaultdict(lambda: {'jp': 0, 'en': 0, 'odd': 0, 'stages': set()})


ASSET_ID = re.compile(r'^(vc|vr|d|s)[0-9a-z_]*[0-9]$', re.I)


def base_stage(name, usa_names):
    """the USA stage an Integral-only stage is a variant of.

    Every one of the 13 Integral-only names is a base name plus a trailing `r`
    except `init_ve`, which is `init` plus a suffix. Without this pairing the
    Integral-only stages are invisible to this tool, because its universe is
    the names the two discs share - which is how the third copy of the
    controller-port string in `s07br` went unnoticed until 2026-09-08.
    """
    for candidate in (name[:-1], name.split('_')[0]):
        if candidate != name and candidate in usa_names:
            return candidate
    return None


def integral_only(isd, usd, samples=False):
    """The Japanese-where-USA-has-English question, asked of the stages this
    sweep's own pairing cannot reach."""
    ints, usas = set(portio.entries(isd)), set(portio.entries(usd))
    names = sorted(ints - usas)
    print('%d Integral-only stage(s); each compared against the USA stage it '
          'is a variant of\n' % len(names))
    print('%-9s %-7s %s' % ('stage', 'base', 'owners with Japanese where the base has English'))
    total, unpaired = 0, []
    for name in names:
        base = base_stage(name, usas)
        if base is None:
            unpaired.append(name)
            print('%-9s %-7s (no base stage found - not compared)' % (name, '?'))
            continue
        I, U = blank(), blank()
        tally(_stage_bytes(isd, name), I, name)
        tally(_stage_bytes(usd, base), U, base)
        hits = []
        for who in sorted(set(I) | set(U)):
            i, u = I[who], U[who]
            if i['jp'] and u['en']:
                covered = PORTED.get(name)
                hits.append('%s %djp/%den vs %djp/%den%s'
                            % (who, i['jp'], i['en'], u['jp'], u['en'],
                               '  [covered by %s]' % covered if covered else
                               '  <-- NOT covered by any patch family'))
                total += i['jp']
                if samples and i.get('sample'):
                    hits.append('e.g. ' + i['sample'][:60])
        print('%-9s %-7s %s' % (name, base, '; '.join(hits) if hits else '-'))
    print('\nJapanese strings in Integral-only stages whose base-stage owner has'
          ' English on the USA disc: %d' % total)
    if unpaired:
        print('NOT compared (no base stage): %s' % ', '.join(unpaired))
    return total


def _ui_text(text):
    """text a player could read, as opposed to an asset id the script names"""
    if ASSET_ID.match(text):
        return False
    return bool(re.search('[A-Za-z]', text)) and (
        ' ' in text or len(text) > 12 or text.endswith('.'))


def english_strings(stage_data):
    """(owner, text) for every plain-English string, in script walk order"""
    out = []
    for gcx in scripts(stage_data):
        for who, raw in strings(gcx):
            text, japanese = game_text(raw[:-1])
            if text is not None and not japanese and text.strip():
                out.append((who, text))
    return out


def diff_english(isd, usd):
    """Where both discs are already English and merely SAY something different.

    Every other sweep in this project hunts Japanese, so a string that is
    English on both discs and worded differently passes all of them unremarked
    - the blind spot `NextSteps.md` 5.14 records, and the shape of both cases
    that were found by accident (`SCARF` against `HANDKER`, and the `abst`
    location spellings). A positional diff is meaningless because the two
    builds lay their data out differently, but both known cases sit in a
    SEQUENCE whose neighbours match, which is what a diff is for: equal runs
    align themselves and a `replace` hunk with matching context either side is
    exactly the shape being looked for.

    Expect noise and know its shape: voice-clip and stage ids (`vc319010`,
    `vr01`), and any hunk where one side's counterpart is Japanese and so never
    entered an English list at all. The useful output is the residue - the same
    UI element, worded differently.
    """
    names = sorted(set(portio.entries(isd)) & set(portio.entries(usd)))
    print('%d stage(s) present on both discs\n' % len(names))
    hunks = readable = 0
    warned = set()
    for name in names:
        a = english_strings(_stage_bytes(isd, name))
        b = english_strings(_stage_bytes(usd, name))
        at, bt = [t for _, t in a], [t for _, t in b]
        matcher = difflib.SequenceMatcher(a=at, b=bt, autojunk=False)
        for tag, i1, i2, j1, j2 in matcher.get_opcodes():
            if tag != 'replace':
                continue
            hunks += 1
            interesting = [t for t in at[i1:i2] + bt[j1:j2] if _ui_text(t)]
            owner = a[i1][0] if i1 < len(a) else (b[j1][0] if j1 < len(b) else '?')
            print('%-8s %-9s replace %2d<->%-3d owner %s'
                  % (name, 'UI-TEXT' if interesting else 'asset ids',
                     i2 - i1, j2 - j1, owner))
            if not interesting:
                continue
            readable += 1
            if name in PORTED:
                # THIS TOOL READS RETAIL. For the Japanese question that is
                # deliberate - a gap cannot then hide behind a deployed patch.
                # For this question it is backwards: what the port itself
                # rewrites is exactly what must be subtracted, or the sweep
                # reports its own work as a finding. It did once, on the VR
                # disc: `FAMAS` against USA's `FA-MAS`, which `vr_en_missions`
                # had already replaced (NextSteps.md 5.14 step 3).
                warned.add(name)
                print('     !! %s is owned by %s and these are RETAIL bytes -'
                      ' check what that patch writes here before believing it'
                      % (name, PORTED[name]))
            for text in at[i1:i2][:24]:
                print('     INT: %r' % text[:78])
            for text in bt[j1:j2][:24]:
                print('     USA: %r' % text[:78])
            if max(i2 - i1, j2 - j1) > 24:
                print('     ... hunk truncated')
    print()
    print('%d replace hunk(s); %d hold player-readable text. A one-against-one hunk'
          % (hunks, readable))
    print('of readable text is the case worth deciding; everything else is context.')
    if warned:
        print('%d stage(s) flagged !!: %s. Those bytes are RETAIL and a patch family'
              % (len(warned), ', '.join(sorted(warned))))
        print('owns them, so what a player sees may already differ. This tool cannot')
        print('reconstruct a deployed stage - `en_abst` and `en_brf` relocate theirs')
        print('into DUMMY3M - so read the builder or the PPF before believing a')
        print('finding there (NextSteps.md 5.14 step 3).')
    return hunks


def census(isd, usd):
    """Account for EVERY Japanese GCL string, not just the flagged ones.

    `audit_text.py` inventories candidates by framing and cannot say whether a
    candidate has an English counterpart, which is what left about 160 of disc
    1's unclassified in `COVERAGE.md`. This asks a question that has a complete
    answer instead: for each Japanese string Integral has, what is at the same
    owner on the USA disc? Every string lands in exactly one bucket and the
    buckets sum to the total, so a residue cannot hide in the framing.

    The Integral-only stages are folded in through `base_stage`, so this covers
    every stage on the disc rather than the shared names alone.
    """
    ints, usas = set(portio.entries(isd)), set(portio.entries(usd))
    I, U = blank(), blank()
    for name in sorted(ints & usas):
        tally(_stage_bytes(isd, name), I, name)
        tally(_stage_bytes(usd, name), U, name)
    for name in sorted(ints - usas):
        base = base_stage(name, usas)
        tally(_stage_bytes(isd, name), I, name)
        if base:
            tally(_stage_bytes(usd, base), U, base)

    owned = usa_jp = absent = open_ = 0
    open_owners = []
    for who, i in I.items():
        if not i['jp']:
            continue
        u = U[who] if who in U else None
        if u is None or not (u['jp'] or u['en']):
            absent += i['jp']
        elif not u['en']:
            usa_jp += i['jp']
        elif all(s in PORTED for s in i['stages']):
            owned += i['jp']
        elif u['jp'] >= i['jp'] * 0.8:
            usa_jp += i['jp']
        else:
            open_ += i['jp']
            open_owners.append('%s (%s)' % (who, ', '.join(sorted(i['stages'])[:4])))
    total = sum(v['jp'] for v in I.values())
    print('%d Japanese GCL string(s) in Integral, every one accounted for:' % total)
    print('   inside a stage a patch family owns ............ %4d' % owned)
    print('   USA is Japanese there too (never translated) .. %4d' % usa_jp)
    print('   the owner does not exist on the USA disc ...... %4d' % absent)
    print('   UNACCOUNTED - a porting target ................ %4d' % open_)
    for line in open_owners:
        print('      %s' % line)
    assert owned + usa_jp + absent + open_ == total, 'buckets must sum to the total'
    print()
    print('The first bucket is "accounted for there", not "ported": it includes the'
          ' strings')
    print('kept Japanese by rule (the READ MISSION LOG? caption, preope\'s unread'
          ' recap')
    print('bytes) as well as the ported ones. The last bucket is the one that must'
          ' be 0.')
    return open_


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--disc', type=int, choices=(1, 2), default=1)
    parser.add_argument('--samples', action='store_true')
    parser.add_argument('--integral-only', action='store_true',
                        help="the 13 Integral-only stages, against the USA stage each"
                             " is a variant of - outside the shared-name universe")
    parser.add_argument('--diff-english', action='store_true',
                        help='where both discs are English and merely say something'
                             ' different (5.14 step 2)')
    parser.add_argument('--census', action='store_true',
                        help='account for every Japanese GCL string; the last'
                             ' bucket must be 0 (5.8)')
    args = parser.parse_args()

    isd = open('%s/int%d_stage.dir' % (WORK, args.disc), 'rb').read()
    usd = open('%s/usa%d_stage.dir' % (WORK, args.disc), 'rb').read()
    if args.integral_only:
        print('disc %d, Integral-only stages' % args.disc)
        print()
        integral_only(isd, usd, args.samples)
        return 0
    if args.diff_english:
        print('disc %d, English against English' % args.disc)
        print()
        diff_english(isd, usd)
        return 0
    if args.census:
        print('disc %d, the census' % args.disc)
        print()
        return 1 if census(isd, usd) else 0
    names = sorted(set(portio.entries(isd)) & set(portio.entries(usd)))
    print('disc %d: %d stage(s) present on both discs\n' % (args.disc, len(names)))

    I, U = blank(), blank()
    for name in names:
        tally(_stage_bytes(isd, name), I, name)
        tally(_stage_bytes(usd, name), U, name)

    targets, done, both_jp = [], [], []
    for who in sorted(set(I) | set(U)):
        i, u = I[who], U[who]
        # A target is any owner Integral still has Japanese in where USA has
        # English AT ALL. Requiring USA to be free of Japanese was too strict
        # and hid the mission log's own actor, which USA ships as 907 English
        # strings beside 2 Japanese ones.
        if i['jp'] and u['en']:
            targets.append((who, i, u))
        elif i['jp'] and u['jp']:
            both_jp.append((who, i, u))
        elif i['en']:
            done.append((who, i, u))

    print('%-12s %7s %7s | %7s %7s   stages' % ('owner', 'INT jp', 'INT en', 'USA jp', 'USA en'))
    print('-- USA has English where Integral is Japanese: PORTING TARGETS --')
    if not targets:
        print('   (none)')
    for who, i, u in sorted(targets, key=lambda t: -t[1]['jp']):
        covered = sorted({PORTED[s] for s in i['stages'] if s in PORTED})
        rest = sorted(s for s in i['stages'] if s not in PORTED)
        note = ('  [covered by %s]' % ', '.join(covered)) if covered and not rest else ''
        if not note:
            note = '  <-- NOT covered by any patch family'
        print('%-12s %7d %7d | %7d %7d   %s%s'
              % (who, i['jp'], i['en'], u['jp'], u['en'],
                 ', '.join(sorted(i['stages'])[:6]) + ('...' if len(i['stages']) > 6 else ''), note))
        if args.samples:
            print('             e.g. %s' % (i.get('sample') or '')[:72])

    print('\n-- Japanese on BOTH discs: nothing to port (USA never translated it) --')
    for who, i, u in sorted(both_jp, key=lambda t: -t[1]['jp'])[:12]:
        print('%-12s %7d %7d | %7d %7d   %s'
              % (who, i['jp'], i['en'], u['jp'], u['en'], ', '.join(sorted(i['stages'])[:5])))
    if len(both_jp) > 12:
        print('   ... and %d more owners' % (len(both_jp) - 12))

    ijp = sum(v['jp'] for v in I.values())
    ien = sum(v['en'] for v in I.values())
    ujp = sum(v['jp'] for v in U.values())
    uen = sum(v['en'] for v in U.values())
    print('\nIntegral: %d Japanese, %d English   USA: %d Japanese, %d English'
          % (ijp, ien, ujp, uen))
    unported = sum(i['jp'] for _, i, _ in targets
                   if any(s not in PORTED for s in i['stages']))
    print('Japanese strings whose owner has English on the USA disc, in stages the'
          ' port does NOT already own: %d' % unported)
    return 0


def _stage_bytes(sd, name):
    lba, _ = portio.entries(sd)[name]
    tags, payloads, offsets = portio.stage(sd, name)
    size = sum(len(v) for v in payloads.values())
    del tags, offsets, size
    # portio.stage() already resolved the stage; re-slice its raw extent so the
    # Gcx offsets inside the chunk stay meaningful.
    base = lba * 2048
    count = int.from_bytes(sd[base+2:base+4], 'little')
    return sd[base:base + count * 2048]


if __name__ == '__main__':
    _sys.exit(main())
