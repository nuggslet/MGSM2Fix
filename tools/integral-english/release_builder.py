"""Runtime for release-prepared binary differences. Never compiles game code."""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile
import zipfile
import lz4.block
import mkimage
from disc_inputs import raw_input
from m2archive import Archive


def digest(data):
    return hashlib.sha256(data).hexdigest()


def file_hash(path):
    with open(path, 'rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def xor(a, b):
    if len(a) != len(b):
        raise ValueError('Patch source is truncated')
    return bytes(x ^ y for x, y in zip(a, b))


def snapshot(game, member):
    blob = Archive(str(Path(game) / 'windata/alldata.bin')).read(member)
    first = blob.find(b'lz4a')
    second = blob.find(b'lz4a', first + 4)
    if first < 0 or second < 0:
        raise ValueError('Unsupported Steam startup snapshot')
    pos = second + 4
    size, expanded, name_length = struct.unpack_from('<2IB', blob, pos)
    if expanded != 2097152:
        raise ValueError('Unexpected Steam RAM size')
    start = pos + 9 + name_length
    if start + size > len(blob):
        raise ValueError('Truncated Steam snapshot')
    return lz4.block.decompress(blob[start:start + size], expanded)


def transform_ppf(data, source):
    """Symmetric XOR of each record payload with its original disc bytes."""
    result = bytearray(data)
    if data[:6] != b'PPF30\x02' or len(data) < 60:
        raise ValueError('Invalid PPF recipe')
    pos = 60 + (1024 if data[57] else 0)
    while pos < len(data):
        offset, length = struct.unpack_from('<QB', data, pos)
        pos += 9
        if not length or pos + length > len(data):
            raise ValueError('Truncated PPF recipe')
        source.seek(offset)
        result[pos:pos + length] = xor(data[pos:pos + length], source.read(length))
        pos += length
    return bytes(result)


def steam_disc(game, disc, target, bundle, manifest):
    spec = mkimage.DISCS[disc]
    info = manifest['discs'][disc]
    boot = manifest['boots'][disc]
    ram = snapshot(game, boot['member'])
    if digest(ram) != boot['snapshot_sha256']:
        raise ValueError('Unsupported Steam startup snapshot; verify your game files.')
    base = bytes(2048) + ram[boot['address']:boot['address'] + boot['size'] - 2048]
    exe = xor(base, bundle.read('boots/' + disc + '.xor'))
    if digest(exe) != boot['sha256']:
        raise ValueError('Restored PlayStation executable failed verification')
    container = Path(game) / mkimage.CONTAINER
    size = mkimage.collection_size(container, disc)
    if size != info['size']:
        raise ValueError('Unsupported Steam disc length')
    with container.open('rb') as src, target.open('xb') as dst:
        src.seek(spec['base'])
        left = size
        while left:
            chunk = src.read(min(left, 4 * 1024 * 1024))
            if not chunk:
                raise ValueError('Truncated Steam image')
            dst.write(chunk)
            left -= len(chunk)
        lba, extent = mkimage.exe_extent(container, spec['base'], spec['exe'])
        if extent != len(exe):
            raise ValueError('Unsupported Steam executable extent')
        for offset in range(0, len(exe), 2048):
            dst.seek((lba + offset // 2048) * 2352 + 24)
            dst.write(exe[offset:offset + 2048].ljust(2048, b'\0'))
    verify_disc(target, disc, manifest)


def verify_disc(path, disc, manifest):
    expected = manifest['discs'][disc]
    if path.stat().st_size != expected['size'] or file_hash(path) != expected['sha256']:
        raise ValueError('Disc ' + disc + ' does not match the supported unpatched Integral image.')


def materialize(bundle, manifest, variant, discs, output, selected):
    for name, info in manifest['packages'][variant].items():
        disc = info.get('disc')
        if disc and disc not in selected:
            continue
        target = output / name
        if not target.resolve().is_relative_to(output.resolve()):
            raise ValueError('Invalid release recipe path')
        data = bundle.read(variant + '/' + name)
        if name.endswith('.ppf'):
            with discs[disc].open('rb') as source:
                data = transform_ppf(data, source)
        if digest(data) != info['sha256']:
            raise ValueError('Generated patch failed verification: ' + name)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)


DEFAULT_PATCHES = {'story': True, 'vr': True, 'grenade': True, 'english_default': True}


def output_label(operation, options):
    english_content = options['story'] or options['vr'] or (
        operation == 'images' and options['english_default'])
    return 'Integral English' if english_content else 'Integral'


def output_directory_name(operation, options):
    suffix = 'patches' if operation == 'patches' else 'images'
    return output_label(operation, options).replace(' ', '-') + '-' + suffix


def keep_patch(name, options, raw=False):
    if '_zz_ecc' in name:
        return not raw  # Raw parity is always regenerated for the selected set.
    if '_fix_grenade_delay' in name:
        return options['grenade'] and (options['vr'] or '_english' not in name)
    if '_disc1_' in name or '_disc2_' in name:
        return options['story']
    if '_vr_en_' in name:
        return options['vr']
    raise ValueError('Unclassified release patch: ' + name)


def select_patches(bundle, manifest, package, discs, selected, options, raw):
    """Keep supported loader groups and rebuild parity for that exact selection."""
    for path in package.rglob('*.ppf'):
        if not keep_patch(path.name, options, raw):
            path.unlink()
            path.with_suffix('.ppf.json').unlink(missing_ok=True)
    if raw:
        from portio import ppf
        import rawdisc
        if 'vr' in selected and options['grenade'] and not options['vr']:
            # The English addon targets relocated English briefing text. Without
            # VR English use the original-layout correction, then compute raw parity.
            name = mkimage.DISCS['vr']['folder'] + '/INTEGRAL_vr_fix_grenade_delay.ppf'
            with discs['vr'].open('rb') as source:
                data = transform_ppf(bundle.read('collection/' + name), source)
            if digest(data) != manifest['packages']['collection'][name]['sha256']:
                raise ValueError('Grenade patch recipe failed verification')
            target = package / name
            target.parent.mkdir(parents=True, exist_ok=True)
            from portio import add_blockcheck
            target.write_bytes(add_blockcheck(data, mkimage.blockcheck_of_image(discs['vr'])))
        for disc in selected:
            folder = package / mkimage.DISCS[disc]['folder']
            folder.mkdir(parents=True, exist_ok=True)
            records = rawdisc.tails(discs[disc], 0, sorted(folder.glob('*.ppf')))
            # An empty correction is intentional: mkimage can still apply the
            # optional boot-language patch or write a verified unmodified image.
            (folder / 'INTEGRAL_selected_zz_ecc.ppf').write_bytes(ppf(
                records, 'Integral selected patches: sector parity',
                mkimage.blockcheck_of_image(discs[disc])))
    else:
        report_path = package / 'build-report.json'
        report = json.loads(report_path.read_text())
        names = {p.name for p in package.rglob('*.ppf')}
        report['outputs'] = {n: value for n, value in report['outputs'].items() if n in names}
        report['selected_patches'] = {k: v for k, v in options.items() if k != 'english_default'}
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        readme = (
            output_label('patches', options) + ' - selected collection patches\n\n' +
            '\n'.join(k + ': ' + ('enabled' if v else 'disabled')
                      for k, v in report['selected_patches'].items()) +
            '\n\nCopy mods into MGS1 with an MGSM2Fix build that includes the Integral patch controls. Retain all JSON companions.\n'
            'Remove the previous package using its manifest before installing this selection;\n'
            'unchecked patches already installed are not removed by this builder.\n'
            'Enable the corresponding M2Fix INI controls. English at startup is controlled\n'
            'by the collection EnglishText setting, not a PPF.\n\n')
        if options['story'] or options['vr']:
            readme += (
                'English text port: MGSM2Fix tools/integral-english.\n'
                'Compiled overlay changes derived from FoxdieTeam/mgs_reversing, commit ' +
                report['base_commit'] + '.\nhttps://github.com/FoxdieTeam/mgs_reversing\n')
        readme += (
            'See the builder project CREDITS.md for source provenance and licensing.\n\n'
            'Included PPFs:\n' + '\n'.join(sorted(names)) + '\n')
        (package / 'README.txt').write_text(readme, encoding='utf-8')
        (package / 'SHA256SUMS.txt').write_text(''.join(
            file_hash(p) + '  ' + p.relative_to(package).as_posix() + '\n'
            for p in sorted(package.rglob('*')) if p.is_file() and p.name != 'SHA256SUMS.txt'), encoding='utf-8')


def build(source, source_path, operation, selection, destination, recipe=None, options=None):
    options = {**DEFAULT_PATCHES, **(options or {})}
    if operation not in ('patches', 'images') or source not in ('steam', 'redump'):
        raise ValueError('Invalid build operation')
    selected = list(mkimage.DISCS) if selection == 'all' else [selection]
    if any(d not in mkimage.DISCS for d in selected):
        raise ValueError('Invalid disc selection')
    if operation == 'patches':
        selected = (['1', '2'] if options['story'] else []) + (['vr'] if options['vr'] or options['grenade'] else [])
        if not selected:
            raise ValueError('Select at least one patch to build.')
    destination = Path(destination).resolve()
    if not destination.is_dir():
        raise ValueError('Choose an existing destination folder')
    english_content = output_label(operation, options) == 'Integral English'
    final = destination / output_directory_name(operation, options)
    if final.exists():
        raise ValueError(str(final) + ' already exists. Choose another destination or move the previous output.')
    recipe = Path(recipe) if recipe else Path(__file__).with_name('release-recipes.zip')
    with zipfile.ZipFile(recipe) as bundle, tempfile.TemporaryDirectory(prefix='.m2fix-', dir=destination) as temp:
        work = Path(temp)
        manifest = json.loads(bundle.read('manifest.json'))
        if manifest.get('format') != 1:
            raise ValueError('Unsupported release recipe version')
        discs = {}
        if source == 'steam':
            for disc in selected:
                print('Restoring and verifying Integral disc ' + disc + ' from Steam...', flush=True)
                target = work / (disc + '.bin')
                steam_disc(source_path, disc, target, bundle, manifest)
                discs[disc] = target
        else:
            folder = Path(source_path)
            candidates = [folder] if folder.is_file() else sorted(p for p in folder.iterdir() if p.suffix.lower() in ('.bin', '.zip'))
            # Prefer Integral filenames, but identify every accepted input by ISO and full hash.
            candidates.sort(key=lambda p: 'integral' not in p.name.lower())
            scan_folder = folder.is_dir()
            for candidate in candidates:
                if set(discs) == set(selected):
                    break
                try:
                    with raw_input(candidate) as raw:
                        disc = mkimage.identify(str(raw))
                        if disc not in selected or disc in discs:
                            continue
                        print('Verifying Integral disc ' + disc + '...', flush=True)
                        verify_disc(raw, disc, manifest)
                        target = work / (disc + '.bin')
                        shutil.copyfile(raw, target)
                        discs[disc] = target
                except Exception as error:
                    if not scan_folder:
                        raise
                    print('Skipping ' + candidate.name + ': ' + str(error), flush=True)
            missing = set(selected) - discs.keys()
            if missing:
                raise ValueError('Missing Integral discs: ' + ', '.join(sorted(missing)) + '. Select a folder containing their unpatched BINs or ZIPs.')
        result = work / 'result'
        result.mkdir()
        variant = 'collection' if operation == 'patches' else 'raw'
        package = result if operation == 'patches' else work / 'patches'
        package.mkdir(exist_ok=True)
        patch_names = [name for enabled, name in (
            (options['story'], 'Story English'),
            (options['vr'], 'VR English'),
            (options['grenade'], 'grenade correction')) if enabled]
        print('Building ' + (', '.join(patch_names) if patch_names else 'no optional') +
              ' patches...', flush=True)
        materialize(bundle, manifest, variant, discs, package, selected)
        select_patches(bundle, manifest, package, discs, selected, options, operation == 'images')
        if operation == 'patches':
            archive_name = output_label('patches', options).replace(' ', '-') + '-collection.zip'
            with zipfile.ZipFile(result / archive_name, 'w', zipfile.ZIP_DEFLATED) as archive:
                for path in sorted(result.rglob('*')):
                    if path.is_file() and path.suffix != '.zip':
                        archive.write(path, path.relative_to(result).as_posix())
        else:
            for disc in selected:
                print('Writing ' + ('English ' if english_content else '') + 'disc ' + disc + '...', flush=True)
                image_name = output_label(operation, options) + ' ' + disc + '.bin'
                mkimage.main(['--redump', str(discs[disc]), '--ppfs', str(package), '--output',
                              str(result / image_name), '--cue', '--english-default',
                              'yes' if options['english_default'] else 'no'])
            (result / 'selected-patches.json').write_text(json.dumps(options, indent=2) + '\n', encoding='utf-8')
        # Publish only after every selected disc/patch has passed verification.
        if final.exists():
            raise ValueError('Output appeared during the build; refusing to replace it.')
        result.rename(final)
    print('Completed: ' + str(final), flush=True)
    return final
