# Requested navigation and editing refinements

These changes are explicitly requested departures from the original behavior
inventory. The frozen behavior spec and reference fixtures remain intact.

## Completed

- Channel Rack horizontal zoom: per-pattern step widths default to 16 logical
  units (100%), bounded to 4–128 (25–800%). Ctrl-wheel on Linux/Windows,
  Cmd-wheel on macOS, and native pinch reuse the Piano Roll navigation input
  normalization and `1.08` exponential zoom curve. The step under the pointer
  stays anchored unless the start-of-timeline clamp is reached. Shift-wheel,
  horizontal precise scroll and middle drag pan. Ordinary wheel still scrolls
  one channel per notch; precise vertical deltas accumulate fractional rows.
  Modal dialogs, other captures and focus loss block/cancel navigation as before.
- Rack footer `-`/`+` zoom around the viewport center; the percentage button
  resets to 100% around that center. Channel row heights and control columns
  remain fixed. Drawing, painting, erasing, melodic previews, scrollbar and
  ruler dragging all use the same step width. Zoom survives pattern switching,
  shifts with pattern deletion, and resets on project replacement. This is view
  state; the project format has not changed.
- Piano Roll and Arrangement have separate grid/snap dropdowns: Auto, None,
  1 bar, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/8 triplet and 1/16 triplet. Fixed
  values are musical divisions of a bar: a quarter is four internal steps.
  None disables snapping and vertical grid lines. Auto retains the reference
  algorithm. Note creation/movement/resizing/brush, clip operations, file drops
  and ruler operations use the selected quantum. At distant zoom, fixed visual
  lines are thinned to at least 3 pixels while the selected snap stays fixed.
  Preferences are independent view state, retained during this app session.
- Tempo keeps its dark display on hover/drag; only its border highlights.
  Generic button fill no longer washes out the LCD. Drag, wheel, recording
  restrictions and right-click entry/reset remain unchanged. Numeric readouts
  draw only lit segments, removing the dark ghost digits.
- Right-click an Arrangement track-header divider to reset that track's base
  height to the default 52 units (still affected by global vertical zoom).
  Right-click the Arrangement vertical-zoom control in its upper-right corner
  to reset all custom heights and global track zoom. Existing divider hit
  tolerance remains 3 units; neither gesture edits project/audio data.
- Creating automation still places/selects the clip and focuses Arrangement,
  but preserves both horizontal view start and vertical scroll position.

## Verified

Release developer build succeeds; all **125 CTest checks pass**. New tests
exercise actual note creation/dragging and clip placement for all grid modes at
three zoom levels, Auto equivalence, rack pointer anchoring and wheel/pinch
mapping at logical scale 1 and 2, precise scroll, middle capture/cancel, modal
blocking, zoom limits, and step painting/erasure across 4–128-unit widths.
Automation creation is exercised with twelve occupied lanes, asserting the
created clip and unchanged viewport. New assertion-based tests explicitly keep
assertions enabled in Release, including the earlier font-fit test. Original
reference audio/interaction/pixel fixtures remain unchanged and pass.

Actual SDL dummy/software captures were inspected at 1448×1086:
`../baseline-artifacts/navigation-controls-review/` contains rack-50, rack-200,
grid-menu and fixed-grid. Inspection caught a selector/scrollbar overlap; the
Arrangement selector now ends at y=44, above the existing scrollbar. Existing
ruler/grid/content geometry is unchanged. The app bundle signature verifies;
the original repository remains clean.

## Unverified

Physical mouse/trackpad feel, native GPU/HiDPI appearance, audible playback and
hands-on tempo/height-reset gestures require the user's machine. The captures
and logical-scale tests do not establish those results. No CPU/GPU improvement
is claimed. The executable is 3,691,968 bytes after this pass (+528 from the
preceding envelope build); no new assets, fonts or dependencies were added.
The existing installed dynamic SDL developer build and deployment warning remain.
