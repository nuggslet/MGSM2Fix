"""Tests for the parts of the toolchain that need no game data.

    py selftest.py            run them
    py selftest.py -v         say what each one checks

Until 2026-09-07 the only test this port had was `rebuild.py`, which needs an
installed collection, four retail executables, a decomp checkout and the PSY-Q
toolchain, and takes minutes. That is the right test for the *build*, and it is
no test at all for the pieces underneath it: a PPF emitter that must split runs
at two different boundaries, a texture codec with a run-length cap that was once
one short, a checksum with a rounding step that is easy to get subtly wrong.
Those are pure functions over bytes and they can be checked in a second.

What this deliberately does NOT do is check the algorithms against ground truth
- that needs the real discs. `py cdecc.py` is where the EDC/ECC is proved, by
recomputing retail sectors and matching what is stored on them, and `rebuild.py
--compare-deployed` is where the whole build is. This file checks the algebra
those rest on: round trips, boundary conditions, and the invariants each module
documents about itself.
"""
import os
import struct
import sys
import unittest
import tempfile
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import cdecc
import hazards
import langdefault
import pad2
import pcx4
import portio
import widths
import workdir


class PathResolutionTests(unittest.TestCase):
    def test_bad_explicit_paths_do_not_fall_back(self):
        with tempfile.TemporaryDirectory() as temp:
            missing = str(Path(temp) / 'missing')
            with patch.dict(os.environ, {'INTEGRAL_ENGLISH_GAME': missing}):
                with self.assertRaises(SystemExit): workdir.find_game()
            with patch.dict(os.environ, {'INTEGRAL_ENGLISH_DECOMP': missing}):
                with self.assertRaises(SystemExit): workdir.find_decomp()
            with patch.dict(os.environ, {'INTEGRAL_ENGLISH_WORK': missing}):
                with self.assertRaises(SystemExit): workdir._root()

    def test_explicit_game_overrides_environment(self):
        with tempfile.TemporaryDirectory() as temp:
            game = Path(temp)
            (game / 'windata').mkdir()
            (game / 'windata/alldata.bin').touch()
            with patch.dict(os.environ, {'INTEGRAL_ENGLISH_GAME': 'missing'}):
                self.assertEqual(workdir.find_game(game), game.as_posix())

    def test_decomp_requires_both_markers(self):
        with tempfile.TemporaryDirectory() as temp:
            decomp = Path(temp)
            (decomp / 'source/main').mkdir(parents=True)
            (decomp / 'source/main/main.c').touch()
            with self.assertRaises(SystemExit): workdir.find_decomp(decomp)
            (decomp / 'build').mkdir()
            (decomp / 'build/build.py').touch()
            self.assertEqual(workdir.find_decomp(decomp), decomp.as_posix())


class Ppf(unittest.TestCase):
    """portio's PPF3 emitter and reader"""

    def test_round_trip(self):
        records = [(0x1000, b'hello'), (0x2000, bytes(range(200)))]
        blob = portio.ppf(records, 'test')
        path = _tmp('rt.ppf', blob)
        self.assertEqual(portio.read_ppf(path), records)

    def test_runs_split_at_255(self):
        """A record's length is one byte, so nothing may exceed 255."""
        blob = portio.ppf([(0, bytes(1000))], 'long')
        path = _tmp('long.ppf', blob)
        out = portio.read_ppf(path)
        self.assertTrue(all(len(d) <= 255 for _, d in out))
        self.assertEqual(sum(len(d) for _, d in out), 1000)
        self.assertEqual(out[0][0], 0)

    def test_map_runs_never_crosses_a_payload_boundary(self):
        """Ketchup drops the part of a record that lands in a sector's 304-byte
        tail while still logging success - the fault that silently cost
        `en_savemsg` 142 of 442 bytes. `map_runs` must split there."""
        for start in (0, 1, 2000, 2047, 2048, 4095):
            for length in (1, 2, 255, 256, 5000):
                for offset, data in portio.map_runs(100, [(start, bytes(length))]):
                    within = offset % 2352
                    self.assertGreaterEqual(within, 24)
                    self.assertLessEqual(within + len(data), 24 + 2048,
                                         'run at %d+%d crosses a payload boundary' % (start, length))
                    self.assertLessEqual(len(data), 255)

    def test_image_offset_is_2352_byte_geometry(self):
        """A PPF offset is (lba + off//2048)*2352 + 24 + off%2048. Dividing by
        2048 instead made a finished VR port look unported for twenty minutes."""
        self.assertEqual(portio.image_offset(0, 0), 24)
        self.assertEqual(portio.image_offset(0, 2047), 24 + 2047)
        self.assertEqual(portio.image_offset(0, 2048), 2352 + 24)
        self.assertEqual(portio.image_offset(10, 0), 10 * 2352 + 24)

    def test_description_must_fit_50_bytes(self):
        """`ljust(50)` pads but never truncates; a 60-byte description once
        shifted every record offset and filled a 306 MB log."""
        with self.assertRaises(AssertionError):
            portio.ppf([(0, b'x')], 'y' * 51)

    def test_blockcheck_is_transparent_to_the_reader(self):
        records = [(0x40, b'abcd')]
        block = bytes(range(256)) * 4
        with_bc = portio.ppf(records, 'bc', blockcheck=block)
        plain = portio.ppf(records, 'bc')
        self.assertEqual(len(with_bc) - len(plain), 1024)
        self.assertEqual(with_bc[57], 1)
        self.assertEqual(portio.read_ppf(_tmp('bc.ppf', with_bc)), records)
        self.assertEqual(portio.add_blockcheck(plain, block), with_bc)


