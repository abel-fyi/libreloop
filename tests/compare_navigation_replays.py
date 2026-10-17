#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
executable, root = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
manifest = json.loads((root / 'manifest.json').read_text())
for scenario in manifest['scenarios']:
    with tempfile.TemporaryDirectory(prefix='libreloop-navigation-replay-') as directory:
        mode, scale = scenario['mode'], scenario['layout_scale']
        subprocess.run([str(executable), str(mode), str(scale)], cwd=directory, check=True, timeout=10)
        for name, digest in scenario['files'].items():
            expected = (root / f'{mode}-{scale}' / name).read_bytes()
            assert hashlib.sha256(expected).hexdigest() == digest, f'Frozen reference changed: {name}'
            assert expected == (Path(directory) / name).read_bytes(), f'Navigation mode{mode} scale{scale} differs: {name}'
print(f"{len(manifest['scenarios'])} original wheel/precise/momentum/pinch/modifier/pan replays match exactly")
