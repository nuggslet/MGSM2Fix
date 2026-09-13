#!/usr/bin/env python
"""Write a patched PSX disc image from a raw-variant patch set.

    py mkimage.py --redump <disc.bin> --ppfs <dir> --output <patched.bin>
    py mkimage.py --collection --disc 1 --exe int1.exe --ppfs <dir> --output ...

Everything else in this directory produces PPFs and stops there, because the
collection applies them itself at load time. A real PlayStation image has no
such loader, so this is the step that turns the raw variant into something a
disc burner or an emulator can open. It never writes to the game and never
touches an input.

**Two sources, and they are not equally capable.**

A **Redump dump** (`MODE2/2352`, one track, no audio) is the whole disc, and
that is what the PPF offsets already address: `portio.image_offset` computes
`(lba + off//2048) * 2352 + 24 + off%2048`, an offset from LBA 0 of exactly
such an image. Nothing is relocated; the patches apply where they fall.

The **collection's embedded image** is the same disc with one hole in it: the
executable's ISO extent is zero-filled while keeping the parity of the sector
it used to be (`rawdisc.py` explains why that matters). A patched image built
from it is not bootable unless the retail executable is put back, so
`--collection` requires `--exe` and refuses without it. That is not a
formality - the collection cannot supply those bytes, and neither can this
repository.

Measured 2026-09-10 on all three discs: the Redump dumps and the collection's
embedded images are byte-identical everywhere **except** those executable
extents, and the 1024-byte block check at 0x9320 agrees between them. So the
two sources are interchangeable as long as the executable is accounted for.

**What is checked before anything is written**, because an image that is wrong
in a way nobody notices is the failure this project keeps guarding against:

1. every PPF's block check must equal the source image's own 1024 bytes at
   0x9320 - this is what refuses a patch aimed at a different release, and a
   raw build always carries it;
2. no record may reach past the end of the image;
3. every sector the set touches must verify against its **own stored parity
   before patching**, which is a 280-byte checksum over 2048 bytes and cannot
   pass by luck - it is what proves the dump is the pressing the patches were
   computed against;
4. every sector must verify **again after patching**, against the parity the
   set's own `zz_ecc` PPF wrote.

Step 4 is why the set is all-or-nothing: a tail is computed from the final
payload of the whole set, so leaving one family out leaves the image with
correct data and wrong parity. `--allow-partial` exists for bisecting a fault
and says so loudly; do not ship what it produces.
"""
import argparse
import glob
import hashlib
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import cdecc
import langdefault
import rawdisc
import workdir
from portio import BLOCKCHECK_AT, BLOCKCHECK_LEN, image_offset, read_ppf
from iso import Disc

# The collection concatenates the three Integral images into one container.
CONTAINER = 'windata/dlc/dlc_japan.bin'
DISCS = {
    '1':  dict(base=0x00000000, exe='/MGS/SLPM_862.47;1', folder='mods/INTEGRAL/INTEGRAL/0',
               label='Integral disc 1 (SLPM-86247)'),
    '2':  dict(base=0x2AE54800, exe='/MGS/SLPM_862.48;1', folder='mods/INTEGRAL/INTEGRAL/1',
               label='Integral disc 2 (SLPM-86248)'),
    'vr': dict(base=0x57592000, exe='/MGS/SLPM_862.49;1', folder='mods/INTEGRAL/VR-DISK',
               label='Integral VR disc (SLPM-86249)'),
}


def blockcheck_of_image(path, base=0):
    with open(path, 'rb') as handle:
        handle.seek(base + BLOCKCHECK_AT)
        return handle.read(BLOCKCHECK_LEN)


def ppf_blockcheck(data):
    """The 1024 bytes a PPF3 carries, or None if it declares none."""
    return data[60:60 + BLOCKCHECK_LEN] if data[57] else None


def exe_extent(container, base, iso_path):
    """(lba, size) of the executable's ISO extent on this disc."""
    disc = Disc(container, base)
    try:
        return next((l, s) for n, l, s, d in disc.walk()
                    if not d and n.upper() == iso_path)
    finally:
        disc.f.close()


def exe_lba(container, base, iso_path):
    return exe_extent(container, base, iso_path)[0]


def identify(image, base=0):
    """Which disc an image is, read from the disc rather than from its name.

    A filename is a guess and a `--disc` flag is a chance to be wrong; the
    executable in the ISO says which disc this is and cannot disagree with
    itself. Returns the key into DISCS, or None.
    """
    try:
        disc = Disc(image, base)
    except (OSError, AssertionError):
        return None
    try:
        names = {n.upper() for n, l, s, d in disc.walk() if not d}
    except (OSError, AssertionError, IndexError, KeyError):
        return None
    finally:
        disc.f.close()
    for key, spec in DISCS.items():
        if spec['exe'] in names:
            return key
    return None


