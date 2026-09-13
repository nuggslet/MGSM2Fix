"""Correct Integral VR's grenade decal and briefing to USA's four-second delay.

Standalone asset correction, independent of the English patches. Run
`py vr_grenade.py` to build collection and raw-disc PPFs in WORK;
`py vr_grenade.py --deploy` installs both collection layouts and JSON companions.
The default output works on original Japanese Integral. If the deployed
en_missions PPF exists (or --missions-ppf is supplied), also build _english
variants against that exact layout. MGSM2Fix's INI selects the active language.
Raw PPFs include regenerated EDC/ECC and a disc block check.

The texture is 4D80.p in selectvr's second DAR, NOT vab_grn's cache.
Losslessly re-encode USA's PCX to fit Integral's existing 10,512-byte slot,
zero-pad the unused end. Additionally change one briefing digit in each of the
five vr_grn stages, preserving every other byte and all Japanese glyphs.
The English builder uses correct_briefing too, so its writes agree with this
addon regardless of load order. The loader uses the mission companion to set
its five digits to 5 when GrenadeDelayFix is off. Rebuild this addon when the
mission PPF changes; merely toggling English no longer requires rebuilding.
"""
import argparse
import json
from pathlib import Path

import cdecc
import pcx4
import portio
import ppfcheck
import vrlib as v
from optscan import geo, parse

STAGE = 'selectvr'
TEXTURE = 0x4D80
NAME = 'INTEGRAL_vr_fix_grenade_delay.ppf'
MISSION_NAME = 'INTEGRAL_vr_en_missions.ppf'
BRIEFING_STAGES = tuple('vr_grn%02d' % n for n in range(1, 6))
JP_LINE = bytes.fromhex(
    '8210824c822dd0068229812f907c8108812690fa803590fb90fc812b90a2907e8119814bd00300')
EN_LINE = b'the goal! Grenades explode in 5 \0'
HASHES = (
    '3385810e2df7b03689fcb09797ac59ed1b83e14dfeac03b0a7c1889f1cb5927a',
    '72589944f74a9005a44c270b7fa7aa84d397d149e677b19a6482aae260bb4d4d',
)


def briefing_offset(stage, language):
    """Find the fuse digit structurally; never patch other 5s or target counts."""
    line = {'japanese': JP_LINE, 'english': EN_LINE}[language]
    digit = line.index(b'5')
    fixed = line[:digit] + b'4' + line[digit + 1:]
    _, payloads, ci, _, gcx = v.stage_gcx(stage)
    offsets = portio.stage(stage)[2]
    matches = []
    for pid, body in gcx.procs + [('script', gcx.script)]:
        for win in v.windows_in(body, v.parse_arg(body), pid):
            records = win.records(body)
            if [r.replace(b' ', b'') for r in records[:2]] != [
                    b'WEAPONMODE\0', b'GRENADELEVEL02\0']:
                continue
            for value in win.cmd.option('b').values:
                if value.kind != 'STRING':
                    continue
                record = body[value.pos + 2:value.end]
                if record not in (line, fixed):
                    continue
                assert payloads[ci].count(body) == 1, 'ambiguous proc location'
                pos = offsets[ci] + payloads[ci].index(body) + value.pos + 2 + digit
                assert stage[pos] in (ord('4'), ord('5'))
                matches.append(pos)
    assert len(matches) == 1, '%s: expected one grenade fuse sentence, got %d' % (
        language, len(matches))
    return matches[0]


def correct_briefing(stage, language='english'):
    """One-byte correction shared with the English builder; accepts 5 or 4."""
    pos = briefing_offset(stage, language)
    result = stage[:pos] + b'4' + stage[pos + 1:]
    assert briefing_offset(result, language) == pos
    return result


def fingerprint(data):
    """FNV-1a layout guard shared with mgs1_patch_options.h (not authentication)."""
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return '%016x' % value


