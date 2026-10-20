#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Import approved SDL source from a local, checksum-verified archive.
No network access. Refuses to replace any existing third_party/SDL checkout.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import shutil
import tarfile
import tempfile
VERSION = '3.4.16'
EXPECTED = '7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68'
PREFIX = f'SDL3-{VERSION}'
parser=argparse.ArgumentParser()
parser.add_argument('archive',type=Path)
parser.add_argument('--destination',type=Path,default=Path(__file__).resolve().parent.parent/'third_party/SDL')
args=parser.parse_args()
archive=args.archive.resolve();destination=args.destination.resolve()
try:
    actual=hashlib.sha256(archive.read_bytes()).hexdigest()
except OSError as error:
    parser.error(f'Cannot read SDL source archive {archive}: {error}. Download SDL3-{VERSION}.tar.gz from https://github.com/libsdl-org/SDL/releases/tag/release-{VERSION}; see README.md.')
if actual!=EXPECTED:parser.error(f'Archive SHA256 mismatch: {actual}; expected {EXPECTED}')
if destination.exists():parser.error(f'Destination already exists: {destination}')
destination.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='libreloop-sdl-import-',dir=destination.parent) as temporary:
    root=Path(temporary)
    with tarfile.open(archive,'r:gz') as package:
        members=package.getmembers()
        for member in members:
            path=PurePosixPath(member.name)
            if path.is_absolute() or '..' in path.parts or not path.parts or path.parts[0]!=PREFIX:
                parser.error(f'Unsafe archive path: {member.name}')
            if not (member.isdir() or member.isfile() or member.issym() or member.islnk()):
                parser.error(f'Unsupported archive entry: {member.name}')
            if member.issym() or member.islnk():
                target=posixpath.normpath(posixpath.join(str(path.parent),member.linkname) if member.issym() else member.linkname)
                if target!=PREFIX and not target.startswith(PREFIX+'/'):
                    parser.error(f'Archive link escapes source: {member.name}')
        package.extractall(root,members=members)
    source=root/PREFIX
    if not (source/'CMakeLists.txt').is_file() or not (source/'LICENSE.txt').is_file():
        parser.error('SDL source layout/license missing')
    files={}
    for path in sorted(source.rglob('*')):
        if path.is_file():files[path.relative_to(source).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
    (source/'libreloop-source.json').write_text(json.dumps({'version':VERSION,'archive_sha256':actual,'source_url':f'https://github.com/libsdl-org/SDL/releases/download/release-{VERSION}/SDL3-{VERSION}.tar.gz','files':files},indent=2)+'\n')
    shutil.move(str(source),destination)
print(f'Imported SDL{VERSION}: {actual}; {len(files)} source files with license/provenance')
