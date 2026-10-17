# Reference captions, dropdowns and control proportions

The user confirmed that Arrange/Pattern switching and the modern workspace stay.
The simultaneous Arrangement/Piano split in `../../90s.png` is an AI artifact
from an old screenshot. We match its material language and control proportions
without reinstating obsolete window behavior. This is a further fidelity
checkpoint, not a declaration of exact visual completion.

## Shared controls and typography

`ui_dropdown_arrow` paints a separately raised arrow cell inside the existing
choice rectangle. `choice_button` shares the production button input path,
reserves caption width for the arrow, and fits the existing font. The whole
original control remains clickable; there is no second arrow hit target or
new selection state machine. It is used for synth engines, FM routing, oscillator
and LFO shapes, sampler processing mode and EQ band shapes. Grid and audio-device
selectors share the same arrow treatment. Legacy controls retain their original
drawing and caption behavior. Sampler's obsolete second arrow is suppressed.

The Record lamp has a brighter red face and narrow dark perimeter instead of
fading its entire disk directly to black. Its 40-segment drawing is one batch of
360 vertices; no animation, texture generation or audio work is added.

Editor titles use 32 logical units, matching the reference's caption height.
They use a real 24px atlas from the existing Liberation Sans font, yielding
readable capital ink within that height. The Browser caption uses the same
font size. `text_fonts_large_captions` opts the production UI into cached 23–28px
atlases. Default callers retain the exact old 22px limit and fallback behavior;
the original text-cache fixture remains unchanged.

All workspace floating editors retain only Close. Its visible face is 26×26
inside the caption, and its hit area is the rightmost 32 units. Docked main
pages still have no minimize/maximize/close controls. Mixer collapse/restore
uses the same 32-unit caption height. No obsolete caption gadgets are restored.

## Geometry changes, kept separate from interaction assertions

The taller caption adds 14 units at the top of each editor's content. The pure
`windows_content_frame` helper translates its origin down 14 and subtracts 14
from its height, preserving its physical bottom edge and horizontal coordinates.
Carried controllers keep their original local 18-unit title coordinates.
Legacy windows return their original frame exactly.

The frontend consistently uses this frame for rendering and local/global
conversion: note hit testing, Pattern channel drops, Arrangement dragging and
scrollbars, timeline marker dragging, FM envelope capture, knob context menus,
Mixer fader anchors/cable targets, device popup anchors and channel-route drags.
Navigation receives an explicit Arrangement content frame, as it already did
for Piano. Its wheel, momentum, pinch and zoom-anchor formulas are unchanged.
Piano row height remains independent of window height; the caption removes
visible rows rather than changing vertical zoom.

Geometry fixture updates are limited to the translated Pattern channel frame
and the optional Arrangement content rectangle in the existing replay. Existing
assertions still compare the same state results and zoom anchors against the
carried controller. New assertions cover the caption's lower hit area, Close,
constant bottom edge, and global-to-local envelope dragging. Frozen original
fixture bytes are not changed.

New default instrument rectangles are 376×802, matching the reference's wider
Synth panel while preserving a 788-unit controller frame beneath the larger
caption. Existing floating sizes remain persisted. For saved 788-high Analog
windows, the overview retains every slider by shortening just its envelope
plot and shifting the controls below that plot by 14; footer and sliders do not
overlap. Compact windows continue to use the existing tabbed editor.

The six-op Envelope/Keyboard/Ratio row now divides the available width evenly
instead of leaving a large gap before Ratio. Radio/tab meanings and frequency
mode changes are unchanged. Synth vertical spacing and graph-to-control
proportions still need another reference comparison.

## Additional detail alignment

Piano keys now use neutral silver whites, near-black accidentals and thin lit
edges. C labels keep their fitted ink bounds and existing key geometry.
Track headers, drawing-tool buttons and row-zoom controls use the shared shaded
button face. Sampler waveforms use the same black-blue display paper as the EQ
and envelopes. Velocity-lane and meter wells use the matching dark glass.
Signal, note, waveform, grid and effect math are unchanged; spectra and meters
remain continuous and retain independent fractional amplitudes.

## Evidence and limits

Release build, all 136 CTest cases, app signature and whitespace checks pass.
The new `large_caption` check verifies the real caption font, ink fitting in a
32-unit bar, and densities 1, 1.25, 1.5, 2 and 3. The shared skin render test also
checks the arrow cell, lamp, shaded knobs, fill, handles and continuous curve.
Original audio/project/input/palette/text fixtures remain unchanged and pass.

Actual SDL software-rendered application captures and a repeatable capture
script are in `../../baseline-artifacts/90s-details/`. Captures cover Sampler,
Custom FM, six-op envelope/keyboard/global/pitch pages, Analog overview and its
other tabs, Chorus, EQ and Arrangement. Compact captures are maintained there
separately. These verify the rendered application; physical display/HiDPI,
mouse/trackpad handling and device latency remain unverified in this environment.

The release executable is 3,744,112 bytes, +208 from `83fa071` (3,743,904).
This is total binary size, not isolated code attribution. The reusable pattern
texture remains 32,768 raw bytes. Font atlases are cached on demand. Isolated
effect costs, a matched prior-revision CPU comparison and physical GPU costs
have not been measured for this checkpoint; no speed improvement is claimed.

Remaining visual work includes the Synth's vertical proportions and label
spacing, consistent popup captions and field styling, and a final active-audio
comparison across all views. The full visual goal remains active.

Follow-up: [90s-synth-proportions.md](90s-synth-proportions.md) records the six-op
spacing and label work completed after this checkpoint.
