# 90s.png skin implementation and fidelity checklist

This records the initial `83fa071` skin checkpoint. For the subsequent caption,
dropdown and content-geometry work, see [90s-caption-controls.md](90s-caption-controls.md).
The user has since explicitly confirmed keeping modern Arrange/Pattern page
switching: the image's simultaneous Arrangement/Piano split came from an old
screenshot and is not a layout requirement.

Reference: `../../90s.png`, 1448×1086, SHA-256
`1e91e630fe631edaf70389d750783384bb23f8da46e004e2500864e9872eef16`.
This supersedes the earlier neutral Windows system-colour finish. The target is
the actual image, including its small material details, with continuous signal
displays replacing the image's segmented LEDs. This document does **not** certify
an exact visual match. The remaining differences below are part of the goal.

## Shared drawing vocabulary

`src/ui/theme.h` / `theme.c` own the concrete skin primitives. These are drawing
functions accepting existing logical rectangles/points; they own no input,
focus, parameter, layout or window state. They use LibreLoop's draw interface,
with no SDL type in their public interface. No generic widget tree or event
framework is introduced.

| Primitive | Finish | Current users |
| --- | --- | --- |
| `ui_panel` | steel vertical gradient, faint ordered grain | editor bodies, instrument groups, toolbar, Browser, channel sidebar, Mixer strips |
| `ui_button_face` | shaded face, directional two-edge bevel, recessed selected face | push buttons, floating close buttons, sliders, scroll thumbs, faders |
| `ui_titlebar` | cobalt gradient, pale top edge, dark lower edge | editors, Browser, titled menus |
| `ui_knob_face` | shaded metal face, dark perimeter, black radial pointer; cyan during capture/hover | production knobs in every editor and transport |
| `ui_lamp` | shaded red glass | Record button |
| `ui_graph_paper` / `ui_graph_dots` | black-blue glass, fine cyan dot lattice | envelopes, LFO, master analyzer, EQ |
| `ui_graph_area` | continuous blue fill fading toward its baseline | FM/Analog/DX7 envelopes and LFO fills |
| `ui_graph_handle` | light square, dark outline, highlighted edge | envelope points |
| `ui_level_fill` | existing continuous green/yellow/orange/red ramp | transport and Mixer levels |

`DesktopSkin ui_skin` holds graph/display colours separately from `Theme`.
The legacy `Theme` structure and original palette bytes are preserved for the
frozen parity fixtures. Production typography reuses the existing font, with
slightly larger small captions; no new font, icon pack or dependency is added.
Existing original GPL pixel icons remain in use. The AI reference's unclear
toolbar glyphs are not interpreted as new controls or features.

Reference pixel sampling through OS ImageIO/CoreGraphics found blank Synth body
pixels around RGB (94,98,103), Browser-region mean (49,53,60), caption-region mean
(37,59,122), Arrangement grid mean (4,31,37), and Piano grid mean (81,136,138).
These guide the shared palette; averages include the image's texture and are
not flat-colour specifications. Sampling script lives alongside the captures
at `../../baseline-artifacts/90s-reference/reference-colors.swift`.

The darker reference body with black text has lower contrast than the prior
light system theme. Palette tests explicitly check body contrast ≥3:1 and
control-face / numeric-display contrast ≥4.5:1. This is an observed property of
the requested image, not a claim that every small caption meets WCAG AA.

## Accuracy and interaction boundaries

The existing 96 FFT bins, independent fractional bin heights, frequency mapping,
meter scaling, peak hold, response computation and parameter values are retained.
Dot paper is background decoration. Envelope dots are drawn after the fill and
before repainting the real contour/handles; the envelope's coordinates and
capture formulas are unchanged. No horizontal LED gaps, amplitude buckets,
frequency grouping, invented idle activity or animation timers are introduced.
EQ shares the black-blue paper and cyan response treatment, with its original
continuous spectrum and response underneath/above the grid as appropriate.

