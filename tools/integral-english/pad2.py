"""Port the controller-port message `second.c` draws - the last main-disc string
that had a USA counterpart and no patch family.

`mainsweep.py` pairs every GCL string on both discs by the command that owns it.
After the mission log went in, exactly one owner was left holding Japanese where
USA has English inside a stage no family covers: `chara 2D0A` in `s07b`, the
Psycho Mantis room (`stage/s07b.c` registers `CHARA_PSYCHOMANTIS`). The actor is
`CHARA_2D0A_2ND` -> `NewSecond` (`game/second.c`), 45 lines long, and it takes
exactly ONE string per spawn:

    work->message = GCL_GetString(GCL_NextStr());
    ...
    if (GV_PadData[1].status && work->using_pad2 == FALSE)
        MENU_JimakuWrite(work->message, 20000);   /* pad 2 went live: nag */
    else if (work->using_pad2 == TRUE && GV_PadData[0].status)
        MENU_JimakuClear();                       /* pad 1 came back: clear */

So it is the subtitle shown when the controller moves to port 2 for the Mantis
fight, telling the player to put it back. Integral's string reads
コントローラ端子1のコントローラを|使用してください。 - drawn with `rendertext.py`,
since three of its codes fall outside the located font bank; read the PNG, not
the codes. USA's is `PLUG CONTROLLER INTO | CONTROLLER PORT 1.`

FIVE SITES, NOT ONE, AND NO SWEEP SAW THEM ALL

Because the actor takes one string per spawn, the "two records" the notes
described are two separate SPAWNS, in two branches of the same stage script:

    site A   string at script body+0x88C   (elif 1903 -> if 2043)
    site B   string at script body+0xC3A   (elif 2845 -> if 2985)

USA has English at site B (its own body+0xC0E) and Japanese at site A, which is
its own inconsistency: both branches hand the same message to the same actor, so
copying USA's placement literally would leave a player who trips the other
branch reading Japanese. The user's call, 2026-09-08, was to port every site;
the text is USA's own either way, and the rule exists to forbid invention, not
to reproduce an oversight.

A third site is in `s07br`, at body+0xC3A. That stage is one of the 13
Integral-only names, so `mainsweep.py` cannot see it: it walks the 82 stage
names the two discs share, which puts every Integral-only stage outside its
universe - a second blind spot beside the one `NextSteps.md` 5.14 records. Its
overlay source is byte-identical to `s07b`'s (`diff` is empty), so it is the
same code over different stage data.

Three sites per disc, two discs. Both discs hold both stages at the same LBA
with identical bytes, so the two PPFs are the same writes at the same offsets.

WHY NOTHING HAS TO MOVE

A GCL STRING is length-prefixed and `GCL_GetNextValue` advances by that length
byte, never by `strlen` (README, "Why `en_menu3` is raw-disc only"). USA's 42
bytes fit inside Integral's 55-byte slot, so the length byte stays at 55, the
English and its terminator go in at the front, and the walk steps over the same
55 bytes it always did. `MENU_JimakuWrite` only stores the pointer and
`font_print_string` reads to the NUL (`menu/jimaku.c`), so the tail is never
looked at. No container to re-stamp, no sector to grow, no relocation, no
DUMMY3M slot - the smallest shape a text patch in this project can have.

The tail is filled with spaces after the terminator and the record's own final
NUL is left in place, which is `menu2.py`'s convention. The width needs no
check: this is the very line USA draws at the same call site, and jimaku centres
it itself (`field_4_x = (FRAME_WIDTH - max_width) / 2`).

The English is read out of `usa1_stage.dir` at build time rather than retyped -
the port takes USA text from the real USA discs (README, "The English source
discs"), and a string typed from a screenshot is a string that can be wrong.

    py pad2.py
"""
from pathlib import Path

import portio
from iso import Disc
from portio import INTEGRAL_IMAGES, changed_runs, map_runs, ppf
from vrlib import Gcx, be16, be32, cache_files, chunk_index, parse_arg, walk_commands
from workdir import GAME, WORK

CMD_CHARA = 0x9906
ACTOR = 0x2D0A          # CHARA_2D0A_2ND -> NewSecond, game/second.c
STAGES = ('s07b', 's07br')      # s07br is Integral-only; absent from USA

