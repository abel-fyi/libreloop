#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check portable source and configure contracts without requiring native devices."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

source = Path(sys.argv[1]).resolve()
cmake = sys.argv[2]
foreign = [str(p.relative_to(source)) for p in (source / 'src').rglob('*')
           if p.suffix.lower() in ('.m', '.mm', '.cpp', '.cc', '.cxx')]
assert not foreign, f'Application sources must be C: {foreign}'
manifest = json.loads((source / 'third_party/manifest.json').read_text())
for name, digest in manifest['files'].items():
    assert hashlib.sha256((source / name).read_bytes()).hexdigest() == digest, name
with tempfile.TemporaryDirectory(prefix='libreloop-build-contract-') as temporary:
    root = Path(temporary)
    result = subprocess.run([sys.executable, str(source / 'tools/vendor_sdl.py'),
                             str(root / 'missing-archive.tar.gz')],
                            capture_output=True, text=True, timeout=15)
    assert result.returncode == 2 and 'Traceback' not in result.stderr, result.stderr
    assert 'Cannot read SDL source archive' in result.stderr and 'Download SDL3-' in result.stderr
    missing = root / 'missing SDL source'
    missing.mkdir()
    dependency_file = (source / 'cmake/dependencies.cmake').as_posix()
    (missing / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.20)\nproject(dependency_check LANGUAGES C)\n'
        f'include("{dependency_file}")\n')
    result = subprocess.run([cmake, '-S', str(missing), '-B', str(root / 'missing-build')],
                            capture_output=True, text=True, timeout=60)
    assert result.returncode, 'Missing SDL must not silently select a core-only build'
    assert 'tools/vendor_sdl.py' in result.stdout + result.stderr, result.stdout + result.stderr
    core = root / 'core without SDKs'
    result = subprocess.run([cmake, '-S', str(source), '-B', str(core),
                             '-DLIBRELOOP_BUILD_DESKTOP=OFF', '-DBUILD_TESTING=OFF',
                             '-DCMAKE_DISABLE_FIND_PACKAGE_ALSA=TRUE',
                             '-DCMAKE_DISABLE_FIND_PACKAGE_SDL3=TRUE',
                             '-DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE'],
                            capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    targets = (core / 'CMakeFiles/TargetDirectories.txt').read_text()
    assert '/engine.dir' in targets and '/libreloop.dir' not in targets
print('C-only application, pinned headers, explicit desktop dependency errors and SDK-free core configuration passed')
