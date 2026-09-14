"""Pull every subtitle out of RADIO.DAT by walking the game's own records.

    py radiotext.py                     what it finds, and the checks
    py radiotext.py --disc 2
    py radiotext.py --dump out.txt      every line, in file order
    py radiotext.py --check             THE REGRESSION GUARD on the
                                        fragment map - see check()
    py radiotext.py --selftest          slip bases on purpose; --check
                                        must catch every one

WHY NOT `japanese-inventory.tsv`

`jplist`'s scanner looks for runs of glyph codes and **ends a run at any code
it does not recognise**, so the code is dropped and the text after it starts a
new row. Two ranges it does not recognise carry real text: `0x91xx` (bank 0's
second kanji page, of which only four characters were ever identified) and
`0x97xx` (bank 1 above index 255 - the commentary's font blobs hold up to 441
glyphs and the codes run straight on into it). Measured over the commentary,
the inventory holds **85.2%** of the glyph instances; the visible symptom is
「無限バンダナは制作チーム内では昆」, where the 布 of 昆布 is a `0x9106` and the run stops
on it.

The fix is not a better scanner. The game knows exactly where its text is, so
walk what it walks.

THE GRAMMAR, FROM menu/radiomes.c

A block is `[marker][BE16 totalSize][records]`, ending at `+totalSize+1` or on
a 0 byte; a record is `FF <code> <BE16 size> <payload>` and the next one is at
`+size+2` (`menu_gcl_exec_block_800478B4`). The payloads that matter:

* **TALK** (`radio_anim_with_subtitles_800471AC`) - three BE16 words
  (chara, image, unk), then the subtitle to the end of the record.
* **IF** (`radio_if_80047514`) - a GCL value, then a nested block; then
  `FF 12 <value><block>` for each ELSEIF and `FF 11 <block>` for ELSE.
* **SWITCH** (`radio_switch_800475B8`) - a GCL value, then `21 <BE16 case>
  <block>` repeated, `22 <block>` for default, a 0 byte to end. Note the case
  marker is a bare byte here, *not* preceded by FF.
* **RANDSWITCH** (`radio_randSwitch_80047660`) - a BE16 (not a GCL value),
  then `31 <BE16 weight><block>` repeated, 0 to end.

GCL value sizes are `GCL_GetNextValue` (libgcl/parse.c), which is what
`gclparse.parse_values` already implements.
"""
import argparse
import collections
import contextlib
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import jptext
import radiomap
from workdir import WORK

GLYPH = 36
TALK, IF, ELSE, ELSEIF = 0x01, 0x10, 0x11, 0x12
SWITCH, CASE, DEFAULT, RANDSWITCH, RANDCASE = 0x20, 0x21, 0x22, 0x30, 0x31


def be16(d, p):
    return (d[p] << 8) | d[p + 1]


def value_end(d, p):
    """one GCL value at p -> the offset just past it (GCL_GetNextValue)"""
    t = d[p]
    if t & 0xF0 == 0x10:                 # GCL_VAR
        return p + 4
    if t == 0x00:
        return p + 1
    if t == 0x01:
        return p + 3                     # SHORT
    if t in (0x02, 0x03, 0x04):
        return p + 2                     # BYTE/CHAR/BOOL
    if t in (0x06, 0x08):
        return p + 3                     # STRID/PROCID
    if t == 0x07:
        return p + 2 + d[p + 1]          # STRING
    if t in (0x09, 0x0A):
        return p + 5                     # INT/SYMBOL
    if t == 0x20:
        return p + 2                     # ARRAY
    if t == 0x30:
        return p + 1 + d[p + 1]          # EXPR
    if t == 0x40:
        return p + 1 + be16(d, p + 1)    # ARG
    if t == 0x50:
        return p + 2 + d[p + 2]          # OPTION
    raise ValueError('value opcode %02X at 0x%X' % (t, p))


def block_end(d, p):
    return p + 1 + be16(d, p + 1)


