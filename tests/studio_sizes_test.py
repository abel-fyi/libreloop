#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the actual desktop at compact and reference startup dimensions."""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile
exe = str(Path(sys.argv[1]).resolve())
env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
saved = None
with tempfile.TemporaryDirectory(prefix="libreloop-studio-sizes-") as root:
    for width, height in ((900, 562), (1200, 800), (1448, 1086)):
        directory = Path(root) / f"{width}x{height}"
        directory.mkdir()
        subprocess.run([exe, "--smoke", "--visual-preview", "--smoke-size", f"{width}x{height}"],
                       cwd=directory, env=env, check=True, timeout=30)
        for name in ("libreloop-smoke.png", "libreloop-view-1.png", "libreloop-view-2.png", "libreloop-view-3.png"):
            data = (directory / name).read_bytes()
            assert data[:8] == b"\x89PNG\r\n\x1a\n"
            assert struct.unpack(">II", data[16:24]) == (width, height)
        project = (directory / "libreloop-smoke.llp").read_bytes()
        if saved is not None:
            assert project == saved, "Startup dimensions changed the project"
        saved = project
print("Compact and reference desktops render four scenes and save byte-identical projects")
