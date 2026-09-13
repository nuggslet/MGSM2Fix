#!/usr/bin/env python
"""Port the USA VR Missions window text into Integral's VR-DISC stages.

Every mission stage's scenerio.gcx spawns the briefing window (`chara`
0x9906 with vrwindow 0xD44E) from `if` trees keyed on the mode/level
variables; the window's -i is its line count and -b its lines (koba/vr/
vrwindow.c: `i_num`, `b_text[16]`, 18 px per line, the window height is
i_num*18+18 and it is centred). USA's scripts add one more `if` level per
window, on GCL variable 0x11, with the English text in the `var == 0` body
and German/French/Italian/Spanish in the else-if branches.

Integral's scripts are templates: a stage carries the windows of every
mission its family can host (vab_sud lists all nine weapons, vijkl_01 all
27 VR MISSION / VARIETY / PUZZLE / NG SELECTION windows), while USA's carry
only the missions the stage really hosts. So the English for a window is
looked up in a pool built from ALL USA stages, keyed by the two title lines
(whitespace-normalised: USA writes `LEVEL  01`, Integral `LEVEL 01`) and
the numbers in the body (`15 sec.` / `Ammo 5` / `Enemies 1` are the same
numbers in both). The same-stage window wins when the stage has one; a key
that appears with different text in different USA stages is reported and
skipped. Windows with no English anywhere stay Japanese and are listed.

For a matched window -w -m -i -b come from USA: the window size, margins,
line count and lines. -f -p -s and everything else stay Integral's. The
script-local font (the descriptions' Japanese glyphs) is dropped when no
remaining string references it; that is what keeps most stages inside their
original sector count.

usage: vr_windows.py [stage ...]      measure only (no files written)
       vr_windows.py --build          write work/INTEGRAL_vr_en_missions.ppf
       vr_windows.py --deploy         also copy it into the mods folder

Stage filters cannot be combined with --build/--deploy: the runtime companion
requires all five grenade briefing copies from a complete mission build.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import re, struct, sys
import portio
from audit_text import game_text
from vrlib import (INT_STAGE, USA_STAGE, int_disc, stage_lba, stage_bytes, stage_gcx, repack_stage,
                   parse_arg, emit_arg, windows_in, walk_commands, option_bytes, ENGLISH,
                   inplace_records, write_ppf, deploy, Bad, Gcx, WORK, fit_in_place)
import widths
from vr_grenade import BRIEFING_STAGES, correct_briefing, write_mission_metadata

# Integral's own advances, because Integral's font is what draws these windows.
# Measured 2026-09-07: identical to Integral's main-game font for all 96 ASCII
# glyphs, which is what lets the main game's on-screen measurements carry over.
FONT = widths.font(INT_STAGE)

PPF_NAME = 'INTEGRAL_vr_en_missions.ppf'
DESC = 'MGS Integral VR-DISC: English mission text'
TAKE = 'wmib'                     # window size, margins, line count, lines: USA's
LOCAL_FONT_FIRST = 0x9A00         # font.c get_zen_font_data: codes >= 0x9A00 are the script's own font


def glyph_codes(s):
    out, p = [], 0
    while p < len(s):
        c = s[p]
        if c < 0x80:
            out.append(0x8000 | c); p += 1
        else:
            out.append((c << 8) | s[p+1]); p += 2
    return out


def bare(c):
    """the glyph code without its style flags (font.c: code &= ~0x6000)"""
    return c & ~0x6000


def is_local(c):
    return c >= 0x8000 and bare(c) >= LOCAL_FONT_FIRST


def code_of(index):
    """glyph index (1-based) -> script-local code (font.c get_zen_font_data inverted:
    codes ending in 00 are skipped, so index 256 is 0x9B01)"""
    return 0x9A00 + (index if index < 256 else index + 1)


def index_of(code):
    a = (bare(code) - 0x9A00) % 512
    return a - a // 256


def encode_codes(codes):
    out = bytearray()
    for c in codes:
        if c & 0xFF00 == 0x8000 and c < 0x8080:
            out.append(c & 0x7F)
        else:
            out += bytes((c >> 8, c & 0xFF))
    return bytes(out)


def strings_of(d, v):
    if v.kind == 'STRING':
        return [d[v.pos+2:v.end]]
    if v.kind == 'OPTION':
        out = []
        for w in v.values:
            out += strings_of(d, w)
        return out
    return []


def all_strings(gcx):
    out = set()
    for body in [b for _, b in gcx.procs] + [gcx.script]:
        for c, lang, path in walk_commands(body, parse_arg(body)):
            for v in c.values:
                out.update(strings_of(body, v))
    return out


def uses_local_font(gcx, usa_strings=()):
    """does any STRING still reference a local glyph? Strings USA's own script
    carries byte for byte do not count: USA ships them with NO local font, so
    the game never draws them through it (debug text such as `!<8A06><9A1F>`)."""
    for s in all_strings(gcx):
        if s in usa_strings:
            continue
        if any(is_local(code) for code in glyph_codes(s[:-1])):
            return True
    return False


def collect(gcx):
    """{proc id or 'script': [Window...]} in script order, plus the parsed blocks"""
    out, blocks = {}, {}
    for pid, body in gcx.procs:
        block = parse_arg(body)
        blocks[pid] = block
        w = windows_in(body, block, pid)
        if w:
            out[pid] = w
    block = parse_arg(gcx.script)
    blocks['script'] = block
    w = windows_in(gcx.script, block, 'script')
    if w:
        out['script'] = w
    return out, blocks


def check_identity(gcx):
    for pid, body in gcx.procs:
        assert emit_arg(body, parse_arg(body), {}) == body, 'serialiser mismatch in proc %04X' % pid
    assert emit_arg(gcx.script, parse_arg(gcx.script), {}) == gcx.script, 'serialiser mismatch in script'


def text(rec):
    t, jp = game_text(rec[:-1])
    return t if t is not None else rec.hex()


def norm_title(rec):
    """USA writes `LEVEL  01` and `FA-MAS`, Integral `LEVEL 01` and `FAMAS` / `C 4`:
    compare letters and digits only"""
    return re.sub(r'[^0-9A-Za-z]', '', text(rec)).upper()


def is_ascii(rec):
    t, jp = game_text(rec[:-1])
    return t is not None and not jp


def key_of(body, win):
    """the two title lines, normalised; None when they are not plain ASCII (movie descriptions)"""
    recs = win.records(body)
    if len(recs) < 2 or not (is_ascii(recs[0]) and is_ascii(recs[1])):
        return None
    return (norm_title(recs[0]), norm_title(recs[1]))


NUMBER = re.compile(r'(?<![A-Za-z])\d+(?![A-Za-z])')     # 15, 43, 100 - but not the 4 of C4 or the 3 of E3


def nums_of(body, win):
    recs = win.records(body)
    plain = ' '.join(re.sub(r'<[0-9A-F]{2,4}>', ' ', text(r)) for r in recs[2:])
    return tuple(NUMBER.findall(plain))


def canonical(body, win):
    """the bytes that will be taken: the w m i b options, in that order"""
    out = []
    for letter in TAKE:
        o = win.cmd.option(letter)
        if o is None:
            raise Bad('window at %X lacks -%s' % (win.cmd.pos, letter))
        out.append(option_bytes(body, letter, o.values))
    return b''.join(out)


def usa_pool(usa_sd, names):
    """key -> {stage: [(canonical bytes, nums) in script order]} over every USA
    stage's English windows; plus each stage's English windows per proc in order
    (for the positional fallback) and every USA string (for the font check)"""
    pool, per_stage, strings, fonts = {}, {}, {}, {}
    for name in names:
        data = stage_bytes(usa_sd, name)
        try:
            tags, pay, ci, files, gcx = stage_gcx(data)
        except AssertionError:
            continue
        try:
            wins, _ = collect(gcx)
        except Bad as e:
            raise Bad('USA %s: %s' % (name, e))
        strings[name] = all_strings(gcx)
        fonts[name] = gcx.font
        mine = {}
        for key, ws in wins.items():
            body = gcx.script if key == 'script' else gcx.procs[gcx.proc(key)][1]
            eng = [w for w in ws if w.lang in (None, ENGLISH)]
            mine[key] = [(canonical(body, w), nums_of(body, w), key_of(body, w)) for w in eng]
            for c, n, k in mine[key]:
                if k is not None:
                    pool.setdefault(k, {}).setdefault(name, []).append((c, n))
        per_stage[name] = mine
    return pool, per_stage, strings, fonts


def choose(k, name, proc, ordinal, pool, per_stage, fallback):
    """the (canonical, nums) for the ordinal-th window titled k in stage `name`:
    the same stage first, else any USA stage listing that title at least that
    often (the scripts are templates of one text), else nothing"""
    if k is None:
        # movie-style windows without ASCII titles: pair by position in the same proc
        eng = per_stage.get(name, {}).get(proc, [])
        if fallback is not None and len(eng) == fallback:
            return eng[ordinal][:2], 'position'
        return None, 'no titles and no positional match'
    same = pool.get(k, {}).get(name)
    if same is not None and ordinal < len(same):
        return same[ordinal], 'stage'
    variants = pool.get(k)
    if not variants:
        return None, 'no English anywhere'
    texts = {}
    for stage, lst in variants.items():
        if ordinal < len(lst):
            texts.setdefault(lst[ordinal], []).append(stage)
    if not texts:
        return None, 'English only for the first %d of this title' % max(len(l) for l in variants.values())
    if len(texts) > 1:
        return None, 'ambiguous: %d texts in %s' % (len(texts), sorted(sum(texts.values(), [])))
    (cn, where), = texts.items()
    return cn, 'pool(%s)' % where[0]


def rebuild_window(body, win, taken):
    """the Integral window command with its w m i b replaced by `taken` (canonical bytes)"""
    # split the canonical blob back into per-letter option bytes
    parts, p = {}, 0
    for letter in TAKE:
        L = taken[p+2]
        end = p + 2 + L
        parts[letter] = taken[p:end]
        p = end
    assert p == len(taken)
    c = win.cmd
    vals = bytearray()
    for v in c.values:
        if v.kind == 'OPTION' and v.letter in TAKE:
            vals += parts[v.letter]
        elif v.kind == 'OPTION':
            vals += option_bytes(body, v.letter, v.values)
        else:
            vals += body[v.pos:v.end]
    hdr = struct.pack('>H', c.id) + bytes((c.ofs,)) + bytes(vals)
    return bytes((0x60,)) + struct.pack('>H', len(hdr) + 2) + hdr


def fit_to_usa(new, old, W):
    """Never let a substituted line render wider than USA's own.

    The column-keeping rule below pads or eats whole *spaces*, and a space is
    not the width of the digit it stands in for - this font is proportional. On
    one line that left the result 4 px wider than the line USA ships in the very
    same window (`vr_sud09`, 193 px against 189). USA's width is the only budget
    here that is known to be safe, because USA drew it and shipped it; the
    engine's own budget cannot be the test, since retail Integral itself has 107
    lines over that and works. So take a space back until the line is no wider
    than its donor.
    """
    target = widths.width(old[:-1], W)
    body = bytearray(new[:-1])
    while widths.width(bytes(body), W) > target:
        run = max(re.finditer(rb'  +', bytes(body)), key=lambda m: m.end() - m.start(),
                  default=None)
        if run is None:
            break                       # nothing left to give; the assert reports it
        del body[run.start()]
    return bytes(body) + b'\0'


def substitute_numbers(records, inums, unums, W=None):
    """USA's lines with Integral's numbers where the two differ (same count of
    numbers; a right-aligned number keeps its column by eating or adding spaces,
    and `fit_to_usa` then keeps the pixel width no greater than USA's)"""
    out = []
    k = 0
    for i, r in enumerate(records):
        t = r[:-1]
        if i < 2 or not is_ascii(r):            # the two title lines are not part of the numbers
            out.append(r)
            continue
        pieces, pos, changed = [], 0, False
        for m in re.finditer(rb'(?<![A-Za-z])\d+(?![A-Za-z])', t):
            want = inums[k].encode()
            have = m.group()
            k += 1
            pieces.append(t[pos:m.start()])
            if want != have:
                changed = True
                delta = len(want) - len(have)
                if delta < 0:
                    pieces[-1] = pieces[-1] + b' ' * (-delta)      # keep the column: pad before
                elif delta > 0 and pieces[-1].endswith(b' ' * delta):
                    pieces[-1] = pieces[-1][:-delta]
            pieces.append(want)
            pos = m.end()
        pieces.append(t[pos:])
        if not changed:
            out.append(r)
            continue
        rec = b''.join(pieces) + b'\0'
        if W is not None:
            rec = fit_to_usa(rec, r, W)
            assert widths.width(rec[:-1], W) <= widths.width(r[:-1], W), \
                'substitution widened a line past USA\'s own: %r -> %r' % (r, rec)
        out.append(rec)
    assert k == len(unums), (k, unums)
    return out


def split_taken(taken):
    parts, p = {}, 0
    for letter in TAKE:
        L = taken[p+2]
        end = p + 2 + L
        parts[letter] = taken[p:end]
        p = end
    assert p == len(taken)
    return parts


def b_records(opt):
    """the STRING payloads of a -b option's bytes"""
    out, p = [], 3
    while p < len(opt):
        assert opt[p] == 7
        L = opt[p+1]
        out.append(opt[p+2:p+2+L])
        p += 2 + L
    return out


def b_option(records):
    payload = b''.join(bytes((7, len(r))) + r for r in records)
    return bytes((0x50, ord('b'), (len(payload) + 1) & 0xFF)) + payload


def port_stage(name, int_sd, pool, per_stage, usa_strings, usa_font):
    idata = stage_bytes(int_sd, name)
    itags, ipay, ici, ifiles, igcx = stage_gcx(idata)
    check_identity(igcx)
    iwins, iblocks = collect(igcx)
    rep = dict(name=name, windows=0, ported=0, kept=[], sources={}, numdiff=[], subst=[])
    if not iwins:
        return rep, None
    ported_records = {}                 # record bytes -> source USA stage (for its font)
    for key, ws in iwins.items():
        body = igcx.script if key == 'script' else igcx.procs[igcx.proc(key)][1]
        assert all(w.lang is None for w in ws), '%s: Integral script has a language branch' % name
        replace = {}
        seen = {}
        for pos, w in enumerate(ws):
            rep['windows'] += 1
            k = key_of(body, w)
            ordinal = seen.get(k, 0)
            seen[k] = ordinal + 1
            taken, how = choose(k, name, key, pos if k is None else ordinal, pool, per_stage, len(ws) if k is None else None)
            if taken is None:
                rep['kept'].append((k or ('<no ASCII titles>', ''), how))
                continue
            cbytes, unums = taken
            source = how[how.index('(')+1:-1] if '(' in how else name
            inums = nums_of(body, w)
            if inums != unums:
                rep['numdiff'].append((k, inums, unums, how))
                if k is not None and len(inums) == len(unums) and not SUBSTITUTE_NUMBERS_OFF:
                    parts = split_taken(cbytes)
                    recs = substitute_numbers(b_records(parts['b']), inums, unums, FONT)
                    parts['b'] = b_option(recs)
                    cbytes = b''.join(parts[l] for l in TAKE)
                    rep['subst'].append((k, inums, unums))
            if cbytes == canonical(body, w):
                continue                                  # already identical (debug windows USA kept Japanese)
            for r in b_records(split_taken(cbytes)['b']):
                ported_records[r] = source
            replace[id(w.cmd)] = rebuild_window(body, w, cbytes)
            rep['ported'] += 1
            src = how.split('(')[0]
            rep['sources'][src] = rep['sources'].get(src, 0) + 1
        if replace:
            newbody = emit_arg(body, iblocks[key], replace)
            if key == 'script':
                igcx.script = newbody
            else:
                igcx.procs[igcx.proc(key)][1] = newbody
    if rep['ported'] == 0:
        return rep, None
    # The budget no window can lift: kcb->max_width is a u8 read with lbu, so a
    # line wider than 255 px cannot even be measured by the renderer. The
    # per-window budget is deliberately NOT asserted here - retail Integral
    # itself has 107 lines over it and works, so it is not the invariant it
    # looks like (widths.py says why). What IS enforced is that the port never
    # renders a line wider than USA does, which substitute_numbers guarantees.
    rep['widest_px'] = widths.check_ceiling(
        [r[:-1] for r in ported_records], name, FONT)
    # the script-local font. Integral's holds the Japanese descriptions' glyphs;
    # USA's the typographic quotes its English uses. Every remaining string decides:
    # Integral glyphs still referenced by strings that are not ported text keep
    # Integral's font; USA glyphs referenced by ported text are appended to it
    # (or form the whole font when nothing Japanese remains) and the ported
    # strings' codes are rewritten to the appended indices.
    remaining = [s for s in all_strings(igcx) if s not in ported_records
                 and any(is_local(c) for c in glyph_codes(s[:-1]))]
    base_font = igcx.font if remaining else b''
    n_base = len(base_font) // 36
    appended, remap = [], {}          # (source, code) -> new code
    for r, source in ported_records.items():
        for c in glyph_codes(r[:-1]):
            if is_local(c) and (source, bare(c)) not in remap:
                font = usa_font[source]
                gi = index_of(c)
                glyph = font[(gi-1)*36:gi*36]
                assert len(glyph) == 36, '%s: USA glyph %04X missing from %s font' % (name, c, source)
                appended.append(glyph)
                remap[(source, bare(c))] = code_of(n_base + len(appended))
    if remap:
        def rewrite(body):
            out = bytearray(body)
            for c, lang, path in walk_commands(body, parse_arg(body)):
                for v in c.values:
                    for sv in ([v] if v.kind == 'STRING' else [x for x in v.values if x.kind == 'STRING'] if v.kind == 'OPTION' else []):
                        rec = body[sv.pos+2:sv.end]
                        if rec in ported_records:
                            src = ported_records[rec]
                            codes = [((cc & 0x6000) | remap[(src, bare(cc))]) if is_local(cc) else cc for cc in glyph_codes(rec[:-1])]
                            new = encode_codes(codes) + b'\0'
                            assert len(new) == len(rec)
                            out[sv.pos+2:sv.end] = new
            return bytes(out)
        igcx.procs = [[pid, rewrite(b)] for pid, b in igcx.procs]
        igcx.script = rewrite(igcx.script)
    igcx.font = base_font + b''.join(appended)
    font_note = ('Integral+%d' % len(appended)) if remaining and appended else ('USA(%d)' % len(appended)) if appended else ('Integral' if remaining else 'none')
    new_gcx = igcx.build()
    g2 = Gcx(new_gcx, 0)
    check_identity(g2)
    # every glyph code left in the script must exist in the new font
    nglyphs = len(igcx.font) // 36
    for s2 in all_strings(g2):
        for c in glyph_codes(s2[:-1]):
            if is_local(c) and s2 not in usa_strings.get(name, set()) - set(ported_records):
                assert index_of(c) <= nglyphs, '%s: code %04X beyond the %d-glyph font' % (name, c, nglyphs)
    new_stage = repack_stage(idata, new_gcx)
    # Standalone grenade-fix compatibility: Integral's printed five-second
    # fuse is an authoring error, not a different mechanic. Keep the same
    # five digits as vr_grenade's English addon so patch order cannot undo it.
    # This helper also fixes the original Japanese scripts independently.
    if name in BRIEFING_STAGES:
        new_stage = correct_briefing(new_stage)
    rep.update(gcx_before=igcx.end - igcx.start, gcx_after=len(new_gcx), font_kept=font_note,
               sectors_before=len(idata)//2048, sectors_after=len(new_stage)//2048)
    return rep, (idata, new_stage)


SUBSTITUTE_NUMBERS_OFF = False

# The `movie` stage is ported here like any other - its clip descriptions are
# mission windows - but its RECORDS are left to vr_movie.py, which also rewrites
# the captions and the overlay in the same stage. Two patches writing one stage
# with different layouts is decided by nothing better than Ketchup's file-name
# order, and that is a silent dependency: rename a file, or leave the older
# _movie_e3 fallback beside them, and the result changes with no error anywhere.
# So this hands the ported stage over as a file and writes none of its bytes,
# leaving vr_movie the stage's only owner. The two PPFs are then disjoint by
# construction rather than by convention.
HANDOVER = {'movie': 'vr_movie_base.bin'}


def main():
    build = '--build' in sys.argv or '--deploy' in sys.argv
    only = [a for a in sys.argv[1:] if not a.startswith('--')]
    if build and only:
        raise SystemExit('Stage filters are inspection-only: omit --build/--deploy, '
                         'or omit stage names to build the complete mission PPF '
                         'and its five-digit companion.')
    int_sd = open(INT_STAGE, 'rb').read()
    usa_sd = open(USA_STAGE, 'rb').read()
    allnames = sorted(portio.entries(int_sd), key=lambda k: portio.entries(int_sd)[k][0])
    names = only or allnames
    pool, per_stage, usa_strings, usa_font = usa_pool(usa_sd, allnames)
    print('USA pool: %d window keys from %d stages' % (len(pool), len(per_stage)))
    disc = int_disc()
    records, grown, total, ported, kept, numdiffs, substs = [], [], 0, 0, {}, [], []
    for name in names:
        try:
            rep, result = port_stage(name, int_sd, pool, per_stage, usa_strings, usa_font)
        except Bad as e:
            print('%-9s FAILED: %s' % (name, e))
            continue
        total += rep['windows']
        ported += rep['ported']
        for k, how in rep['kept']:
            kept.setdefault((k, how), []).append(name)
        if result is None:
            if rep['windows']:
                print('%-9s windows %3d ported   0 (%s)' % (name, rep['windows'], '; '.join(sorted({h for _, h in rep['kept']})) or 'identical'))
            continue
        flag = ''
        if rep['sectors_after'] > rep['sectors_before']:
            flag = '  ** GROWS **'
            grown.append(name)
        else:
            if rep['sectors_after'] < rep['sectors_before']:
                flag = '  (padded to the original count)'
            lba = stage_lba(disc, int_sd, name)
            fitted = fit_in_place(result[0], result[1])
            if name in HANDOVER:
                open(_os.path.join(WORK, HANDOVER[name]), 'wb').write(fitted)
                flag += '  -> %s (vr_movie owns this stage)' % HANDOVER[name]
            else:
                records += inplace_records(lba, result[0], fitted)
        numdiffs.extend((name,) + t for t in rep['numdiff'])
        substs.extend((name,) + t for t in rep['subst'])
        print('%-9s windows %3d ported %3d %-22s gcx %6d -> %6d font %-8s sectors %3d -> %3d%s' % (
            name, rep['windows'], rep['ported'], str(rep['sources']), rep['gcx_before'], rep['gcx_after'],
            rep['font_kept'], rep['sectors_before'], rep['sectors_after'], flag))
    print('%d windows, %d ported; %d PPF records, %d bytes; grown: %s' % (
        total, ported, len(records), sum(len(d) for _, d in records), grown or 'none'))
    if numdiffs:
        print('windows whose numbers differ between Integral and the USA text taken (same-count cases carry Integral\'s numbers):')
        seen = set()
        for name, k, inums, unums, how in numdiffs:
            if (k, inums, unums) in seen:
                continue
            seen.add((k, inums, unums))
            result_note = ('Integral numbers in USA text' if len(inums) == len(unums)
                           else 'USA text unchanged (different count: not a number)')
            if name in BRIEFING_STAGES and k == ('WEAPONMODE', 'GRENADELEVEL02') and inums == ('5', '3'):
                result_note = 'four-second authoring correction (vr_grenade); Targets 3 unchanged'
            print('   %-40s Integral %s USA %s -> %s' % (
                ' | '.join(k) if k else '-', inums, unums, result_note))
    if kept:
        print('kept Japanese (no or ambiguous English):')
        for (k, how), stages in sorted(kept.items(), key=lambda kv: str(kv[0])):
            print('   %-40s %s: %s' % (' | '.join(k), how, ','.join(stages)))
    if build and not grown:
        data = write_ppf(_os.path.join(WORK, PPF_NAME), records, DESC)
        companion = write_mission_metadata(_os.path.join(WORK, PPF_NAME), int_sd, disc)
        print('wrote', _os.path.join(WORK, PPF_NAME), len(data), 'bytes')
        if '--deploy' in sys.argv:
            print('deployed', deploy(PPF_NAME, data))
            print('deployed', deploy(companion.name, companion.read_bytes()))


if __name__ == '__main__':
    main()
