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
pixel shading and graph-paper dots remain separate details. Do not introduce a
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
or material generation/upload step. The graph and knob atlases occupy 32 KiB of
RGBA storage, plus a graph-density variant (64 KiB at 2×, 144 KiB at 3×). The
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
against both mapped row colors. Existing graph-density checks remain. Native
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

Numeric and meter wells now call the same graph-paper surface helper as the
master frequency spectrum. Their background, dot color, density and pixel phase
are identical; the display background token also aliases the graph background.
The former near-black readout face is removed. Spectrum graticules remain
specific to graph content, and readings and meter bars remain above the surface.

A rendering regression check compares the well and spectrum material pixels
exactly at 75–200% UI scale, fractional placement and simulated 1×/2×/3× display
density. Both theme tests and the continuous meter test pass. A software showcase
screenshot was reviewed; native display appearance remains unverified.

## Compact mixer reference scale

The mixer starts with a 256-unit body plus its existing adder/FX shelf. At the
1448×1086 reference viewport this changes its empty outer height from 294 to
278 units and gives 60 units of fader/meter travel. Startup derives the split
ratio from this logical size rather than a fraction of the monitor height.
Double-clicking the divider restores the same default. Saved stock proportions
migrate to it; deliberate saved split sizes remain intact.

Compact meters prioritize +6, −12, −36 and −60 dB labels, retaining every tick
mark. At 120 or more units of travel the intermediate labels can appear under
the existing non-overlap rule. Meter response, gain mapping and level length are
unchanged. The default preserves room for these labels at supported startup
sizes, including three FX rows. At the minimum 900×562 viewport, the arrangement
therefore gives more space to the mixer; its minimum outer height while resizing
is now 122 units rather than 246. This is an intentional layout change. The
workspace geometry assertions were updated separately; note/grid behavior
assertions and frozen interaction fixtures were not changed.

Geometry checks cover startup heights 562–1200 with zero through three FX rows,
requiring at least 60 units of meter travel and non-overlapping workspace panes.
Software screenshots were reviewed at 1448×1086 and 900×562 with three effects.
Native display appearance remains a hands-on check.
All 158 Release tests pass, with a further preference test confirming stock-size
migration and preservation of a custom split.
