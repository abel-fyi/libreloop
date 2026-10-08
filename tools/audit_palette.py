#!/usr/bin/env python3
"""Measure source palette contrast, saturation and perceptual color separation.

Contrast uses linear sRGB luminance. OKLab conversion follows Björn Ottosson's
public-domain reference: https://bottosson.github.io/posts/oklab/
"""
import argparse
import colorsys
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def linear(rgb):
    return tuple(c / 12.92 if c <= .04045 else ((c + .055) / 1.055) ** 2.4
                 for c in (v / 255 for v in rgb))


def luminance(rgb):
    return sum(c * w for c, w in zip(linear(rgb), (.2126, .7152, .0722)))


def contrast(a, b):
    x, y = sorted((luminance(a), luminance(b)))
    return (y + .05) / (x + .05)


def foreground(rgb):
    return max(((0, 0, 0), (255, 255, 255)), key=lambda c: contrast(c, rgb))


def oklab(rgb):
    r, g, b = linear(rgb)
    l = (.4122214708*r + .5363325363*g + .0514459929*b) ** (1/3)
    m = (.2119034982*r + .6806995451*g + .1073969566*b) ** (1/3)
    s = (.0883024619*r + .2817188376*g + .6299787005*b) ** (1/3)
    return (.2104542553*l + .793617785*m - .0040720468*s,
            1.9779984951*l - 2.428592205*m + .4505937099*s,
            .0259040371*l + .7827717662*m - .808675766*s)


def distance(a, b):
    return math.dist(oklab(a), oklab(b))


def clip_color(lightness, chroma, hue):
    """Keep perceptual lightness/hue; reduce chroma only to fit the sRGB gamut."""
    def convert(c):
        a, b = c*math.cos(math.radians(hue)), c*math.sin(math.radians(hue))
        l = (lightness + .3963377774*a + .2158037573*b) ** 3
        m = (lightness - .1055613458*a - .0638541728*b) ** 3
        s = (lightness - .0894841775*a - 1.291485548*b) ** 3
        return (4.0767416621*l - 3.3077115913*m + .2309699292*s,
                -1.2684380046*l + 2.6097574011*m - .3413193965*s,
                -.0041960863*l - .7034186147*m + 1.707614701*s)
    low, high = 0., chroma
    for _ in range(30):
        mid = (low+high)/2
        if all(0 <= v <= 1 for v in convert(mid)):
            low = mid
        else:
            high = mid
    values = convert(low)
    return tuple(round(255*(12.92*v if v <= .0031308 else 1.055*v**(1/2.4)-.055)) for v in values)


def hex_color(rgb):
    return '#' + ''.join(f'{v:02x}' for v in rgb)


