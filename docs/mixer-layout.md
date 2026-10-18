# Mixer layout and scrolling

Master stays fixed beside the dBFS scale. Inserts occupy a clipped viewport
with a floating-point horizontal offset and fixed 112-unit pitch (108 visible
plus a four-unit gap). Scrolling reveals partial strips. Precise input and
native momentum retain their fractional deltas; no new animation timer keeps
the app awake. Existing fader mapping, drag anchoring, meters and record arming
remain in use.

## FX stacks

Each chain has a fixed, icon-only add button directly above its effects. Empty
chains show only that button. Adding an effect puts it immediately beneath the
button, replacing the button's previous position as the button moves upward.
Existing effects follow it in top-to-bottom processing order. The new effect
is revealed at scroll position zero. Drag effects to change their order.

The docked Mixer grows upward by 22 logical units for each occupied row until
three effect rows are visible. The full stack is four blocks: one fixed add
button and three independently scrollable effect rows. Further effects do not
grow the Mixer. Removing effects shrinks it again. The shelf follows the
longest chain, while each shorter chain's add button/effects sit together
against its strip header. There is no blank space between them or placeholder
row at the bottom. Empty processing slots are omitted from the displayed list.
The existing FX toggle hides/restores the shelf.

Each strip, including Master, owns its scroll offset. Only the effect list
scrolls; the add button is outside its clip and input region. At the minimum
window height, fewer effect rows are shown so the page and faders remain usable;
the add button remains visible. At nine occupied normal slots it remains
visible but disabled, with a full-chain tooltip. Legacy occupied slot ten is
still displayed. No effect slots or bindings are removed by hiding gaps.

Clicking an effect toggles its floating editor. Dragging past four logical
units begins reordering, with a target line; wheel motion scrolls that chain
during the drag. Escape/right-click or release outside its list cancels.
Settings, wet/dry, bypass, automation, MIDI bindings, pending MIDI learn/control
targets and open editors follow the effect. Undo records the finished move.
Reordering restarts DSP history on that bus; unrelated buses keep their tails.

Right-click an occupied effect and choose **Replace effect...**, then select a
built-in effect. The replacement keeps the same processing slot, opens its
editor and starts with its own defaults. Old slot automation and MIDI bindings
are removed; pending MIDI learn for that slot is cancelled. Other effects and
routing remain in place. Dismissing the chooser leaves the effect untouched.

## Geometry and preferences

At 1448x1086, an empty Mixer is 294 high and the page is 702 high. One/two/three
FX rows grow the Mixer to 316/338/360 and reduce the page to 680/658/636. The
bottom edge, strip headers and faders stay in place during this growth. Extra
chains with fewer effects do not reserve blank rows within their stacks.
The six-unit shared split divider still allows manual sizing. Floating editors
remain independent of the Mixer.

Workspace preference version 4 stores the strip/body size separately from the
dynamic shelf height. Older preferences subtract their former reserved 88-unit
shelf before the new shelf is added. Project serialization is unaffected by
this geometry change. Workspace tests explicitly cover automatic growth,
shrinkage, divider resizing/reset, minimum page size and floating-editor bounds.

## Routing and controls

M/S are separate Mute and Solo buttons shared with Arrangement track headers.
Solo retains additive behavior and clears that channel's mute when activated.
Master S clears all insert/instrument/Arrangement solos; mono stays on width.
Right-click M retains mute automation. Right-click an insert name to rename it,
or its numbered left edge for channel actions.

Dragging an output to an input adds a unity-gain destination without replacing
existing ones. Inputs sum all their sources. Duplicate routes are ignored;
self-connections and cycles through any branch are blocked. Right-click a port
to remove individual connections or all its cables. **Remove all cables** is
the last entry, after the individual connections. Selecting a strip shows
its visible incoming/outgoing cables; dragging shows a new preview cable.

The optional typed `x-insert-sends` project field stores extra destinations and
is omitted when unused. Original single-output fixtures remain byte-identical.
The prepared playback plan holds compact destination lists and topological
order; rendering adds processed buses to destinations without allocating,
locking or searching the routing matrix. New studio projects have 99 inserts
plus Master; the engine still supports 100 for existing data.

## Validation and limits

`mixer_view_test` checks fractional/DPI navigation, fixed Master, clipped ports,
scrollbar grab/endpoints, stack geometry and add-row exclusion from scrolling.
`mixer_slots_test` checks sparse lists, nine-slot capacity, legacy slot ten,
actual drag/release/cancel handling, editor/binding identity and prepend/reveal.
`effect_chain_test` covers complete setting/automation/MIDI permutations.
`mixer_routing_test` covers actual cached/uncached fan-out/fan-in sums, persistence,
reset, individual removal and cycle rejection. Workspace preferences roundtrip
and migrate; grid-menu ordering is tested independently of stored choice IDs.

Software captures outside the repository: `../baseline-artifacts/pinned-fx/`
(zero, one, three, six scrolled effects, plus compact sorted grid menu).
Native device audio, trackpad feel, HiDPI display quality and CPU/GPU performance
remain unverified. No performance improvement is claimed.

## Separated cable paths

The studio renderer now assigns lanes from the actual visible connections,
replacing the four repeating Bezier depths. Connections sort deterministically
by their horizontal bounds and identity. A lane is reused only when its spans
are disjoint with a six-unit clearance. Overlapping spans receive separate
lanes. Angled leads spread from shared sockets into distinct horizontal runs;
shared endpoints and isolated crossings can still meet.