def write_metadata(path, **fields):
    path = Path(path)
    meta = dict(schema=1, fingerprint=fingerprint(path.read_bytes()), **fields)
    companion = Path(str(path) + '.json')
    companion.write_text(json.dumps(meta, indent=2) + '\n', encoding='utf-8')
    return companion


def write_mission_metadata(path, isd, disc):
    """Pin the English PPF and structurally located digits for the INI toggle."""
    offsets = []
    for name in BRIEFING_STAGES:
        lba = v.stage_lba(disc, isd, name)
        stage = portio.patched_file(v.stage_bytes(isd, name), lba, [path])
        offsets.append(portio.image_offset(lba, briefing_offset(stage, 'english')))
    assert len(set(offsets)) == 5
    return write_metadata(path, grenade_digits=offsets)


def texture(stage):
    tags, payloads, offsets = portio.stage(stage)
    found = []
    for k, payload in payloads.items():
        if tags[k][2] != ord('d'):
            continue
        entries, rest = parse(payload)
        assert not rest, 'unparsed DAR tail'
        pos = 0
        for tid, ext, size, blob in entries:
            if tid == TEXTURE and ext == ord('p'):
                found.append((offsets[k] + pos + 8, blob))
            pos += 8 + size
    assert len(found) == 1, 'expected exactly one grenade decal'
    return found[0]


def build_stage(original, usa):
    start, old = texture(original)
    _, source = texture(usa)
    assert (portio.sha256(old), portio.sha256(source)) == HASHES
    assert old[:128] == source[:128], 'PCX header/VRAM placement differs'
    assert geo(source)['bpp'] == 4
    decoded = pcx4.decode(source)
    encoded = pcx4.encode(source, *decoded)
    assert len(encoded) <= len(old), 'USA texture does not fit'
    replacement = encoded + bytes(len(old) - len(encoded))
    assert pcx4.decode(replacement) == decoded, 'lossless round trip failed'
    result = original[:start] + replacement + original[start + len(old):]
    assert len(result) == len(original)
    assert texture(result) == (start, replacement)
    print('selectvr 4D80.p: %dx%d, %d encoded bytes in %d-byte slot' %
          (decoded[0], decoded[1], len(encoded), len(old)))
    return result


def raw_package_records(isd, usd, disc, missions):
    """English raw package payload; rebuild.py computes parity for the full set."""
    original = v.stage_bytes(isd, STAGE)
    modified = build_stage(original, v.stage_bytes(usd, STAGE))
    lba = v.stage_lba(disc, isd, STAGE)
    records = v.inplace_records(lba, original, modified, merge_gap=0)
    usa_briefing = v.stage_bytes(usd, 'vr_grn02')
    assert usa_briefing[briefing_offset(usa_briefing, 'english')] == ord('4')
    for name in BRIEFING_STAGES:
        lba = v.stage_lba(disc, isd, name)
        stage = portio.patched_file(v.stage_bytes(isd, name), lba, [missions])
        pos = briefing_offset(stage, 'english')
        assert stage[pos] == ord('4'), 'English mission builder must correct the fuse'
        records.extend(portio.map_runs(lba, [(pos, b'4')]))
    # The only shared writes must be the five agreeing briefing digits.
    owned = {off + k: byte for off, data in portio.read_ppf(missions)
             for k, byte in enumerate(data)}
    overlaps = [(off + k, byte) for off, data in records
                for k, byte in enumerate(data) if off + k in owned]
    assert len(overlaps) == 5
    assert all(owned[off] == byte == ord('4') for off, byte in overlaps)
    return records