# コントローラ端子1のコントローラを|使用してください。 - font indices, not
# Shift-JIS, so it is matched as bytes and read with rendertext.py.
JAPANESE = bytes.fromhex(
    '821382538228824dd00682499a0190a78031812e821382538228824dd0068249'
    '8152807c9048904981178126810f812081158104d00300')


def sites(data, name):
    """(offset within the .dir file, payload) for every string second.c is
    handed in this stage. Offsets are .dir-relative because that is what
    `map_runs` wants, the same way `menu2.py` diffs the whole archive."""
    base = portio.entries(data)[name][0] * 2048
    tags, payloads, offsets = portio.stage(data, name)
    ci = chunk_index(tags)
    chunk = payloads[ci]
    for ext, _tid, start, _end in cache_files(tags):
        if ext != 'g':
            continue
        gcx = Gcx(chunk, start)
        # Gcx keeps the parsed bodies, not their positions: the script body
        # follows the proc table and every proc body, each length-prefixed.
        script_at = start + 4 + be32(chunk, start) + 4
        body = gcx.script
        for cmd, _lang, _path in walk_commands(body, parse_arg(body)):
            if cmd.id != CMD_CHARA:
                continue
            args = cmd.args()
            if not args or args[0].kind != 'STRID':
                continue
            if be16(body, args[0].pos + 1) != ACTOR:
                continue
            for value in cmd.values:
                if value.kind != 'STRING':
                    continue
                off = base + offsets[ci] + script_at + value.pos + 2
                payload = body[value.pos + 2:value.end]
                # The offset arithmetic above is worth proving rather than
                # trusting: the record's own length byte must be where it says.
                assert data[off - 1] == len(payload), (name, hex(off))
                assert data[off:off + len(payload)] == payload, (name, hex(off))
                yield off, payload


def fill(payload, english):
    """USA's line inside Integral's slot, with nothing moved.

    The terminator goes immediately after the text. Padding BEFORE it would
    be drawn, and `font_print_string` measures what it draws, so trailing
    spaces would widen `max_width` and pull jimaku's own centring off. The
    record's final NUL is left in place, which is `menu2.py`'s convention;
    the dead tail between the two is never read.
    """
    assert payload and payload[-1] == 0, 'slot is not NUL-terminated'
    assert english and english[-1] == 0, 'replacement is not NUL-terminated'
    cap = len(payload) - 1
    assert len(english) <= cap, (len(english), cap)
    return english.ljust(cap, b' ') + payload[cap:]


def english_from_usa(work):
    """USA's own line, taken from the real USA disc's archive."""
    data = (work / 'usa1_stage.dir').read_bytes()
    found = [payload for _, payload in sites(data, 's07b')]
    latin = [p for p in found if p[-1] == 0 and all(0x20 <= c < 0x7F for c in p[:-1])]
    assert len(found) == 2 and len(latin) == 1, [p.hex() for p in found]
    assert [p for p in found if p not in latin] == [JAPANESE]
    return latin[0]


def build(original, english):
    out = bytearray(original)
    present = portio.entries(original)
    count = 0
    for name in STAGES:
        assert name in present, name
        for off, payload in sites(original, name):
            assert payload == JAPANESE, (name, hex(off), payload.hex())
            out[off:off + len(payload)] = fill(payload, english)
            count += 1
    assert len(out) == len(original)
    return bytes(out), count


def main():
    work = Path(WORK)
    english = english_from_usa(work)
    print('USA: %r (%d bytes)' % (english[:-1].decode('ascii'), len(english)))
    for disc, base in enumerate(INTEGRAL_IMAGES):
        original = (work / ('int%d_stage.dir' % (disc + 1))).read_bytes()
        ported, count = build(original, english)
        image = Disc(Path(GAME) / 'windata/dlc/dlc_japan.bin', base)
        try:
            lba = next(l for n, l, s, d in image.walk()
                       if n.upper() == '/MGS/STAGE.DIR;1')
        finally:
            image.f.close()
        runs = list(changed_runs(original, ported))
        result = ppf(map_runs(lba, runs),
                     'MGS Integral: English controller port message')
        name = work / ('INTEGRAL_disc%d_en_pad2.ppf' % (disc + 1))
        name.write_bytes(result)
        print('%s  %d site(s), %d run(s), %d byte(s)'
              % (name, count, len(runs), sum(len(d) for _, d in runs)))


if __name__ == '__main__':
    main()