def palettes():
    source = (ROOT / 'src/theme.c').read_text()
    result = {}
    for mode in ('dark',):
        block = source.split(f'static const Theme {mode}_theme={{', 1)[1].split('};', 1)[0]
        result[mode] = {name: tuple(map(int, rgb)) for name, *rgb in
                        re.findall(r'\.(\w+)=\{(\d+),(\d+),(\d+),255\}', block)}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--generate-clips', action='store_true')
    args = parser.parse_args()
    if args.generate_clips:
        for lightness, chroma in ((.68, .11), (.57, .11)):
            print(','.join('0x'+hex_color(clip_color(lightness, chroma, 300+30*i))[1:]
                           for i in range(12)) + ',')
        return
    lines = ['# Palette measurements', '',
             'Source colors, including composited directional knob hints. Text target: 4.5:1; '
             'functional indicators: 3:1. Decorative grid lines and disabled controls stay subdued.', '',
             'Contrast: [W3C](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html). '
             'Perceptual separation: [OKLab](https://bottosson.github.io/posts/oklab/). '
             'OKLab distances are design measurements, not an accessibility certification.', '']
    failures = []
    def check(name, ratio, minimum):
        if ratio < minimum:
            failures.append(f'{name}: {ratio:.2f} < {minimum}')
        return ratio
    for mode, p in palettes().items():
        lines += [f'## {mode.title()}', '', '| Color role | RGB | HSV saturation | HSV value | OKLab lightness | Minimum contrast |',
                  '| --- | --- | ---: | ---: | ---: | ---: |']
        for role in ('highlight', 'signal', 'loop', 'pan_left', 'pan_right', 'knob', 'note',
                     'meter_low', 'meter_mid', 'meter_high'):
            rgb = p[role]
            backgrounds = ('control', 'surface') if role in ('highlight', 'knob', 'pan_left', 'pan_right') else ('background',)
            ratio = min(contrast(rgb, p[b]) for b in backgrounds)
            if role == 'note':
                ratio = contrast(rgb, p['track'])
            check(f'{mode} {role}', ratio, 3)
            _, saturation, value = colorsys.rgb_to_hsv(*(v/255 for v in rgb))
            lines.append(f'| {role} | `{hex_color(rgb)}` | {100*saturation:.1f}% | {100*value:.1f}% | {100*oklab(rgb)[0]:.1f}% | {ratio:.2f}:1 |')
        text = min(contrast(p[role], p[b]) for role in ('text', 'secondary')
                   for b in ('background', 'control', 'surface', 'browser', 'title', 'title_focus'))
        check(f'{mode} labels', text, 4.5)
        selected = min(contrast(foreground(p[b]), p[b]) for b in ('highlight', 'active_hover'))
        check(f'{mode} selected labels', selected, 4.5)
        check(f'{mode} hover labels', contrast(foreground(p['hover']),p['hover']),4.5)
        lines += ['',f'Shared lavender fill contrast against controls: **{contrast(p["hover"],p["control"]):.2f}:1** (logo spiral lavender).']
        check(f'{mode} inactive knob ring',contrast(p['knob_track'],p['control']),3)
        hint = []
        for role in ('pan_left', 'pan_right', 'stereo', 'mono'):
            alpha = .35
            composite = tuple(round(a*alpha+b*(1-alpha)) for a, b in zip(p[role], p['knob_track']))
            hint.append(min(contrast(composite, p[b]) for b in ('control', 'surface')))
        check(f'{mode} directional hints', min(hint), 3)
        lamp = tuple(round(a*.48+b*.52) for a,b in zip(p['signal'],p['control']))
        check(f'{mode} idle channel lamp',min(contrast(lamp,p[b]) for b in ('control','surface','title_focus')),3)
        check(f'{mode} fader mark', contrast(p['fader'], p['fader_mark']), 4.5)
        for a, b in (('pan_left', 'pan_right'), ('stereo', 'mono'), ('meter_low', 'meter_mid'), ('meter_mid', 'meter_high')):
            delta = distance(p[a], p[b])
            if delta < .10:
                failures.append(f'{mode} {a}/{b} insufficient perceptual separation: {delta:.3f}')
            lines += ['', f'{a}/{b} OKLab separation: **{delta:.3f}**.']
        lines += ['', f'Minimum ordinary label contrast: **{text:.2f}:1**; selected/hover label contrast: '
                  f'**{selected:.2f}:1**; directional hints: **{min(hint):.2f}:1**.', '']
    # Fully covered spectrum bar colors; the EQ response has a contrasting outer stroke.
    for mode, p in palettes().items():
        saturation, value = .75, 1.
        spectrum = [tuple(int(v*255) for v in colorsys.hsv_to_rgb((260-240*i/1000)/360,saturation,value))
                    for i in range(1001)]
        ratio = min(contrast(c,p['background']) for c in spectrum)
        check(f'{mode} spectrum bars',ratio,3)
        lines += [f'{mode.title()} colored spectrum minimum contrast: **{ratio:.2f}:1**.', '']
    block = (ROOT / 'src/engine.c').read_text().split('pattern_palette[COLOR_COUNT]={', 1)[1].split('};', 1)[0]
    clips = [tuple(bytes.fromhex(v)) for v in re.findall(r'0x([0-9a-f]{6})', block)]
    clips = [tuple(int(v*255) for v in colorsys.hsv_to_rgb(h,min(s,.25),v))
             for h,s,v in (colorsys.rgb_to_hsv(*(c/255 for c in rgb)) for rgb in clips)]
    clip_contrast = min(contrast(foreground(c), c) for c in clips)
    check('clip artwork', clip_contrast, 4.5)
    adjacent = min(distance(clips[row*12+i], clips[row*12+(i+1)%12]) for row in range(2) for i in range(12))
    if adjacent < .015:
        failures.append(f'Adjacent clip colors insufficiently separated: {adjacent:.3f}')
    check('white knob pointer',contrast((255,255,255),(35,24,49)),4.5)
    lines += ['',f'White knob pointer against its violet rim: **{contrast((255,255,255),(35,24,49)):.2f}:1**.', '']
    lines += ['## Clip palette', '', f'{len(clips)} displayed colors (saturation capped at 25%); minimum artwork contrast **{clip_contrast:.2f}:1**; '
              f'minimum adjacent-hue OKLab separation **{adjacent:.3f}**.', '']
    lines += ['## Result', '', '\n'.join(failures) if failures else 'All measured targets passed.', '']
    report = '\n'.join(lines)
    if args.output:
        args.output.write_text(report)
    print(report)
    raise SystemExit(bool(failures))


if __name__ == '__main__':
    main()