class Records(unittest.TestCase):
    """the 07-length-payload record chain every text patch rewrites"""

    def test_round_trip(self):
        items = [b'one\0', b'two\0', b'\0']
        blob = portio.encode_records(items)
        out, end = portio.records(blob, 0)
        self.assertEqual(out, items)
        self.assertEqual(end, len(blob))

    def test_every_record_must_be_nul_terminated(self):
        with self.assertRaises(AssertionError):
            portio.encode_records([b'no terminator'])

    def test_changed_runs_reports_only_differences(self):
        a = bytes(10)
        b = bytearray(a)
        b[2] = 1
        b[3] = 1
        b[8] = 1
        self.assertEqual(list(portio.changed_runs(a, bytes(b))),
                         [(2, b'\x01\x01'), (8, b'\x01')])


class Ecc(unittest.TestCase):
    """cdecc's algebra. Ground truth is `py cdecc.py`, against the real discs."""

    def sector(self, payload=None, submode=0x08):
        raw = bytearray(2352)
        raw[0:12] = b'\x00' + b'\xff' * 10 + b'\x00'
        raw[12:16] = bytes((0x00, 0x02, 0x00, 0x02))
        raw[16:24] = bytes((0, 0, submode, 0)) * 2
        raw[24:2072] = (payload or bytes(2048))
        return bytes(raw)

    def test_fixed_sector_verifies(self):
        s = cdecc.fixed(self.sector(bytes(range(256)) * 8))
        self.assertTrue(cdecc.verify(s))

    def test_tail_is_280_bytes_and_only_the_tail_changes(self):
        s = self.sector(b'\x5a' * 2048)
        f = cdecc.fixed(s)
        self.assertEqual(len(cdecc.tail(s)), 280)
        self.assertEqual(f[:2072], s[:2072])

    def test_payload_changes_the_tail(self):
        a = self.sector(bytes(2048))
        b = bytearray(a)
        b[24] = 1
        self.assertNotEqual(cdecc.tail(a), cdecc.tail(bytes(b)))

    def test_subheader_is_covered_by_the_edc(self):
        """The EDC runs from byte 16, not byte 24. Getting that wrong passes
        every test that only ever changes user data."""
        a = self.sector(bytes(2048))
        b = bytearray(a)
        b[17] = b[21] = 3
        self.assertNotEqual(cdecc.tail(a)[:4], cdecc.tail(bytes(b))[:4])

    def test_address_is_not_covered_by_the_ecc(self):
        """In Mode 2 the header is zeroed before the parity is computed, so two
        sectors differing only in their address have identical parity. A build
        that protected the address would fail on every disc ever pressed."""
        a = self.sector(b'\x11' * 2048)
        b = bytearray(a)
        b[12:16] = bytes((0x00, 0x03, 0x10, 0x02))
        self.assertEqual(cdecc.tail(a)[4:], cdecc.tail(bytes(b))[4:])

    def test_idempotent(self):
        s = cdecc.fixed(self.sector(b'\x7e' * 2048))
        self.assertEqual(cdecc.fixed(s), s)

    def test_form_2_is_refused(self):
        with self.assertRaises(AssertionError):
            cdecc.tail(self.sector(submode=0x28))       # bit 5 set: Form 2
        self.assertEqual(cdecc.form(self.sector(submode=0x28)), 2)
        self.assertEqual(cdecc.form(self.sector()), 1)


