"""Installed PPF requirements for the settings exposed by M2FixGUI."""
from functools import lru_cache
import json
from pathlib import Path
import zipfile


@lru_cache(maxsize=1)
def requirements():
    # Use the release manifest so the English sets track the packaged recipes.
    with zipfile.ZipFile(Path(__file__).with_name('release-recipes.zip')) as bundle:
        names = json.loads(bundle.read('manifest.json'))['packages']['collection']
    groups = {
        ('Patches', 'IntegralEnglishPatch'): ('Story English', 'story',
            [n for n in names if '/INTEGRAL/INTEGRAL/' in n and n.endswith('.ppf')]),
        ('Patches', 'IntegralVREnglishPatch'): ('VR English', 'vr',
            [n for n in names if '/INTEGRAL_vr_en_' in n]),
    }
    vr = 'mods/INTEGRAL/VR-DISK/'
    for key, suffix, label in [('UnlockVRMissions', 'missions', 'VR mission unlock'),
                               ('UnlockVRExtras', 'extras', 'VR extras unlock'),
                               ('UnlockVRMovies', 'movies', 'VR movies unlock')]:
        paths = [vr + 'INTEGRAL_vr_unlock_' + suffix + '.ppf']
        if suffix == 'missions':
            paths.append('mods/VR-DISK_US/VRUS_unlock_missions.ppf')
        groups['Game', key] = (label, None, paths)
    groups['Game', 'UnlockTitleBonuses'] = ('Title bonuses unlock', None, [
        f'mods/INTEGRAL/INTEGRAL/{i}/INTEGRAL_disc{i+1}_unlock_title.ppf' for i in range(2)
    ] + [f'mods/MGS1_US/{i}/MGS1_disc{i+1}_unlock_title.ppf' for i in range(2)])
    return groups


def enabled(values, section, key, default=False):
    value = values.get((section, key), '(not set)').strip().lower()
    if value in ('', '(not set)'):
        return default
    return value in ('true', '1', 'yes', 'on')


def missing_patches(values, game_root):
    """Return enabled feature groups and missing installed files (not build history).

    game_root is the selected INI's directory, never an unrelated detected install.
    None means the INI has not been saved and no install location is known.
    """
    if enabled(values, 'Patches', 'DisableCDROM'):
        return []
    groups = dict(requirements())
    english = enabled(values, 'Patches', 'IntegralVREnglishPatch', True)
    grenade = 'mods/INTEGRAL/VR-DISK/INTEGRAL_vr_fix_grenade_delay' + ('_english' if english else '') + '.ppf'
    groups['Patches', 'GrenadeDelayFix'] = ('Grenade display correction', 'grenade', [grenade, grenade + '.json'])
    result = []
    for (section, key), (label, builder, paths) in groups.items():
        if not enabled(values, section, key, section == 'Patches'):
            continue
        missing = []
        for name in paths:
            try:
                path = Path(game_root) / name if game_root is not None else None
                present = path is not None and path.is_file() and path.stat().st_size > 0
            except OSError:
                present = False
            if not present:
                missing.append(name)
        if missing:
            result.append(dict(label=label, builder=builder, files=missing))
    return result
