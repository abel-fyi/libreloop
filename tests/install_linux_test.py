#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Launch the installed Linux desktop after relocation, outside the checkout."""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile

build = Path(sys.argv[1]).resolve()
cmake = sys.argv[2]
with tempfile.TemporaryDirectory(prefix='libreloop-linux-install-') as temporary:
    root = Path(temporary)
    stage = root / 'initial install'
    subprocess.run([cmake, '--install', str(build), '--prefix', str(stage)],
                   check=True, stdout=subprocess.DEVNULL, timeout=60)
    moved = root / 'relocated install'
    stage.rename(moved)
    resources = moved / 'share/libreloop'
    for name in ('LICENSE', 'third-party.md', 'branding/logo.png',
                 'fonts/LiberationSans-Regular.ttf', 'fonts/PxPlus_IBM_VGA_8x16.ttf',
                 'fonts/LICENSE'):
        assert (resources / name).is_file(), f'Missing installed resource: {name}'
    assert list((resources / 'samples').rglob('*.wav')), 'No installed samples'
    assert list((resources / 'presets').glob('*.llpreset')), 'No installed presets'
    assert (resources / 'licenses').is_dir(), 'No dependency notices'
    working = root / 'unrelated working directory'
    working.mkdir()
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software',
               XDG_CONFIG_HOME=str(root / 'config'))
    env.pop('LD_LIBRARY_PATH', None)
    subprocess.run([str(moved / 'bin/libreloop'), '--smoke', '--visual-preview'],
                   cwd=working, env=env, check=True, timeout=60)
    assert (working / 'libreloop-smoke.llp').stat().st_size > 0
    for name in ('libreloop-smoke.png', 'libreloop-view-1.png',
                 'libreloop-view-2.png', 'libreloop-view-3.png'):
        data = (working / name).read_bytes()
        assert data[:8] == b'\x89PNG\r\n\x1a\n', name
        assert struct.unpack('>II', data[16:24]) == (1448, 1086), name
print('Linux install relocation, paths with spaces, resources, four rendered views and project save passed')
