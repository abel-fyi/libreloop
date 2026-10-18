# Luice

LibreLoop’s GUI framework is called Luice. It consists of the shared theme,
materials, drawing interface, icons and control faces inside LibreLoop. It is
not a separate dependency or a generic widget toolkit. Layout and interaction
remain in their existing concrete modules.

## Color is a framework constraint

The studio UI enables a fixed 64-color paint palette at the draw boundary.
There are sixteen neutral grays and eight color families with six shades each.
The palette lives in `src/platform/draw_palette.h`; it cannot grow at runtime.
Free RGB values supplied by drawing code are mapped into these colors, as are
uploaded icon and knob pixels. Alpha is retained. Neutral colors use the gray
ramp; chromatic colors select a hue family before shade selection, preserving
track identities and avoiding cross-hue dithering.

Shared panels, buttons, title bars, clips and graph-area fills use flat palette
colors. Meter zones use discrete color bands while preserving continuous level
length. There are no material gradients or decorative grain overlays. Knob/icon
pixel shading remains; display backgrounds have no decorative dot texture. Do not introduce a
per-window gradient or local palette; use the shared flat surface helpers.

This is a paint/material constraint, not an indexed framebuffer. SDL still
composites transparency, texture modulation and interpolated data geometry;
those operations can create intermediate output RGB values. In particular,
translucent shadows, graph fills and antialiased edges are not guaranteed to
produce only 64 framebuffer colors. A screenshot-wide 64-color assertion would
therefore be incorrect. Strict final-output quantization remains unimplemented;
there is no CPU framebuffer readback or full-screen postfilter.

## Rendering and scaling

Flat materials use ordinary renderer fills; there is no gradient texture cache
or material generation/upload step. The knob atlas occupies 16 KiB of RGBA
storage. The graph-dot atlas and its display-density variants are removed. The
previous 8 MiB gradient cache has been removed. Audio and interaction logic are
unchanged except for the explicitly requested FM window height increase of 32 UI
units, assigned to its envelope display.

Piano rows use distinct palette grays (34 and 51), with minor grid 85 and major
grid 153. These keep the rows and grid distinct after palette mapping. Horizontal
piano row lines are opaque in the studio UI. Grid timing and snapping are
unchanged.

The legacy rendering path leaves palette mapping disabled so original-behavior
fixtures retain their rendering semantics. The CPU renderer experiment implements
the material API too, but is not adopted or visually validated by this change.

## Verification

Rendering tests assert that flat material output uses only the fixed palette
at 75%, 100%, 125%, 150% and 200% UI scale, both legacy gradient-helper directions
and fractional placement, without intermediate shading. Every palette entry
is unchanged by dithering. Piano minor lines must retain at least 1.5:1 contrast
against both mapped row colors. Flat display checks cover simulated display densities of 1×, 2× and 3×. Native
Retina appearance, mixed-DPI transitions and hardware rendering cost remain
unverified.

The following measurements describe the preceding gradient revision, not the
current flat-material implementation.

All 158 Release tests pass. The experimental CPU backend passes a C11 syntax
check only. The executable is 5,739,664 bytes, +368 bytes from the preceding
build. A single software-renderer probe of scene 12 at 1448×1086 measured 200
frames after 40 warm-up frames: render plus flush p50 4.583458 ms, p99 5.019917 ms,
maximum 5.112917 ms, aggregate render CPU 0.965459 seconds. Raw local output:
`/tmp/libreloop-luice-profile/results.log`. This is not an isolated before/after
comparison and does not establish hardware GPU cost or startup/upload latency.

## Display wells

Dark instrument displays share `ui_display_well`: a flat dark face with the
existing cached graph-dot texture inset inside its edge. Tempo, playback
position, master L/R meter wells, mixer meters and mixer gain readouts use it.
Digits, labels, level fills and peak markers are painted afterwards. Panels,
buttons and colored clips remain flat. Existing spectrum, waveform, envelope,
piano-roll and arrangement display textures remain unchanged.

This adds no texture assets, generation path, timer or audio-thread work.
Additional covered areas submit cached texture tiles; their isolated GPU cost
has not been measured. The executable is 5,739,648 bytes (+48 bytes). A software
showcase screenshot was reviewed; native Retina appearance remains unverified.
All 158 Release tests pass for the shared display-well revision.

## Paired source colors

The studio color picker offers eight swatches: blue, orange, green and purple,
each paired light/dark. Light fills use dark text, dark fills use white text.
The lighter shade is less pastel than the previous 24-color presentation. All
eight swatches are exact members of Luice's fixed paint palette, with at least
4.5:1 foreground contrast after palette mapping.

