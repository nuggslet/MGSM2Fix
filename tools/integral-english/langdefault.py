#!/usr/bin/env python
"""Make English the power-on default, by rewriting one function.

    from langdefault import patch_for
    writes = patch_for(exe_bytes)      # {ram address: 4-byte word}

Integral is an (En,Ja) release and the language is one bit,
`GM_CONFIG_ENGLISH` (0x0100) in `GM_Configuration`, which is `linkvarbuf[2]`.
Four places in the game act on it (`radiomes.c:526` picks the English half of
a codec fragment, `radio.c:1320` `NO RESPONSE`, `movie.c:54` and
`jimctrl.c:390` pick the stream); the option screen sets it and the memory
card restores it. `linkvarbuf` is BSS, so at power-on the bit is clear and the
game starts in Japanese.

**This does not touch any text.** Everything this port wrote was written into
the slots the game reads either way, so the menus are English with or without
it. What the bit selects is *Integral's own* English: the codec and the
cutscene subtitle stream. Without it you get English menus and a Japanese
story until you visit the OPTION screen.

## Where the bit is set, and why there

`GCL_StartDaemon` runs exactly once, from `Main()`, and its second call is
`GCL_InitVar` - which reads `GM_Configuration`, zeroes the whole of
`linkvarbuf`, and writes the value back. So a store placed in that call's
**delay slot** lands before `GCL_InitVar`'s body and is carried through it by
the game's own code. Once per boot, before anything reads the bit, and the
option screen and the memory card still overwrite it afterwards exactly as
they do on a retail disc - so a player who chooses Japanese keeps Japanese.

## Why it fits in place

The function is 18 instructions and the rewrite is also 18. Three instructions
have to be found room for, and they are paid for by:

* the `nop` in `jal GCL_InitVar`'s delay slot, which now holds the store;
* the `nop` after `lw $ra`, which now holds the stack pop;
* turning the last call into a **tail call** - `GCL_ChangeSenerioCode` is a
  leaf that ends in `jr $ra`, so jumping to it with `$ra` already restored
  returns straight to `Main()` and saves the `jr $ra` word.

Nothing moves, nothing relocates, no free space is needed - which matters,
because there is none: every zero run in this executable's image turns out to
be `.sdata`/`.sbss` holding live library variables (`Hcount`, `GM_StageName`),
and `GCL_ResetSystem`, the one do-nothing function next door, is called from
0x8002AA68.

## Nothing is hardcoded

`find_start_daemon` matches the function by its exact instruction shape and
requires exactly one match; `linkvarbuf` is then read out of `GCL_InitVar`'s
own `lui`/`addiu` pair and confirmed by the two `lh` at +2 and +4 that the C
gives as `GM_GameLevel` and `GM_Configuration`. That is why this works
unchanged on the VR executable, where the function sits at a different address
and `linkvarbuf` is 0x23A0 lower.
"""
import struct

TADDR, HDR = 0x80010000, 0x800
GM_CONFIG_ENGLISH = 0x0100
CONFIG_INDEX = 2                      # GM_Configuration is linkvarbuf[2]
LEVEL_INDEX = 1                       # GM_GameLevel is linkvarbuf[1]

NOP = 0x00000000
JR_RA = 0x03E00008
ADDIU_SP_DOWN = 0x27BDFFE8            # addiu $sp, $sp, -0x18
ADDIU_SP_UP = 0x27BD0018              # addiu $sp, $sp, 0x18
SW_RA = 0xAFBF0010                    # sw    $ra, 0x10($sp)
LW_RA = 0x8FBF0010                    # lw    $ra, 0x10($sp)
ADDIU_A0_G = 0x24040067               # addiu $a0, $zero, 0x67   ('g')
MOVE_A0_ZERO = 0x00002021             # move  $a0, $zero
LUI_A1_8002 = 0x3C058002              # lui   $a1, 0x8002


def words_of(data, at, count):
    return list(struct.unpack_from('<%dI' % count, data, at))


def is_jal(word):
    return (word >> 26) == 0x03


def jal_target(word, pc):
    return (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2)


def jal(target):
    return 0x0C000000 | ((target >> 2) & 0x03FFFFFF)


def jump(target):
    return 0x08000000 | ((target >> 2) & 0x03FFFFFF)


def find_start_daemon(exe):
    """(ram address, the 18 words) of GCL_StartDaemon - exactly one match."""
    hits = []
    for off in range(HDR, len(exe) - 72, 4):
        w = words_of(exe, off, 18)
        if (w[0] == ADDIU_SP_DOWN and w[1] == SW_RA
                and is_jal(w[2]) and w[3] == NOP
                and is_jal(w[4]) and w[5] == NOP
                and is_jal(w[6]) and w[7] == NOP
                and w[8] == ADDIU_A0_G and w[9] == LUI_A1_8002
                and is_jal(w[10]) and (w[11] >> 26) == 0x09      # addiu $a1
                and is_jal(w[12]) and w[13] == MOVE_A0_ZERO
                and w[14] == LW_RA and w[15] == NOP
                and w[16] == JR_RA and w[17] == ADDIU_SP_UP):
            hits.append((TADDR + off - HDR, w))
    if len(hits) != 1:
        raise AssertionError(
            'GCL_StartDaemon: expected exactly one match, found %d%s'
            % (len(hits), ' at ' + ', '.join('0x%08X' % a for a, _ in hits) if hits else ''))
    return hits[0]