def find_ppfs(where, disc):
    """Accept a package root, a mods/ tree, or the folder itself.

    Someone handed a raw build has `package/mods/INTEGRAL/INTEGRAL/0` to type
    correctly for disc 1 and a different one for the VR disc. Any level of
    that will do here, and the disc is already known, so the right leaf is
    picked rather than typed.
    """
    where = where.rstrip('/\\')
    leaf = DISCS[disc]['folder']                      # mods/INTEGRAL/...
    tries = [where,
             os.path.join(where, leaf),
             os.path.join(where, 'package', leaf)]
    # ...and, if they pointed at the ZIP's extracted root, the same again
    for extra in ('mods', os.path.join('package', 'mods')):
        tries.append(os.path.join(where, extra, *leaf.split('/')[1:]))
    for candidate in tries:
        if glob.glob(os.path.join(candidate, '*.ppf')):
            return candidate.replace(os.sep, '/')
    return None


ENGLISH_QUESTION = """
  Integral is a bilingual disc, and one bit decides which language the game
  itself uses: the codec dialogue and the cutscene subtitle stream. It is
  clear at power-on, so a retail disc starts in Japanese and the player sets
  it in Integral's own OPTION screen, where it saves to the memory card.

  This port's text is not affected either way - the menus, items, briefings
  and mission log are English whatever the bit says. The question is only
  whether the disc should *start* in English instead of asking the player to
  go and turn it on.

  Choosing yes rewrites one function, GCL_StartDaemon, in place: same 18
  instructions in the same 72 bytes, setting the bit once at boot. The OPTION
  screen and the memory card still override it afterwards, so a player who
  picks Japanese keeps Japanese.
"""


