# Arrangement and Pattern pages — 2026-10-17

The user approved replacing the sequencer/Rack split with two main pages:
Arrangement and Pattern. This supersedes `flexible-workspace.md` and its shared
Rack/Piano docking controls. It is an intentional approved interaction/layout
change; the reference repository and original interaction fixtures are unchanged.

## Workspace

- Arrange / Pattern buttons occupy the previous four editor-button cells.
  Arrangement is the initial page. Both pages fill the same available rectangle
  above the Mixer; exactly one is visible. There is no third split, detached Rack,
  maximize, minimize or close control on either main page.
- Pattern has a 166-logical-unit channel column and the Piano Roll. Back to
  Arrangement is a visible button. All legacy Rack-open entry points, including
  pattern double-clicks, lead to Pattern. Original note editing remains in the
  carried controller, translated as a unit to the right of the channel column.
- Each page retains its zoom, pan/scroll and selections when switching. Both
  hidden/visible page rectangles are maintained so a switch cannot change the
  horizontal scaling helper. Resizing still preserves pixels per timeline step
  and piano-note row height. The new studio default vertical zoom is 2 (10.4
  logical units per row); short-screen startup frames C4. Later resizing does
  not reset zoom or reframe notes.
- Mixer remains a full-width bottom dock. Its divider resizes it and the active
  page; double-click resets the split. Dragging proposed height below 100 snaps
  to the existing 18-unit restore strip. Expanded minimum is 208 units.
- Instruments are floating editors. Effects retain their independent floating
  instances and stacking. Clicking a page focuses it without covering floating
  tools. Editors have close only; closing hides the UI and preserves audio.
  Mixer resizing does not stretch a floating instrument. Window-size constraints
  can still constrain editors when the application itself becomes smaller.
- Workspace preference v3 retains Mixer size and float rectangles, reads v1/v2,
  discards obsolete editor-split docking, and starts on Arrangement with editors
  closed. No project-format change.

## Channels and tools

Channel rows expose Mute, Solo, channel selection, and `...` instrument access.
The selected channel has pan, volume and mixer routing below the list. Other
instrument controls remain in its editor. Add channel opens the existing built-in
instrument chooser. Right-click a channel keeps rename, delete, replace/relink,
Piano Roll and Fill actions. Sample drops replace a channel row or add to blank
channel-list space; sampler and Arrangement drops retain their original paths.
The list has wheel scrolling and explicit up/down buttons when needed.

Pencil / Brush / Select, Grid/snap, Hits and Fill X are above the piano
canvas. Global Swing remains beside the pattern name in the channel header. The existing Browser Patterns / Audio / Automation picker remains usable
on both pages, so patterns can be selected/created without returning to a Rack.

## Drum hits

Hits is an explicit per-channel entry-mode toggle, initially off. It is UI state,
not inferred from instrument type and not stored in project notes. Left press and
horizontal drag add C4 (MIDI 60), velocity 100, length-zero one-shots on whole
steps. Right drag erases C4 hits at those steps. It does not convert existing
notes or change other pitches; existing C4 notes are kept by an additive stroke.
Select tool, Shift and shortcut selection retain the normal controller path.
Velocity editing remains available. Fill X uses the existing capacity-preflighted
whole-pattern fill and number dialog.

A stroke preflights each interpolated segment against note capacity, extends the
pattern before insertion, and uses the existing capture/history boundary for one
undoable continuous edit. A segment that cannot fit is rejected without partial
insertion. Original note limits, project representation and audio engine remain.

## Ghost notes

Other channels' notes in the current pattern, including drums and muted channels,
are drawn dimly behind active-channel notes, at their actual pitches, starts and
lengths (length-zero hits display as one step). They do not appear in the active
velocity lane and have no hit targets, previews or selection handles. All editing
and erasing still addresses only the active channel.

## Verification and limits

Tests cover exclusive pages at multiple viewport sizes, Mixer resize/reset,
floating-editor stacking/independence, channel/drop hit areas, view/selection
preservation across repeated switches, v2 preference migration and invalid-file
rejection, C4 strokes/capacity/extension, whole-stroke history, and ghost-channel
notes being excluded from active-channel hit testing. The existing reference
replays and audio/project fixtures are retained unchanged. Legacy-only capture
harnesses gained stubs for the new workspace toolbar; the clipboard/history
capture retains the real history functions and its original assertions.

Actual app software-rendered captures at 1448x1086 and 900x562 are stored in
`../baseline-artifacts/pattern-pages/{arrangement,pattern,compact,hits,instrument}`.
These validate layout appearance, not physical mouse/trackpad feel, GPU/HiDPI
rendering performance or audible device playback. No new dependency, font,
bitmap decoration, animation or audio-thread work was introduced.

Nine-slot Mixer UI and slide-note synthesis are separate earlier requests, not
implemented by this page-layout change.

Final validation: Release build and all 131 CTest cases passed; the app signature
and `git diff --check` passed. The original reference Git tree remains clean at
5e860696fe6cb6cd4c170f6ebf5378039b3e4af6. Release executable is 3,726,272 bytes,
144 bytes above the preceding vintage-controls build. This is a total binary
measurement, not an isolated rendering-effect size. Per-frame ghost-note cost,
active/idle CPU comparison and physical GPU costs have not been measured in this
pass; no performance improvement is claimed. The existing system SDL deployment
version warning remains a packaging limitation.
