"""Read user-supplied single-track Redump BINs, including ZIP archives."""
from contextlib import contextmanager
from pathlib import Path
import shutil
import tempfile
import zipfile


@contextmanager
def raw_input(path):
    path = Path(path)
    if path.suffix.lower() != '.zip':
        yield path
        return
    with zipfile.ZipFile(path) as archive, tempfile.TemporaryDirectory(prefix='m2fix-disc-') as temp:
        bins = [i for i in archive.infolist() if not i.is_dir() and i.filename.lower().endswith('.bin')]
        if len(bins) != 1:
            raise ValueError('Expected exactly one BIN in ' + str(path))
        target = Path(temp) / 'disc.bin'
        print('Unpacking ' + path.name + '…', flush=True)
        with archive.open(bins[0]) as source, target.open('xb') as output:
            shutil.copyfileobj(source, output, 4 * 1024 * 1024)
        yield target