class Pcx(unittest.TestCase):
    """the 4-plane RLE PCX the texture loader expects"""

    def template(self, w, h, nplanes=4):
        stride = (w + 7) // 8
        head = bytearray(128)
        struct.pack_into('<HHHH', head, 4, 0, 0, w - 1, h - 1)
        head[65] = nplanes
        struct.pack_into('<H', head, 66, stride)
        return bytes(head)

    def test_round_trip(self):
        w, h = 32, 4
        pal = [(i * 8, i * 4, i * 2) for i in range(16)]
        rows = [[(x + y) % 16 for x in range(w)] for y in range(h)]
        blob = pcx4.encode(self.template(w, h), w, h, pal, rows)
        w2, h2, pal2, rows2 = pcx4.decode(blob)
        self.assertEqual((w2, h2), (w, h))
        self.assertEqual(rows2, rows)
        self.assertEqual(pal2, pal)

    def test_run_cap_is_a_parameter_and_63_is_legal(self):
        """PCX_RLE_CODE + run allows 63; the original call site capped at 62 and
        the VR option archive needed the last byte back. The default stays 62 so
        every shipped patch still rebuilds byte for byte."""
        data = b'\xaa' * 63
        self.assertLessEqual(len(pcx4._rle(data, maxrun=63)), len(pcx4._rle(data)))
        with self.assertRaises(AssertionError):
            pcx4._rle(data, maxrun=64)


class Widths(unittest.TestCase):
    """widths.py's measuring and the window budget derived from the decomp"""

    W = {c: 8 for c in range(32, 128)}

    def test_ascii_is_one_byte_and_zenkaku_two(self):
        self.assertEqual(widths.width(b'AB', self.W), 16)
        self.assertEqual(widths.width(b'\x9a\x01', self.W), 12)
        self.assertEqual(widths.width(b'A\x9a\x01B', self.W), 28)

    def test_lead_byte_0x80_is_a_pair_not_an_ascii_advance(self):
        """Codes 0x8080..0x80FF exist; a `>= 0x81` test would read the lead byte
        as ASCII and under-measure the line."""
        self.assertEqual(widths.width(b'\x80\x90', self.W), 12)

    def test_window_budget_matches_the_worked_example(self):
        """-w 32 53 256 118 gives 20 whole cells, 240 px, 228 px of room."""
        self.assertEqual(widths.window_budget([32, 53, 256, 118]), 228)

    def test_budget_rounds_down_to_whole_cells(self):
        for w in range(200, 260):
            self.assertEqual(widths.window_budget([0, 0, w, 0]) % 12, 0)

    def test_split_lines_finds_the_pool_separator(self):
        self.assertEqual(widths.split_lines(b'A\x80\x7cB'), [b'A', b'B'])
        self.assertEqual(widths.split_lines(b'AB'), [b'AB'])


