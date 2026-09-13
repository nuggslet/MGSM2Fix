"""English that USA's copy of a stage overlay has and Integral's does not.

    py overlaydiff.py [--vr] [--all] [--stage NAME]

The blind spot this closes, found 2026-09-11: three VR overlays each carry a
memory-card caption module whose USA build is English and whose Integral build
is Japanese, and nothing had compared the two *per stage*. `vr_sweep.py` and
`mainsweep.py` read GCL script records, so overlay `.rodata` pools were
invisible to them; the byte inventory (`jplist.py`) did see the Japanese but
judged each *string*, and the same caption also lives in `vrtitle`, where USA
carries the identical Japanese - so every copy inherited "USA has the same
Japanese" and the two stages where USA has English were never looked at.

This tool asks the question the other way round, per stage and per overlay:
which printable ASCII strings does USA's overlay contain that Integral's does
not? Every such string is either English text the port has not carried, a
debug/format string, or a symbol name - and the last two are cheap to read
past. Both discs are read from the extracted STAGE.DIRs in work/. With --vr the
VR pair is used; --all lists every stage, otherwise stages with no candidates
are folded into one line. Debug strings (those with '%', '\\n', or no space and
no capital) are dropped unless --debug is given.
"""
import os as _os, sys as _sys, re
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import portio
from workdir import WORK

MAIN = (WORK + '/int1_stage.dir', WORK + '/usa1_stage.dir')
VR = (WORK + '/vrint_stage.dir', WORK + '/vrus_stage.dir')
ASCII = re.compile(rb'[ -~]{4,}')


def overlays(sd):
    """stage name -> concatenated overlay ('s*') payload bytes"""
    d = open(sd, 'rb').read()
    out = {}
    for name, (sector, _) in portio.entries(d).items():
        try:
            tags, pays, _ = portio.stage(d, name)
        except Exception:
            continue
        body = b''.join(pays[k] for k, t in enumerate(tags) if k in pays and t[1] == ord('s'))
        if body:
            out[name] = body
    return out


def strings(blob):
    return {m.group() for m in ASCII.finditer(blob)}


def looks_like_text(s):
    t = s.decode('latin1')
    if '%' in t or '\n' in t or '\r' in t:
        return False
    letters = sum(c.isalpha() for c in t)
    if letters < 4 or letters / len(t) < 0.6:
        return False                       # code bytes: '<`Cc$@', '$#(C'
    if any(c in t for c in '$<>#`@^{}|~\\'):
        return False
    if ' ' not in t and not any(c.isupper() for c in t):
        return False                       # identifiers: cur_lu, vr_font, savemngr.c
    if t.endswith('.c') or t.endswith('.h') or t.endswith('.bin'):
        return False
    return any(c in 'aeiouAEIOU' for c in t)


NON_ENGLISH = ('Memory Card ...', 'formateada', 'Tarjeta', 'guardar', 'cargar', 'Speicher', 'gespeichert',
               'geladen', 'Spielstand', 'Formatieren', 'fehlgeschlagen', 'Salvataggio', 'Caricamento',
               'fallito', 'Errore', 'Formattaz', 'sauvegard', 'chargement', 'Carte m', 'Formatage',
               'Sobreescribir', 'Formatear', 'Sostituisci', 'Formattare', 'Superposer', 'Formater',
               'berschreiben', 'formatieren', 'Nessun', 'Dati ', 'Datos ', 'Donn', 'Ocurri', 'Fallo ',
               'Ning', 'Kein ', 'Ladevorgang', 'Es wird', 'Daten ')


def foreign(s):
    t = s.decode('latin1')
    return any(k in t for k in NON_ENGLISH)


def ported(folder):
    """every byte string a deployed PPF writes, concatenated per record"""
    from portio import read_ppf
    import glob
    blobs = []
    for path in glob.glob(_os.path.join(folder, '*.ppf')):
        for off, data in read_ppf(path):
            blobs.append(bytes(data))
    return b'\x00'.join(blobs)


def main(argv):
    from workdir import GAME
    vr = '--vr' in argv
    show_all = '--all' in argv
    debug = '--debug' in argv
    only = argv[argv.index('--stage') + 1] if '--stage' in argv else None
    isd, usd = VR if vr else MAIN
    mods = _os.path.join(GAME, 'mods/INTEGRAL/VR-DISK' if vr else 'mods/INTEGRAL/INTEGRAL/0')
    carried = ported(mods) if _os.path.isdir(mods) else b''
    I, U = overlays(isd), overlays(usd)
    common = sorted(set(I) & set(U))
    print('%d stages in both; %d Integral-only, %d USA-only; deployed PPFs read from %s' % (
        len(common), len(set(I) - set(U)), len(set(U) - set(I)), mods))
    quiet, total = [], 0
    for name in common:
        if only and name != only:
            continue
        ui, ii = strings(U[name]), strings(I[name])
        cand = sorted(s for s in ui - ii if debug or looks_like_text(s))
        cand = [s for s in cand if not any(s in t for t in ii if t != s)]
        eng = [s for s in cand if not foreign(s)]
        done = [s for s in eng if s in carried]
        todo = [s for s in eng if s not in carried]
        if not todo:
            quiet.append('%s%s' % (name, ' (%d ported)' % len(done) if done else ''))
            continue
        total += len(todo)
        print('== %s: %d English strings USA has that neither Integral nor a deployed PPF carries%s' % (
            name, len(todo), '; %d already ported' % len(done) if done else ''))
        for s in todo[:60]:
            print('   %r' % s.decode('latin1'))
        if len(todo) > 60:
            print('   ... %d more' % (len(todo) - 60))
    if not show_all:
        print('-- nothing left: %s' % ', '.join(quiet))
    print('%d candidate strings in total' % total)


if __name__ == '__main__':
    main(_sys.argv[1:])
