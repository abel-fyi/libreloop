#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise CMake's final staged macOS bundle, including install-time signing."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

build = Path(sys.argv[1]).resolve()
cmake = sys.argv[2]
with tempfile.TemporaryDirectory(prefix="libreloop-install-") as temporary:
    root = Path(temporary)
    # A space in the destination also exercises CMake's install-script quoting.
    stage = root / "staged app"
    subprocess.run([cmake, "--install", str(build), "--prefix", str(stage)],
                   check=True, stdout=subprocess.DEVNULL, timeout=30)
    bundle = stage / "LibreLoop.app"
    subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(bundle)],
                   check=True, timeout=15)
    resources = bundle / "Contents/Resources"
    for name in ("LICENSE", "third-party.md", "branding/logo.png",
                 "fonts/LiberationSans-Regular.ttf", "fonts/LICENSE"):
        assert (resources / name).is_file(), f"Missing installed resource: {name}"
    assert list((resources / "samples").rglob("*.wav")), "No installed samples"
    assert list((resources / "presets").glob("*.llpreset")), "No installed presets"
    assert (resources / "licenses").is_dir(), "No installed dependency notices"
    executable = bundle / "Contents/MacOS/LibreLoop"
    dependencies = subprocess.check_output(["/usr/bin/otool", "-L", str(executable)], text=True)
    assert "/opt/homebrew" not in dependencies, "Installed app depends on development SDK path"
    if "libSDL3" in dependencies:
        assert "@executable_path/../Frameworks/" in dependencies
        assert list((bundle / "Contents/Frameworks").glob("libSDL3*.dylib"))
    smoke = root / "smoke"
    smoke.mkdir()
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
    env.pop("DYLD_LIBRARY_PATH", None)
    env.pop("DYLD_FRAMEWORK_PATH", None)
    subprocess.run([str(executable), "--smoke"], cwd=smoke, env=env, check=True, timeout=30)
    assert (smoke / "libreloop-smoke.llp").stat().st_size > 0
    for name in ("libreloop-smoke.png", "libreloop-view-1.png",
                 "libreloop-view-2.png", "libreloop-view-3.png"):
        assert (smoke / name).read_bytes()[:8] == b"\x89PNG\r\n\x1a\n"
print("Installed bundle signature, SDK-independent runtime path, resources and headless launch passed")
