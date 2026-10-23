#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the Windows installation after relocation to a Unicode path."""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile

build = Path(sys.argv[1]).resolve()
cmake = sys.argv[2]
with tempfile.TemporaryDirectory(prefix='libreloop-windows-install-') as temporary:
    root = Path(temporary)
    stage = root / 'initial install'
    subprocess.run([cmake, '--install', str(build), '--prefix', str(stage)],
                   check=True, stdout=subprocess.DEVNULL, timeout=60)
    moved = root / 'relocated \u00e9 \u97f3 install'
    stage.rename(moved)
    resources = moved / 'share/libreloop'
    for name in ('LICENSE', 'third-party.md', 'branding/logo.png',
                 'fonts/LiberationSans-Regular.ttf', 'fonts/PxPlus_IBM_VGA_8x16.ttf'):
        assert (resources / name).is_file(), name
    assert list((resources / 'samples').rglob('*.wav'))
    assert list((resources / 'presets').glob('*.llpreset'))
    working = root / 'unrelated working directory'
    working.mkdir()
    # Exclude build tool directories so missing runtime DLLs cannot be hidden.
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software',
               APPDATA=str(root / 'config'), PATH=str(Path(os.environ['SystemRoot']) / 'System32'))
    subprocess.run([str(moved / 'bin/libreloop.exe'), '--smoke'],
                   cwd=working, env=env, check=True, timeout=60)
    assert (working / 'libreloop-smoke.llp').stat().st_size > 0
    for name in ('libreloop-smoke.png', 'libreloop-view-1.png',
                 'libreloop-view-2.png', 'libreloop-view-3.png'):
        data = (working / name).read_bytes()
        assert data[:8] == b'\x89PNG\r\n\x1a\n', name
        assert struct.unpack('>II', data[16:24]) == (1448, 1086), name
print('Windows relocated Unicode installation, resources, rendering and save passed')
