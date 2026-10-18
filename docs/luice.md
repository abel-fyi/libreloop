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

Shared panel, button, title-bar and clip gradients use ordered 4×4 Bayer
selection between palette shades. Every opaque material pixel belongs to the
palette. Exact palette colors remain unchanged. Flat fills stay flat: there is
no decorative grain overlay. Graph-paper dots are a separate graph affordance.
Do not add a new per-window texture, smooth material gradient or local color
ramp; extend the shared material implementation when a real need arises.

This is a paint/material constraint, not an indexed framebuffer. SDL still
composites transparency, texture modulation and interpolated data geometry;
those operations can create intermediate output RGB values. In particular,
translucent shadows, graph fills and antialiased edges are not guaranteed to
produce only 64 framebuffer colors. A screenshot-wide 64-color assertion would
therefore be incorrect. Strict final-output quantization remains unimplemented;
there is no CPU framebuffer readback or full-screen postfilter.

## Rendering and scaling

The SDL renderer caches native-pixel gradient strips and repeats them across
the short axis. The Bayer phase follows physical target coordinates, not manual
UI zoom. Resizing regenerates affected strips; stable geometry reuses textures.
The cache holds up to 128 strips with an 8 MiB pixel-storage budget. This budget
excludes driver bookkeeping, graph/knob atlases and other app textures. Temporary
construction buffers and an upload in progress can exceed the retained budget.
Oversize strips or allocation failures fall back to a palette-mapped flat fill.
No animation or idle wakeup was added. Audio and interaction logic are unchanged.

The legacy rendering path leaves palette mapping disabled so original-behavior
fixtures retain their rendering semantics. The CPU renderer experiment implements
the material API too, but is not adopted or visually validated by this change.

## Verification

Rendering tests assert that opaque gradient output uses only the fixed palette
at 75%, 100%, 125%, 150% and 200% UI scale, both gradient directions and fractional
placement. Every palette entry is unchanged by dithering. Existing graph-density
checks remain; flat shared surfaces now assert uniformity instead of decorative
noise. Software-rendered showcase screenshots were inspected at 150% scale.
Native Retina appearance, mixed-DPI transitions and hardware rendering cost
remain unverified.

All 158 Release tests pass. The experimental CPU backend passes a C11 syntax
check only. The executable is 5,739,664 bytes, +368 bytes from the preceding
build. A single software-renderer probe of scene 12 at 1448×1086 measured 200
frames after 40 warm-up frames: render plus flush p50 4.583458 ms, p99 5.019917 ms,
maximum 5.112917 ms, aggregate render CPU 0.965459 seconds. Raw local output:
`/tmp/libreloop-luice-profile/results.log`. This is not an isolated before/after
comparison and does not establish hardware GPU cost or startup/upload latency.