def collection_package(isd, usd, disc, folder):
    """Build both collection layouts against the packaged mission PPF, read back each.

    No installed mod or working-directory artifact participates in this build.
    """
    folder = Path(folder)
    mission = folder / MISSION_NAME
    found = {name for name in portio.entries(isd) if JP_LINE in v.stage_bytes(isd, name)}
    assert found == set(BRIEFING_STAGES), ('unexpected briefing coverage', found)
    original = v.stage_bytes(isd, STAGE)
    modified = build_stage(original, v.stage_bytes(usd, STAGE))
    lba = v.stage_lba(disc, isd, STAGE)
    texture_records = v.inplace_records(lba, original, modified, merge_gap=0)
    paths = []
    for language in ('japanese', 'english'):
        records = list(texture_records)
        expected = [(lba, original, modified)]
        digits = []
        for name in BRIEFING_STAGES:
            before = v.stage_bytes(isd, name)
            stage_lba = v.stage_lba(disc, isd, name)
            if language == 'english':
                before = portio.patched_file(before, stage_lba, [mission])
            pos = briefing_offset(before, language)
            digits.append(portio.image_offset(stage_lba, pos))
            records.extend(portio.map_runs(stage_lba, [(pos, b'4')]))
            expected.append((stage_lba, before, correct_briefing(before, language)))
        path = folder / (NAME if language == 'japanese' else Path(NAME).stem + '_english.ppf')
        path.write_bytes(portio.ppf(records, 'Integral VR grenade: four-second decal and text'))
        for stage_lba, before, after in expected:
            assert portio.patched_file(before, stage_lba, [path]) == after
        write_metadata(path, layout=language, grenade_digits=digits,
                       base_fingerprint=fingerprint(mission.read_bytes()) if language == 'english' else '')
        paths.append(path)
    return paths


