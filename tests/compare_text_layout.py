#!/usr/bin/env python3
"""Compare geometry/metrics/bitmap against the pinned original, without GL."""
from fixture_compare import compare
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

executable, root, font = (Path(p).resolve() for p in sys.argv[1:])
manifest = json.loads((root / 'manifest.json').read_text())
assert hashlib.sha256(font.read_bytes()).hexdigest() == manifest['font_sha256']
with tempfile.TemporaryDirectory(prefix='libreloop-text-layout-') as directory:
    subprocess.run([str(executable), str(font)], cwd=directory, check=True, timeout=10)
    for name, digest in manifest['files'].items():
        expected = (root / name).read_bytes()
        assert hashlib.sha256(expected).hexdigest() == digest, f'Original reference changed: {name}'
        compare(expected, (Path(directory) / name).read_bytes(), 'text' if name == 'layout.txt' else 'exact', label=name)
print(manifest.get('description', 'Original bitmap fallback and 160 text layout cases match within selected comparison policy'))
