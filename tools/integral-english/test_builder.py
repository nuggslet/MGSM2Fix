"""Data-free checks for release recipe integrity and input handling."""
import io
import json
import os
import subprocess
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace
import zipfile
import disc_inputs
import mkimage
import release_builder as r


class BuilderTests(unittest.TestCase):
    def test_patch_requirements(self):
        from patch_requirements import missing_patches
        values = {('Patches', 'IntegralEnglishPatch'): 'false',
                  ('Patches', 'IntegralVREnglishPatch'): 'false',
                  ('Patches', 'GrenadeDelayFix'): 'true'}
        missing = missing_patches(values, self.root)
        self.assertEqual(len(missing), 1)
        self.assertTrue(all('_english' not in n for n in missing[0]['files']))
        for name in missing[0]['files']:
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b'installed')
        self.assertEqual(missing_patches(values, self.root), [])
        target.unlink()  # Missing companion must still warn.
        self.assertEqual(missing_patches(values, self.root)[0]['files'], [name])
        values['Patches', 'IntegralVREnglishPatch'] = 'true'
        self.assertTrue(any('_english.ppf' in n for g in missing_patches(values, self.root) for n in g['files']))
        values['Patches', 'DisableCDROM'] = 'true'
        self.assertEqual(missing_patches(values, self.root), [])
        self.assertEqual({g['builder'] for g in missing_patches({}, None)}, {'story','vr','grenade'})

    def install_fixture(self):
        import hashlib
        game, package = self.root / 'game', self.root / 'package'
        (game / 'windata').mkdir(parents=True)
        (game / 'METAL GEAR SOLID.exe').touch()
        (game / 'windata/alldata.bin').touch()
        names = ['mods/INTEGRAL/test.ppf', 'mods/INTEGRAL/test.ppf.json']
        for name in names:
            target = package / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b'new')
        (package / 'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(b'new').hexdigest() + '  ' + n + '\n' for n in names))
        original = game / names[0]
        original.parent.mkdir(parents=True)
        original.write_bytes(b'old')
        (game / 'mods/unrelated.ppf').write_bytes(b'keep')
        return game, package, names

    def test_install_backups_and_preserves_other_mods(self):
        from patch_install import install
        game, package, names = self.install_fixture()
        count, backup = install(package, game)
        self.assertEqual(count, 2)
        self.assertEqual((backup / names[0]).read_bytes(), b'old')
        self.assertEqual((game / names[0]).read_bytes(), b'new')
        self.assertEqual((game / names[1]).read_bytes(), b'new')
        self.assertEqual((game / 'mods/unrelated.ppf').read_bytes(), b'keep')

    def test_install_checks_before_writing_and_rolls_back(self):
        from patch_install import install
        import patch_install
        game, package, names = self.install_fixture()
        (package / names[1]).write_bytes(b'bad')
        with self.assertRaisesRegex(ValueError, 'verification'):
            install(package, game)
        self.assertEqual((game / names[0]).read_bytes(), b'old')
        (package / names[1]).write_bytes(b'new')
        replace = os.replace
        def fail_second(source, target):
            if str(target).endswith('.json'):
                raise OSError('simulated write failure')
            replace(source, target)
        with patch.object(patch_install.os, 'replace', side_effect=fail_second):
            with self.assertRaises(OSError):
                install(package, game)
        self.assertEqual((game / names[0]).read_bytes(), b'old')
        self.assertFalse((game / names[1]).exists())
        with (package / 'SHA256SUMS.txt').open('a') as stream:
            stream.write('bad  mods/../../outside.ppf\n')
        with self.assertRaisesRegex(ValueError, 'Unsafe'):
            install(package, game)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def test_ppf_delta_roundtrip(self):
        from portio import ppf
        original = bytes(range(256)) * 20
        target = ppf([(100, b'English'), (300, b'changed')], 'test')
        delta = r.transform_ppf(target, io.BytesIO(original))
        self.assertNotEqual(delta, target)
        self.assertEqual(r.transform_ppf(delta, io.BytesIO(original)), target)

    def test_truncated_disc_rejected(self):
        from portio import ppf
        with self.assertRaisesRegex(ValueError, 'truncated'):
            r.transform_ppf(ppf([(100, b'text')], 'test'), io.BytesIO(b'short'))

    def test_source_hash_rejected(self):
        path = self.root / 'disc.bin'
        path.write_bytes(b'wrong')
        with self.assertRaisesRegex(ValueError, 'unpatched'):
            r.verify_disc(path, '1', {'discs': {'1': {'size': 5, 'sha256': r.digest(b'right')}}})

    def test_recipe_output_hash_rejected(self):
        archive = self.root / 'recipe.zip'
        with zipfile.ZipFile(archive, 'w') as z:
            z.writestr('collection/readme.txt', b'wrong')
        manifest = {'packages': {'collection': {'readme.txt': {'sha256': r.digest(b'right')}}}}
        with zipfile.ZipFile(archive) as z, self.assertRaisesRegex(ValueError, 'verification'):
            r.materialize(z, manifest, 'collection', {}, self.root / 'out', [])

    def test_recipe_path_cannot_escape(self):
        manifest = {'packages': {'collection': {'../escape': {}}}}
        with self.assertRaisesRegex(ValueError, 'recipe path'):
            r.materialize(None, manifest, 'collection', {}, self.root, [])

    def test_existing_output_preserved(self):
        target = self.root / 'Integral-English-images'
        target.mkdir()
        marker = target / 'precious.bin'
        marker.write_bytes(b'keep')
        with self.assertRaisesRegex(ValueError, 'already exists'):
            r.build('steam', 'unused', 'images', 'all', self.root)
        self.assertEqual(marker.read_bytes(), b'keep')

    def test_japanese_grenade_image_uses_neutral_name(self):
        options = {'story': False, 'vr': False, 'grenade': True,
                   'english_default': False}
        self.assertEqual(r.output_label('images', options), 'Integral')
        self.assertEqual(r.output_directory_name('images', options), 'Integral-images')
        self.assertEqual(r.output_directory_name('patches', options), 'Integral-patches')
        self.assertEqual(r.output_label('patches', r.DEFAULT_PATCHES), 'Integral English')
        self.assertEqual(r.output_label('images', r.DEFAULT_PATCHES), 'Integral English')

    def test_folder_scan_skips_multibin_zip(self):
        source = self.root / 'redump'
        source.mkdir()
        with zipfile.ZipFile(source / 'other-disc.zip', 'w') as z:
            z.writestr('track1.bin', b'a')
            z.writestr('track2.bin', b'b')
        with patch('sys.stdout', new_callable=io.StringIO) as output:
            with self.assertRaisesRegex(ValueError, 'Missing Integral discs'):
                r.build('redump', source, 'images', '1', self.root)
        self.assertIn('Skipping other-disc.zip', output.getvalue())

    def test_zip_member_name_cannot_escape(self):
        archive = self.root / 'disc.zip'
        with zipfile.ZipFile(archive, 'w') as z:
            z.writestr('../../outside.bin', b'disc data')
        with disc_inputs.raw_input(archive) as raw:
            self.assertEqual(raw.read_bytes(), b'disc data')
            extracted = raw
        self.assertFalse(extracted.exists())

    def test_multitrack_zip_rejected(self):
        archive = self.root / 'disc.zip'
        with zipfile.ZipFile(archive, 'w') as z:
            z.writestr('track1.bin', b'a')
            z.writestr('track2.bin', b'b')
        with self.assertRaisesRegex(ValueError, 'exactly one BIN'):
            with disc_inputs.raw_input(archive):
                self.fail('multitrack input accepted')

    def test_steam_vr_stops_at_archive_member_not_container_end(self):
        size = 2352 * 3
        base = mkimage.DISCS['vr']['base']
        archive = SimpleNamespace(files={'system/roms/INTEGRAL_DISC3-washed.bin': (base, size)})
        with patch('m2archive.Archive', return_value=archive), patch('mkimage.os.path.getsize', return_value=base + size + 2352 * 10):
            self.assertEqual(mkimage.collection_size('unused.bin', 'vr'), size)

    def test_steam_index_cannot_point_outside_container(self):
        archive = SimpleNamespace(files={'system/roms/INTEGRAL_DISC1-washed.bin': (0, 2352 * 3)})
        with patch('m2archive.Archive', return_value=archive), patch('mkimage.os.path.getsize', return_value=2352):
            with self.assertRaisesRegex(ValueError, 'truncated'):
                mkimage.collection_size('unused.bin', '1')

    def test_committed_recipe_structure(self):
        with zipfile.ZipFile(Path(r.__file__).with_name('release-recipes.zip')) as z:
            manifest = json.loads(z.read('manifest.json'))
            self.assertEqual(set(manifest['discs']), {'1', '2', 'vr'})
            self.assertEqual(set(manifest['boots']), {'1', '2', 'vr'})
            for variant, count in [('collection', 30), ('raw', 34)]:
                files = manifest['packages'][variant]
                self.assertEqual(sum(n.endswith('.ppf') for n in files), count)
                for name, info in files.items():
                    payload = z.read(variant + '/' + name)
                    if not name.endswith('.ppf'):
                        self.assertEqual(r.digest(payload), info['sha256'])

    def test_patch_groups_and_grenade_layout(self):
        opts = dict(r.DEFAULT_PATCHES, story=False, vr=False)
        self.assertFalse(r.keep_patch('INTEGRAL_disc1_en_items.ppf', opts))
        self.assertFalse(r.keep_patch('INTEGRAL_vr_en_missions.ppf', opts))
        self.assertTrue(r.keep_patch('INTEGRAL_vr_fix_grenade_delay.ppf', opts))
        self.assertFalse(r.keep_patch('INTEGRAL_vr_fix_grenade_delay_english.ppf', opts))
        self.assertFalse(r.keep_patch('INTEGRAL_vr_zz_ecc.ppf', opts, raw=True))
        opts['grenade'] = False
        self.assertFalse(r.keep_patch('INTEGRAL_vr_fix_grenade_delay.ppf', opts))

    def test_empty_patch_package_rejected(self):
        with self.assertRaisesRegex(ValueError, 'at least one patch'):
            r.build('steam', 'unused', 'patches', 'all', self.root,
                    options={'story': False, 'vr': False, 'grenade': False})

    @unittest.skipUnless(os.environ.get('M2FIX_BUILDER_EXE'), 'packaged EXE smoke test')
    def test_packaged_worker_loads_recipes_without_game_data(self):
        inputs = self.root / 'empty-source'
        inputs.mkdir()
        log = self.root / 'worker.log'
        result = subprocess.run([os.environ['M2FIX_BUILDER_EXE'], '--worker',
            '--source', 'redump', '--input', str(inputs), '--operation', 'images',
            '--destination', str(self.root), '--log', str(log)], timeout=30)
        self.assertEqual(result.returncode, 1)
        self.assertIn('Missing Integral discs', log.read_text(encoding='utf-8'))
        self.assertFalse((self.root / 'Integral-English-images').exists())
        self.assertFalse(list(self.root.glob('.m2fix-*')))


if __name__ == '__main__':
    unittest.main()