def ask_english_default():
    print(ENGLISH_QUESTION)
    while True:
        try:
            answer = input('  Default to English? [y/N] ').strip().lower()
        except EOFError:
            return False
        if answer in ('', 'n', 'no'):
            return False
        if answer in ('y', 'yes'):
            return True
        print('  Please answer y or n.')


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__.split('\n\n')[0],
        formatter_class=argparse.RawDescriptionHelpFormatter)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--redump', metavar='DISC.BIN',
                        help='a Redump MODE2/2352 single-track image')
    source.add_argument('--collection', action='store_true',
                        help="read the image out of the collection's dlc_japan.bin")
    parser.add_argument('--game', help='the collection install (for --collection). '
                                       'Found via Steam if omitted')
    parser.add_argument('--disc', choices=sorted(DISCS),
                        help='which Integral disc: 1, 2 or vr. Read from the '
                             'image itself if omitted')
    parser.add_argument('--exe', metavar='INT1.EXE',
                        help='the retail executable, required with --collection')
    parser.add_argument('--ppfs', required=True, metavar='DIR',
                        help="the raw build's PPF folder, or the package/ or "
                             'unzipped root above it - the right one for this '
                             'disc is picked')
    parser.add_argument('--output', required=True, metavar='PATCHED.BIN')
    parser.add_argument('--english-default', choices=('ask', 'yes', 'no'),
                        default='ask', metavar='ask|yes|no',
                        help='set English as the power-on language (default: ask). '
                             'Affects the codec and cutscene subtitles, not this '
                             "port's text - see langdefault.py")
    parser.add_argument('--cue', action='store_true',
                        help='also write a matching single-track .cue')
    parser.add_argument('--allow-partial', action='store_true',
                        help='skip the post-patch parity check (bisecting only)')
    args = parser.parse_args(argv)

    # --- which disc, read from the image rather than trusted from a flag ---
    if args.redump:
        if not os.path.isfile(args.redump):
            parser.error('%s does not exist' % args.redump)
        found = identify(args.redump)
        if found is None:
            parser.error(
                '%s does not look like an Integral disc image.\n'
                '  Expected a MODE2/2352 single-track .bin holding one of\n'
                '  %s.\n'
                '  If it is a .cue/.bin pair, pass the .bin.'
                % (args.redump, ', '.join(d['exe'] for d in DISCS.values())))
        if args.disc and args.disc != found:
            parser.error('--disc %s was given but %s is disc %s'
                         % (args.disc, os.path.basename(args.redump), found))
        args.disc = found
    else:
        args.game = workdir.require_game(args.game)
        if not args.disc:
            parser.error(
                '--collection needs --disc (1, 2 or vr): all three images live '
                'in the same container, so there is nothing to detect.')

    spec = DISCS[args.disc]

    # --- the patches, from whatever level of the package they pointed at ---
    folder = find_ppfs(args.ppfs, args.disc)
    if folder is None:
        parser.error(
            'no PPFs for disc %s under %s.\n'
            '  Looked for %s/*.ppf and the same under package/.\n'
            '  Give the folder from a `rebuild.py --variant raw` build, or the\n'
            '  root of its unzipped package.' % (args.disc, args.ppfs, spec['folder']))
    paths = sorted(glob.glob(os.path.join(folder, '*.ppf')))

    # --- the source, and the executable question -------------------------
    substitutes = rawdisc.Substitutes()
    if args.redump:
        image, base = args.redump, 0
        if args.exe:
            parser.error('--exe belongs to --collection; a real dump already '
                         'holds its executable')
    else:
        image = os.path.join(args.game, CONTAINER).replace('\\', '/')
        base = spec['base']
        if not args.exe:
            parser.error(
                "--collection needs --exe: the collection zero-fills the "
                "executable's ISO extent, so an image built from it without the "
                "retail file will not boot. A Redump dump of this disc supplies "
                "it (extract %s)." % spec['exe'])
        data = open(args.exe, 'rb').read()
        substitutes.add(exe_lba(image, base, spec['exe']), data,
                        os.path.basename(args.exe))
        print('executable  : %s, %d bytes, sha256 %s'
              % (args.exe, len(data), hashlib.sha256(data).hexdigest()))

    lba, exe_size = exe_extent(image, base, spec['exe'])

    size = os.path.getsize(image) - base
    if args.collection:
        # The container holds the next image right after this one, and pads to
        # a 2048-byte boundary between them - which is not a sector boundary, so
        # the gap is up to a sector longer than the disc. Flooring to whole
        # 2352-byte sectors removes exactly that padding: it reproduces the
        # Redump length of all three discs to the byte.
        others = [d['base'] for d in DISCS.values() if d['base'] > base]
        if others:
            size = min(others) - base
    size -= size % cdecc.SECTOR
    print('disc        : %s' % spec['label'])
    print('source      : %s%s' % (image, '' if not base else ' @ 0x%X' % base))
    print('patches     : %d from %s' % (len(paths), folder))

    # --- 1. the block check ----------------------------------------------
    expected = blockcheck_of_image(image, base)
    unchecked = []
    for path in paths:
        carried = ppf_blockcheck(open(path, 'rb').read())
        if carried is None:
            unchecked.append(os.path.basename(path))
        elif carried != expected:
            raise SystemExit(
                'STOP: %s carries a block check that does not match this image.\n'
                '      This patch was built for a different release or a different\n'
                '      dump. Applying it would corrupt the disc.' % os.path.basename(path))
    if unchecked:
        print('warning     : %d PPF(s) carry no block check, so nothing verified '
              'they belong to this image: %s'
              % (len(unchecked), ', '.join(unchecked)))
        print('              (a collection build omits it by design - it is a raw '
              'build you want here)')
    else:
        print('block check : all %d PPFs match the image at 0x%X' % (len(paths), BLOCKCHECK_AT))

    # --- the optional language default -----------------------------------
    # Asked here, after the patches have been named and before anything is
    # read, so the question comes with the run it applies to.
    english = {'yes': True, 'no': False}.get(args.english_default)
    if english is None:
        if not sys.stdin or not sys.stdin.isatty():
            raise SystemExit(
                'STOP: --english-default is "ask" and this is not a terminal.\n'
                '      Pass --english-default yes or no; a build script must say\n'
                '      which it wants rather than have one chosen for it.')
        english = ask_english_default()

    language = {}
    if english:
        with open(image, 'rb') as handle:
            handle.seek(base + lba * cdecc.SECTOR)
            raw = handle.read(((exe_size + 2047) // 2048) * cdecc.SECTOR)
        exe = b''.join(raw[k + cdecc.DATA:k + cdecc.DATA_END]
                       for k in range(0, len(raw), cdecc.SECTOR))[:exe_size]
        for span_lba, _end, data, _name in substitutes.spans:
            if span_lba == lba:
                exe = data[:exe_size].ljust(exe_size, b'\0')
        for address, word in sorted(langdefault.patch_for(exe).items()):
            at = langdefault.HDR + address - langdefault.TADDR
            language[image_offset(lba, at)] = struct.pack('<I', word)
        print('language    : English at power-on - %s'
              % langdefault.describe(exe))
    else:
        print('language    : left as the disc has it (Japanese until the '
              'OPTION screen)')

    # --- 2/3. bounds, and the parity of the disc we were handed -----------
    changed = rawdisc.all_writes(paths)
    touched_by_language = set()
    for offset, payload in language.items():
        sector, within = divmod(offset, cdecc.SECTOR)
        assert within + len(payload) <= cdecc.DATA_END, hex(offset)
        changed.setdefault(sector, {}).update(
            {within + k: b for k, b in enumerate(payload)})
        touched_by_language.add(sector)
    past = [s for s in changed if (s + 1) * cdecc.SECTOR > size]
    if past:
        raise SystemExit('STOP: %d record(s) reach past the end of the image, '
                         'first at sector %d' % (len(past), min(past)))

    # Both parity checks run here, in memory, on the touched sectors alone -
    # about a megabyte of them on a main disc. Nothing is written until they
    # have both passed, because the alternative is a 700 MB image that fails
    # its own parity sitting on disk under a plausible name.
    out = os.path.abspath(args.output)
    if os.path.exists(out):
        raise SystemExit('STOP: %s exists; refusing to overwrite it' % out)

    bad, failures, patched = [], [], {}
    with open(image, 'rb') as handle:
        for sector in sorted(changed):
            handle.seek(base + sector * cdecc.SECTOR)
            raw = bytearray(handle.read(cdecc.SECTOR))
            replacement = substitutes.payload(sector)
            if replacement is not None:
                raw[cdecc.DATA:cdecc.DATA_END] = replacement
            if not cdecc.verify(bytes(raw)):
                bad.append(sector)
            for within, byte in changed[sector].items():
                raw[within] = byte
            if sector in touched_by_language:
                # The set's own zz_ecc tail was computed without these bytes,
                # so for these sectors - and only these - the tail is recomputed
                # here. Everywhere else the set's tail still governs, and the
                # check below is what says so.
                raw = bytearray(cdecc.fixed(bytes(raw)))
            if not cdecc.verify(bytes(raw)):
                failures.append(sector)
            patched[sector] = bytes(raw)
    if bad:
        raise SystemExit(
            'STOP: %d of %d touched sectors do not verify against their own stored\n'
            '      parity BEFORE patching (first: %d). This image is not the pressing\n'
            '      these patches were computed against%s.'
            % (len(bad), len(changed), bad[0],
               ', or the executable is wrong' if substitutes.spans else ''))
    print('parity in   : all %d touched sectors verify against the untouched disc'
          % len(changed))
    if failures and not args.allow_partial:
        raise SystemExit(
            'STOP: %d of %d touched sectors would fail parity AFTER patching (first:\n'
            '      %d), so nothing was written. The EDC/ECC PPF is missing or does not\n'
            '      belong to this set - a raw build emits one named *_zz_ecc.ppf, and\n'
            '      the set is all-or-nothing because a tail is computed from the final\n'
            '      payload of the whole set.' % (len(failures), len(changed), failures[0]))
    if args.allow_partial:
        print('parity out  : %d of %d sectors fail, ignored (--allow-partial).'
              ' Do not ship this image.' % (len(failures), len(changed)))
    else:
        print('parity out  : all %d touched sectors verify after patching' % len(changed))

    # --- write it ---------------------------------------------------------
    try:
        with open(image, 'rb') as src, open(out, 'wb') as dst:
            src.seek(base)
            left = size
            while left:
                chunk = src.read(min(1 << 22, left))
                if not chunk:
                    raise SystemExit('STOP: the source ended %d bytes early' % left)
                dst.write(chunk)
                left -= len(chunk)
            for sector, raw in patched.items():
                dst.seek(sector * cdecc.SECTOR)
                dst.write(raw)
            for start, end, data, _ in substitutes.spans:
                for sector in range(start, end):
                    if sector in patched:
                        continue          # already written, patches included
                    dst.seek(sector * cdecc.SECTOR + cdecc.DATA)
                    at = (sector - start) * 2048
                    dst.write(data[at:at + 2048].ljust(2048, b'\0'))
    except BaseException:
        if os.path.exists(out):
            os.unlink(out)
        raise
    written = sum(len(p) for path in paths for _, p in read_ppf(path))
    written += sum(len(p) for p in language.values())
    print('written     : %s, %d bytes, %d patch bytes applied'
          % (out, os.path.getsize(out), written))

    if args.cue:
        cue = os.path.splitext(out)[0] + '.cue'
        with open(cue, 'w', newline='\r\n') as handle:
            handle.write('FILE "%s" BINARY\n  TRACK 01 MODE2/2352\n'
                         '    INDEX 01 00:00:00\n' % os.path.basename(out))
        print('cue         : %s' % cue)
    return 0


if __name__ == '__main__':
    sys.exit(main())
