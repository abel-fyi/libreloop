# Docked workspace and mixer effect slots

This is the user's approved move from startup-only tiling to persistent workspace
regions. It applies to the default/studio UI, including `--visual-preview`.
`--legacy-ui` keeps the reference floating-window behavior.

## Completed

Arrangement is the main center canvas. Mixer occupies the bottom across the
workspace; its top divider changes the space shared with the center. Collapsing
Mixer leaves an 18-unit strip that expands on click. The toolbar also restores it.
Piano Roll and Channel Rack open as center regions beneath Arrangement. Their
horizontal dividers resize the split. The center maximize button temporarily
fills that region and restores the previous split. Arrangement cannot be hidden
into an empty desktop. Divider grabs use the existing 6-unit gaps (8 horizontally
beside the inspector), with a resize cursor and small procedural grip. A second
click within .3 seconds and 4 logical units resets that split.

Instruments and effects share the right inspector. Opening a channel or a filled
effect slot explicitly switches it; merely selecting a mixer bus does not replace
the editor. Right-click a title for Float/Dock. Floating editors remain above the
docked workspace while keyboard focus follows the clicked panel. Arrangement and
Mixer stay docked. Existing editor instances are retained: this does not add
multiple independent instrument editors or multiple effect editor instances.

The reference 1448×1086 initial split is retained: Mixer 208 units, Piano 276,
Arrangement 506, instrument inspector 334 wide. Mixer and Piano proportions
scale with the available area, with minimum control sizes. Rack defaults to 168
units high; inspector width is adjustable. EQ requires 480 units; Chorus and the
instrument editor can use 330. Where a dock would make controls unusable, the
inspector floats or the selected center editor uses the center region. In very
short layouts opening Piano can collapse Mixer. Hidden regions return through
the existing toolbar/channel/effect actions. Each frame's geometry remains
concrete rectangles, not a generic docking/widget framework.

Resizing preserves timeline pixels per step through Arrangement zoom and all
Piano pattern spans. The leftmost timeline position is retained. Piano note
height continues using the fixed reference area; more height reveals more notes.
Rack step width stays unchanged. Layout changes are explicitly approved new
interaction geometry; original snap/drag formulas and regression fixtures remain.

Proportions, panel visibility and floating rectangles persist in `workspace.txt`
beside browser settings, saved atomically on normal exit. Invalid/incomplete
preferences are ignored. Smoke captures bypass user preferences. Project format,
engine/DSP, routing and plugin policy are unchanged.

The mixer now lists all ten slots vertically, with names or Empty. Click Empty to
choose the existing Chorus or Equalizer; click a filled row to open its inspector.
The indicator toggles bypass. Right-click exposes open, bypass, removal and preset
load/save. Removing an effect retains the existing cleanup of its automation.
The Chorus inspector retains rate, depth, mix, automation access, bypass and
presets. EQ retains its existing editor and spectrum. Chorus does not request the
unused EQ spectrum tap. No new effects or plugin formats were added.

## Verified

Release build and bundle signature verify. The full **128-test CTest suite**
passes, followed by targeted checks after final boundary refinements. Original
reference audio, interaction and raster fixtures remain unchanged.

New tests exercise default geometry, divider resizing/reset, non-overlap across
window sizes, collapse/open, center maximize/restore, inspector switching,
floating priority and narrow layouts. The actual frontend scale helper is checked
for constant pixels per step across reflow. Preferences round-trip through the
actual file code and reject an invalid file. Effect helpers are checked for
correct defaults, opening the inspector, and removing only the target effect's
automation. The original repository remains clean.

Actual SDL dummy/software captures are outside both repositories in
`../baseline-artifacts/docked-workspace/`: default, tall-mixer, rack, effect,
eq, float and compact. They show synthetic editable projects; normal startup does
not create these fixtures.

## Unverified and costs

Native drag/trackpad feel, physical HiDPI, audible device playback and native GPU
cost need hands-on verification. No CPU or latency improvement is claimed. The
layout math adds no animations or wakeups; the original event wait remains. New
visuals are small procedural divider grips and slot rows, with no assets/fonts or
new dependencies. Their isolated per-frame CPU/GPU cost has not been measured
reliably. The release executable measures 3,709,328 bytes, up 944 bytes from the preceding
3,708,384-byte build. This remains the installed dynamic SDL developer build,
with the previously documented SDK/deployment warning.
