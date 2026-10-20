#!/usr/bin/env python3
from fixture_compare import compare
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
executable, root, font = (Path(p).resolve() for p in sys.argv[1:])
manifest = json.loads((root / 'manifest.json').read_text())
assert hashlib.sha256(font.read_bytes()).hexdigest() == manifest['font_sha256'], 'Existing font bytes changed'
for scenario in manifest['scenarios']:
    with tempfile.TemporaryDirectory(prefix='libreloop-font-parity-') as directory:
        size = scenario['pixel_size']
        subprocess.run([str(executable), str(font), str(size)], cwd=directory, check=True, timeout=10)
        for name, digest in scenario['files'].items():
            expected = (root / str(size) / name).read_bytes()
            assert hashlib.sha256(expected).hexdigest() == digest, f'Original glyph reference changed: {size}/{name}'
            compare(expected, (Path(directory) / name).read_bytes(), 'raster' if name in ('glyphs.bin', 'atlas.bin') else 'exact', label=f'{size}/{name}')
print(f"{len(manifest['scenarios'])} original font raster sizes pass selected comparison policy")
