"""Compare the packaged native worker with the Python reference on local discs.

Usage: py tools/M2FixGUI/test_matrix.py --exe dist/M2FixGUI.exe \
       --redump DIR --output NEW_DIR

DIR may contain raw BINs, ZIPs, or a larger Redump library. The test extracts
only the three supported Integral discs into a temporary directory. It never
modifies the source library or a game installation.
"""
import argparse
import hashlib
import itertools
import json
import mmap
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import zipfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS / 'integral-english'))

import cdecc
import langdefault
import mkimage
import release_builder as builder
from disc_inputs import raw_input
from portio import read_ppf


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load_originals(redump, staging, manifest):
    """Find, verify and retain only the three supported unpatched disc images."""
    candidates = sorted(
        (path for path in redump.iterdir()
         if path.is_file() and path.suffix.lower() in ('.bin', '.zip')),
        key=lambda path: 'integral' not in path.name.lower())
    originals = {}
    for candidate in candidates:
        if len(originals) == len(mkimage.DISCS):
            break
        try:
            with raw_input(candidate) as raw:
                disc = mkimage.identify(str(raw))
                if disc not in mkimage.DISCS or disc in originals:
                    continue
                builder.verify_disc(raw, disc, manifest)
                retained = staging / (disc + '.bin')
                shutil.copyfile(raw, retained)
                originals[disc] = retained
        except Exception as error:
            print(f'Skipping {candidate.name}: {error}', flush=True)
    missing = set(mkimage.DISCS) - originals.keys()
    check(not missing, 'Missing supported unpatched Integral discs: ' + ', '.join(sorted(missing)))
    return originals


def prepare_references(reference, originals, recipe, manifest):
    """Materialize Python collection and raw patch selections for the matrix."""
    reference.mkdir()
    selected_all = list(mkimage.DISCS)
    with zipfile.ZipFile(recipe) as bundle:
        for flags in itertools.product((False, True), repeat=3):
            bits = ''.join(str(int(enabled)) for enabled in flags)
            options = {
                'story': flags[0],
                'vr': flags[1],
                'grenade': flags[2],
                'english_default': True,
            }

            selected_patches = (['1', '2'] if options['story'] else [])
            if options['vr'] or options['grenade']:
                selected_patches.append('vr')
            if selected_patches:
                package = reference / ('collection-' + bits)
                package.mkdir()
                builder.materialize(bundle, manifest, 'collection', originals,
                                    package, selected_patches)
                builder.select_patches(bundle, manifest, package, originals,
                                       selected_patches, options, raw=False)

            package = reference / ('raw-' + bits)
            package.mkdir()
            builder.materialize(bundle, manifest, 'raw', originals, package, selected_all)
            builder.select_patches(bundle, manifest, package, originals,
                                   selected_all, options, raw=True)


