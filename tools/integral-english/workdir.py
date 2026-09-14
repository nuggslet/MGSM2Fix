"""Resolve build inputs: explicit arguments, environment overrides, then discovery.

INTEGRAL_ENGLISH_WORK names an existing root containing work/ (created by
builders as needed); otherwise use cwd/work. Game discovery uses Steam libraries;
decomp discovery uses sibling checkouts. Invalid explicit overrides are errors.
Run this module to print the resolved locations.
"""
import os


# The collection's MGS1 folder is recognised by its own data, never by name.
_GAME_MARKERS = ('windata/alldata.bin', 'windata/dlc/dlc_japan.bin')
_STEAM_APP = 'MGS1'          # under <library>/steamapps/common/
_DECOMP_MARKERS = ('source/main/main.c', 'build/build.py')

# --- which build is being made.
#
# Two constants differ between the collection patch and a raw PSX disc patch,
# and until 2026-09-07 both were edited by hand before a raw build:
#
#   SC_KEEP_LINES               optsctext.py   4 collection / 6 raw
#   OPTION_MC_CONTROL_SETTINGS  opt.c          1 collection / 0 raw
#
# plus `en_menu3`, which exists ONLY for the raw disc (the collection patches
# that block itself - README, "Why `en_menu3` is raw-disc only"). VARIANT is the
# one switch; `rebuild.py --variant raw` sets it for every tool it runs, and a
# tool run by hand picks it up from the environment the same way.
VARIANT = os.environ.get('INTEGRAL_ENGLISH_VARIANT', 'collection').strip().lower()
if VARIANT not in ('collection', 'raw'):
    raise SystemExit('INTEGRAL_ENGLISH_VARIANT must be "collection" or "raw", not %r' % VARIANT)
RAW = VARIANT == 'raw'


def pick(collection, raw):
    """the value for this build: pick(4, 6) is 4 normally, 6 under --variant raw"""
    return raw if RAW else collection


def _root():
    env = os.environ.get('INTEGRAL_ENGLISH_WORK')
    if env:
        if not os.path.isdir(env):
            raise SystemExit('INTEGRAL_ENGLISH_WORK must name an existing root directory: ' + env)
        return os.path.abspath(env)
    return os.getcwd()


def _looks_like(directory, markers):
    return bool(directory) and any(
        os.path.isfile(os.path.join(directory, m)) for m in markers)