Automatic pattern creation alternates light/dark tokens. It checks the preceding
standard color so deletion or recoloring does not force two adjacent automatic
assignments into the same tone. Unused choices of the required tone are preferred;
colors repeat once the eight choices are in use. Legacy stored RGB tokens stay
unchanged and fold into the smaller presentation set; the manual picker marks
the matching visible swatch. Arbitrary custom colors retain the normal conversion.
The legacy rendering path retains its original picker and presentation.

Engine checks cover the alternating sequence across 64 patterns. Rendering checks
require all eight swatches to be distinct, remain inside the palette and select
the expected black/white foreground with sufficient contrast. A twelve-pattern
studio capture was reviewed, including the first light/dark pairs.

## Uniform display material

Numeric and meter wells call the same flat display surface helper as the
master frequency spectrum. Their background colors are identical; the display
background token also aliases the graph background.
The former near-black readout face is removed. Spectrum graticules remain
specific to graph content, and readings and meter bars remain above the surface.

A rendering regression check compares the well and spectrum material pixels
exactly at 75–200% UI scale, fractional placement and simulated 1×/2×/3× display
density. Both theme tests and the continuous meter test pass. A software showcase
screenshot was reviewed; native display appearance remains unverified.

## Full mixer scale on startup

The default mixer body is 440 logical units plus the existing adder/FX shelf,
replacing the mistaken 256-unit compact default. At the 1448×1086 reference
viewport its empty outer height is 462 units, with 244 units of fader travel.
All scale labels fit: +6, 0, −6, −12, −24, −36, −48 and −60 dB. The dBFS caption
also has more separation from the top label.

Startup expands saved compact splits to this default target instead of reopening
the cropped scale. Larger saved splits remain intact. Double-clicking the divider
restores the same target. Manual compaction is still possible during use. The
scale now prioritizes 0 dB first, followed by the main attenuation marks, adding
−6 and +6 only when space permits. This supersedes the former +6/−12/−36/−60
subset. All tick positions and gain mappings are unchanged.

Very short logical viewports still constrain mixer height to preserve a usable
workspace; not all eight labels can fit there without overlapping. The full
scale fits with no FX shelf rows at logical heights of at least 674, or with all
three FX rows at heights of at least 740. Smaller or manually compressed views
retain 0 dB ahead of less useful detail. UI scale and font size are not silently
changed to force the scale into a short viewport.

The real frontend draw-command test asserts all eight labels at the default
size and 0 dB priority when compacted. Geometry checks cover supported startup
sizes, including full meter travel at heights of 740 and above with three FX
rows. Preference checks cover expanding compact saved splits and preserving
larger saved splits. Software captures were reviewed for the full scale and
twelve patterns with light/dark pairs. All 158 Release tests pass; the final
startup preference change also passed its targeted rerun. Native display
appearance remains a hands-on check.


## Selected-track monitor and flat displays

A fixed stereo output meter sits to the left of Master, beside the full dBFS
scale. It follows the selected bus using the same level and peak-hold state as
its strip meter. Startup and project reset select Master. The fixed column is
42 logical units wider; scrolling, routing ports and strip hit areas derive
from the updated geometry. Individual strip meters remain available.

Decorative graph dots are removed from all displays, including Arrangement,
Piano Roll, envelopes, waveforms, spectra and numeric/meter wells. Real grid
lines, signal traces, palette colors and bitmap knob/icon shading remain.
There is no replacement LCD digit design in this change.

Release build and all 158 tests pass, including selection-following meter draw
checks, routing/scroll geometry and flat display checks across UI scales and
simulated display densities. A software showcase capture was reviewed. Native
Retina appearance and hands-on interaction remain unverified.

## Bitmap transport numerals

Tempo and playback position use compact 5×7 bitmap numerals instead of thin
seven-segment digits. Their faces retain the shared recessed display treatment.
The glyph rows are drawn as solid rectangle runs directly from a ten-digit
constant table: no new font, texture, dependency or decorative dot background.
Normal numerals are 14 logical units high; clock centiseconds are 10.5 units,
aligned to the same baseline. Long values scale down to fit their existing
readout bounds. Transport input areas and editing behavior are unchanged.
The legacy presentation retains its original seven-segment renderer.

Draw-command checks cover the smaller centisecond baseline, long-value bounds
and readable stroke dimensions at 75% UI scale. A software-rendered showcase
capture was reviewed. Native Retina appearance remains a hands-on check.