def walk(d, s, limit, emit, depth=0, stats=None):
    """every TALK payload in the block at `s` and everything nested in it"""
    if depth > 24:
        return
    recs, _ = radiomap.walk_block(d, s, limit)
    for pos, code, size in recs:
        payload, rend = pos + 4, pos + size + 2
        if stats is not None:
            stats[radiomap.RDCODE[code]] += 1
        try:
            if code == TALK:
                emit(payload + 6, rend, depth)
            elif code == IF:
                p = value_end(d, payload)
                while p < rend:
                    walk(d, p, rend, emit, depth + 1, stats)
                    p = block_end(d, p)
                    if p + 1 >= rend or d[p] != 0xFF:
                        break
                    c = d[p + 1]
                    p += 2
                    if c == ELSEIF:
                        p = value_end(d, p)
                    elif c != ELSE:
                        break
            elif code in (SWITCH, RANDSWITCH):
                p = payload + 2 if code == RANDSWITCH else value_end(d, payload)
                while p < rend and d[p]:
                    c = d[p]
                    p += 1
                    if c in (CASE, RANDCASE):
                        p += 2
                    elif c != DEFAULT:
                        break
                    walk(d, p, rend, emit, depth + 1, stats)
                    if c == DEFAULT:
                        break
                    p = block_end(d, p)
        except (ValueError, IndexError):
            if stats is not None:
                stats['!' + radiomap.RDCODE[code]] += 1


def blobs(m):
    """fragment start -> its font blob, bounded by the NEXT fragment.

    Bounding matters: a commentary blob holds 224-441 glyphs and the codes
    reach index 511, so an unbounded slice reads the next fragment's bytes and
    renders plausible-looking wrong kanji.
    """
    frags = m['frags']
    starts = sorted(frags)
    out = {}
    for i, f in enumerate(starts):
        nxt = starts[i + 1] if i + 1 < len(starts) else len(m['radio'])
        out[f] = m['radio'][frags[f]:nxt]
    return out


def subtitles(m):
    """(fragment, offset, raw bytes) for every subtitle on the disc"""
    radio, frags = m['radio'], m['frags']
    starts = sorted(frags)
    out = []
    stats = collections.Counter()
    for i, f in enumerate(starts):
        base = frags[f]
        got = []
        walk(radio, f + 8, base, lambda a, b, dep: got.append((a, b)), 0, stats)
        for a, b in got:
            out.append((f, a, bytes(radio[a:b])))
    return out, stats


def render(raw, blob):
    """subtitle bytes -> text, resolving bank 1 through the fragment's font"""
    out, p = [], 0
    while p < len(raw):
        b = raw[p]
        if b < 0x80:
            out.append(chr(b) if 0x20 <= b < 0x7F else
                       ('\n' if b == 0x0A else ''))
            p += 1
            continue
        if p + 1 >= len(raw):
            break
        v = ((b << 8) | raw[p + 1]) & ~0x6000
        p += 2
        if 0x9600 <= v < 0x9A00 and blob is not None:
            i = radiomap.bank1_index(v) * GLYPH
            g = blob[i:i + GLYPH]
            ch = jptext.char_for_shape(g) if len(g) == GLYPH else None
            out.append(ch or '⟪%04X⟫' % v)
        else:
            out.append(jptext.glyph_char(v) or '⟪%04X⟫' % v)
    return ''.join(out)