def _steam_libraries():
    """Every Steam library root Steam itself admits to, best effort."""
    roots = []
    try:                                     # where Steam is installed
        import winreg
        for hive, key in ((winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam'),
                          (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\WOW6432Node\Valve\Steam')):
            try:
                with winreg.OpenKey(hive, key) as handle:
                    for name in ('SteamPath', 'InstallPath'):
                        try:
                            roots.append(winreg.QueryValueEx(handle, name)[0])
                        except OSError:
                            pass
            except OSError:
                pass
    except ImportError:                      # not Windows
        pass
    roots += [r'C:/Program Files (x86)/Steam', r'C:/Program Files/Steam']
    roots += ['%s:/Steam' % chr(d) for d in range(ord('A'), ord('Z') + 1)]

    libraries, seen = [], set()
    for root in roots:
        root = root.replace('\\', '/').rstrip('/')
        if not root or root.lower() in seen or not os.path.isdir(root):
            continue
        seen.add(root.lower())
        libraries.append(root)
        # libraryfolders.vdf lists the other drives Steam installs to. Parsed
        # by picking out quoted "path" values rather than by understanding VDF.
        for vdf in (root + '/steamapps/libraryfolders.vdf',
                    root + '/config/libraryfolders.vdf'):
            try:
                with open(vdf, encoding='utf-8', errors='replace') as handle:
                    text = handle.read()
            except OSError:
                continue
            import re
            for path in re.findall(r'"path"\s*"([^"]+)"', text):
                path = path.replace('\\\\', '/').replace('\\', '/').rstrip('/')
                if path.lower() not in seen and os.path.isdir(path):
                    seen.add(path.lower())
                    libraries.append(path)
    return libraries


def find_game(explicit=None):
    """The collection's MGS1 directory, or '' if it cannot be found.

    Order: an explicit path, the environment variable, then Steam libraries. A candidate counts only if it holds the
    collection's data files - a directory called MGS1 is not evidence.
    """
    # A path the caller typed is a statement of intent: if it is wrong, say so
    # rather than quietly using a different install they did not ask for.
    explicit = os.fspath(explicit) if explicit is not None else os.environ.get('INTEGRAL_ENGLISH_GAME')
    if explicit is not None and not _looks_like(explicit, _GAME_MARKERS):
        raise SystemExit(
            '%s is not the collection\'s MGS1 directory.\n'
            '  Expected to find %s under it.\n'
            '  (Searching instead would use an install you did not name.)'
            % (explicit, ' or '.join(_GAME_MARKERS)))
    candidates = [explicit]
    for library in _steam_libraries():
        candidates.append(library + '/steamapps/common/' + _STEAM_APP)
    for candidate in candidates:
        if _looks_like(candidate, _GAME_MARKERS):
            return candidate.replace('\\', '/').rstrip('/')
    return ''


def find_decomp(explicit=None):
    """The MGS decomp checkout, or '' - same rules, checked by its own files."""
    explicit = os.fspath(explicit) if explicit is not None else os.environ.get('INTEGRAL_ENGLISH_DECOMP')
    if explicit is not None and not all(os.path.isfile(os.path.join(explicit, m)) for m in _DECOMP_MARKERS):
        raise SystemExit(
            '%s is not the MGS decomp checkout.\n'
            '  Expected to find %s under it.'
            % (explicit, ' and '.join(_DECOMP_MARKERS)))
    candidates = [explicit]
    here = os.path.dirname(os.path.abspath(__file__))
    for up in (3, 4):                       # a sibling of the repo
        parent = os.path.abspath(os.path.join(here, *(['..'] * up)))
        candidates += [os.path.join(parent, n) for n in ('d', 'mgs', 'mgs-decomp')]
    for candidate in candidates:
        if candidate and all(os.path.isfile(os.path.join(candidate, m)) for m in _DECOMP_MARKERS):
            return candidate.replace('\\', '/').rstrip('/')
    return ''


def require_game(explicit=None):
    """find_game(), but say what to do instead of failing later on open()."""
    found = find_game(explicit)
    if not found:
        raise SystemExit(
            'Cannot find the Master Collection MGS1 directory.\n'
            '  Looked in: the --game argument, $INTEGRAL_ENGLISH_GAME, and every\n'
            '  Steam library on this machine.\n'
            '  It is the folder holding windata/alldata.bin (and, for Integral,\n'
            '  windata/dlc/dlc_japan.bin). Pass --game <path>, or set\n'
            '  INTEGRAL_ENGLISH_GAME.')
    return found


def require_decomp(explicit=None):
    found = find_decomp(explicit)
    if not found:
        raise SystemExit(
            'Cannot find the MGS decomp checkout.\n'
            '  Looked in: the --decomp argument, $INTEGRAL_ENGLISH_DECOMP, and\n'
            '  beside this repository.\n'
            '  It is the git checkout holding source/main/main.c. Pass\n'
            '  --decomp <path>, or set INTEGRAL_ENGLISH_DECOMP.')
    return found


ROOT = _root()
WORK = os.path.join(ROOT, 'work').replace('\\', '/')
GAME = find_game()
DECOMP = find_decomp()

if __name__ == '__main__':
    print('VARIANT =', VARIANT)
    print('ROOT    =', ROOT)
    print('WORK    =', WORK, '(exists)' if os.path.isdir(WORK) else '(MISSING)')
    print('GAME    =', GAME or '(NOT FOUND - pass --game or set INTEGRAL_ENGLISH_GAME)')
    print('DECOMP  =', DECOMP or '(NOT FOUND - pass --decomp or set INTEGRAL_ENGLISH_DECOMP)')
    if not GAME:
        print()
        print('Steam libraries searched:')
        for library in _steam_libraries():
            print('   ', library)
