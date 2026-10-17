> Subsequent user-directed changes: the current single-strip toolbar and updated
> workspace geometry are documented in [colorful-console-review.md](colorful-console-review.md).
> This earlier layout record remains historical; its 98-unit workspace top is superseded by 60.

# Classic studio layout — image 2 direction

The user superseded the earlier charcoal/cyan, geometry-preserving proposal:
image 2 alone is now the target, with its sober, old-school DAW appearance.
Layout and aesthetic changes are authorized; existing functionality remains.

The normal SDL application now starts with a 1448×1086 window and simultaneous
Arrangement, Piano Roll, Mixer and instrument panels, plus the existing Browser.
Editors retain their title-bar move, resize, maximize and hide operations. The
Channel Rack and Equalizer remain available through their existing actions.
`--legacy-ui` retains the original startup geometry and palette for comparison.
`--smoke` retains the original fixture geometry; `--smoke --visual-preview`
captures the new layout. No project or audio fixture was changed.

## First implementation and remaining work

The first pass supplies cool gray panels, darker inset fields, mint selection,
silver faders, fine borders and restrained control bevels. It uses the original
Liberation Sans font. The Sampler's actual sample-processing controls occupy the
right panel; the concept's nonexistent synth controls are not introduced.

The Mixer now uses a 208-unit panel and 112-unit strips. Eight strips are
visible at the reference window size; the existing wheel/scrollbar browses all
100 inserts. The Browser is 238 units and instrument column 334 units wide.
The transport now uses separate menu and control rows, larger procedural
play/stop/record controls and inset position/tempo fields. The workspace starts
at logical y=98, matching the concept. The instrument column is 334 units wide;
Sampler mode/tempo/region controls reflow at widths below 440, and FM preset
footer buttons avoid overlap at those widths. Contrast is selected per control;
dark Browser and timeline labels remain light while panel labels are dark.
The Piano now has a 166-unit left tool area, a lighter gray note grid and dark
velocity area. It exposes Pencil, Brush and Select, readouts for the existing
auto-grid and last note length, a selected-note count and access to Channel Rack.
It does not invent the concept's quantize/scale selectors or extra edit views.
Instrument groups and larger typography are now implemented. The source picker now occupies the lower Browser pane. Responsive startup sizing is implemented. Final software measurements and review are recorded in `classic-render-measurements.md`.
This is a working first pass, not a claim of visual completion.

## Geometry fixture changes

Visual capture dimensions are classic 1448×1086 versus legacy
1200×675. The visual test still requires byte-identical saved projects and four
valid, distinct rendered scenes. Interaction and audio assertions are unchanged.
The original six-window model and its constants are unchanged; the desktop's
new startup layout sets concrete rectangles and reduces the Piano's minimum
height and Sampler's minimum width. Transport hit targets move to the new
control row and increase in size; their actions and numeric sensitivities stay
unchanged. The Browser's top and row origins follow the workspace top. These are authorized layout changes.

## Verification limits

Software SDL screenshots can be inspected here. Real-display legibility,
HiDPI, pointer-device feel and GPU rendering need hands-on validation.

## Second-pass validation

The AppleClang developer-SDL release build succeeds and all 113 CTest checks
pass. Fresh dummy-driver/software-renderer captures show the actual new
transport, light panels, dark Browser, 350-unit Sampler and all four editors.
The saved-project equality check remains intact. These tests do not constitute
a real mouse/trackpad or HiDPI hands-on acceptance check.

## Piano geometry and validation

The classic Piano content rectangle translates the original controller's local
coordinates by +166 horizontally and -34 vertically, reducing its content width
by 166 and increasing virtual content height by 34. This moves tools left and
shortens the blank header while retaining the exact original 60-unit keyboard,
86-unit grid origin, grid math, hit margins and controller formulas in content
coordinates. Drawing, keyboard preview, note/velocity drags, ruler drags,
scrollbars, vertical zoom and navigation all use that same content rectangle.
The navigation boundary accepts a concrete content rectangle; the interaction
layer remains free of graphics/platform APIs.

All 114 developer-SDL tests pass. The added navigation regression compares the
translated content against the same original navigation applied directly to
that viewport, at logical scales 1 and 2, covering wheels, precise momentum,
pinch anchors, middle drag/cancel and exclusion of tools and velocity areas.
Original interaction replay and audio fixtures stay unchanged. Fresh four-view
software captures were inspected; real mouse/trackpad checks remain unverified.

## Compact Mixer geometry and validation

