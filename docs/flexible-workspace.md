# Shared editor region, flexible splits and effect popups

Historical layout: the page design in [pattern-workspace.md](pattern-workspace.md)
supersedes the Rack/Piano split and inspector docking described below.

These follow-up changes supersede the separate Rack/Piano regions and shared
right effect inspector described in `docked-workspace.md`. They implement the
user's subsequent layout decisions; legacy reference behavior remains available
with `--legacy-ui`.

## Completed

Rack and Piano Roll are mutually exclusive views of one editor region. Rack is
the default at launch. Opening a channel in Piano Roll replaces Rack in exactly
the same rectangle; Back to Rack, the Rack toolbar icon, or closing Piano Roll
returns to Rack. The toolbar focuses a view rather than hiding it on a repeated
click. Existing piano view, notes, selection and zoom remain intact.

Right-click the shared region's title and choose Dock right or Dock below. The
shared view and Arrangement get a vertical or horizontal divider respectively.
The right layout reserves at least 440 logical units for the editor and 350 for
Arrangement, with a 6-unit gap; narrower windows fall back to the bottom layout
while remembering the requested orientation. Divider dragging resizes the split;
double-click resets its proportion. The saved view preferences now include the
orientation and editor width. Version-1 workspace preferences remain readable;
this UI preference change does not alter the project format.

Arrangement has no maximize control or title-double-click maximize action.
Mixer has no minimize/window chrome controls. Drag its top divider down until
its proposed height is below 100 units to snap it to an 18-unit bottom strip;
click the strip or toolbar icon to reopen it. Its expanded minimum remains 208.

Effects always open as compact, movable floating editors: Chorus 360×280 and EQ
620×380 logical units, constrained to the workspace. No effect maximize or docking
control stretches them into the inspector. Up to 16 effect editors can coexist,
with one editor per bus/slot. Clicking an open slot closes only that popup;
clicking a closed slot opens it. Clicking a popup raises it above the others.
The entire floating group stays above the tiled workspace; keyboard focus still
follows the clicked panel. Changing mixer channels leaves other effect editors
open. Titles identify the effect, mixer bus and slot. Closing/deleting an effect
or replacing the project disposes of the associated editor as appropriate.

Each effect editor retains its target bus/slot and EQ band selection; EQ drag
ownership and shape/reset context menus are tied to the originating editor.
All live Chorus/EQ controls, bypass, presets and automation still use the existing
engine parameters. Instruments remain in the right inspector, with their existing
optional Float action. This adds effect editor instances, not duplicate instrument
editors or new effects/plugin formats.

The existing audio backend supplies one optional EQ spectrum bus. The frontmost
open EQ selects that bus; other EQs on the same bus can display it, while EQs on
other buses show their response curve and indicate that the live spectrum follows
the selected EQ. No spectrum is displayed against the wrong bus, and audio-thread
capture architecture has not been expanded. Effect popup instances are not restored
across launches, since they refer to the currently loaded project.

## Custom Rack fill

Right-click a Rack channel and choose Fill every X steps. The existing number
dialog accepts a whole-step interval from 1 to 1,000,000 (for example 2, 3, 4 or
8). Apply adds default C4 step hits at 0, X, 2X … before the current pattern end.
Existing notes, including their velocity/length, remain intact; matching C4 hits
are not duplicated. The operation does not extend the pattern. It checkpoints
once and supports normal undo/redo. Capacity is checked before mutation; if the
entire fill cannot fit the existing 128-note channel limit, nothing is changed.
Cancel does nothing; Reset restores the interval field to 4 without applying it.

## Verified

The release build and bundle signature verify. All **129 CTest checks pass**.
Existing original audio, interaction and raster fixtures remain unchanged.
Policy tests were deliberately updated for the newly requested shared region and
floating effect behavior, with new assertions for identical Rack/Piano bounds,
right-side geometry, popup target identities, three simultaneous effect editors,
z-order and closing one while preserving the others. Tests also cover collapse
snap, preference round-trip, timeline scale retention, custom fill spacing,
preservation of existing notes, atomic capacity failure and grouped undo/redo.

Actual SDL dummy/software captures live outside both repositories under
`../baseline-artifacts/flexible-workspace/`: default, right-rack, right-piano,
stacked, fill and compact. The stacked case contains two Choruses and one EQ.
The original repository remains clean.

## Unverified and costs

Native mouse/trackpad feel, physical HiDPI, native GPU cost and audible playback
still need hands-on verification. Rendering cost naturally depends on how many
editors are open; isolated CPU/GPU cost has not been measured reliably, and no
CPU/latency improvement is claimed. The original blocking idle event wait remains.
No new bitmap assets, font, library or dependency were introduced. The release executable measures 3,726,064 bytes (+16,736 from the preceding
3,709,328-byte build). This remains the installed dynamic SDL
macOS developer build with the documented deployment warning.
