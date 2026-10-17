The logo is the original two-path mark from the LibreLoop website. Its paths,
colors, stroke widths and round caps are preserved in `logo.svg`.

The website uses SVG directly. The desktop loads the transparent 256-pixel
`logo.png` for the native window and Linux launcher icon. `libreloop.icns` contains 256-, 512- and
1024-pixel PNG representations for the macOS application bundle.

To regenerate the raster assets from the master SVG, install librsvg's
`rsvg-convert` and Python Pillow, then run `python3 tools/export_logo.py` from the repository root.
The exporter measures the visible alpha bounds and centers the artwork in the
square PNG canvas using a temporary translated SVG viewport. The master SVG
is not edited. The toolbar has no logo. Normal builds and runtime do not require an SVG renderer or Pillow.

The DAW's curated colors in `src/theme.c` are also the website palette. Run
`python3 tools/sync_website_palette.py ~/libreloop-website` after changing them
to regenerate the website's `assets/theme.css`. It exports the exact dark
background, text, selection/hover, pink, lime and border colors without adding
a website or DAW build dependency. The logo artwork retains its original colors.

Run `python3 tools/audit_palette.py --output docs/palette-measurements.md` to
measure text/indicator contrast, HSV saturation and OKLab separation. It includes
the composited pan/width rings, not just their unblended palette entries. Clip
colors are generated with `--generate-clips` at matched OKLCH lightness, reducing
chroma per hue only where necessary to stay inside the sRGB gamut.

Verify website/DAW color equality without editing either repository with
`python3 tools/sync_website_palette.py --check ~/libreloop-website`.
The spiral lavender `#b39add`, original logo colors and violet-black background
`#09070f` are kept exact. Supporting colors use simple values where the measured
visual difference and contrast permit it; a shorter hex spelling is not itself
an improvement in appearance.