class Pad2(unittest.TestCase):
    """pad2.py's slot fill: USA's line into Integral's longer slot"""

    SLOT = bytes(range(1, 55)) + bytes(1)        # 55 bytes, NUL-terminated
    EN = b'PLUG CONTROLLER INTO | CONTROLLER PORT 1.' + bytes(1)

    def test_terminator_immediately_follows_the_text(self):
        """The trap: pad BEFORE the terminator and the spaces are drawn.
        `font_print_string` measures what it draws and jimaku centres on that
        width, so trailing spaces would pull the line off centre.
        """
        out = pad2.fill(self.SLOT, self.EN)
        self.assertEqual(out[:len(self.EN)], self.EN)
        self.assertEqual(out.index(0), len(self.EN) - 1)

    def test_length_and_final_nul_are_preserved(self):
        """Nothing may move: the record's length byte is not rewritten, and
        `portio.records` requires the payload's last byte to be NUL.
        """
        out = pad2.fill(self.SLOT, self.EN)
        self.assertEqual(len(out), len(self.SLOT))
        self.assertEqual(out[-1], 0)

    def test_dead_tail_is_spaces(self):
        out = pad2.fill(self.SLOT, self.EN)
        tail = out[len(self.EN):-1]
        self.assertEqual(tail, b' ' * (len(self.SLOT) - len(self.EN) - 1))

    def test_refuses_a_replacement_that_does_not_leave_the_final_nul(self):
        """A replacement as long as the slot would need no padding and is out
        of scope: this port only ever writes a shorter line.
        """
        with self.assertRaises(AssertionError):
            pad2.fill(self.SLOT, bytes(len(self.SLOT) - 1) + bytes(1))

    def test_refuses_an_unterminated_replacement(self):
        with self.assertRaises(AssertionError):
            pad2.fill(self.SLOT, b'NO TERMINATOR')


class LangDefault(unittest.TestCase):
    """The English-at-boot rewrite of GCL_StartDaemon (langdefault.py).

    Built over a synthetic executable rather than a real one, so it runs with
    no game data: what it checks is the shape contract - that the rewrite is
    the same length, keeps all five calls in the same order, and refuses
    anything that does not look like the function it means to rewrite. That
    the real executables match the shape is `py langdefault.py`.
    """

    CALLS = (0x80020B68, 0x80021264, 0x8002040C, 0x80015418, 0x8001FCB0)

    def exe(self, at=0x8001FCDC, linkvar=0x800B4D98):
        L = langdefault
        body = [
            L.ADDIU_SP_DOWN, L.SW_RA,
            L.jal(self.CALLS[0]), L.NOP,
            L.jal(self.CALLS[1]), L.NOP,
            L.jal(self.CALLS[2]), L.NOP,
            L.ADDIU_A0_G, L.LUI_A1_8002,
            L.jal(self.CALLS[3]), 0x24A5FC88,
            L.jal(self.CALLS[4]), L.MOVE_A0_ZERO,
            L.LW_RA, L.NOP, L.JR_RA, L.ADDIU_SP_UP,
        ]
        init_var = [
            L.ADDIU_SP_DOWN,
            0x3C100000 | (linkvar >> 16),                  # lui   $s0, hi
            0x26100000 | (linkvar & 0xFFFF),               # addiu $s0, $s0, lo
            0x86120000 | (L.CONFIG_INDEX * 2),             # lh    $s2, 4($s0)
            0x86110000 | (L.LEVEL_INDEX * 2),              # lh    $s1, 2($s0)
            L.JR_RA, L.NOP, L.NOP,
        ]
        change = [0x24020001, L.JR_RA, L.NOP]              # a leaf
        image = bytearray(bytes(L.HDR + 0x30000))
        def put(address, words):
            off = L.HDR + address - L.TADDR
            image[off:off + 4 * len(words)] = struct.pack('<%dI' % len(words), *words)
        put(at, body)
        put(self.CALLS[1], init_var)
        put(self.CALLS[4], change)
        return bytes(image), at

    def test_same_length_and_same_calls(self):
        exe, at = self.exe()
        writes = langdefault.patch_for(exe)
        self.assertEqual(len(writes), 18)
        self.assertEqual(sorted(writes), [at + 4 * k for k in range(18)])
        words = [writes[at + 4 * k] for k in range(18)]
        targets = [langdefault.jal_target(w, at + 4 * k)
                   for k, w in enumerate(words) if langdefault.is_jal(w)]
        self.assertEqual(targets, list(self.CALLS[:4]),
                         'the four jal targets must survive, in order')
        self.assertEqual(words[16], langdefault.jump(self.CALLS[4]),
                         'the fifth call becomes a tail jump')

    def test_the_store_lands_on_gm_configuration(self):
        exe, at = self.exe(linkvar=0x800B4D98)
        words = langdefault.patch_for(exe)
        self.assertEqual(words[at + 4 * 5], 0x3C02800B)         # lui $v0, 0x800b
        self.assertEqual(words[at + 4 * 7], 0xA4434D9C)         # sh  $v1, 0x4d9c($v0)
        self.assertEqual(words[at + 4 * 4], 0x24030100)         # addiu $v1, $zero, 0x100

    def test_it_follows_linkvarbuf_rather_than_assuming_it(self):
        exe, at = self.exe(linkvar=0x800B29F8)                  # the VR layout
        words = langdefault.patch_for(exe)
        self.assertEqual(words[at + 4 * 7] & 0xFFFF, 0x29FC)

    def test_the_store_sits_in_the_call_s_delay_slot(self):
        exe, at = self.exe()
        words = langdefault.patch_for(exe)
        self.assertTrue(langdefault.is_jal(words[at + 4 * 6]))
        self.assertEqual(words[at + 4 * 7] >> 26, 0x29, 'sh follows the jal')

    def test_it_refuses_a_function_it_does_not_recognise(self):
        exe, at = self.exe()
        broken = bytearray(exe)
        off = langdefault.HDR + at - langdefault.TADDR
        struct.pack_into('<I', broken, off, 0x27BDFFF0)         # a different frame
        with self.assertRaises(AssertionError):
            langdefault.patch_for(bytes(broken))

    def test_it_refuses_when_the_tail_call_is_not_a_leaf(self):
        exe, at = self.exe()
        broken = bytearray(exe)
        off = langdefault.HDR + self.CALLS[4] - langdefault.TADDR
        struct.pack_into('<I', broken, off, langdefault.jal(self.CALLS[0]))
        with self.assertRaises(AssertionError):
            langdefault.patch_for(bytes(broken))

    def test_it_refuses_two_matches(self):
        exe, at = self.exe()
        doubled = bytearray(exe)
        off = langdefault.HDR + at - langdefault.TADDR
        doubled[off + 0x400:off + 0x400 + 72] = exe[off:off + 72]
        with self.assertRaises(AssertionError):
            langdefault.patch_for(bytes(doubled))


