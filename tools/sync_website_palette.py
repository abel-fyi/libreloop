#!/usr/bin/env python3
"""Export the DAW's dark palette to the static website without a build dependency."""
import argparse
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('website', nargs='?', type=Path, default=Path.home() / 'libreloop-website')
    parser.add_argument('--check', action='store_true', help='Verify colors without changing files')
    args = parser.parse_args()
    source = (Path(__file__).resolve().parents[1] / 'src/theme.c').read_text()
    palette = source.split('static const Theme dark_theme={', 1)[1].split('};', 1)[0]
    roles = {'background': 'background', 'text': 'text', 'secondary': 'secondary',
             'accent': 'highlight', 'accent-hover': 'active_hover', 'pink': 'loop',
             'lime': 'signal', 'border': 'border'}
    colors = {}
    for role, field in roles.items():
        match = re.search(r'\.' + field + r'=\{(\d+),(\d+),(\d+),255\}', palette)
        if not match:
            raise ValueError(f'Missing opaque theme color: {field}')
        colors[role] = '#' + ''.join(f'{int(channel):02x}' for channel in match.groups())
    css = '/* Generated from LibreLoop src/theme.c by tools/sync_website_palette.py. */\n:root {\n'
    css += ''.join(f'  --{role}: {color};\n' for role, color in colors.items())
    css += '''}
body { background-color: var(--background); color: var(--text); }
.accent { color: var(--accent); }
.pink { color: var(--pink); }
a:link { color: var(--accent); }
a:visited { color: var(--pink); }
a:hover, a:active { color: var(--accent-hover); }
hr { color: var(--border); background-color: var(--border); border-color: var(--border); }
'''
    stylesheet = args.website / 'assets/theme.css'
    index = args.website / 'index.html'
    original_html = index.read_text()
    html = original_html
    def body_colors(match):
        tag = match.group()
        for attribute, role in {'bgcolor': 'background', 'text': 'text', 'link': 'accent',
                                'vlink': 'pink', 'alink': 'accent-hover'}.items():
            tag = re.sub(r'(' + attribute + r'=")#[0-9a-fA-F]{6}(?=")',
                         lambda m: m[1] + colors[role], tag)
        return tag
    def font_colors(match):
        tag = match.group()
        role = re.search(r'class="(accent|pink)"', tag)[1]
        return re.sub(r'color="#[0-9a-fA-F]{6}"', 'color="' + colors[role] + '"', tag)
    html = re.sub(r'<body\b[^>]*>', body_colors, html)
    html = re.sub(r'<font\b[^>]*class="(?:accent|pink)"[^>]*>', font_colors, html)
    html = re.sub(r'(<hr\b[^>]*color=")#[0-9a-fA-F]{6}("[^>]*>)',
                  lambda m: m[1] + colors['border'] + m[2], html)
    if args.check:
        mismatches = []
        if not stylesheet.is_file() or stylesheet.read_text() != css:
            mismatches.append(str(stylesheet))
        if html != original_html:
            mismatches.append(str(index))
        if mismatches:
            raise SystemExit('Palette mismatch: ' + ', '.join(mismatches))
        print(f'All {len(colors)} website colors and HTML fallbacks match the DAW exactly.')
    else:
        stylesheet.write_text(css)
        index.write_text(html)
        print(f'Exported {len(colors)} exact DAW colors to {stylesheet}')


if __name__ == '__main__':
    main()
