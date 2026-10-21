#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise offline preparation and verified download reuse without a network."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from unittest import mock

SOURCE = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('build_helper', SOURCE / 'tools/build.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)


class PreparationTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='libreloop-build-helper-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name) / 'checkout with spaces \u00e9'
        (self.root / 'tools').mkdir(parents=True)
        (self.root / 'third_party').mkdir()
        shutil.copyfile(SOURCE / 'tools/vendor_sdl.py', self.root / 'tools/vendor_sdl.py')
        self.archive = self.root / 'SDL.tar.gz'
        with tarfile.open(self.archive, 'w:gz') as package:
            for name, data in [('CMakeLists.txt', b'# source fixture\n'), ('LICENSE.txt', b'fixture license\n')]:
                item = tarfile.TarInfo('SDL3-3.4.16/' + name)
                item.size = len(data)
                package.addfile(item, io.BytesIO(data))
        self.pin = dict(version='3.4.16', source_url='https://example.invalid/SDL.tar.gz',
                        archive_sha256=hashlib.sha256(self.archive.read_bytes()).hexdigest())
        self.write_pin()
        patched_root = mock.patch.object(helper, 'ROOT', self.root)
        patched_root.start()
        self.addCleanup(patched_root.stop)

    def write_pin(self):
        (self.root / 'third_party/sdl-source.json').write_text(json.dumps(self.pin), encoding='utf-8')

    def test_offline_archive(self):
        with mock.patch.object(helper, 'urlopen', side_effect=AssertionError('Offline preparation used network')):
            helper.prepare_sdl(self.archive)
        source = self.root / 'third_party/SDL'
        provenance = json.loads((source / 'libreloop-source.json').read_text(encoding='utf-8'))
        self.assertEqual(provenance['archive_sha256'], self.pin['archive_sha256'])
        self.assertEqual((source / 'LICENSE.txt').read_bytes(), b'fixture license\n')

    def test_bad_local_archive_is_not_imported(self):
        self.archive.write_bytes(b'corrupt download')
        with self.assertRaises(subprocess.CalledProcessError):
            helper.prepare_sdl(self.archive)
        self.assertFalse((self.root / 'third_party/SDL').exists())

    def test_bad_download_is_not_cached(self):
        with mock.patch.object(helper, 'urlopen', return_value=io.BytesIO(b'corrupt download')):
            with self.assertRaisesRegex(ValueError, 'checksum mismatch'):
                helper.prepare_sdl(None)
        self.assertEqual(list((self.root / '.cache/downloads').iterdir()), [])
        self.assertFalse((self.root / 'third_party/SDL').exists())

    def test_download_then_offline_rebuild(self):
        with mock.patch.object(helper, 'urlopen', return_value=io.BytesIO(self.archive.read_bytes())):
            helper.prepare_sdl(None)
        cached = self.root / '.cache/downloads/SDL3-3.4.16.tar.gz'
        self.assertEqual(cached.read_bytes(), self.archive.read_bytes())
        with mock.patch.object(helper, 'urlopen', side_effect=AssertionError('Rebuild downloaded again')):
            helper.prepare_sdl(None)


if __name__ == '__main__':
    unittest.main()