At 1448×1086, the initial rectangles are Arrangement {244,98,858,468},
Piano {244,572,858,276}, instrument {1110,98,334,750} and
Mixer {244,854,1200,208}. The retained 22-unit status bar accounts for the
vertical difference from the concept. Original editor drag/resize remains.

The Mixer keeps the original gain curve, drag anchoring, reset/value/automation
menus, mute/solo, pan, width, record arm and cable routing. The new strip width
is used consistently for drawing, cable hit tests and scrolling. Pan, width and
arm sit above the fader; routing sockets sit below. Effect-slot actions, Chorus
parameters, Equalizer entry, bypass and preset actions fit the compact inspector.
Colored strip headers are procedural presentation colors; saved projects are
unchanged. Dense dB tick labels are omitted when they would overlap; the tick
positions and meter/fader mappings stay exact.

All 115 developer-SDL tests pass. A new test exercises the actual frontend
routing hit test at new and legacy coordinates, checks its exclusive 12-unit
margins and scrolled bus IDs, and connects an insert through the returned target.
The saved-project equality test remains unchanged. Initial empty-effect scenes
were inspected in software captures. Occupied-effect and all instrument pages,
real input devices, HiDPI and hardware rendering still need further review.

## Instrument groups, typography and expanded capture review

The Analog Sound page shows a classic overview when its panel is at least 750
logical units tall: existing oscillator controls, filter controls, the existing
amplitude envelope and its editable graph, and the existing per-note LFO.
Amplitude, Filter and Motion tabs keep their original focused pages. Shorter
panels use the original focused Sound page. No oscillator, modulation destination,
filter drive or other unsupported concept control was introduced. Custom FM,
DX7 and Sampler pages receive flat bordered groups; synth names sit in a dark
readout, and the full channel name is visible in the editor title. Narrow-page
explanatory text wraps rather than clipping.

The overview's envelope records its parameter group when its handle is captured,
so restoring the other view state cannot redirect a drag. A regression exercises
actual capture and compares all three handle formulas against the carried legacy
implementation for amplitude and modulation envelopes. It also checks that a
restored Filter view does not redirect an amplitude handle.

Classic text under 20 units increases by two units, with a minimum of 10. The
same font and atlas are used; width measurement, fitting and drawing agree on
the size. Button captions reduce their size as needed to retain complete labels.
Legacy text metrics remain unchanged. These are authorized visual geometry/text
changes; no interaction or audio assertion was weakened.

`--smoke --smoke-scene N` is a development capture fixture (0–11), covering
Sampler, Custom FM, DX7 operator/keyboard/global pages, all four Analog pages,
occupied Chorus/Equalizer slots and the open Equalizer. It has no effect on
normal startup. The comparison test renders 48 scenes per style and requires
byte-identical project files for every scene state. All 116 developer-SDL tests
pass. Captures are in `../baseline-artifacts/classic-final-device-review/`.
The inspected scenes also exposed a dark-on-dark EQ response trace; the classic
trace and sample-playback marker now use the visible signal color.

Hardware rendering, real-display HiDPI and hands-on pointer-device behavior
remain unverified. Exact visual fidelity remains under review; this record does
not claim completion of the visual goal or a shipping/static SDL build.

## Browser and source-picker layout

The carried Patterns / Audio clips / Automation picker now occupies the lower
left pane, beginning at y=746 in the reference window. It maps the concept's
lower library area to real LibreLoop clip sources instead of inventing a new
preset catalog. Its 52-unit row pitch, 50-unit hit height, three-unit drag-preview
threshold, 0.35-second double-click interval and five-unit click tolerance remain.
Rows/thumbnails widen to 222 units; the existing new-pattern action stays below.
The upper Browser retains its tree, audition waveform, folder action and keys.
The folder action is a plus button in the classic Browser header. Picker clicks
focus Arrangement instead of the file tree; recording/capture/modal gates remain.
Audio drops onto the relocated list import without placing a clip, as before.

Arrangement now has a 148-unit grid origin, 74-unit grid top and four-unit track
header origin. At the reference size its grid begins at x=392, matching the
concept's approximately x=393. Drawing, previews, ruler/drag math, import targets
and navigation use those same dimensions. Navigation accepts concrete geometry
at its boundary and retains its original formulas and default legacy dimensions.
Track/mixer/channel mute indicators are recessed M/S squares; the original
left-click mute and right-click solo/menu behavior and 16-unit hit box remain.

