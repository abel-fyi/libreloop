#!/usr/bin/env python3
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

executable, root = (Path(p).resolve() for p in sys.argv[1:])
manifest = json.loads((root / 'manifest.json').read_text())
with tempfile.TemporaryDirectory(prefix='libreloop-input-parity-') as directory:
    subprocess.run([str(executable)], cwd=directory, check=True, timeout=10)
    for name, digest in manifest['files'].items():
        expected = gzip.decompress((root/(name+'.gz')).read_bytes()) if manifest.get('compression',{}).get(name)=='gzip' else (root/name).read_bytes()
        assert hashlib.sha256(expected).hexdigest() == digest, f'Original reference changed: {name}'
        assert expected == (Path(directory) / name).read_bytes(), f'Input frame behavior differs: {name}'
print('Original key/button/queue/pointer/wheel frame behavior matches exactly')