def linkvarbuf_of(exe, init_var):
    """linkvarbuf's address, read out of GCL_InitVar rather than assumed."""
    off = HDR + init_var - TADDR
    if not 0 <= off <= len(exe) - 64:
        raise AssertionError('GCL_InitVar 0x%08X is outside the executable' % init_var)
    w = words_of(exe, off, 16)
    # lui $s0, hi / addiu $s0, $s0, lo  - then lh $s2, 4($s0) and lh $s1, 2($s0)
    lui = next((k for k, x in enumerate(w) if (x >> 16) == 0x3C10), None)
    if lui is None or (w[lui + 1] >> 16) != 0x2610:
        raise AssertionError('GCL_InitVar does not load linkvarbuf into $s0 as expected')
    address = ((w[lui] & 0xFFFF) << 16) + struct.unpack('<h', struct.pack('<H', w[lui + 1] & 0xFFFF))[0]
    lh = {x for x in w if (x >> 26) == 0x21 and ((x >> 21) & 0x1F) == 16}   # lh rt, imm($s0)
    want = {0x86000000 | (18 << 16) | (CONFIG_INDEX * 2),                   # lh $s2, 4($s0)
            0x86000000 | (17 << 16) | (LEVEL_INDEX * 2)}                    # lh $s1, 2($s0)
    if not want <= lh:
        raise AssertionError(
            'GCL_InitVar does not read linkvarbuf[%d]/[%d] into $s1/$s2; the '
            'layout is not what this patch assumes' % (LEVEL_INDEX, CONFIG_INDEX))
    return address


def patch_for(exe):
    """{ram address: word} rewriting GCL_StartDaemon to default to English.

    Same 18 words in the same 72 bytes, the same five calls in the same order.
    """
    at, w = find_start_daemon(exe)
    parse_init = jal_target(w[2], at + 2 * 4)
    init_var = jal_target(w[4], at + 4 * 4)
    init_basic = jal_target(w[6], at + 6 * 4)
    set_loader = jal_target(w[10], at + 10 * 4)
    change_scenerio = jal_target(w[12], at + 12 * 4)

    config = linkvarbuf_of(exe, init_var) + CONFIG_INDEX * 2
    if config >> 16 != (config + 1) >> 16 or (config & 0xFFFF) >= 0x8000:
        # `lui` + a signed 16-bit offset has to reach it in two instructions
        raise AssertionError('GM_Configuration 0x%08X is not reachable as '
                             'lui/offset' % config)

    # The tail call needs GCL_ChangeSenerioCode to be a leaf that ends in
    # `jr $ra`: if it kept a frame, returning past our restored $ra would
    # unbalance the stack. Checked, not assumed.
    off = HDR + change_scenerio - TADDR
    body = words_of(exe, off, 16)
    end = next((k for k, x in enumerate(body) if x == JR_RA), None)
    if end is None or any(x == ADDIU_SP_DOWN or (x >> 26) == 0x03 for x in body[:end]):
        raise AssertionError('GCL_ChangeSenerioCode is not a leaf; the tail '
                             'call in this patch would be wrong')

    new = [
        ADDIU_SP_DOWN,                                    #  0
        SW_RA,                                            #  1
        jal(parse_init),                                  #  2
        NOP,                                              #  3
        0x24030000 | GM_CONFIG_ENGLISH,                   #  4 addiu $v1,$zero,0x100
        0x3C020000 | (config >> 16),                      #  5 lui   $v0,0x800b
        jal(init_var),                                    #  6
        0xA4430000 | (config & 0xFFFF),                   #  7 sh    $v1,off($v0)  [delay slot]
        jal(init_basic),                                  #  8
        NOP,                                              #  9
        ADDIU_A0_G,                                       # 10
        LUI_A1_8002,                                      # 11
        jal(set_loader),                                  # 12
        w[11],                                            # 13 addiu $a1,$a1,imm
        LW_RA,                                            # 14
        ADDIU_SP_UP,                                      # 15 [fills the load delay]
        jump(change_scenerio),                            # 16 tail call
        MOVE_A0_ZERO,                                     # 17 [branch delay slot]
    ]
    assert len(new) == len(w) == 18
    return {at + 4 * k: word for k, word in enumerate(new)}


def describe(exe):
    at, w = find_start_daemon(exe)
    config = linkvarbuf_of(exe, jal_target(w[4], at + 4 * 4)) + CONFIG_INDEX * 2
    return ('GCL_StartDaemon at 0x%08X, GM_Configuration at 0x%08X'
            % (at, config))


if __name__ == '__main__':
    import sys
    from workdir import WORK
    for name in sys.argv[1:] or ['int1.exe', 'int2.exe', 'vrint.exe']:
        path = name if '/' in name or '\\' in name else WORK + '/' + name
        data = open(path, 'rb').read()
        writes = patch_for(data)
        print('%-10s %s, %d words' % (name, describe(data), len(writes)))