All 117 developer-SDL tests pass. Navigation compares the new Arrangement geometry
against the carried logic at scales 1 and 2, including pointer-anchored pinch,
wheels, momentum and middle drag. Import replays map only their coordinates into
the new grid/library and still match the frozen original project, PCM, selection
and state files exactly for all 12 scenarios. No expected fixture was changed.
The inspected capture is `../baseline-artifacts/classic-sidebar-picker/`.

## Smaller windows and dialogs

The initial studio arrangement adapts to available logical space. The reference
1448×1086 rectangles remain exact. Below 720 units of workspace height the Mixer
starts hidden; below 506 units the Piano starts hidden. Below the width needed
for a 440-unit central editor plus the 334-unit inspector, the inspector starts
hidden. All remain available through the carried view/instrument actions.
Visible initial panes satisfy their minimum sizes, stay inside the workspace
and do not overlap. The classic minimum window is 900×562 so an opened FM editor
can still fit its carried 420-unit minimum. Legacy minimum remains 900×506.
The compact transport retains its actions and numeric sensitivities while
narrowing its readouts and shifting editor controls to fit a 900-unit window.

While the initial arrangement is untouched, window/sidebar resizing reflows it.
Moving, resizing, maximizing, hiding or opening an editor exits automatic reflow;
the original floating-window behavior then remains authoritative. Normal native
startup asks SDL to fit/center the window in the display's usable bounds; capture
runs use explicit requested dimensions. Real-display fit/centering remains
unverified here. These SDL calls add no dependency beyond the approved SDL3.

The owned file chooser, rename, number, folder, help and MIDI dialogs were
captured for review. Recessed text uses a foreground chosen for its actual
background, including selection fills and light file-list rows. Selection paint
ordering and all edit/keyboard behavior remain unchanged. Development-only
`--smoke-size WxH` and `--smoke-popup N` support reproducible review.

The 119-test developer-SDL suite passes: new layout checks cover compact,
intermediate and reference dimensions with bounds/minimum/overlap assertions;
actual desktop captures at 900×562, 1200×800 and 1448×1086 save byte-identical
projects. Existing file-chooser/text-edit replays still use unchanged frozen
expectations. Inspected captures are in
`../baseline-artifacts/classic-responsive-dialogs/`. Native rendering, HiDPI,
real mouse/trackpad use and rendering-cost measurements remain unverified.

## Final visual review and remaining verification

The full editable showcase (capture scene 12) exercises eight Arrangement tracks,
pattern notes, sample waveforms, an automation curve, polyphonic Piano notes,
colored Mixer strips and the real Analog preset/controls. It is test data only;
normal startup remains an empty project. Track indices are drawn without changing
track selection/resize regions. The capture is
`../baseline-artifacts/classic-showcase/libreloop-smoke.png` and the editable
project is in the same directory. Scene comparisons now cover 52 captures per
style and require unchanged project state for every case.

Remaining differences from image 2 are deliberate mappings to LibreLoop:
- The retained status bar shifts the lower panels upward by about 24 units.
- Real menus and transport actions replace unsupported Track/Clip/Options,
  signature/jump/diagnostic controls. Position remains bar:sixteenth.
- The Browser displays actual files and the carried clip-source picker rather
  than a fabricated preset/device catalog.
- Piano uses real tools plus auto-grid/last-length readouts; unsupported scale,
  quantize selectors and extra editor tabs are omitted. Keyboard/hit geometry
  retains the carried controller's dimensions.
- Synth controls map to the actual Custom FM, DX7 and Analog engines. The Analog
  overview uses existing parameters; engine-specific focused pages remain.
- Mixer keeps Master/selected meter at the left, existing routing sockets,
  recording input and actual Chorus/Equalizer slots at the right. Unsupported
  sends/grouping tabs are not invented.
- Named section markers, extra oscillators/LFO destinations and new effects are
  omitted because no original implementation exists.
- Existing Liberation Sans replaces the art's ambiguous condensed typography.
  Project content differs; the concept's fabricated clips/presets are not assets.

Completed implementation: the default classic layout, controls/panels, clip
library, instrument grouping, responsive startup and owned dialogs. Verified:
Release developer-SDL build, 119 tests, frozen audio/interaction fixtures,
project-state comparisons, actual software captures, same-toolchain pre-visual
binary comparison and measured software render/edge timings. Unverified:
real-display fit/HiDPI, hands-on feel, native GPU/idle/power/latency, Linux/Windows
runtime and offline/static SDL shipping. The broader rewrite's remaining gates
are still listed in `remaining-acceptance.md`; this visual record does not claim
those unrelated platform/shipping gates complete.