Original note, clip, grid, knob, slider and drag hit rectangles remain unchanged
in this pass. Knob angles retain the original parameter mappings. Existing saved
pattern/clip colours are retained; Mixer strip identity colours are brighter,
as in the reference. Arrangement / Pattern remain the recently approved pages,
with the docked Mixer and floating instrument/effect editors.

## Resource and cost accounting

One 128×64 RGBA texture (32,768 raw bytes) contains two reusable 64×64 tiles:
two-tone ordered face grain and one cyan dot every four logical units. It is
generated once after creating the renderer, nearest filtered, and explicitly
destroyed before renderer teardown. No per-frame bitmap generation, readback,
allocation or file access is added. Tiles follow logical UI scale; fractional
physical-density patterns are checked by software rendering.

Each metal knob submits 360 vertices in one geometry call plus one pointer line;
the old production ring/arc drawing is skipped. A lamp submits 120 vertices.
Panels/buttons use two gradient triangles plus the clipped pattern tiles;
envelope fill segments use two interpolated triangles. No blur, glow, animated
decoration, new audio-thread work or changes to blocking idle waits are added.

The release executable at this checkpoint is 3,743,904 bytes, +656 bytes against
the preceding f4dfb92 executable (3,743,248). This is a total-binary comparison,
not an attribution of linker sections to individual effects.

A 500-frame software-renderer run at 1448×1086, scene 12, scripted pan/zoom and
40 warmup frames measured rendering plus flush: p50 14.147 ms, p99 14.575 ms,
maximum 14.936 ms, 7.121464 process CPU seconds over measured frames. Raw output:
`../../baseline-artifacts/90s-reference/render-profile.log`. This is not an
isolated effect cost or a matched comparison against the prior revision. It
excludes presentation, input, frame pacing and audio latency. Physical GPU,
battery and native HiDPI costs remain unverified.

## Verification

Release build succeeds. All 135 CTest cases passed after integration, including
the unchanged original audio, project, palette, controls and interaction
fixtures. The skin rendering checks exercise 1, 1.25, 1.5, 2 and 3x logical
scales: gradient direction, raised/recessed edge polarity, blue title colour,
dot/background separation, continuous contour over the dots, shaded knob faces,
fill gradients, white handles and red lamp. Existing continuous meter-colour
and per-bin spectrum assertions are retained.

The draw-disabled FM graph test now supplies the shared skin fill token and
stubs the new cosmetic helpers. It still asserts eight fill triangles, nine
guide lines, all operators' hit capture and identical drag results. No frozen
fixture bytes or behavioral assertions are changed.

Actual application software captures were inspected under
`../../baseline-artifacts/90s-reference/`: `dots`, `compact`, `arrangement`,
`equalizer`, and `analog`. These verify rendered appearance, not native display
or pointer-device behavior. The original repository remains untouched.

## Still required for the full visual goal

- Recheck against `90s.png` at each material/layout pass, rather than against
  the preceding implementation alone.
- Match taller reference title bars, border spacing, toolbar grouping and
  caption hierarchy. Current geometry is inherited from the earlier layout.
- Refine Synth proportions: the current tall floating six-op editor gives its
  envelope far more vertical space than the reference. Knob/label spacing and
  graph-to-control proportions still differ.
- Give existing dropdowns the reference's separately bevelled arrow cells,
  without adding controls for hallucinated functionality.
- Audit every view and popup for leftover flat fills, old display colours,
  mismatched border styles, and clipped or poorly sized captions.
- Review active audio displays and all Synth engines at compact and large sizes;
  make a matched performance comparison before claiming the decoration is cheap
  or faster. Exact visual parity and native HiDPI appearance remain unproven.
- Resolve the reference's simultaneous Arrangement/Piano layout against the
  user's later approved Arrangement/Pattern page policy before changing it.

Earlier nine-slot Mixer and glide-note requests remain separate pending work;
this skin pass does not implement or silently discard them.
