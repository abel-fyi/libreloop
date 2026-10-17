# Resizable velocity lane and desktop controls

The studio piano roll has an eight-unit divider just above the velocity lane.
Drag it vertically with the left mouse button. The pointer changes to the
vertical resize cursor; a dotted grip identifies the divider. The lane is at
least 40 logical units high and leaves at least 96 units of note-grid height
at supported window sizes. Its default is the original 62 units. Resizing
changes visible note count while retaining note row height, zoom and pan.
Velocity painting/ramp coordinates and the displayed stems use the same scale;
62 units still reproduces the original .48-unit-per-velocity mapping exactly.
Release ends the gesture even outside the divider; Escape, focus loss, hiding
the piano roll or opening a modal cancels it. The size is session-local.

Click the playback readout to switch bars/steps to seconds.milliseconds and
back. Time derives directly from the audio sample position, using integer
seconds and remainder milliseconds. Count-in still temporarily shows steps.
No transport or audio behavior changes. The view choice is session-local.

Dropdown/action buttons now retain readable labels alongside their colored
icons when space permits; compact symbolic controls remain icon buttons.
Sampler switches and mixer record-arm keys use a shared square, raised/sunken
button face with an illuminated inset, matching the other desktop controls.
Their hit areas and toggle semantics are retained. Hits/Fill controls were
widened to retain their labels with the common text size.

The top spectrum/stereo display no longer shows numeric frequency/dB legends.
All studio spectra, including EQ, now use one muted green translucent filled
trace and a fine outline. This supersedes the earlier rainbow-bin styling.
Every one of the 96 bin centers retains its exact height; the fill connects
those samples without new temporal averaging, peak quantization or rebinning.
The outline is piecewise linear, not a spline that overshoots the data.
Existing FFT/decay/dB mappings and continuous level meters are unchanged.
A single triangle batch fills the plot; no new textures or animation timers.
No reliable per-frame performance measurement was taken for this change.

The user selected PxPlus IBM VGA 8x16 as the next font. The publisher distributes
its font pack under CC BY-SA 4.0:
https://int10h.org/oldschool-pc-fonts/readme/ . This is an approved direction,
not a completed import: shell downloads of the official archive and GitHub
fail DNS resolution, and no local copy was found. DotGothic16 (OFL 1.1) remains
installed until the actual selected font can be obtained, verified and bundled
with attribution and its complete license. No proprietary Windows assets
were added. LibreLoop code remains GPL-3.0-only; third-party assets retain
their documented open licenses.

Regression coverage: drag/release/cancel and window bounds; velocity-value
round trips across lane sizes; unchanged vertical note zoom; sample-accurate
milliseconds at 44.1/48 kHz and very large frame counts; all 96 actual rendered
spectrum vertices against the original bin heights. The signal test now
inspects the fill's vertices instead of the retired rectangular bars, retaining
the same independent-height assertions. Original legacy fixtures are unchanged.

FM studio editors no longer show Play C4/Release C4. Their footer contains
Factory, Load and Save; the spacious footer distributes those three actions
across its width. Laptop/MIDI keyboard audition remains available.

Verified: Release SDL validation build, all 140 CTest cases, strict deep app
codesign verification, software-rendered expanded velocity lane/seconds view,
and live-playback EQ/master traces. Original repository remains clean.
Executable size: 3,761,520 bytes. No native mouse/trackpad or native HiDPI
hands-on check was performed; selected PxPlus font remains unavailable.
Captures are under `../baseline-artifacts/velocity-lane/`.

## Collapsing, shared grips, and velocity shading

Follow-up requested by the user: drag the velocity divider down until its
requested lane height is below 20 logical units to collapse it. The eight-unit
separator stays visible and interactive. Click its five-dot grip/rail to
restore the size from before the collapse gesture, or drag upward to reopen
and resize it. A three-unit movement threshold separates clicking a collapsed
rail from dragging. Escape/focus loss restores both the pre-drag size and
collapsed state. Remembered size is independent of pointer-event coalescing.
The expanded minimum remains 40, with at least 96 units of note grid; collapsing
exposes more notes at the same vertical zoom. Hidden lanes draw no velocity
bars and accept no velocity editing. This is session-local UI state.

`ui_divider_grip` in `src/ui/theme.c` draws exactly five 2x2 logical-unit dots
at four-unit spacing, centered horizontally or vertically in the given rail.
Velocity, Mixer (expanded and collapsed), and Browser all call this one
routine. Callers retain their existing drag/collapse state and hit geometry;
no generic widget/event framework was added. This shared-skin approach is the
rule for subsequent controls: reuse concrete drawing and interaction helpers
instead of adding separate copies of the same visual behavior.

Studio notes and their velocity stems share `ui_note_velocity_color`. Brightness
increases continuously from velocity 1 to 127, with no velocity buckets. Selected
notes retain their selection hue and stronger minimum brightness, plus the
existing selection outline. Ghost notes and all note/audio values are unchanged.
Legacy parity rendering retains its original colors.

Additional regression checks cover collapse/restore/cancel, remembered size
under different input event frequencies, monotonic color progression across all
127 velocities, and the same five separate dots in both orientations at
1x, 1.25x, 1.5x, 2x and 3x software raster densities. Captures are under
`../baseline-artifacts/shared-grips/`. Hands-on mouse/native HiDPI checks remain
unverified. No performance or CPU saving claim is made; grips require five
rectangle draws each and no textures, timers, or extra dependencies.

Validation for this follow-up: rebuilt Release SDL app; all 140 tests passed,
including original fixtures and the new collapse/grip/color cases. Strict
app signature verification and diff checks passed. Executable size is
3,778,064 bytes; this is a measured total, not a per-feature cost estimate.
Original reference repository remains unmodified.
