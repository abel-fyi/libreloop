#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare SDL, build LibreLoop and install a local runnable folder."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent.parent


def run(command):
    subprocess.run([str(part) for part in command], cwd=ROOT, check=True)


def prepare_sdl(archive):
    if (ROOT / 'third_party/SDL/CMakeLists.txt').is_file():
        return
    pin = json.loads((ROOT / 'third_party/sdl-source.json').read_text(encoding='utf-8'))
    if archive is None:
        cache = ROOT / '.cache/downloads'
        cache.mkdir(parents=True, exist_ok=True)
        archive = cache / ('SDL3-' + pin['version'] + '.tar.gz')
        if not archive.exists():
            print('Downloading SDL ' + pin['version'] + ' (only needed once)...', flush=True)
            # Complete and check the download before publishing the cached file.
            # The importer independently checks the same checksum before extraction.
            with tempfile.TemporaryDirectory(dir=cache) as temporary:
                downloaded = Path(temporary) / 'SDL.tar.gz'
                request = Request(pin['source_url'], headers={'User-Agent': 'LibreLoop build helper'})
                with urlopen(request, timeout=60) as response, downloaded.open('wb') as output:
                    shutil.copyfileobj(response, output)
                if hashlib.sha256(downloaded.read_bytes()).hexdigest() != pin['archive_sha256']:
                    raise ValueError('SDL download checksum mismatch; no source files were imported.')
                downloaded.replace(archive)
    run([sys.executable, ROOT / 'tools/vendor_sdl.py', archive])


def main():
    windows = os.name == 'nt'
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=Path('build-windows' if windows else 'build'))
    parser.add_argument('--prefix', type=Path, default=Path('stage-windows' if windows else 'stage'))
    parser.add_argument('--jobs', type=int, default=2, help='parallel build/test jobs (default: 2)')
    parser.add_argument('--sdl-archive', type=Path, help='use a downloaded SDL archive without network access')
    parser.add_argument('--system-sdl', action='store_true', help='use an installed SDL3 SDK instead')
    parser.add_argument('--test', action='store_true', help='also build and run the test suite')
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be at least 1')
    for tool in ('cmake', 'ninja'):
        if not shutil.which(tool):
            parser.error(tool + ' was not found. Install the build tools listed for your OS in README.md.')
    build = ROOT / args.build_dir
    prefix = ROOT / args.prefix
    if not args.system_sdl:
        prepare_sdl(args.sdl_archive.resolve() if args.sdl_archive else None)
    print('Building LibreLoop...', flush=True)
    configure = ['cmake', '-S', ROOT, '-B', build,
                 '-DCMAKE_BUILD_TYPE=Release', '-DLIBRELOOP_BUILD_DESKTOP=ON',
                 '-DBUILD_TESTING=' + ('ON' if args.test else 'OFF'),
                 '-DLIBRELOOP_USE_SYSTEM_SDL=' + ('ON' if args.system_sdl else 'OFF')]
    # Respect a previously chosen generator when updating an existing build.
    if not (build / 'CMakeCache.txt').exists():
        configure += ['-G', 'Ninja']
    if args.test:
        configure += ['-DPython3_EXECUTABLE=' + sys.executable]
    run(configure)
    run(['cmake', '--build', build, '--config', 'Release', '--parallel', args.jobs])
    if args.test:
        run(['ctest', '--test-dir', build, '--build-config', 'Release', '--output-on-failure',
             '--parallel', args.jobs, '--timeout', '180'])
    print('Updating the local app folder (close LibreLoop first)...', flush=True)
    run(['cmake', '--install', build, '--config', 'Release', '--prefix', prefix])
    if sys.platform == 'darwin':
        app = prefix / 'LibreLoop.app'
    else:
        app = prefix / 'bin' / ('libreloop.exe' if windows else 'libreloop')
    print('\nReady: ' + str(app))


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        print('\nBuild stopped. See the error above. If the app is open, close it and try again.', file=sys.stderr)
        sys.exit(error.returncode or 1)
    except (OSError, ValueError) as error:
        print('\nBuild stopped: ' + str(error), file=sys.stderr)
        sys.exit(1)