class Hazards(unittest.TestCase):
    """hazards.py, the R3000 load-delay scanner every rewritten block goes through.

    The eleven words below mirror brf_widen.ROW_H_OLD / ROW_H_NEW (brf_widen
    needs the game data to import, so they are copied, and brf_build.py asserts
    the live ones on every build). BUGGY is what the port shipped from
    2026-09-02 to 2026-09-10: `subu` reads a1 in the slot right after `lbu a1`.
    """
    BASE = 0x800C69C8
    RETAIL = [0x84440008, 0x84450020, 0x24C3000D, 0xA446000A, 0xA4460012, 0xA443001A,
              0xA4430022, 0xA4440008, 0xA4450010, 0xA4440018, 0xA4450020]
    BUGGY = [0x9044001D, 0x9045000D, 0x00851823, 0x30A50007, 0x00C52023, 0x00831821,
             0xA444000A, 0xA4440012, 0xA443001A, 0xA4430022, 0x00000000]
    FIXED = [0x9044001D, 0x9045000D, 0x00000000, 0x00851823, 0x30A50007, 0x00C52023,
             0x00831821, 0xA444000A, 0xA4440012, 0xA443001A, 0xA4430022]

    def scan(self, words, retail=None):
        pack = lambda ws: b''.join(struct.pack('<I', w) for w in ws)
        return [(a, k) for a, k, _ in hazards.scan(pack(words), self.BASE,
                                                   None if retail is None else pack(retail))]

    def test_the_briefing_bug_is_caught_at_its_own_address(self):
        self.assertEqual(self.scan(self.BUGGY), [(self.BASE + 4, 'load-use')])

    def test_the_fix_and_retail_are_both_clean(self):
        self.assertEqual(self.scan(self.FIXED), [])
        self.assertEqual(self.scan(self.RETAIL), [])

    def test_a_store_in_the_slot_is_a_use(self):
        # the stub's X normalisation: lh a1, 8(v0) then sh a1, 0x18(v0)
        self.assertEqual(self.scan([0x84450008, 0xA4450018]), [(self.BASE, 'load-use')])

    def test_a_branch_in_the_slot_is_a_use(self):
        # lw v0, 0(a0) then bne v0, s3
        self.assertEqual(self.scan([0x8C820000, 0x14530002]), [(self.BASE, 'load-use')])

    def test_a_gap_of_one_instruction_is_enough(self):
        self.assertEqual(self.scan([0x8C820000, 0x00000000, 0x14530002]), [])

    def test_a_second_load_into_the_register_is_the_compiler_s_idiom(self):
        self.assertEqual(self.scan([0x808D0000, 0x808D0000]), [])          # lb t5; lb t5

    def test_a_non_load_writer_in_the_slot_is_reported(self):
        self.assertEqual(self.scan([0x8C820000, 0x24020001]), [(self.BASE, 'load-write')])

    def test_a_branch_in_a_branch_s_delay_slot_is_reported(self):
        self.assertEqual(self.scan([0x03E00008, 0x08030000]), [(self.BASE, 'branch-slot')])

    def test_words_identical_to_retail_are_not_judged(self):
        self.assertEqual(self.scan(self.BUGGY, retail=self.BUGGY), [])
        one_off = list(self.BUGGY); one_off[1] ^= 1          # the load itself differs
        self.assertEqual(self.scan(self.BUGGY, retail=one_off), [(self.BASE + 4, 'load-use')])