def check(m, floor=50.0, min_lines=4, min_len=8):
    """per fragment: does its text also appear in some OTHER fragment?

    THE REGRESSION GUARD ON THE FRAGMENT MAP. `radiomap.py` prints two
    aggregates - distinct bitmaps produced and their blank-twelfth-row rate -
    and they catch a map that is grossly wrong. They caught the one that was:
    91,834 distinct bitmaps at 9.8% blank rows.

    They cannot catch ONE fragment's base going bad, because a base wrong by a
    whole number of glyphs still slices on glyph boundaries: every bitmap it
    reads is a real glyph with a real blank twelfth row. Valid bitmaps, wrong
    characters, aggregates unmoved, output fluent-looking nonsense.

    WHAT DOES NOT WORK, MEASURED, SO NOBODY REBUILDS IT:

    * "what share of this fragment's bank-1 codes hit a NAMED shape" - the
      obvious test, and useless now the table names everything. Slipping a
      base one glyph moves it from 100.00% to 99.89%: it just reads a
      different named shape.
    * "what share of its characters are in the corpus top 100" - real text
      over-samples common characters, so this looked promising. It is not:
      bank 1 holds the RARE kanji, so its frequency profile is flat and
      fragment-specific. Legitimate fragments run down to 34.1% and a slipped
      base sits at ~35%. No margin at all.

    WHAT WORKS. The commentary is duplicated across fragments, and each copy
    carries its OWN font blob with its OWN codes - so the same sentence is
    encoded differently in each and must still decode identically. That is
    the invariant that exposed the `0x97xx` index bug (one sentence rendering
    two ways), and it does not care how rare a fragment's vocabulary is.

    Measured on both discs at the correct bases: every commentary fragment
    scores 96.0% or better, the worst fragment anywhere with `min_lines` lines
    is 91.3%, and slipping one base by a single glyph puts that fragment at
    0.0%. The default floor of 50% sits in the middle of that gap.

    What it does NOT check is whether a shape is named CORRECTLY - that is the
    78-answer holdout in `glyphfill.py --score`. This checks the base. Two
    fragments holding the same text whose bases both slipped identically would
    also agree with each other, but a slip that systematic moves the aggregates
    `radiomap.py` already prints.

    ONLY LINES CONTAINING A BANK-1 CODE ARE SCORED, because those are the only
    lines a wrong base can change. Scoring all of a fragment's text dilutes the
    signal by however much of it is bank 0: fragment 0x03F6800 has 23 lines and
    two that use bank 1, so a slip there moves its all-lines score by 8.7% and
    walks straight through any sane floor. On bank-1 lines alone the same slip
    is unmissable.

    Fragments with no bank-1 line at all are reported separately rather than
    counted as passes - they have no base to get wrong, so passing is vacuous.
    One of them holds nothing but four copies of a leftover English developer
    warning.
    """
    fonts = blobs(m)
    try:
        subs, _ = subtitles(m)
    except Exception as exc:
        # a base wrong by enough puts the script length out of range, so the
        # walk throws before any text exists to compare. Still a detection.
        print('FAIL - the record walk itself threw: %s' % exc)
        print('       a fragment base is far enough out to break parsing')
        return 1
    raws = collections.defaultdict(list)
    for f, _off, raw in subs:
        raws[f].append(raw)

    lines, b1lines, over, codes, named = {}, {}, 0, 0, 0
    for f, rs in raws.items():
        blob = fonts[f]
        keep, hot = [], []
        for raw in rs:
            txt = render(raw, blob).strip()
            n = 0
            p = 0
            while p < len(raw):
                b = raw[p]
                if b < 0x80:
                    p += 1
                    continue
                if p + 1 >= len(raw):
                    break
                v = ((b << 8) | raw[p + 1]) & ~0x6000
                p += 2
                if not 0x9600 <= v < 0x9A00:
                    continue
                n += 1
                codes += 1
                i = radiomap.bank1_index(v) * GLYPH
                g = blob[i:i + GLYPH] if blob is not None else b''
                if len(g) != GLYPH:
                    over += 1              # index past the end of the blob
                elif jptext.char_for_shape(g):
                    named += 1
            if len(txt) >= min_len:
                keep.append(txt)
                if n:
                    hot.append(txt)        # a slip CAN change this line
        lines[f] = keep
        b1lines[f] = hot

    owners = collections.defaultdict(set)
    for f, ls in lines.items():
        for l in ls:
            owners[l].add(f)

    scored, thin, nobank = [], [], []
    for f, hot in b1lines.items():
        if not hot:
            # no line uses a bank-1 code, so this fragment has no base to get
            # wrong: slipping its blob cannot change a character. Counting it
            # as a pass would inflate the number that means something.
            nobank.append(f)
            continue
        if len(hot) < min_lines:
            thin.append((f, len(hot)))
            continue
        shared = sum(1 for l in hot if owners[l] - {f})
        scored.append((100.0 * shared / len(hot), f, len(hot)))
    scored.sort()

    print('fragments: %d checked, %d with no bank-1 line (nothing to check), '
          '%d too thin (<%d bank-1 lines of >=%d chars)'
          % (len(scored), len(nobank), len(thin), min_lines, min_len))
    print('bank-1 codes %d, named %d (%.3f%%), blob over-runs %d'
          % (codes, named, 100.0 * named / codes if codes else 100.0, over))
    if scored:
        print('cross-fragment agreement: worst %.1f%%, median %.1f%% (floor %.1f%%)'
              % (scored[0][0], scored[len(scored) // 2][0], floor))
    bad = [r for r in scored if r[0] < floor]
    if over:
        print('WARNING: %d bank-1 index/indices ran past the end of a blob' % over)
    if not bad and not over:
        print('OK - every checked fragment agrees with another; no base suspect')
        return 0
    if bad:
        print()
        print('%d fragment(s) below the floor - suspect their base in radiomap:'
              % len(bad))
        for share, f, n in bad[:20]:
            print('   0x%07X  %5.1f%%  %4d line(s)' % (f, share, n))
    return 1


def selftest(m, floor=50.0):
    """slip one fragment's base on purpose; --check must fail.

    A guard nobody has watched fail is not a guard. This is here because the
    FIRST version of `check` passed this test's ancestor with flying colours
    and was worthless: it measured the share of bank-1 codes hitting a NAMED
    shape, and since the table names everything, a slipped base just reads a
    different named shape (100.00% -> 99.89%). The second version scored
    characters against the corpus top 100 and had no margin at all - real
    fragments run to 34.1%, a slipped one sits at 35%. Only the third,
    cross-fragment agreement, separates them: 91.3% worst legitimate against
    0.0% slipped.

    Victims are drawn from exactly the set `check` claims to cover: fragments
    with `min_lines` or more lines that USE a bank-1 code. Anything thinner is
    reported by `check` as unchecked rather than passed, and asserting it gets
    caught would be asserting something never promised - 0x03F6800 has 23
    lines and two of them bank-1, and it is honestly out of scope.
    """
    fonts = blobs(m)
    subs, _ = subtitles(m)
    b1lines = collections.Counter()
    for f, _off, raw in subs:
        if len(render(raw, fonts[f]).strip()) < 8:
            continue
        p = 0
        while p < len(raw):
            b = raw[p]
            if b < 0x80:
                p += 1
                continue
            if p + 1 >= len(raw):
                break
            v = ((b << 8) | raw[p + 1]) & ~0x6000
            p += 2
            if 0x9600 <= v < 0x9A00:
                b1lines[f] += 1            # this LINE can move under a slip
                break
    # exactly check()'s checkable set - anything else it reports as too thin
    usable = sorted(f for f in b1lines if b1lines[f] >= 4)
    if not usable:
        print('selftest: no usable fragment')
        return 1
    victims = [usable[0], usable[len(usable) // 2], usable[-1]]

    print('baseline (no slip):')
    rc = check(m, floor)
    ok = (rc == 0)
    if not ok:
        print('  FAIL - the unmodified map should pass')
    for v in victims:
        for slip in (1, -1, 8):
            m2 = dict(m, frags=dict(m['frags']))
            m2['frags'][v] += slip * GLYPH
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                rc = check(m2, floor)
            named = ('0x%07X' % v) in buf.getvalue()
            threw = 'FAIL - the record walk' in buf.getvalue()
            good = rc != 0 and (named or threw)
            ok = ok and good
            print('  0x%07X slip %+d: %s' % (v, slip, 'caught' if good else
                                             '*** MISSED ***'))
    print('selftest: %s' % ('PASS' if ok else 'FAIL'))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--disc', type=int, default=1, choices=(1, 2))
    ap.add_argument('--dump', help='write every line here, in file order')
    ap.add_argument('--selftest', action='store_true',
                    help='slip a base on purpose and require --check to'
                         ' catch it')
    ap.add_argument('--check', nargs='?', type=float, const=50.0,
                    metavar='FLOOR',
                    help='regression check on the fragment map: every'
                         ' fragment must share text with another one.'
                         ' Exits non-zero below FLOOR percent (default 50)')
    args = ap.parse_args()

    jptext.load_shape_table()
    m = radiomap.build(args.disc - 1, verbose=False)
    if args.selftest:
        return selftest(m)
    if args.check is not None:
        return check(m, args.check)
    radio, frags = m['radio'], m['frags']
    subs, stats = subtitles(m)
    print('records walked:', ', '.join('%s %d' % kv for kv in stats.most_common()))
    print('subtitles found: %d' % len(subs))

    unknown = 0
    fonts = blobs(m)
    fh = io.open(args.dump, 'w', encoding='utf-8', newline='') if args.dump else None
    last = None
    for f, a, raw in subs:
        txt = render(raw, fonts[f])
        for _ in re.finditer(r'⟪[0-9A-F]{4}⟫', txt):
            unknown += 1
        if fh:
            if f != last:
                fh.write('\n--- fragment 0x%07X ---\n' % f)
                last = f
            fh.write(txt.rstrip() + '\n')
    if fh:
        fh.close()
        print('-> %s' % args.dump)
    print('unresolved glyph codes in the text: %d' % unknown)
    return 0


if __name__ == '__main__':
    sys.exit(main())