def expected_image(disc, bits, english, originals, reference):
    image_path = originals[disc]
    folder = reference / ('raw-' + bits) / mkimage.DISCS[disc]['folder']
    with image_path.open('rb') as source, mmap.mmap(
            source.fileno(), 0, access=mmap.ACCESS_COPY) as image:
        for patch in sorted(folder.glob('*.ppf')):
            for offset, data in read_ppf(str(patch)):
                image[offset:offset + len(data)] = data
        if english:
            lba, size = mkimage.exe_extent(
                str(image_path), 0, mkimage.DISCS[disc]['exe'])
            executable = b''.join(
                image[(lba + i) * 2352 + 24:(lba + i) * 2352 + 2072]
                for i in range((size + 2047) // 2048)
            )[:size]
            touched = set()
            for address, word in langdefault.patch_for(executable).items():
                logical = langdefault.HDR + address - langdefault.TADDR
                offset = (lba + logical // 2048) * 2352 + 24 + logical % 2048
                image[offset:offset + 4] = struct.pack('<I', word)
                touched.add(offset // 2352)
            for sector in touched:
                start = sector * 2352
                image[start:start + 2352] = cdecc.fixed(image[start:start + 2352])
        return hashlib.sha256(image).hexdigest()


def worker_command(executable, source, operation, destination, log_path, flags,
                   english=True, source_kind='redump', disc=None):
    command = [
        str(executable), '--worker', '--source', source_kind,
        '--input', str(source), '--operation', operation,
        '--destination', str(destination), '--log', str(log_path),
    ]
    command.extend('--no-' + name for name, enabled in zip(
        ('story', 'vr', 'grenade'), flags) if not enabled)
    if not english:
        command.append('--no-english-default')
    if disc is not None:
        command.extend(('--discs', disc))
    return command


def run_matrix(executable, originals_dir, output, reference, originals):
    results = []
    for flags in itertools.product((False, True), repeat=3):
        bits = ''.join(str(int(enabled)) for enabled in flags)
        for operation, english in (('patches', False), ('images', False), ('images', True)):
            case = f'{operation}-{bits}-{int(english)}'
            destination = output / case
            destination.mkdir()
            log_path = destination / 'worker.log'
            command = worker_command(executable, originals_dir, operation,
                                    destination, log_path, flags, english)

            started = time.time()
            process = subprocess.run(command, timeout=300)
            log = log_path.read_text(encoding='utf-8', errors='replace')
            record = {'case': case, 'exit': process.returncode}
            if operation == 'patches' and bits == '000':
                check(process.returncode == 1 and 'Select at least one patch' in log,
                      case + ': empty patch selection was not rejected')
                record['rejected_empty_selection'] = True
            else:
                check(process.returncode == 0, case + ': ' + log)
                label = ('Integral English' if flags[0] or flags[1] or english
                         else 'Integral')
                suffix = 'patches' if operation == 'patches' else 'images'
                package = destination / (label.replace(' ', '-') + '-' + suffix)
                if operation == 'patches':
                    reference_package = reference / ('collection-' + bits)
                    actual = {
                        path.relative_to(package).as_posix(): sha256(path)
                        for path in package.rglob('*.ppf*')
                    }
                    wanted = {
                        path.relative_to(reference_package).as_posix(): sha256(path)
                        for path in reference_package.rglob('*.ppf*')
                    }
                    check(actual == wanted, case + ': patch files differ from Python reference')
                    record['matched_patch_files'] = len(actual)
                    for line in (package / 'SHA256SUMS.txt').read_text().splitlines():
                        digest, name = line.split('  ', 1)
                        check(sha256(package / name) == digest,
                              case + ': output manifest hash mismatch for ' + name)
                else:
                    record['sha256'] = {}
                    for disc in ('1', '2', 'vr'):
                        image = package / f'{label} {disc}.bin'
                        actual_hash = sha256(image)
                        wanted_hash = expected_image(
                            disc, bits, english, originals, reference)
                        check(actual_hash == wanted_hash,
                              f'{case} {disc}: BIN hash differs from Python reference')
                        record['sha256'][disc] = actual_hash
                        cue = image.with_suffix('.cue').read_text(encoding='ascii')
                        expected_cue = (
                            f'FILE "{image.name}" BINARY\n'
                            '  TRACK 01 MODE2/2352\n'
                            '    INDEX 01 00:00:00\n'
                        )
                        check(cue == expected_cue, case + ' ' + disc + ': CUE mismatch')
                    shutil.rmtree(package)
                check(not list(destination.glob('.m2fix-*')),
                      case + ': temporary build files were left behind')

            record['seconds'] = round(time.time() - started, 2)
            results.append(record)
            (output / 'results.json').write_text(json.dumps(results, indent=2))
            print(case + ' PASS ' + str(record['seconds']) + 's', flush=True)
    print('All 24 build configurations passed (48 raw disc images, 7 nonempty PPF packages).',
          flush=True)


def compare_patch_package(actual_package, reference_package, label):
    actual = {
        path.relative_to(actual_package).as_posix(): sha256(path)
        for path in actual_package.rglob('*.ppf*')
    }
    wanted = {
        path.relative_to(reference_package).as_posix(): sha256(path)
        for path in reference_package.rglob('*.ppf*')
    }
    check(actual == wanted, label + ': patch files differ from Redump reference')


def run_source_smokes(executable, redump, steam, output, reference, originals):
    """Cover valid ZIP extraction and the separate Steam restoration path."""
    default_flags = (True, True, True)

    zip_destination = output / 'redump-zip-input'
    zip_destination.mkdir()
    zip_log = zip_destination / 'worker.log'
    process = subprocess.run(worker_command(
        executable, redump, 'patches', zip_destination, zip_log, default_flags),
        timeout=300)
    log = zip_log.read_text(encoding='utf-8', errors='replace')
    check(process.returncode == 0, 'Redump ZIP input: ' + log)
    check('Extracting ' in log and 'Verified Redump disc' in log,
          'Redump ZIP input was not extracted and verified')
    compare_patch_package(zip_destination / 'Integral-English-patches',
                          reference / 'collection-111', 'Redump ZIP input')
    print('redump-valid-zip PASS', flush=True)

    for disc in ('1', '2', 'vr'):
        destination = output / ('redump-disc-' + disc)
        destination.mkdir()
        log_path = destination / 'worker.log'
        process = subprocess.run(worker_command(
            executable, originals['1'].parent, 'images', destination, log_path,
            default_flags, disc=disc), timeout=300)
        log = log_path.read_text(encoding='utf-8', errors='replace')
        check(process.returncode == 0, f'Redump selected disc {disc}: ' + log)
        package = destination / 'Integral-English-images'
        image = package / f'Integral English {disc}.bin'
        check(sha256(image) == expected_image(
            disc, '111', True, originals, reference),
            f'Redump selected disc {disc}: BIN differs from reference')
        check(len(list(package.glob('*.bin'))) == 1 and image.with_suffix('.cue').is_file(),
              f'Redump selected disc {disc}: selection or CUE mismatch')
        shutil.rmtree(package)
    print('redump-disc-selection PASS (1, 2, VR)', flush=True)

    if steam is None:
        return
    check(steam.is_dir(), 'Steam MGS1 folder not found: ' + str(steam))
    patch_destination = output / 'steam-patches'
    patch_destination.mkdir()
    patch_log = patch_destination / 'worker.log'
    process = subprocess.run(worker_command(
        executable, steam, 'patches', patch_destination, patch_log, default_flags,
        source_kind='steam'),
        timeout=300)
    log = patch_log.read_text(encoding='utf-8', errors='replace')
    check(process.returncode == 0, 'Steam patch build: ' + log)
    compare_patch_package(patch_destination / 'Integral-English-patches',
                          reference / 'collection-111', 'Steam patch build')
    print('steam-patches PASS', flush=True)

    image_destination = output / 'steam-images'
    image_destination.mkdir()
    image_log = image_destination / 'worker.log'
    process = subprocess.run(worker_command(
        executable, steam, 'images', image_destination, image_log, default_flags,
        source_kind='steam'),
        timeout=600)
    log = image_log.read_text(encoding='utf-8', errors='replace')
    check(process.returncode == 0, 'Steam image build: ' + log)
    package = image_destination / 'Integral-English-images'
    for disc in ('1', '2', 'vr'):
        image = package / f'Integral English {disc}.bin'
        check(sha256(image) == expected_image(
            disc, '111', True, originals, reference),
            f'Steam image {disc}: BIN differs from verified Redump reference')
        check(image.with_suffix('.cue').is_file(),
              f'Steam image {disc}: CUE was not generated')
    shutil.rmtree(package)
    check(not list(image_destination.glob('.m2fix-*')),
          'Steam image build left temporary files behind')
    print('steam-images PASS (3 BIN hashes match Redump)', flush=True)

    destination = output / 'steam-vr-selection'
    destination.mkdir()
    log_path = destination / 'worker.log'
    process = subprocess.run(worker_command(
        executable, steam, 'images', destination, log_path, default_flags,
        source_kind='steam', disc='vr'), timeout=300)
    log = log_path.read_text(encoding='utf-8', errors='replace')
    check(process.returncode == 0, 'Steam VR disc selection: ' + log)
    package = destination / 'Integral-English-images'
    image = package / 'Integral English vr.bin'
    check(sha256(image) == expected_image('vr', '111', True, originals, reference),
          'Steam VR disc selection: BIN differs from Redump reference')
    check(len(list(package.glob('*.bin'))) == 1 and image.with_suffix('.cue').is_file(),
          'Steam VR disc selection: selection or CUE mismatch')
    shutil.rmtree(package)
    print('steam-vr-selection PASS', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--redump', type=Path, required=True)
    parser.add_argument('--steam', type=Path,
                        help='Optional MGS1 Master Collection installation folder')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    executable = args.exe.resolve()
    redump = args.redump.resolve()
    steam = args.steam.resolve() if args.steam else None
    output = args.output.resolve()
    check(executable.is_file(), 'M2FixGUI executable not found: ' + str(executable))
    check(redump.is_dir(), 'Redump source folder not found: ' + str(redump))
    output.mkdir(parents=True, exist_ok=False)
    recipe = Path(builder.__file__).with_name('release-recipes.zip')
    with zipfile.ZipFile(recipe) as bundle:
        manifest = json.loads(bundle.read('manifest.json'))
    with tempfile.TemporaryDirectory(prefix='m2fix-matrix-') as temporary:
        staging = Path(temporary)
        originals_dir = staging / 'originals'
        originals_dir.mkdir()
        originals = load_originals(redump, originals_dir, manifest)
        reference = staging / 'reference'
        prepare_references(reference, originals, recipe, manifest)
        run_matrix(executable, originals_dir, output, reference, originals)
        run_source_smokes(executable, redump, steam, output, reference, originals)


if __name__ == '__main__':
    main()
