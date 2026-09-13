"""Shared code for the Integral VR-DISC (SLPM-86249) English port.

The VR disc is a third disc with its own executable and 105 stages; nothing
from the main-game port carries over except the file formats. This module holds
what every VR builder needs:

  * where the two VR images live inside the collection's containers and how
    Ketchup addresses the VR executable (`VR_EXE_IMAGE`)
  * `Gcx`: the GCL_LoadScript file (proc table, proc bodies, script body,
    script-local font) as found in a stage's cache section
  * a structural GCL parser (`parse_block`) that does NOT trust an OPTION's
    u8 length: the VR scripts overflow it routinely (an `if` whose else-if
    branch holds a dozen windows), and `gclparse.py` stops at the first one.
    Every container is re-serialised from its parts (`emit_block`), so an
    edit anywhere re-stamps every enclosing size. `emit_block` with no edits
    reproduces the original bytes; every builder asserts that first.
  * VR window extraction (`chara` commands spawning `vrwindow`, id 0xD44E)
    with the language branch each sits in (USA scripts switch on GCL variable
    0x11: 0 English, 1 German, 2 French, 3 Italian, 4 Spanish)
  * in-place stage patching: PPF records for the bytes that changed inside a
    stage that kept its sector count

Working data (see workdir.py): work/vrint_stage.dir, work/vrus_stage.dir,
work/vrint.exe (built from the decomp, `build.py --variant vr_exe`, SHA-256
c370f8e4...; the collection's copy is zero-filled), work/vrus.exe (retail USA).
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, GAME
import struct
import portio
from iso import Disc

# --------------------------------------------------------------- the discs

INT_CONTAINER = GAME + '/windata/dlc/dlc_japan.bin'
USA_CONTAINER = GAME + '/windata/alldata.bin'
INT_VR_BASE = 0x57592000          # SLPM_862.49 image inside dlc_japan.bin
USA_VR_BASE = 0xD39B7000          # SLUS_009.57 image inside alldata.bin
VR_EXE_IMAGE = 0x000865F8         # Ketchup: image offset of RAM 0x80010000 (LBA 234)
VR_EXE_RANGE = 0x99800
INT_STAGE = WORK + '/vrint_stage.dir'
USA_STAGE = WORK + '/vrus_stage.dir'
INT_EXE = WORK + '/vrint.exe'
USA_EXE = WORK + '/vrus.exe'
MODS = _os.path.join(GAME, 'mods/INTEGRAL/VR-DISK')
TADDR, HDR = 0x80010000, 0x800
SECTOR_DATA, SECTOR_RAW = 0x800, 0x930

VR_LANG_VAR = 0x11                # GCL variable the USA scripts switch text on
ENGLISH = 0
VRWINDOW = 0xD44E                 # GV_StrCode("vrwindow"), the mission text window
CMD_IF, CMD_CHARA = 0x0D86, 0x9906


def int_disc():
    d = Disc(INT_CONTAINER, INT_VR_BASE)
    assert any('SLPM_862.49' in n for n, _, _, _ in d.walk()), 'not the Integral VR disc'
    return d


def usa_disc():
    d = Disc(USA_CONTAINER, USA_VR_BASE)
    assert any('SLUS_009.57' in n for n, _, _, _ in d.walk()), 'not the USA VR disc'
    return d


def files_of(disc):
    return {n.upper(): (l, s) for n, l, s, d in disc.walk() if not d}


def stage_lba(disc, stage_dir_bytes, name):
    """LBA of a stage's first sector on the disc (STAGE.DIR lba + entry sector)."""
    sd_lba, _ = files_of(disc)['/MGS/STAGE.DIR;1']
    sector, _ = portio.entries(stage_dir_bytes)[name]
    return sd_lba + sector


def stage_bytes(stage_dir_bytes, name):
    sector, _ = portio.entries(stage_dir_bytes)[name]
    count = struct.unpack_from('<h', stage_dir_bytes, sector*2048 + 2)[0]
    return stage_dir_bytes[sector*2048:(sector+count)*2048]


