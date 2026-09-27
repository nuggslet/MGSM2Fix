"""Maintainer-only: prepare release binary differences from verified builds.
No complete disc, executable, SDK or decomp source is bundled.
"""
import argparse
import json
from pathlib import Path
import struct
import zipfile
import mkimage
from iso import Disc
from release_builder import digest, file_hash, snapshot, transform_ppf, xor


def prepare(game, redump, collection, raw, output):
    originals = {}
    for path in Path(redump).glob('*.bin'):
        disc = mkimage.identify(str(path))
        if disc:
            originals[disc] = path
    if set(originals) != set(mkimage.DISCS):
        raise ValueError('Need all three unpatched Integral BINs')
    manifest = dict(format=1, discs={}, boots={}, packages={})
    with zipfile.ZipFile(output, 'x', zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
        for disc, path in originals.items():
            manifest['discs'][disc] = dict(size=path.stat().st_size, sha256=file_hash(path))
            iso = Disc(path)
            try:
                lba, size = mkimage.exe_extent(path, 0, mkimage.DISCS[disc]['exe'])
                exe = iso.read(lba, size)
            finally:
                iso.f.close()
            member = 'system/roms/mgs_integral_disc%s-20230407.lz4a' % ('3' if disc == 'vr' else '1')
            ram = snapshot(game, member)
            address = struct.unpack_from('<I', exe, 24)[0] & 0x1fffff
            base = bytes(2048) + ram[address:address + len(exe) - 2048]
            bundle.writestr('boots/' + disc + '.xor', xor(exe, base))
            manifest['boots'][disc] = dict(member=member, snapshot_sha256=digest(ram), address=address,
                                           size=len(exe), sha256=digest(exe))
        for variant, folder in [('collection', collection), ('raw', raw)]:
            root = Path(folder) / 'package'
            report = json.loads((root / 'build-report.json').read_text())
            if report['variant'] != variant:
                raise ValueError('Wrong package variant')
            # Validate the maintainer's package before turning it into recipes.
            for line in (root / 'SHA256SUMS.txt').read_text().splitlines():
                expected, name = line.split('  ', 1)
                path = (root / name).resolve()
                if not path.is_relative_to(root.resolve()) or file_hash(path) != expected:
                    raise ValueError('Invalid input package: ' + name)
            files = {}
            contents = {}
            for path in sorted(root.rglob('*')):
                if not path.is_file():
                    continue
                name = path.relative_to(root).as_posix()
                if name == 'SHA256SUMS.txt':
                    continue
                data = path.read_bytes()
                if name == 'build-report.json':
                    # Release provenance, without the maintainer's local paths.
                    data = (json.dumps({key: report[key] for key in
                        ('variant', 'base_commit', 'decomp_origin', 'outputs', 'overlays', 'variant_constants')}, indent=2) + '\n').encode()
                contents[name] = data
            contents['SHA256SUMS.txt'] = ''.join(digest(data) + '  ' + name + '\n'
                for name, data in sorted(contents.items())).encode()
            for name, data in sorted(contents.items()):
                disc = next((d for d, spec in mkimage.DISCS.items() if name.startswith(spec['folder'] + '/')), None)
                files[name] = dict(sha256=digest(data), disc=disc)
                if name.endswith('.ppf'):
                    with originals[disc].open('rb') as source:
                        data = transform_ppf(data, source)
                bundle.writestr(variant + '/' + name, data)
            manifest['packages'][variant] = files
        bundle.writestr('manifest.json', json.dumps(manifest, indent=2))
    print('Prepared release recipes:', output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('game', 'redump', 'collection', 'raw', 'output'):
        parser.add_argument('--' + name, required=True)
    prepare(**vars(parser.parse_args()))
