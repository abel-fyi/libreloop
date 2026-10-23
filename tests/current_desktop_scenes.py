#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render the current desktop across representative projects and UI scales."""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile

exe = Path(sys.argv[1]).resolve()
env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")


def capture(directory, *options):
    directory.mkdir()
    subprocess.run([str(exe), "--smoke", *options], cwd=directory, env=env,
                   check=True, timeout=30)
    for name in ("libreloop-smoke.png", "libreloop-view-1.png",
                 "libreloop-view-2.png", "libreloop-view-3.png"):
        image = (directory / name).read_bytes()
        assert image[:8] == b"\x89PNG\r\n\x1a\n", name
        assert struct.unpack(">II", image[16:24]) == (1448, 1086), name
    return (directory / "libreloop-smoke.llp").read_bytes()


with tempfile.TemporaryDirectory(prefix="libreloop-current-desktop-") as temporary:
    root = Path(temporary)
    for retired in ("--legacy-ui", "--visual-preview"):
        rejected = subprocess.run([str(exe), retired], cwd=root, env=env,
                                  capture_output=True, timeout=5)
        assert rejected.returncode != 0 and b"removed" in rejected.stderr
    for scene in range(14):
        capture(root / f"scene-{scene}", "--smoke-scene", str(scene))
    saved = capture(root / "new-project", "--smoke-new-project")
    assert b"insert_count i32 1\n1 8\n" in saved
    expanded = capture(root / "expanded-patterns", "--smoke-new-project",
                       "--smoke-patterns", "12")
    assert expanded.startswith(b"LIBRELOOP_PROJECT 2\n")
    assert b"pattern_count i32 1\n1 12\n" in expanded
    for percent in (75, 150, 200):
        directory = root / f"scale-{percent}"
        assert capture(directory, "--smoke-new-project", "--smoke-popup", "14",
                       "--smoke-ui-scale", str(percent)) == saved
        assert (directory / "libreloop-smoke.png").read_bytes() != \
               (root / "new-project/libreloop-smoke.png").read_bytes()
print("Current desktop scenes, project state and UI scales passed")