class GrenadeBriefingTests(unittest.TestCase):
    """Synthetic GCL proves the correction cannot change another numeral."""

    @staticmethod
    def stage(line, title=b'GRENADE  LEVEL  02\0'):
        import vrlib as v
        import portio
        records = [b'WEAPON  MODE\0', title, b'\0', line, b'Targets 5\0']
        strings = b''.join(b'\x07' + bytes([len(r)]) + r for r in records)
        values = b'\x06\xd4\x4e' + b'\x50b' + bytes([len(strings) + 1]) + strings + b'\0'
        body = b'\x99\x06\0' + values
        command = b'\x60' + struct.pack('>H', len(body) + 2) + body
        script = b'\x40' + struct.pack('>H', len(command) + 2) + command
        gcx = object.__new__(v.Gcx)
        gcx.procs, gcx.script, gcx.font = [], script, bytes(36)
        data = gcx.build()
        data += bytes(-len(data) % 4)
        return portio.pack_stage([[0xEA54, ord('c'), ord('g'), 0],
                                  [0, ord('c'), 255, len(data)]], {1: data})

    def test_japanese_only_changes_the_fuse_digit(self):
        import vr_grenade as g
        old = self.stage(g.JP_LINE)
        new = g.correct_briefing(old, 'japanese')
        differences = [(a, b) for a, b in zip(old, new) if a != b]
        self.assertEqual(differences, [(ord('5'), ord('4'))])
        self.assertIn(g.JP_LINE.replace(b'\x805', b'\x804'), new)
        self.assertIn(b'Targets 5\0', new)

    def test_english_only_changes_the_fuse_digit(self):
        import vr_grenade as g
        old = self.stage(g.EN_LINE)
        new = g.correct_briefing(old)
        self.assertEqual(sum(a != b for a, b in zip(old, new)), 1)
        self.assertIn(b'Grenades explode in 4 ', new)
        self.assertIn(b'Targets 5\0', new)

    def test_correction_is_idempotent(self):
        import vr_grenade as g
        for language, line in [('japanese', g.JP_LINE), ('english', g.EN_LINE)]:
            fixed = g.correct_briefing(self.stage(line), language)
            self.assertEqual(g.correct_briefing(fixed, language), fixed)

    def test_wrong_mission_is_rejected(self):
        import vr_grenade as g
        with self.assertRaises(AssertionError):
            g.correct_briefing(self.stage(g.EN_LINE, b'GRENADE LEVEL 03\0'))

    def test_wrong_language_or_unexpected_number_is_rejected(self):
        import vr_grenade as g
        with self.assertRaises(AssertionError):
            g.correct_briefing(self.stage(g.JP_LINE), 'english')
        with self.assertRaises(AssertionError):
            g.correct_briefing(self.stage(g.EN_LINE.replace(b'5', b'6')))


_TMP = os.path.join(os.environ.get('TEMP') or '/tmp', 'integral-english-selftest')


def _tmp(name, data):
    os.makedirs(_TMP, exist_ok=True)
    path = os.path.join(_TMP, name)
    with open(path, 'wb') as handle:
        handle.write(data)
    return path


if __name__ == '__main__':
    unittest.main(verbosity=2 if '-v' in sys.argv else 1,
                  argv=[a for a in sys.argv if a != '-v'])
