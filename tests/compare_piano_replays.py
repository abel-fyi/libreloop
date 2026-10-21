#!/usr/bin/env python3
import gzip
from fixture_compare import compare
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
executable, root = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
manifest = json.loads((root / 'manifest.json').read_text(encoding='utf-8'))
for scenario in manifest['scenarios']:
    with tempfile.TemporaryDirectory(prefix='libreloop-piano-replay-') as directory:
        subprocess.run([str(executable), str(scenario['mode']), *sys.argv[3:]], cwd=directory, check=True, timeout=10)
        for name, digest in scenario['files'].items():
            expected = gzip.decompress((root/str(scenario['mode'])/(name+'.gz')).read_bytes()) if scenario.get('compression',{}).get(name)=='gzip' else (root/str(scenario['mode'])/name).read_bytes()
            assert hashlib.sha256(expected).hexdigest() == digest, f'Frozen reference changed: {name}'
            actual = (Path(directory) / name).read_bytes()
            compare(expected, actual, 'text' if root.name == 'waveform' and name == 'quads.txt' else 'exact', label=f"{root.name}/{scenario['mode']}/{name}")
print(f"{len(manifest['scenarios'])} original piano gesture/selection/audition replays match within selected comparison policy")
