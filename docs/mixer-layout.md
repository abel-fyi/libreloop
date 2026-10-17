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
to remove individual connections or all its cables. Selecting a strip shows
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