def write_variant(out, records, expected, base_records=()):
    """Verify the payload PPF and raw parity against this variant's text base."""
    block = portio.blockcheck_of(v.INT_CONTAINER, v.INT_VR_BASE)
    out.write_bytes(portio.ppf(records, 'Integral VR grenade: four-second decal and text', block))
    assert not ppfcheck.check(str(out))[0]
    for lba, before, after in expected:
        assert portio.patched_file(before, lba, [out]) == after

    sectors = {}
    with open(v.INT_CONTAINER, 'rb') as handle:
        for off, data in records:
            sector, within = divmod(off, 2352)
            assert 24 <= within and within + len(data) <= 2072
            if sector not in sectors:
                handle.seek(v.INT_VR_BASE + sector * 2352)
                raw = handle.read(2352)
                assert cdecc.verify(raw), 'source sector parity mismatch'
                sectors[sector] = bytearray(raw)
    # English raw parity must include the complete English payload in every
    # touched sector, not just the changed digit or the Japanese original.
    for off, data in list(base_records) + list(records):
        sector, within = divmod(off, 2352)
        if sector in sectors:
            assert 24 <= within and within + len(data) <= 2072
            sectors[sector][within:within + len(data)] = data
    raw_records = list(records)
    for sector, data in sorted(sectors.items()):
        tail = cdecc.tail(data)
        raw_records.append((sector * 2352 + 2072, tail))
        data[2072:] = tail
        assert cdecc.verify(data)
    raw_out = out.with_name(out.stem + '_raw.ppf')
    raw_out.write_bytes(portio.ppf(raw_records, 'Integral VR grenade four-second fix + EDC/ECC', block))
    assert not ppfcheck.check(str(raw_out))[0]
    # Read back the actual serialized raw PPF, including split tail records.
    rebuilt = {}
    with open(v.INT_CONTAINER, 'rb') as handle:
        for sector in sectors:
            handle.seek(v.INT_VR_BASE + sector * 2352)
            rebuilt[sector] = bytearray(handle.read(2352))
    for off, data in list(base_records) + portio.read_ppf(raw_out):
        sector, within = divmod(off, 2352)
        if sector in rebuilt:
            rebuilt[sector][within:within + len(data)] = data
    assert rebuilt == sectors
    print('%s: %d records, %d payload bytes; PPF readback exact' %
          (out.name, len(records), sum(len(data) for _, data in records)))
    print('%s: %d sectors, EDC/ECC verified' % (raw_out.name, len(sectors)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--deploy', action='store_true')
    parser.add_argument('--missions-ppf', type=Path,
                        help='English layout to build against; defaults to deployed en_missions')
    args = parser.parse_args()
    installed_missions = Path(v.MODS) / MISSION_NAME
    missions = args.missions_ppf or (installed_missions if installed_missions.exists() else None)
    if args.deploy and args.missions_ppf:
        assert installed_missions.exists() and missions.read_bytes() == installed_missions.read_bytes(), (
            '--deploy requires the supplied English base to match the installed mission patch')
    discs = (v.int_disc(), v.usa_disc())
    directories = []
    for disc in discs:
        sd_lba, sd_size = v.files_of(disc)['/MGS/STAGE.DIR;1']
        directories.append(disc.read(sd_lba, sd_size))
    isd, usd = directories
    # Scan every stage, so a new/overlooked copy cannot silently remain wrong.
    found = {name for name in portio.entries(isd)
             if JP_LINE in v.stage_bytes(isd, name)}
    assert found == set(BRIEFING_STAGES), ('unexpected briefing coverage', found)
    usa_briefing = v.stage_bytes(usd, 'vr_grn02')
    assert usa_briefing[briefing_offset(usa_briefing, 'english')] == ord('4')
    original = v.stage_bytes(isd, STAGE)
    modified = build_stage(original, v.stage_bytes(usd, STAGE))
    lba = v.stage_lba(discs[0], isd, STAGE)
    texture_records = v.inplace_records(lba, original, modified, merge_gap=0)
    outputs = []
    for language in ('japanese', 'english') if missions else ('japanese',):
        records = list(texture_records)
        expected = [(lba, original, modified)]
        base_records = portio.read_ppf(missions) if language == 'english' else []
        for name in BRIEFING_STAGES:
            stage = v.stage_bytes(isd, name)
            stage_lba = v.stage_lba(discs[0], isd, name)
            if language == 'english':
                stage = portio.patched_file(stage, stage_lba, [missions])
            pos = briefing_offset(stage, language)
            if language == 'english':
                assert stage[pos] == ord('4'), (
                    'Rebuild en_missions with vr_windows.py --build first; '
                    'its five fuse digits must agree with the standalone fix')
            corrected = correct_briefing(stage, language)
            # Force ownership even when the English base already agrees.
            records += list(portio.map_runs(stage_lba, [(pos, b'4')]))
            assert corrected[:pos] == stage[:pos] and corrected[pos + 1:] == stage[pos + 1:]
            expected.append((stage_lba, stage, corrected))
            print('%s %s: fuse digit at +%X -> 4, all other bytes preserved' % (language, name, pos))
        out = Path(v.WORK) / (NAME if language == 'japanese' else
                              Path(NAME).stem + '_english.ppf')
        write_variant(out, records, expected, base_records)
        companion = write_metadata(out, layout=language,
                                   base_fingerprint=fingerprint(missions.read_bytes()) if language == 'english' else '',
                                   grenade_digits=[off for off, data in records[-5:]])
        outputs.append((out, companion, expected))
        if language == 'english':
            # Applying the addon BEFORE the base also gives the same result.
            for stage_lba, stage, corrected in expected[1:]:
                assert portio.patched_file(stage, stage_lba, [out, missions]) == corrected
            (out.with_suffix('.base.txt')).write_text(
                'English base: %s\nSHA256: %s\nRebuild this addon when the base changes.\n' %
                (missions, portio.sha256(missions.read_bytes())), encoding='utf-8')
    if args.deploy:
        # Both layouts are installed: the new loader selects one from the INI,
        # so switching English off no longer needs a manual asset rebuild.
        for out, companion, expected in outputs:
            deployed = Path(v.deploy(out.name, out.read_bytes()))
            v.deploy(companion.name, companion.read_bytes())
            assert deployed.read_bytes() == out.read_bytes()
            assert not ppfcheck.check(str(deployed))[0]
            for stage_lba, before, after in expected:
                assert portio.patched_file(before, stage_lba, [deployed]) == after
            print('deployed optional layout', deployed)
        if missions:
            companion = write_mission_metadata(missions, isd, discs[0])
            v.deploy(companion.name, companion.read_bytes())


if __name__ == '__main__':
    main()
