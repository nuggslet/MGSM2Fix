"""Apply the native INI planner's four language/grenade plans to real VR stages.

Build test_patch_options.cpp with C++20 and the nlohmann/json include path,
then run: py verify_patch_options.py <test.exe> <staged-mods-directory>.
The staging directory needs the English set, both grenade layouts and JSON
companions. VR unlock PPFs are optional. The native tests cover selection;
this verifies the final disc bytes, including the runtime numeral overrides.
"""
import argparse
import json
from pathlib import Path
import subprocess

import pcx4
import portio
import vr_grenade as g
import vrlib as v


def apply(stage, lba, patches, root):
    result = bytearray(stage)
    for patch in patches:
        path = root / patch['file']
        if path.suffix.lower() != '.ppf':
            continue
        overrides = dict(patch['overrides'])
        for off, data in portio.read_ppf(path):
            sector, within = divmod(off, 2352)
            if not lba <= sector < lba + len(stage) // 2048:
                continue
            assert 24 <= within and within + len(data) <= 2072
            changed = bytes(overrides.get(off + n, value) for n, value in enumerate(data))
            at = (sector - lba) * 2048 + within - 24
            result[at:at + len(data)] = changed
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native_test', type=Path)
    parser.add_argument('mods', type=Path)
    args = parser.parse_args()
    native = subprocess.run([str(args.native_test), str(args.mods)], check=True,
                            capture_output=True, text=True)
    plans = json.loads(native.stdout)
    disc = v.int_disc()
    sd_at, sd_size = v.files_of(disc)['/MGS/STAGE.DIR;1']
    sd = disc.read(sd_at, sd_size)
    usa = v.usa_disc()
    us_at, us_size = v.files_of(usa)['/MGS/STAGE.DIR;1']
    us = usa.read(us_at, us_size)
    for plan in plans:
        language = 'english' if plan['english'] else 'japanese'
        digit = ord('4') if plan['grenade'] else ord('5')
        for name in g.BRIEFING_STAGES:
            original = v.stage_bytes(sd, name)
            lba = v.stage_lba(disc, sd, name)
            final = apply(original, lba, plan['patches'], args.mods)
            baseline = portio.patched_file(original, lba, [args.mods / g.MISSION_NAME]) if plan['english'] else original
            pos = g.briefing_offset(baseline, language)
            expected = baseline[:pos] + bytes([digit]) + baseline[pos + 1:]
            assert final == expected, (language, plan['grenade'], name)
            assert final[g.briefing_offset(final, language)] == digit
        name = g.STAGE
        original = v.stage_bytes(sd, name)
        lba = v.stage_lba(disc, sd, name)
        final = apply(original, lba, plan['patches'], args.mods)
        texture = g.texture(final)[1]
        expected_texture = g.texture(v.stage_bytes(us, name))[1] if plan['grenade'] else g.texture(original)[1]
        assert pcx4.decode(texture) == pcx4.decode(expected_texture)
        print('%s, grenade %s: all five briefings = %s, correct texture, exact non-digit bytes' %
              (language, plan['grenade'], chr(digit)))
    (Path(v.WORK) / 'verified-patch-options.json').write_text(json.dumps(plans, indent=2) + '\n')
    print('Native selection checks and all four real-disc configurations passed.')


if __name__ == '__main__':
    main()
