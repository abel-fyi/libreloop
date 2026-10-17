#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check the classic layout leaves the smoke project's serialized state unchanged."""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile
exe = Path(sys.argv[1]).resolve()
env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
with tempfile.TemporaryDirectory(prefix="libreloop-visual-review-") as temporary:
    root = Path(temporary)
    for scene in range(14):
        scene_root = root / str(scene)
        scene_root.mkdir()
        for mode in ("legacy", "preview"):
            directory = scene_root / mode
            directory.mkdir()
            args = [str(exe), "--smoke", "--smoke-scene", str(scene)]
            if mode == "preview":
                args.append("--visual-preview")
            subprocess.run(args, cwd=directory, env=env, check=True, timeout=30)
            for name in ("libreloop-smoke.png", "libreloop-view-1.png", "libreloop-view-2.png", "libreloop-view-3.png"):
                image = (directory / name).read_bytes()
                assert image[:8] == b"\x89PNG\r\n\x1a\n"
                assert struct.unpack(">II", image[16:24]) == ((1448, 1086) if mode == "preview" else (1200, 675))
        assert (scene_root / "legacy/libreloop-smoke.llp").read_bytes() == (scene_root / "preview/libreloop-smoke.llp").read_bytes(), "Visual preview changed project state"
        assert (scene_root / "legacy/libreloop-view-2.png").read_bytes() != (scene_root / "preview/libreloop-view-2.png").read_bytes(), "Visual preview did not change the scene"
print("Legacy and classic layouts produce 56 scenes each at their specified dimensions and byte-identical project state")
