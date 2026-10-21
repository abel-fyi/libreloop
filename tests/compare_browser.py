#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

executable, root = (Path(p).resolve() for p in sys.argv[1:])
manifest = json.loads((root / 'manifest.json').read_text(encoding='utf-8'))
with tempfile.TemporaryDirectory(prefix='libreloop-browser-parity-') as directory:
    work = Path(directory)
    layout = manifest['input']
    for name in layout['directories']:
        (work / name).mkdir(parents=True, exist_ok=True)
    for name in layout['files']:
        (work / name).write_bytes(b'fixture\n')
    for name, target in layout['symlinks'].items():
        (work / name).symlink_to(target)
    subprocess.run([str(executable), str(work)], cwd=work, check=True, timeout=10)
    for name, digest in manifest['files'].items():
        expected = (root / name).read_bytes()
        assert hashlib.sha256(expected).hexdigest() == digest, f'Original browser reference changed: {name}'
        assert expected == (work / name).read_bytes(), f'Browser state differs: {name}'
print('Original tree/favorite/selection state matches exactly, including symlinks and filename quirks')
