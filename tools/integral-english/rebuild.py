"""Clean collection-patch build and local packaging; never writes to the game.

Nine families: the eight of 2026-09-04 plus en_abst (the MISSION LOG, added
2026-09-05), whose overlay abst.bin is compiled alongside option.bin and
preope.bin.

Usage: py rebuild.py --output D:/mgsbuild/repro1 [--compare-deployed]
Requires the local decomp Git repository, PSYQ SDK, and installed collection.
The output directory must not exist. It retains inputs, logs and build hashes.
"""
from pathlib import Path
import argparse
import importlib.metadata
import json
import os
import subprocess
import sys
import tarfile
import zipfile
from iso import Disc
from portio import (INTEGRAL_IMAGES, USA_IMAGES, stage, relocation, sha256,
                    read_ppf, ppf as make_ppf, blockcheck_of, add_blockcheck)
import rawdisc
from workdir import WORK, require_game, require_decomp

TOOLS = Path(__file__).resolve().parent
FAMILIES = ('items', 'menu', 'menu2', 'preope', 'brf', 'option', 'savemsg', 'camsave',
            'abst', 'pad2')
# The VR disc's own port (2026-09-06/07). Its tools are separate because the disc
# is a separate game - its own executable, overlays and containers - but the
# build is the same discipline, so it belongs in the same isolated run.
VR_SCRIPTS = ('vr_windows.py --build', 'vr_exe.py', 'vr_option.py', 'vr_menus.py', 'vr_memcard.py',
              'vr_camera.py', 'vr_movie.py')
VR_FAMILIES = ('missions', 'items', 'savemsg', 'option', 'title', 'camsave', 'movie', 'memcard')
VR_EXE_HASHES = {
    # Integral's VR executable is not on the collection's disc in usable form,
    # so it is BUILT from the decomp here (obj_vr/_mgsi.exe) and checked against
    # this hash rather than copied in. USA's is a supplied retail input.
    'vrint.exe': 'c370f8e41ec8fb78238bfe2ddbfc25a6d37ec8f0972c86ebfde075ecd4ee8dca',
    'vrus.exe': '8e8e59a97b5cc7cec137dd782fdeaa09097de1e53b1801c5617aa9132a2fb814',
}
BASE = '7964de7'
EXE_HASHES = {
    'int1.exe': '4b8252b65953a02021486406cfcdca1c7670d1d1a8f3cf6e750ef6e360dc3a2f',
    'int2.exe': '4b8252b65953a02021486406cfcdca1c7670d1d1a8f3cf6e750ef6e360dc3a2f',
    'us1.exe': '615e136083336957ed0b9b3805145bf5bbb35f7a16c2f160dba8f17bb71cc640',
    'us2.exe': '615e136083336957ed0b9b3805145bf5bbb35f7a16c2f160dba8f17bb71cc640',
}


