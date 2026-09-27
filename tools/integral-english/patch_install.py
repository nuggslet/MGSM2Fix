"""Install manifest-verified PPFs and companions with backups and rollback."""
import hashlib
from pathlib import Path, PurePosixPath
import os
import shutil
import tempfile


def install(package, game):
    package, game = Path(package).resolve(), Path(game).resolve()
    if not (game / 'METAL GEAR SOLID.exe').is_file() or not (game / 'windata/alldata.bin').is_file():
        raise ValueError('Choose the MGS1 Master Collection folder containing METAL GEAR SOLID.exe and windata/alldata.bin.')
    entries, seen = [], set()
    for line in (package / 'SHA256SUMS.txt').read_text(encoding='utf-8').splitlines():
        digest, name = line.split('  ', 1)
        relative = PurePosixPath(name)
        if relative.is_absolute() or '..' in relative.parts or '\\' in name or ':' in name:
            raise ValueError('Unsafe package path: ' + name)
        if not name.startswith('mods/') or not name.endswith(('.ppf', '.ppf.json')):
            continue
        if name.lower() in seen:
            raise ValueError('Duplicate package path: ' + name)
        seen.add(name.lower())
        source, target = package / name, game / name
        if not source.resolve().is_relative_to(package) or not target.resolve().is_relative_to(game):
            raise ValueError('Patch path points outside the selected folder: ' + name)
        data = source.read_bytes()
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('Patch verification failed: ' + name)
        if target.exists() and not target.is_file():
            raise ValueError('Install target is not a file: ' + str(target))
        entries.append((name, target, data))
    if not entries:
        raise ValueError('The built package has no PPF files to install.')
    backup = Path(tempfile.mkdtemp(prefix='M2FixGUI-patch-backup-', dir=game))
    changed = []
    try:
        for name, target, data in entries:
            if target.exists():
                saved = backup / name
                saved.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(target, saved)
        (backup / 'installed-files.txt').write_text('\n'.join(n for n, _, _ in entries) + '\n', encoding='utf-8')
        for name, target, data in entries:
            target.parent.mkdir(parents=True, exist_ok=True)
            fd, temporary = tempfile.mkstemp(prefix='.m2fix-install-', dir=target.parent)
            try:
                with os.fdopen(fd, 'wb') as stream:
                    stream.write(data)
                os.replace(temporary, target)
                changed.append((name, target))
            finally:
                Path(temporary).unlink(missing_ok=True)
    except Exception:
        for name, target in reversed(changed):
            saved = backup / name
            if saved.exists():
                shutil.copy2(saved, target)
            else:
                target.unlink(missing_ok=True)
        raise
    return len(entries), backup
