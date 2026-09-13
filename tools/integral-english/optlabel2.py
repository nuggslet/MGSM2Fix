"""Rebuild the shipped option caption chain directly from retail.

Replaces the recovered scratchpad experiment: record 7's colon and all other
unowned Japanese records must survive. No pinned font-text PPF is an input.
The nine spaces in record 27 reproduce the verified layout, including its
container delta of -9 bytes relative to retail.
"""
import struct
from gclparse import parse_script, containers_over, be16, be32
from portio import records, encode_records

TEXT = {
    3: b' ',
    4: b'screen brightness setup',
    5: b'key configuration setup',
    12: b'use directional buttons to test',
    13: b'Adjust the monitor brightness so the',
    14: b'gray scale below the green line',
    15: b'cannot be seen, for the appropriate',
    16: b'brightness to play this game.',
    24: b'Press the \x90\x1b button to return to the',
    26: b'use directional buttons to test',
    27: b'option screen.         ',
}


def build(retail):
    original, end = records(retail)
    assert len(original) == 31
    updated = [TEXT[i]+b'\0' if i in TEXT else s for i, s in enumerate(original)]
    chain = encode_records(updated)
    delta = len(chain)-(end-0x1B8)
    assert delta == -9
    root, size = parse_script(retail, 0x172)
    out = bytearray(retail[:0x1B8]+chain+retail[end:]+bytes(-delta))
    for node in containers_over(root, 0x1B8, end):
        fmt, read = ('>I', be32) if node.size_bits == 32 else ('>H', be16)
        struct.pack_into(fmt, out, node.size_at, read(retail, node.size_at)+delta)
    tree, newsize = parse_script(out, 0x172)
    got, _ = records(out)
    assert newsize == size+delta and tree.kids[0].end == 0x172+newsize
    assert got == updated and len(out) == len(retail)
    return bytes(out)
