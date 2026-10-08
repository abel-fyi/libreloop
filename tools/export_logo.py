#!/usr/bin/env python3
"""Export the unchanged logo SVG for the desktop UI and application icon.

Requires rsvg-convert (librsvg) and Pillow only when regenerating committed assets.
"""
from pathlib import Path
import struct
import subprocess
import tempfile
import re
from PIL import Image


def main():
    assets = Path(__file__).resolve().parents[1] / 'assets/branding'
    chunks = []
    with tempfile.TemporaryDirectory(prefix='libreloop-logo-') as directory:
        # Translate only each export viewport; preserve the master paths and strokes.
        original = (assets / 'logo.svg').read_text()
        nested = re.sub(r'<svg\b[^>]*>', '<svg width="100" height="100" viewBox="0 0 100 100">',
                        original, count=1)
        for size, kind in ((256, b'ic08'), (512, b'ic09'), (1024, b'ic10')):
            probe = Path(directory) / 'probe.png'
            subprocess.run(['rsvg-convert', '--width', str(size), '--height', str(size),
                            '--output', str(probe), str(assets / 'logo.svg')], check=True)
            with Image.open(probe) as image:
                bounds = image.getchannel('A').getbbox()
            if bounds is None:
                raise ValueError('Logo has no visible artwork')
            dx = (size - bounds[0] - bounds[2]) * 50 / size
            dy = (size - bounds[1] - bounds[3]) * 50 / size
            centered = Path(directory) / 'centered.svg'
            centered.write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">'
                                f'<g transform="translate({dx} {dy})">{nested}</g></svg>')
            output = Path(directory) / f'{size}.png'
            subprocess.run(['rsvg-convert', '--width', str(size), '--height', str(size),
                            '--output', str(output), str(centered)], check=True)
            with Image.open(output) as image:
                box = image.getchannel('A').getbbox()
                if box is None or abs(box[0] + box[2] - size) > 1 or abs(box[1] + box[3] - size) > 1:
                    raise ValueError(f'Logo export is not centered at {size} pixels')
            png = output.read_bytes()
            if size == 256:
                logo_png = png
            chunks.append(kind + struct.pack('>I', len(png) + 8) + png)
    (assets / 'logo.png').write_bytes(logo_png)
    data = b''.join(chunks)
    (assets / 'libreloop.icns').write_bytes(b'icns' + struct.pack('>I', len(data) + 8) + data)


if __name__ == '__main__':
    main()
