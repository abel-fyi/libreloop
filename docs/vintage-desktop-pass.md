# Classic desktop visual pass — 2026-10-16

The approved direction is a convincing Windows/Motif/X11 desktop vocabulary:
neutral gray push buttons, directional raised/sunken edges, teal active titles,
coloured small object icons, and dark instrument displays. Existing editor layout,
font, button rectangles, pointer handling and DSP are retained in this pass.

## Implemented

- Shared button rendering keeps a neutral face on mouse hover. Selected buttons
  are sunken; holding the mouse shifts their caption/icon by one logical pixel.
  Menus retain the existing selection highlight. Activation timing is unchanged.
- Bevels now use two directional pairs of light/dark edges rather than a uniform
  inner outline. This applies consistently to controls, wells and panel frames.
- Neutral desktop gray replaces the previous green-tinted gray chrome. Existing
  signal colours, continuous meters, envelope fills and clip colours remain.
- Twenty original 16x16 indexed sprites replace the tinted silhouette atlas in
  studio mode. The editable pixel rows are in `src/ui/classic_icons.h` under
  GPL-3.0-only. They include a small common palette, object outlines, highlights
  and shadows. Legacy rendering/fixtures retain their original icon path.
- Sprites are uploaded into the existing atlas once at startup or a DPI change;
  nearest sampling preserves their hard edges. The atlas contains the existing
  16/20/28 logical-pixel variants. No new decoder, font or runtime dependency.

These are new LibreLoop artworks, not an imported historical icon pack. The user
reference `../icons` depicts shaded XP-era Windows icons; it remains a reference,
not a distributable asset. Chicago95 and ChiDoors were investigated but no assets
from either have been copied: individual asset provenance is not established,
and shell downloads currently fail DNS resolution. Do not describe this pass as
shipping Microsoft, Chicago95 or other third-party icons.

## Verified

Release build and the existing 129 tests passed. The additional `classic_icons`
software-renderer test checks all sprites and atlas variants at 1, 1.25, 1.5, 2
and 3x density: nonempty images, transparency, colour, and binary alpha without
smoothing. Original icon fixtures have not been changed.

Actual desktop smoke scene 12 was rendered with SDL dummy video/software and
visually inspected at 1448x1086. Capture:
`../baseline-artifacts/vintage-desktop/default/libreloop-smoke.png`.

Release executable is 3,726,128 bytes, 64 bytes above the previously recorded
3,726,064-byte executable at revision 6382963. This is total binary size, not
an isolated asset-section measurement. Source sprite storage is 5,440 bytes.
At 1x the existing atlas dimensions remain 560x64 RGBA (143,360 raw bytes,
excluding driver overhead). There is no recurring pixel generation/readback;
button bevels add three hairline submissions per bevel versus the old outline.
No animation or event-wait changes were introduced.

One 500-frame scripted pan/zoom run after 40 warmup frames, 1448x1086 software
renderer, measured rendering plus flush: p50 6.563 ms, p99 6.880 ms, max 7.230 ms.
Raw data: `../baseline-artifacts/vintage-desktop/default/render-profile.log`.
This is not an isolated per-effect cost or a before/after speed comparison.
It excludes presentation, event handling and frame pacing; physical GPU cost,
HiDPI display quality/performance and battery impact remain unverified.

The existing system SDL dylib targets newer macOS than the selected deployment
version. This pass does not resolve that packaging limitation.

## Separate pending work

The discussion about replacing the third workspace split, fixed-size Rack
steps/note previews, nine new effect slots with legacy project protection,
ghost notes, slide notes and instrument resize policy has not been implemented
by this visual change. Do not infer feature completion from this screenshot.