def run(args, cwd, env, log):
    with log.open('ab') as stream:
        stream.write(('\n'+repr([str(a) for a in args])+'\n').encode())
        result = subprocess.run([str(a) for a in args], cwd=cwd, env=env,
                                stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError('Command failed; see '+str(log))


def source_hashes():
    return {p.name: sha256(p.read_bytes()) for p in sorted(TOOLS.iterdir())
            if p.suffix in ('.py', '.patch', '.json') or p.name.startswith('PACKAGE-README')}


def extract(game, work, executables):
    inputs = {}
    for prefix, container, bases, boots in (
        ('int', 'windata/dlc/dlc_japan.bin', INTEGRAL_IMAGES, ('SLPM_862.47','SLPM_862.48')),
        ('usa', 'windata/alldata.bin', USA_IMAGES, ('SLUS_005.94','SLUS_007.76'))):
        for disc, (base, boot) in enumerate(zip(bases, boots), 1):
            image = Disc(game/container, base)
            try:
                files = {n.upper(): (l,s) for n,l,s,d in image.walk() if not d}
                for name, key in ((prefix+str(disc)+'_stage.dir','/MGS/STAGE.DIR;1'),
                                  (('us' if prefix=='usa' else prefix)+str(disc)+'.exe','/MGS/'+boot+';1')):
                    lba, size = files[key]
                    data = image.read(lba,size)
                    source = 'collection ISO'
                    if name.endswith('.exe'):
                        # The collection preloads code from RAM snapshots and
                        # leaves these ISO executable extents zero-filled.
                        data = (executables/name).read_bytes()
                        assert data[:8] == b'PS-X EXE' and len(data) == size, name
                        assert sha256(data) == EXE_HASHES[name], 'unsupported retail executable: '+name
                        source = 'separately supplied retail PS-X EXE'
                    (work/name).write_bytes(data)
                    inputs[name] = dict(container=container, image_base=base, iso_path=key,
                                        source=source, lba=lba, bytes=size, sha256=sha256(data))
            finally:
                image.f.close()
    # The VR disc is a third and fourth image, inside the same two containers.
    # vrlib knows where; this only needs STAGE.DIR out of each.
    from vrlib import INT_VR_BASE, USA_VR_BASE
    for name, container, base in (('vrint_stage.dir', 'windata/dlc/dlc_japan.bin', INT_VR_BASE),
                                  ('vrus_stage.dir', 'windata/alldata.bin', USA_VR_BASE)):
        image = Disc(game/container, base)
        try:
            lba, size = next((l, s) for n, l, s, d in image.walk()
                             if not d and n.upper() == '/MGS/STAGE.DIR;1')
            data = image.read(lba, size)
            (work/name).write_bytes(data)
            inputs[name] = dict(container=container, image_base=base, iso_path='/MGS/STAGE.DIR;1',
                                source='collection ISO (VR)', lba=lba, bytes=size, sha256=sha256(data))
        finally:
            image.f.close()
    return inputs


def place(target, data, block):
    """Write one PPF into the package, giving a raw-disc build its block check.

    The block check is 1024 bytes of the original image at 0x9320. Ketchup skips
    the field, so it buys the collection nothing; on a raw disc an ordinary PPF
    tool compares it and refuses a patch aimed at a different release, which is
    worth having when the patch is being handed to strangers with their own
    dumps."""
    if block is not None:
        data = add_blockcheck(data, block)
    target.write_bytes(data)
    return data


def raw_tails(name, description, image_path, base, folder, substitutes, block, report):
    """The one extra PPF a raw disc needs: every touched sector's EDC/ECC.

    Built from the family patches already in `folder`, so it must come last -
    and it makes the set all-or-nothing, because a tail is computed from the
    final payload of the whole set. See rawdisc.py."""
    stats = {}
    records = rawdisc.tails(image_path, base, sorted(folder.glob('*.ppf')),
                            substitutes, stats)
    data = place(folder/name, make_ppf(records, description), block)
    report['outputs'][name] = dict(sha256=sha256(data), bytes=len(data),
                                   records=len(records), **stats)
    return stats


def effects(path, image):
    """Effective changed bytes, independent of PPF descriptions/run boundaries."""
    result = {}
    for offset, payload in read_ppf(path):
        within = offset % 2352
        assert 24 <= within and within+len(payload) <= 2072, 'sector-tail write'
        image.f.seek(image.base+offset)
        original = image.f.read(len(payload))
        assert len(original) == len(payload)
        for k, (old,new) in enumerate(zip(original,payload)):
            if new != old:
                result[offset+k] = new
            else:
                result.pop(offset+k,None)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    # No absolute defaults: these used to be four literal paths on the author's
    # D: drive, which meant a first run anywhere else failed pointing at a drive
    # that may not exist. workdir searches Steam for the game and looks beside
    # the repository for the decomp, and says what to pass when it cannot.
    parser.add_argument('--game', type=Path,
                        help='the Master Collection MGS1 folder (searched for via Steam)')
    parser.add_argument('--decomp', type=Path,
                        help='the MGS decomp checkout (searched for beside this repo)')
    parser.add_argument('--psyq', type=Path,
                        help='the PSY-Q SDK tree (defaults to <decomp>/../psyq)')
    parser.add_argument('--executables', type=Path,
                        help='the five retail executables (defaults to %s)' % WORK)
    parser.add_argument('--variant', choices=('collection', 'raw'), default='collection',
                        help='collection (the default, what mods/ gets) or raw, for a real PSX '
                             'disc image: SC_KEEP_LINES 6, OPTION_MC_CONTROL_SETTINGS 0, and '
                             'en_menu3 included')
    parser.add_argument('--compare-deployed', action='store_true')
    args = parser.parse_args()
    args.game = Path(require_game(str(args.game) if args.game else None))
    args.decomp = Path(require_decomp(str(args.decomp) if args.decomp else None))
    if args.psyq is None:
        args.psyq = args.decomp.parent / 'psyq'
        if not args.psyq.is_dir():
            parser.error('cannot find the PSY-Q SDK; pass --psyq <path> (looked '
                         'for %s)' % args.psyq)
    if args.executables is None:
        args.executables = Path(WORK)
    output, game, source, psyq = (p.resolve() for p in (args.output,args.game,args.decomp,args.psyq))
    if output.exists():
        parser.error('output must be a new directory; existing runs are never overwritten')
    if args.variant == 'raw' and args.compare_deployed:
        parser.error('--compare-deployed compares against mods/, which is the collection build')
    if len(str(output)) > 65 or ' ' in str(output):
        parser.error('use a short path without spaces for the PSYQ toolchain')
    # Fail before creating output or compiling anything when inputs are missing.
    errors = []
    for relative in ('windata/alldata.bin', 'windata/dlc/dlc_japan.bin'):
        if not (game / relative).is_file(): errors.append('missing game input: ' + str(game / relative))
    for name, expected in {**EXE_HASHES, 'vrus.exe': VR_EXE_HASHES['vrus.exe']}.items():
        path = args.executables / name
        if not path.is_file(): errors.append('missing retail executable: ' + str(path))
        elif sha256(path.read_bytes()) != expected: errors.append('unsupported retail executable: ' + str(path))
    for directory in ('psyq_4.3', 'psyq_4.4', 'psyq_4.5', 'aspsx'):
        if not (psyq / directory).is_dir(): errors.append('missing SDK directory: ' + str(psyq / directory))
    for package in ('Pillow', 'ninja'):
        try: importlib.metadata.version(package)
        except importlib.metadata.PackageNotFoundError: errors.append('missing Python package: ' + package)
    revision = subprocess.run(['git', '-C', str(source), 'cat-file', '-e', BASE + '^{commit}'], capture_output=True)
    if revision.returncode: errors.append('decomp checkout lacks pinned commit ' + BASE)
    if errors: parser.error('Build prerequisites failed:\n  ' + '\n  '.join(errors))
    output.mkdir(parents=True)
    work = output/'work'
    work.mkdir()
    decomp = output/'decomp'
    decomp.mkdir()
    log = output/'build.log'
    env = dict(os.environ, INTEGRAL_ENGLISH_WORK=str(output),
               INTEGRAL_ENGLISH_GAME=str(game), INTEGRAL_ENGLISH_DECOMP=str(decomp),
               INTEGRAL_ENGLISH_VARIANT=args.variant,
               PYTHONIOENCODING='utf-8', PYTHONUTF8='1')
    report = dict(variant=args.variant, base_commit=subprocess.check_output(
        ['git','-C',str(source),'rev-parse',BASE],text=True).strip(),
        # Whose source this is, recorded in every build rather than assumed.
        # The three overlays are compiled from it; see CREDITS.md.
        decomp_origin=subprocess.run(['git','-C',str(source),'remote','get-url','origin'],
                                     capture_output=True,text=True).stdout.strip()
                      or 'unknown',
        python=sys.version, packages={n:importlib.metadata.version(n)
                                    for n in ('Pillow','ninja')}, inputs={}, outputs={})
    report['sources'] = source_hashes()
    report['sdk_files'] = {p.relative_to(psyq).as_posix():sha256(p.read_bytes())
                           for p in sorted(psyq.rglob('*'))
                           if p.is_file() and '.git' not in p.relative_to(psyq).parts}
    print('Extracting original Integral and USA files...',flush=True)
    report['inputs'] = extract(game,work,args.executables.resolve())
    print('Compiling overlays in an isolated decomp export...',flush=True)
    archive = output/'decomp-source.tar'
    run(['git','-C',source,'archive','--format=tar','--output',archive,BASE],TOOLS,env,log)
    with tarfile.open(archive) as tar:
        tar.extractall(decomp,filter='data')
    patch = TOOLS/'decomp-overlay-changes.patch'
    run(['git','apply','--check',patch],decomp,env,log)
    run(['git','apply',patch],decomp,env,log)
    # The second variant constant lives in the decomp, not in a tool: opt.c
    # reproduces the collection's KEY CONFIG doorbell, which a raw PSX disc has
    # nothing to intercept and no RAM at 0x80200000 to write. Flip it in the
    # isolated export so the compiled overlay matches the variant.
    optc = decomp/'source/onoda/option/opt.c'
    want = '#define OPTION_MC_CONTROL_SETTINGS %d' % (0 if args.variant == 'raw' else 1)
    text = optc.read_text(encoding='utf-8')
    assert text.count('#define OPTION_MC_CONTROL_SETTINGS 1') == 1, 'opt.c constant moved'
    optc.write_text(text.replace('#define OPTION_MC_CONTROL_SETTINGS 1', want), encoding='utf-8')
    report['variant_constants'] = {'OPTION_MC_CONTROL_SETTINGS': 0 if args.variant == 'raw' else 1,
                                   'SC_KEEP_LINES': 6 if args.variant == 'raw' else 4}
    # Exported build.py normally builds/compares the entire matching game.
    # Stop after generation so only the three changed overlays are compiled.
    generator = decomp/'build/build.py'
    text = generator.read_text(encoding='utf-8')
    marker = 'time_before = time.time()'
    assert text.count(marker) == 1
    generator.write_text(text.split(marker)[0]+'sys.exit(0)\n',encoding='utf-8')
    run([sys.executable,'build.py','--psyq_path',psyq,'--variant','main_exe'],decomp/'build',env,log)
    run([sys.executable,'-m','ninja','-j','2','../obj/preope.bin','../obj/option.bin','../obj/abst.bin'],decomp/'build',env,log)
    report['overlays'] = {n:sha256((decomp/'obj'/(n+'.bin')).read_bytes()) for n in ('option','preope','abst')}
    # Integral's VR executable: the collection's copy is unusable, so it is built
    # here rather than trusted. The generator has to run again for the vr_exe
    # variant (it writes a different ninja file), then one target.
    print('Building the VR executable from the decomp...',flush=True)
    run([sys.executable,'build.py','--psyq_path',psyq,'--variant','vr_exe'],decomp/'build',env,log)
    run([sys.executable,'-m','ninja','-j','2','../obj_vr/_mgsi.exe'],decomp/'build',env,log)
    vrint = (decomp/'obj_vr/_mgsi.exe').read_bytes()
    assert sha256(vrint) == VR_EXE_HASHES['vrint.exe'], 'the rebuilt VR executable is not the expected build'
    (work/'vrint.exe').write_bytes(vrint)
    report['inputs']['vrint.exe'] = dict(source='built here from the decomp (obj_vr/_mgsi.exe)',
                                         bytes=len(vrint), sha256=sha256(vrint))
    vrus = (args.executables.resolve()/'vrus.exe').read_bytes()
    assert sha256(vrus) == VR_EXE_HASHES['vrus.exe'], 'unsupported USA VR executable'
    (work/'vrus.exe').write_bytes(vrus)
    report['inputs']['vrus.exe'] = dict(source='separately supplied retail SLUS-00957',
                                        bytes=len(vrus), sha256=sha256(vrus))
    # Only source geometry is copied; all derived placements are regenerated.
    (work/'brf_quads_all.json').write_bytes((TOOLS/'brf_quads_all.json').read_bytes())
    scripts = ['items.py','menu2.py','preope_usa.py','brf_build.py','optsctext.py',
               'savemsg.py','camsave.py','abst_build.py','pad2.py']
    families = list(FAMILIES)
    if args.variant == 'raw':
        # en_menu3 exists only here: the collection patches that same block, so
        # deploying it there kills the title stage (README, "Why `en_menu3` is
        # raw-disc only"). menu3.py refuses --deploy for the same reason.
        scripts.append('menu3.py')
        families.append('menu3')
    print('Building %d patch families (%s)...' % (len(families), args.variant),flush=True)
    for script in scripts:
        run([sys.executable,TOOLS/script],TOOLS,env,log)
    print('Building the VR disc: eight patches (vr_windows rebuilds 92 stages)...',flush=True)
    for script in VR_SCRIPTS:
        parts = script.split()
        # Order matters: vr_windows ports the `movie` stage and hands it to
        # vr_movie as work/vr_movie_base.bin rather than writing its records,
        # so that one patch owns that stage. vr_movie runs last in VR_SCRIPTS.
        run([sys.executable,TOOLS/parts[0]]+parts[1:],TOOLS,env,log)
    dist = output/'package'
    mods = dist/'mods/INTEGRAL/INTEGRAL'
    container = game/'windata/dlc/dlc_japan.bin'
    raw = args.variant == 'raw'
    for disc, base in enumerate(INTEGRAL_IMAGES):
        block = blockcheck_of(container,base) if raw else None
        image = Disc(container,base)
        target = mods/str(disc)
        target.mkdir(parents=True)
        try:
            brf = relocation(image,'brf',(work/'brf_en.bin').read_bytes(),128,
                             'MGS Integral: English brf')
            (work/('INTEGRAL_disc%d_en_brf.ppf' % (disc+1))).write_bytes(brf)
            for family in families:
                name = 'INTEGRAL_disc%d_en_%s.ppf' % (disc+1,family)
                built = work/name
                if family == 'option':
                    built = work/('option_sctext_disc%d.ppf' % (disc+1))
                elif family == 'menu3':
                    built = work/('INTEGRAL_disc%d_en_menu3_raw.ppf' % (disc+1))
                data = place(target/name,built.read_bytes(),block)
                from ppfcheck import check
                problems,n,span,desc = check(target/name)
                assert not problems, (name,problems)
                effective = effects(target/name,image)
                item = dict(sha256=sha256(data),bytes=len(data),
                            records=n,changed_bytes=len(effective))
                if args.compare_deployed:
                    reference = game/'mods/INTEGRAL/INTEGRAL'/str(disc)/name
                    prior = effects(reference,image)
                    mismatch = [p for p in effective.keys() | prior.keys() if effective.get(p)!=prior.get(p)]
                    item['reference_sha256'] = sha256(reference.read_bytes())
                    item['reference_effect_equal'] = not mismatch
                    item['difference_count'] = len(mismatch)
                    item['difference_addresses'] = [hex(p) for p in sorted(mismatch)[:12]]
                report['outputs'][name] = item
            if raw:
                # The executable's ISO extent is zero-filled in the collection's
                # image, so the retail file goes back before any sum is taken;
                # rawdisc then refuses to emit a tail for any sector that does
                # not first verify against its own stored parity.
                exe = 'int%d.exe' % (disc+1)
                substitutes = rawdisc.Substitutes()
                substitutes.add(report['inputs'][exe]['lba'],(work/exe).read_bytes(),exe)
                stats = raw_tails('INTEGRAL_disc%d_zz_ecc.ppf' % (disc+1),
                                  'MGS Integral disc %d: raw-disc EDC/ECC' % (disc+1),
                                  container,base,target,substitutes,block,report)
                print('  disc %d: EDC/ECC recomputed for %d sector(s)'
                      % (disc+1,stats['sectors']),flush=True)
        finally:
            image.f.close()
    # The VR disc's own folder. Ketchup gives it no numbered subdirectory (one
    # disk in that version), and its PPF offsets address a different image, so
    # it gets its own overlap check below rather than joining the main one.
    from vrlib import INT_VR_BASE
    vrmods = dist/'mods/INTEGRAL/VR-DISK'
    vrmods.mkdir(parents=True)
    vrblock = blockcheck_of(container,INT_VR_BASE) if raw else None
    vrimage = Disc(container,INT_VR_BASE)
    try:
        for family in VR_FAMILIES:
            name = 'INTEGRAL_vr_en_%s.ppf' % family
            built = work/name
            data = place(vrmods/name,built.read_bytes(),vrblock)
            if family == 'missions':
                from vr_grenade import write_mission_metadata
                write_mission_metadata(vrmods/name, (work/'vrint_stage.dir').read_bytes(), vrimage)
            from ppfcheck import check
            problems,n,span,desc = check(vrmods/name)
            assert not problems, (name,problems)
            report['outputs'][name] = dict(sha256=sha256(data),
                                           bytes=len(data),records=n,
                                           changed_bytes=len(effects(vrmods/name,vrimage)))
            if args.compare_deployed:
                reference = game/'mods/INTEGRAL/VR-DISK'/name
                prior, effective = effects(reference,vrimage), effects(vrmods/name,vrimage)
                mismatch = [p for p in effective.keys() | prior.keys() if effective.get(p)!=prior.get(p)]
                report['outputs'][name]['reference_sha256'] = sha256(reference.read_bytes())
                report['outputs'][name]['reference_effect_equal'] = not mismatch
                report['outputs'][name]['difference_count'] = len(mismatch)
                report['outputs'][name]['difference_addresses'] = [hex(p) for p in sorted(mismatch)[:12]]
        if not raw:
            from vr_grenade import collection_package
            for path in collection_package((work/'vrint_stage.dir').read_bytes(),
                                           (work/'vrus_stage.dir').read_bytes(), vrimage, vrmods):
                problems, n, span, desc = check(path)
                assert not problems, (path.name, problems)
                report['outputs'][path.name] = dict(sha256=sha256(path.read_bytes()),
                    bytes=path.stat().st_size, records=n, changed_bytes=len(effects(path, vrimage)))
        if raw:
            # Include the standalone correction in raw packages. Use payload
            # records only: the final shared ECC pass must include every English
            # patch, rather than a standalone addon's narrower sector base.
            from vr_grenade import raw_package_records
            name = 'INTEGRAL_vr_fix_grenade_delay_english.ppf'
            records = raw_package_records(
                (work/'vrint_stage.dir').read_bytes(),
                (work/'vrus_stage.dir').read_bytes(), vrimage,
                vrmods/'INTEGRAL_vr_en_missions.ppf')
            data = make_ppf(records, 'Integral VR grenade: four-second decal and text', vrblock)
            (vrmods/name).write_bytes(data)
            problems, n, span, desc = check(vrmods/name)
            assert not problems, (name, problems)
            report['outputs'][name] = dict(sha256=sha256(data), bytes=len(data),
                                           records=n, changed_bytes=len(effects(vrmods/name, vrimage)))
            vrlba,_ = next((l,s) for n,l,s,d in vrimage.walk()
                           if not d and n.upper() == '/MGS/SLPM_862.49;1')
            substitutes = rawdisc.Substitutes()
            substitutes.add(vrlba,(work/'vrint.exe').read_bytes(),'vrint.exe')
            stats = raw_tails('INTEGRAL_vr_zz_ecc.ppf',
                              'MGS Integral VR disc: raw-disc EDC/ECC',
                              container,INT_VR_BASE,vrmods,substitutes,vrblock,report)
            print('  VR disc: EDC/ECC recomputed for %d sector(s)'
                  % stats['sectors'],flush=True)
    finally:
        vrimage.f.close()
    # No VR patch may write a byte another one writes. Until 2026-09-07 exactly
    # one pair was allowed to - vr_en_movie was built on top of vr_en_missions
    # and had to load after it, which nothing enforced but Ketchup's file-name
    # order. vr_windows now hands the whole `movie` stage to vr_movie instead
    # (vr_windows.HANDOVER), so the set is disjoint and this is absolute.
    vrwrites = {}
    for path in sorted(vrmods.glob('*en_*.ppf')):
        for off,data in read_ppf(path):
            for k,value in enumerate(data):
                prior = vrwrites.get(off+k)
                assert not (prior and prior[1] != path.name), \
                    ('two VR patches write the same byte',path.name,hex(off+k),prior and prior[1])
                vrwrites[off+k] = (value,path.name)
    # Check overlapping writes in the actual packaged set, before offering it.
    for disc in (0,1):
        writes = {}
        for path in sorted((mods/str(disc)).glob('*en_*.ppf')):
            for off,data in read_ppf(path):
                for k,value in enumerate(data):
                    assert off+k not in writes or writes[off+k][0] == value, (path.name,hex(off+k),writes[off+k][1])
                    writes[off+k] = (value,path.name)
    if args.compare_deployed:
        # `vr_en_missions` and `vr_en_movie` were repartitioned on 2026-09-07:
        # the same disc bytes, split between the two files differently. Compared
        # one file at a time that reads as two failures; compared as the set
        # Ketchup actually applies, it is what it is - identical.
        def vr_effect(folder):
            state = {}
            for path in sorted(Path(folder).glob('INTEGRAL_vr_en_*.ppf')):
                for off,data in read_ppf(path):
                    for k,value in enumerate(data):
                        state[off+k] = value
            return state
        mine, theirs = vr_effect(vrmods), vr_effect(game/'mods/INTEGRAL/VR-DISK')
        vrimage = Disc(container,INT_VR_BASE)
        try:
            differs = []
            for at in mine.keys() | theirs.keys():
                if mine.get(at) == theirs.get(at):
                    continue
                vrimage.f.seek(vrimage.base+at)
                retail = vrimage.f.read(1)[0]
                if mine.get(at,retail) != theirs.get(at,retail):
                    differs.append(at)
        finally:
            vrimage.f.close()
        report['vr_set_effect_equal'] = not differs
        report['vr_set_differences'] = [hex(a) for a in sorted(differs)[:12]]
        if not differs:
            for name in ('INTEGRAL_vr_en_missions.ppf','INTEGRAL_vr_en_movie.ppf'):
                item = report['outputs'].get(name)
                if item and item.get('reference_effect_equal') is False:
                    item['reference_effect_equal'] = 'equal as a set (repartitioned 2026-09-07)'
    if source_hashes() != report['sources']:
        raise RuntimeError('Build scripts or package instructions changed during the build; rerun in a fresh output directory. No ZIP created.')
    (output/'build-report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    bad = [n for n,v in report['outputs'].items() if v.get('reference_effect_equal') is False]
    if report.get('vr_set_effect_equal') is False:
        bad.append('the VR set as a whole')
    if bad:
        raise RuntimeError('Deployed comparison differs: '+', '.join(bad)+'; see build-report.json. No ZIP created.')
    # The packaged README is per variant, and gets the build stamped into it -
    # a hand-kept one drifted out of date every time the port gained a patch.
    readme = TOOLS/('PACKAGE-README-raw.txt' if raw else 'PACKAGE-README.txt')
    stamp = ['', '-- This build ' + '-'*54, '',
             'Variant: %s.  Patches: %d.' % (args.variant, len(report['outputs'])),
             'SC_KEEP_LINES %d, OPTION_MC_CONTROL_SETTINGS %d, en_menu3 %s.'
             % (report['variant_constants']['SC_KEEP_LINES'],
                report['variant_constants']['OPTION_MC_CONTROL_SETTINGS'],
                'included' if raw else 'excluded'),
             'Decomp %s.  See SHA256SUMS.txt and build-report.json.'
             % report['base_commit'][:12], '']
    for name in sorted(report['outputs']):
        item = report['outputs'][name]
        stamp.append('  %-38s %8d bytes  %5d records'
                     % (name, item['bytes'], item['records']))
    (dist/'README.txt').write_bytes(readme.read_bytes()
                                    + '\n'.join(stamp).replace('\n','\r\n').encode())
    (dist/'build-report.json').write_bytes((output/'build-report.json').read_bytes())
    manifest = {str(p.relative_to(dist)).replace('\\','/'):sha256(p.read_bytes())
                for p in sorted(dist.rglob('*')) if p.is_file()}
    (dist/'SHA256SUMS.txt').write_text(''.join(h+'  '+n+'\n' for n,h in manifest.items()),encoding='utf-8')
    # Fixed ZIP metadata and ordering; the report records the build environment.
    zip_path = output/('Integral-English-%s.zip' % args.variant)
    with zipfile.ZipFile(zip_path,'w',compression=zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(dist.rglob('*')):
            if path.is_file():
                info = zipfile.ZipInfo(path.relative_to(dist).as_posix(),(2026,9,4,0,0,0))
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info,path.read_bytes())
    print('Verified %d PPFs (%s variant, %d main + %d VR); %s'
          % (len(report['outputs']), args.variant,
             sum(1 for n in report['outputs'] if '_disc' in n),
             sum(1 for n in report['outputs'] if '_vr_' in n), zip_path),flush=True)


if __name__ == '__main__':
    main()
