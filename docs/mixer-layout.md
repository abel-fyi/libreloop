# Mixer layout and scrolling

The studio Mixer keeps Master fixed immediately beside the dBFS scale. Inserts
occupy a separate clipped viewport with a floating-point horizontal offset.
Strips have a fixed 112 logical-unit pitch (108 visible plus a 4-unit gap), so
scrolling reveals partial strips rather than replacing whole columns. There is
no new scroll animation or timer; incoming trackpad momentum is applied as
received. The existing event-waiting policy is unchanged.

FX slots sit above each strip. The FX button hides/restores the shelf. Depending
on Mixer height, the shelf displays one to three 22-unit rows; wheel/vertical
trackpad motion over a strip’s shelf reveals its other rows. Each strip,
including Master, keeps its own scroll offset; the global row arrows are gone.
An unused strip shows one unlabeled blank slot. Adding an effect exposes one
more blank slot after its last effect, up to the normal nine-slot limit. Gaps
inside existing chains remain in place. A used,
automated or MIDI-bound legacy tenth slot remains accessible on its bus.
Existing effect editors still float, stack and toggle independently. The former
selected-strip FX sidebar is absent from the studio Mixer.

M and S are separate Mute and Solo buttons, shared with Arrangement track
headers. Solo retains the existing additive solo behavior and clears that
channel's mute when activated. Master S clears all insert, instrument and
Arrangement solos; it never sets a solo bit on Master. Right-click M retains
mute automation. Stereo width, including mono, remains on the width knob.
Right-click an insert name to rename it; right-click the numbered left edge
for channel actions. Fader mapping, drag anchoring, routing validation, meters,
record arming and effect processing reuse the existing operations.

## Input rules

`src/interaction/mixer_view.h` owns viewport geometry, routing hit tests,
scrollbar mapping, visibility/reveal and scroll-region rules without graphics
or platform dependencies. The frontend converts existing normalized navigation
motion into that interface and renders each strip using a translated camera.
Controls and text share that translation; clipped portions cannot receive input.

- Horizontal trackpad motion scrolls inserts. Precise input retains fractional
  deltas, scaled into logical units by the existing navigation function.
- Shift+wheel scrolls horizontally; ordinary wheel uses the existing 48-unit
  navigation step. Vertical wheel over the faders or strip headers also scrolls
  inserts. Vertical motion over knobs does not pan the viewport.
- Vertical motion over an FX shelf scrolls only that strip’s FX rows. Horizontal motion can pan
  inserts at the same time. Scrolling over Master never pans inserts.
- Dragging the horizontal scrollbar preserves the pointer's grab position in
  the thumb. Clicking the rail centers the thumb there. Offset clamps to the
  content bounds on resize. Assigning an instrument route reveals its insert.

## Project compatibility

New studio projects have 99 inserts plus Master. The engine capacity remains
100, and loaded projects retain their recorded insert count. No file-format
field or DSP code changed. The legacy renderer and original fixture defaults
remain available for regression comparisons. Legacy slot ten is neither erased
nor migrated.

## Geometry fixture changes

At 1448x1086 the default docked Mixer height changes from 276 to 360 and the
Arrangement/Pattern page from 720 to 636. The split divider remains six units.
The minimum Mixer height is 226; the page retains at least 246 units at the
supported minimum window size. Saved user split proportions still apply within
those limits. Floating instrument geometry does not follow Mixer resizing.

`studio_workspace_test` updates only those explicit height expectations;
`studio_mixer_test` now checks the fixed Master and fractional-offset routing
coordinates, retaining its legacy assertions. `mixer_slots_test` replaces the
former dynamic-width assertions with fixed-pitch viewport checks and retains
legacy tenth-slot tests. `visual_preview_state` explicitly supplies 100 inserts
when comparing the two renderers' byte-identical saved state, then separately
checks that the new studio default is 99. Audio and original interaction
reference fixtures are unchanged.

## Validation and limits

`mixer_view_test` exercises precise fractional/momentum sequences, DPI scaling,
scroll-region/modifier rules, fixed Master, partial-strip routing, thumb grab
mapping, endpoint/reveal clamping, resize bounds and insert 100. FX tests cover
legacy slot visibility from content, automation and MIDI bindings, progressive
slot growth, preserved gaps and independent strip scroll targeting. The complete regression
suite also covers the original additive mute/solo and audio behavior.

Software/dummy SDL captures were inspected at 1448x1086 and 900x562, including
fractional offsets, the last insert, hidden FX and playing meters. Local capture
artifacts live outside this repository in `../baseline-artifacts/mixer-layout/`.
These checks do not establish native macOS trackpad feel, HiDPI display quality,
physical MIDI/audio device behavior, or CPU/GPU performance. Those require
hands-on validation; no performance improvement is claimed.

Release build and strict bundle signature verification passed. All 142 tests
passed; the four Mixer/workspace tests also passed after the final cable-port
clipping check. Native display and device checks remain unverified.

FX shelf refinement: blank slots have no “Empty” label, and only existing
effects plus a trailing add slot are drawn. Per-strip scroll offsets remain
UI state, reset on project replacement, and do not alter the project format.
Local review capture: `../baseline-artifacts/mixer-fx-growth/`.