def exe_image_offset(fo):
    """image offset of a byte at file offset `fo` of the VR executable"""
    k = fo - HDR
    return VR_EXE_IMAGE + (k // SECTOR_DATA) * SECTOR_RAW + (k % SECTOR_DATA)


ram = lambda fo: TADDR + fo - HDR
fofs = lambda a: a - TADDR + HDR


# --------------------------------------------------------------- cache section

def cache_files(tags):
    """[(ext, id, start, end)] of the files inside the 'c' payload; the non-0xFF
    'c' tags carry offsets, the 0xFF one the total (libfs/cdstage.c)."""
    ctags = [t for t in tags if t[1] == ord('c')]
    offs = [(chr(t[2]), t[0], t[3]) for t in ctags if t[2] != 0xFF]
    total = [t[3] for t in ctags if t[2] == 0xFF][0]
    out = []
    for i, (ext, tid, off) in enumerate(offs):
        end = offs[i+1][2] if i+1 < len(offs) else total
        out.append((ext, tid, off, end))
    return out


def chunk_index(tags):
    return [k for k, t in enumerate(tags) if t[1] == ord('c') and t[2] == 0xFF][0]


def be16(d, p): return (d[p] << 8) | d[p+1]
def be32(d, p): return (d[p] << 24) | (d[p+1] << 16) | (d[p+2] << 8) | d[p+3]


class Gcx:
    """One GCL_LoadScript file: BE32 proclen, proc table (id:BE16 off:BE16 ...,
    zero word), contiguous proc bodies (each a GCL ARG), BE32 script length,
    the script ARG, BE32 font length, the font."""

    def __init__(self, buf, start):
        proclen = be32(buf, start)
        tstart = start + 4
        q, table = tstart, []
        while be32(buf, q) != 0:
            table.append((be16(buf, q), be16(buf, q+2)))
            q += 4
        pbody = q + 4
        script_pos = tstart + proclen
        self.procs, p = [], pbody
        for pid, off in table:
            assert pbody + off == p, 'proc bodies not contiguous at %X' % p
            assert buf[p] == 0x40, 'proc body at %X is not GCL_ARG' % p
            L = be16(buf, p+1)
            self.procs.append([pid, bytes(buf[p:p+1+L])])
            p += 1 + L
        assert p == script_pos, 'bodies end %X, script at %X' % (p, script_pos)
        slen = be32(buf, script_pos)
        self.script = bytes(buf[script_pos+4:script_pos+4+slen])
        assert self.script[0] == 0x40 and be16(self.script, 1) + 1 == slen
        fpos = script_pos + 4 + slen
        flen = be32(buf, fpos)
        self.font = bytes(buf[fpos+4:fpos+4+flen])
        assert len(self.font) == flen
        self.end = fpos + 4 + flen
        self.start = start

    def build(self):
        table, bodies, off = bytearray(), bytearray(), 0
        for pid, body in self.procs:
            assert body[0] == 0x40 and be16(body, 1) + 1 == len(body)
            assert off < 0x10000
            table += struct.pack('>HH', pid, off)
            bodies += body
            off += len(body)
        table += bytes(4)
        assert self.script[0] == 0x40 and be16(self.script, 1) + 1 == len(self.script)
        out = struct.pack('>I', len(table) + len(bodies)) + table + bodies
        out += struct.pack('>I', len(self.script)) + self.script
        out += struct.pack('>I', len(self.font)) + self.font
        return bytes(out)

    def proc(self, pid):
        for i, (p, body) in enumerate(self.procs):
            if p == pid:
                return i
        return None


def stage_gcx(stage_data):
    """-> (tags, payloads, chunk index, cache files, Gcx of scenerio.gcx).
    Asserts scenerio.gcx is the last file in the cache section, which is what
    lets a rebuilt script grow without moving anything else."""
    tags, payloads, offsets = portio.stage(stage_data)
    ci = chunk_index(tags)
    files = cache_files(tags)
    g = [f for f in files if f[0] == 'g']
    assert len(g) == 1 and files[-1][0] == 'g', 'scenerio.gcx is not the last cache file: %r' % files
    gcx = Gcx(payloads[ci], g[0][2])
    # the cache section keeps its files 4-aligned: up to three zero bytes may
    # follow the script before the section total
    assert g[0][3] == len(payloads[ci]) and 0 <= g[0][3] - gcx.end < 4, (gcx.end, g[0][3], len(payloads[ci]))
    assert payloads[ci][gcx.end:g[0][3]] == bytes(g[0][3] - gcx.end), 'non-zero padding after scenerio.gcx'
    return tags, payloads, ci, files, gcx


def repack_stage(stage_data, new_gcx_bytes):
    """the stage with scenerio.gcx replaced (chunk total and 0xFF tag re-stamped)"""
    tags, payloads, ci, files, gcx = stage_gcx(stage_data)
    chunk = payloads[ci][:gcx.start] + new_gcx_bytes
    chunk += bytes(-len(chunk) % 4)              # keep the section 4-aligned
    payloads = dict(payloads)
    payloads[ci] = chunk
    return portio.pack_stage(tags, payloads)


# --------------------------------------------------------------- GCL structure

class Val:
    """a value in a GCL value list; kind in SHORT BYTE STRID STRING INT VAR
    ARRAY EXPR ARG OPTION END; ARG has .block (list of Cmd); OPTION has
    .letter and .values (its payload, parsed structurally)"""
    __slots__ = ('kind', 'pos', 'end', 'letter', 'values', 'block', 'u8')

    def __init__(self, kind, pos, end):
        self.kind, self.pos, self.end = kind, pos, end
        self.letter = self.values = self.block = self.u8 = None

    def __repr__(self):
        return '%s[%X,%X)%s' % (self.kind, self.pos, self.end, ' -' + self.letter if self.letter else '')


class Cmd:
    """`60 BE16 id(2) ofs(1) values... 00` or a PROC `70 u8 id(2) values...`.
    `tail` is whatever sits between the value list's END and the command's
    declared end: GCL_ExecBlock skips by the BE16, so those bytes are never
    read; they are carried verbatim."""
    __slots__ = ('kind', 'pos', 'end', 'id', 'ofs', 'values', 'tail')

    def __init__(self, kind, pos, end, cid, ofs, values, tail=b''):
        self.kind, self.pos, self.end, self.id, self.ofs, self.values = kind, pos, end, cid, ofs, values
        self.tail = tail

    def options(self):
        return [v for v in self.values if v.kind == 'OPTION']

    def option(self, letter):
        for v in self.values:
            if v.kind == 'OPTION' and v.letter == letter:
                return v
        return None

    def args(self):
        return [v for v in self.values if v.kind not in ('OPTION', 'END')]

    def __repr__(self):
        return 'CMD %04X[%X,%X)' % (self.id, self.pos, self.end)


class Bad(Exception):
    pass


def _value_end(d, p, limit):
    """end of the single value starting at p (without recursing)"""
    op = d[p]
    if (op & 0xF0) == 0x10: return p + 4
    if op == 0x00: return p + 1
    if op == 0x01: return p + 3
    if op in (0x02, 0x03, 0x04): return p + 2
    if op in (0x06, 0x08): return p + 3
    if op == 0x07: return p + 2 + d[p+1]
    if op in (0x09, 0x0A): return p + 5
    if op == 0x20: return p + 2
    if op == 0x30: return p + 1 + d[p+1]
    if op == 0x40: return p + 1 + be16(d, p+1)
    raise Bad('bad value opcode %02X at %X' % (op, p))


KINDS = {0x01: 'SHORT', 0x02: 'BYTE', 0x03: 'BYTE', 0x04: 'BYTE', 0x06: 'STRID', 0x08: 'STRID',
         0x07: 'STRING', 0x09: 'INT', 0x0A: 'INT', 0x20: 'ARRAY', 0x30: 'EXPR', 0x40: 'ARG'}


def parse_values(d, p, end, in_option=False, u8=None, opt_pos=None):
    """values from p to end. An OPTION's payload is parsed structurally and the
    u8 length is CHECKED against it modulo 256: the length is `payload+1` (parse.c
    GCL_GetNextValue), it overflows for the big else-if branches, and a payload
    may legitimately end with its own GCL_END (an `if`'s last branch)."""
    out = []
    while p < end:
        op = d[p]
        if op == 0x00:
            if in_option:
                # the END is inside the payload only if the u8 says so
                with_end = (p + 1 - (opt_pos + 2)) & 0xFF
                without = (p - (opt_pos + 2)) & 0xFF
                if with_end == u8 and without != u8:
                    out.append(Val('END', p, p+1))
                    p += 1
                break
            out.append(Val('END', p, p+1))
            p += 1
            break
        if op == 0x50:
            if in_option:
                break                     # next option begins: payload over
            letter = chr(d[p+1]) if 32 <= d[p+1] < 127 else '?'
            u8v = d[p+2]
            if u8v == 0:
                # `50 letter 00`: an empty option whose own length byte is the
                # END that follows it (GCL_GetNextValue: p += p[1] + 1 lands on it)
                v = Val('OPTION', p, p+2)
                v.letter, v.values, v.u8 = letter, [], 0
                out.append(v)
                p += 2
                continue
            vals, q = parse_values(d, p+3, end, in_option=True, u8=u8v, opt_pos=p)
            v = Val('OPTION', p, q)
            v.letter, v.values, v.u8 = letter, vals, u8v
            if (q - (p+2)) & 0xFF != u8v:
                raise Bad('option -%s at %X: u8 %d but payload %d' % (letter, p, u8v, q-(p+2)))
            out.append(v)
            p = q
            continue
        if (op & 0xF0) == 0x10:
            kind = 'VAR'
        elif op in KINDS:
            kind = KINDS[op]
        else:
            raise Bad('bad value opcode %02X at %X' % (op, p))
        q = _value_end(d, p, end)
        if q > end:
            raise Bad('%s overruns at %X' % (kind, p))
        v = Val(kind, p, q)
        if kind == 'ARG':
            v.block = parse_block(d, p+3, q)
        out.append(v)
        p = q
    return out, p


def parse_block(d, p, end):
    """a command block: COMMAND (0x60) / PROC (0x70) / EXPR (0x30) / END (0x00)"""
    out = []
    while p < end:
        op = d[p]
        if op == 0x00:
            out.append(Cmd('END', p, p+1, None, None, []))
            p += 1
            continue
        if op == 0x30:
            L = d[p+1]
            out.append(Cmd('EXPR', p, p+1+L, None, None, []))
            p += 1 + L
            continue
        if op == 0x60:
            L = be16(d, p+1)
            q = p + 1 + L
            if q > end:
                raise Bad('COMMAND overruns at %X' % p)
            vals, vend = parse_values(d, p+6, q)
            if vend > q:
                raise Bad('command %04X at %X: values end %X, command ends %X' % (be16(d, p+3), p, vend, q))
            out.append(Cmd('COMMAND', p, q, be16(d, p+3), d[p+5], vals, bytes(d[vend:q])))
            p = q
            continue
        if op == 0x70:
            L = d[p+1]
            q = p + 1 + L
            vals, vend = parse_values(d, p+4, q)
            if vend > q:
                raise Bad('proc call at %X: values end %X, wanted %X' % (p, vend, q))
            out.append(Cmd('PROC', p, q, be16(d, p+2), None, vals, bytes(d[vend:q])))
            p = q
            continue
        raise Bad('bad block opcode %02X at %X' % (op, p))
    if p != end:
        raise Bad('block ended at %X, wanted %X' % (p, end))
    return out


def parse_arg(d):
    """a proc body or the script body: `40 BE16 block`"""
    assert d[0] == 0x40 and be16(d, 1) + 1 == len(d), 'not a GCL ARG of its own length'
    return parse_block(d, 3, len(d))


# --------------------------------------------------------------- re-serialisation

def emit_values(d, values, replace):
    out = bytearray()
    for v in values:
        r = replace.get(id(v))
        if r is not None:
            out += r
            continue
        if v.kind == 'OPTION':
            if v.u8 == 0 and not v.values:
                out += bytes((0x50, ord(v.letter)))          # its END follows as the next value
                continue
            payload = emit_values(d, v.values, replace)
            L = len(payload) + 1
            out += bytes((0x50, ord(v.letter))) + bytes((L & 0xFF,)) + payload
        elif v.kind == 'ARG':
            body = emit_block(d, v.block, replace)
            out += bytes((0x40,)) + struct.pack('>H', len(body) + 2) + body
        else:
            out += d[v.pos:v.end]
    return bytes(out)


def emit_block(d, block, replace):
    """re-serialise a block; `replace` maps id(node) -> bytes for nodes to swap.
    Sizes of every enclosing COMMAND/PROC/ARG/OPTION are recomputed."""
    out = bytearray()
    for c in block:
        r = replace.get(id(c))
        if r is not None:
            out += r
            continue
        if c.kind in ('END', 'EXPR'):
            out += d[c.pos:c.end]
        elif c.kind == 'COMMAND':
            vals = emit_values(d, c.values, replace)
            body = struct.pack('>H', c.id) + bytes((c.ofs,)) + vals + c.tail
            out += bytes((0x60,)) + struct.pack('>H', len(body) + 2) + body
        elif c.kind == 'PROC':
            vals = emit_values(d, c.values, replace)
            body = struct.pack('>H', c.id) + vals + c.tail
            assert len(body) + 1 < 256, 'proc call too long'
            out += bytes((0x70, len(body) + 1)) + body
        else:
            raise Bad('cannot emit %s' % c.kind)
    return bytes(out)


def emit_arg(d, block, replace):
    body = emit_block(d, block, replace)
    return bytes((0x40,)) + struct.pack('>H', len(body) + 2) + body


def option_bytes(d, letter, values, replace=None):
    payload = emit_values(d, values, replace or {})
    return bytes((0x50, ord(letter), (len(payload) + 1) & 0xFF)) + payload


# --------------------------------------------------------------- expressions

OPS = {0: 'END', 1: '-u', 2: '!', 3: '~', 4: '+', 5: '-', 6: '*', 7: '/', 8: '%', 9: '==', 10: '!=',
       11: '<', 12: '<=', 13: '>', 14: '>=', 15: '|', 16: '&', 17: '^', 18: '||', 19: '&&', 20: '='}


def expr_tokens(d, pos, end):
    """the EXPR at pos: -> tokens (('var', id), ('num', n), ('op', name), ('other', kind)).
    GCL_Expr pushes any GCL value, so unknown operands are skipped by size."""
    assert d[pos] == 0x30
    p, toks = pos + 2, []
    while p < end:
        op = d[p]
        if op == 0x31:
            toks.append(('op', OPS.get(d[p+1], '?%d' % d[p+1])))
            p += 2
            if d[p-1] == 0:
                break
        elif (op & 0xF0) == 0x10:
            toks.append(('var', be32(d, p) & 0x0FFFFFFF))
            p += 4
        elif op in (0x02, 0x03, 0x04):
            toks.append(('num', d[p+1]))
            p += 2
        elif op == 0x01:
            toks.append(('num', struct.unpack('>h', d[p+1:p+3])[0]))
            p += 3
        elif op in (0x09, 0x0A):
            toks.append(('num', struct.unpack('>i', d[p+1:p+5])[0]))
            p += 5
        else:
            q = _value_end(d, p, end)
            toks.append(('other', KINDS.get(op, '%02X' % op)))
            p = q
    return toks


def language_of(d, expr):
    """None unless the expression is exactly `var 0x11 == N`; then N"""
    toks = expr_tokens(d, expr.pos, expr.end)
    # a variable reference is `1t ii ii ii`: type nibble t (2 = short in var_buf),
    # then the index; 0x00800000 set would mean linkvarbuf (GCL_GetVar)
    if (len(toks) == 4 and toks[0][0] == 'var' and (toks[0][1] & 0x00FFFFFF) == VR_LANG_VAR
            and toks[1][0] == 'num' and toks[2] == ('op', '==') and toks[3] == ('op', 'END')):
        return toks[1][1]
    return None


# --------------------------------------------------------------- windows

class Window:
    __slots__ = ('cmd', 'proc', 'lang', 'path')

    def __init__(self, cmd, proc, lang, path):
        self.cmd, self.proc, self.lang, self.path = cmd, proc, lang, path

    def records(self, d):
        b = self.cmd.option('b')
        return [d[v.pos+2:v.end] for v in b.values if v.kind == 'STRING'] if b else []

    def texts(self, d):
        from audit_text import game_text
        return [game_text(r[:-1])[0] for r in self.records(d)]


def walk_commands(d, block, lang=None, path=()):
    """yield (Cmd, lang, path) for every command in the block, recursively.
    `lang` is the innermost enclosing language branch (var 0x11 == N), None
    when the command is not under one."""
    for c in block:
        if c.kind not in ('COMMAND', 'PROC'):
            continue
        yield c, lang, path
        if c.kind == 'COMMAND' and c.id == CMD_IF:
            args = c.args()
            cond = args[0] if args and args[0].kind == 'EXPR' else None
            body = args[1] if len(args) > 1 and args[1].kind == 'ARG' else None
            here = language_of(d, cond) if cond is not None else None
            if body is not None:
                yield from walk_commands(d, body.block, here if here is not None else lang, path + (('if', c.pos),))
            for o in c.options():
                if o.letter == 'i':
                    vals = o.values
                    ce = vals[0] if vals and vals[0].kind == 'EXPR' else None
                    ba = vals[1] if len(vals) > 1 and vals[1].kind == 'ARG' else None
                    hl = language_of(d, ce) if ce is not None else None
                    if ba is not None:
                        yield from walk_commands(d, ba.block, hl if hl is not None else lang, path + (('elif', o.pos),))
                elif o.letter == 'e':
                    for v in o.values:
                        if v.kind == 'ARG':
                            yield from walk_commands(d, v.block, 'else' if cond is not None and language_of(d, cond) is not None else lang, path + (('else', o.pos),))
            continue
        # other commands: descend into any ARG blocks (args or option payloads)
        for v in c.values:
            if v.kind == 'ARG':
                yield from walk_commands(d, v.block, lang, path + ((c.id, c.pos),))
            elif v.kind == 'OPTION':
                for w in v.values:
                    if w.kind == 'ARG':
                        yield from walk_commands(d, w.block, lang, path + ((c.id, c.pos, v.letter),))


def windows_in(d, block, proc_id):
    out = []
    for c, lang, path in walk_commands(d, block):
        if c.kind == 'COMMAND' and c.id == CMD_CHARA:
            a = c.args()
            if a and a[0].kind == 'STRID' and be16(d, a[0].pos+1) == VRWINDOW:
                out.append(Window(c, proc_id, lang, path))
    return out


# --------------------------------------------------------------- PPF for a stage

def inplace_records(lba, original, modified, merge_gap=64):
    """PPF records (image offset, bytes) for the bytes that differ between two
    stage images of the same sector count. Runs separated by up to `merge_gap`
    unchanged bytes are written as one (the unchanged bytes are rewritten with
    their own values): a rebuilt script shifts everything after the first edit,
    which leaves thousands of one-byte runs, and every record costs Ketchup a
    call and a log line at boot."""
    assert len(original) == len(modified), 'stage changed size: relocate instead'
    runs = []
    for start, data in portio.changed_runs(original, modified):
        if runs and start - (runs[-1][0] + len(runs[-1][1])) <= merge_gap:
            pstart, pdata = runs[-1]
            runs[-1] = (pstart, bytes(modified[pstart:start + len(data)]))
        else:
            runs.append((start, bytes(data)))
    return list(portio.map_runs(lba, runs))


def write_ppf(path, records, description):
    data = portio.ppf(records, description)
    with open(path, 'wb') as f:
        f.write(data)
    return data


def deploy(name, data):
    _os.makedirs(MODS, exist_ok=True)
    with open(_os.path.join(MODS, name), 'wb') as f:
        f.write(data)
    return _os.path.join(MODS, name)


def fit_in_place(old_stage, new_stage):
    """a rebuilt stage that came out SHORTER keeps the original sector count:
    the header count stays, zero sectors pad the end (the loader walks the tags,
    never the sector count). A longer stage cannot be patched in place."""
    if len(new_stage) == len(old_stage):
        return new_stage
    if len(new_stage) > len(old_stage):
        raise Bad('stage grew from %d to %d sectors' % (len(old_stage)//2048, len(new_stage)//2048))
    out = bytearray(new_stage) + bytes(len(old_stage) - len(new_stage))
    struct.pack_into('<h', out, 2, len(old_stage)//2048)
    return bytes(out)
