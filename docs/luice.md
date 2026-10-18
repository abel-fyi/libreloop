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