Paths are thin signal-colored lines over dark outlines, drawn in two passes so
outlines cannot erase neighboring signal cores. No texture or dependency was
added. A routing strip uses up to 24 extra logical units within the existing
Mixer height: socket/readout positions and the fader bottom move upward by
that amount, with the existing fader value mapping applied to the resulting
travel. The room reduces automatically on short windows. Hit testing uses the
same derived socket position. FX shelf placement, dock size, project data and
DSP are unchanged by this visual refinement.

`mixer_cables_test` checks 1–100 outgoing/incoming connections, deterministic
ordering under shuffled enumeration, disjoint lane reuse, shared-port fan-out,
noncoincident horizontal runs, short routing strips and vertical bounds.
All 146 tests passed; five focused Mixer tests also passed after the compact
lane-spacing correction. Release build and strict signature checks passed.
Normal/compact software captures: `../baseline-artifacts/cable-lanes/`.
Native display quality, hands-on routing feel and performance remain unverified.

## Compact effect editors

Effect panels use the same shared 12-unit-radius knob as the spacious FM and
sampler controls. Delay TIME retains an 18-unit radius for its snapping ring;
the compressor monitor speed stays at 10. Drawing, drag sensitivity, fine
control, reset, automation and MIDI handling use the existing knob implementation.

Fixed logical window sizes: Chorus 330×164; Tube 440×176; Phaser/Flanger
600×176; Reverb 640×176; current Delay 620×184. Chorus now
has a single footer row and readouts for all three parameters. Other single-row
effects share tighter knob/label/readout spacing. EQ already uses small knobs
and retains its graph. The compressor retains its graph and history dimensions,
with smaller knobs and tighter spacing between its two control rows.

These are approved geometry changes: control centers and knob hit areas move
and resize with their panels. Parameter ranges, drag scaling and DSP are
unchanged. Geometry assertions were updated to the new sizes; behavioral
assertions remain intact. Software captures cover Chorus, Delay, Tube, Reverb,
Dynamics, Phaser and Flanger at 900×562. Native pointer use and HiDPI appearance
remain hands-on checks. No new assets or drawing effects were added; rendering
cost has not been measured.

Floating editors now open at their smallest supported size and cannot be
manually resized. Instrument editors share 440×420 (360 crowded the six-op
labels); EQ uses 480×330 and Dynamics 820×480. Title dragging and close remain.
Saved instrument positions are retained, while saved widths/heights normalize
to the compact size. The workspace clamps tools to the available desktop.
Main-window resizing and Browser/Mixer/velocity dividers remain active; docked
pages continue following the workspace. Corner grips and resize-only input
exclusions are removed for floating tools. Tests cover stale-size normalization,
corner presses not capturing a resize, and preserved title movement/close.

## Adding mixer inserts

New projects start with eight inserts plus Master. The Mixer header's + button
adds one insert; right-click it and choose **Add multiple...** to enter a whole
number from one through the remaining capacity. The existing ceiling is 100
inserts, excluding Master. At capacity, the add button is disabled. Newly added
inserts use default gain/width, route to Master and have no effects or device
assignments. Existing inserts, routing and effects remain intact. The first
new insert is selected and revealed. Insert count already belongs to the saved
project format; loaded projects keep their counts. Frozen factory/demo reference
projects retain their original count.

This changes the initial count, not the storage architecture. Project settings
use fixed-capacity arrays, and the runtime rack reserves 101 buses including
Master. Delay, Reverb, Dynamics and Flanger state is allocated when needed off
the audio callback. Playback plans already omit unused buses from the render
order. Fewer displayed inserts therefore do not imply proportional memory or
CPU savings; neither has been benchmarked for this change.

Regression harnesses now explicitly initialize the frozen reference insert count
where serialized project bytes or high bus IDs are compared. Reference fixtures
and comparison assertions are unchanged. Separate tests cover the eight-insert
default, batch addition, limits, initialized state, preservation of existing
settings and save/load of the resulting count.

## UI scale

**View → UI Scale...** provides −/+ in 25% increments (75–200%), an explicit
100% size and Automatic. Automatic is the default and retains window-based
scaling, capped at 200%. Requested enlargement is limited to leave at least
900×562 logical UI units available; the dialog reports the effective percentage
when limited. Enlarging the window can therefore honor a previously limited
request. This limit keeps menus and compact editors reachable on small windows.

OS HiDPI density remains independent: the existing framebuffer-to-window ratio
still controls font rasterization. Manual scale affects drawing, text, pointer
coordinates, clipping and layout together. On a scale change the workspace's
viewport-width reference is refreshed, preserving the current arrangement zoom
and piano span rather than treating the change as a timeline zoom gesture.
Ordinary window/divider resize behavior remains in place.

The preference is stored atomically in `ui-scale.txt` beside Browser/workspace
settings, outside projects. Automatic is encoded as zero. Unsupported versions,
non-finite values and values outside the range are ignored. Changes apply
immediately without restart. Software captures cover 75%, 150% and a 200%
request limited by a small window. LCD segments retain a minimum thickness so
transport numbers remain visible at 75%. Tests check fit bounds, preference persistence
and invalid input, and verify scaling leaves project bytes unchanged. Native
mixed-DPI display transitions and physical pointer checks remain unverified.

The old frontend override forcing 99 inserts was removed from startup and
File → New. The smoke startup test now exercises the actual New workflow and
asserts eight inserts. Demo/reference rendering comparisons retain an explicit
100-insert input to preserve their frozen project comparison.
