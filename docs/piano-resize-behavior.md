# Approved piano resize behavior change

The user requested that resizing or maximizing a piano-roll window preserve
vertical zoom. This intentionally changes the current studio UI from the frozen
original behavior; legacy mode and original replay expectations remain intact.

Previously, visible pitches were `clamp(25 / zoom, 8, 128)` and row height was
`(content_height - 180) / visible_pitches`. Increasing window height stretched
notes and keys. In studio mode, the default 276-pixel pane has a 34-pixel toolbar
translation and 180-pixel content padding, leaving a 130-pixel reference grid.
Row height is now `130 / clamp(25 / zoom, 8, 128)` logical units, independent of
window dimensions. Visible rows are `(content_height - 180) / row_height`.
The default small-pane appearance and manual vertical zoom sensitivity/limits
are preserved. Taller panes reveal more pitches; restoring a pane restores the
visible region size without resetting manual zoom.

Drawing, note/key hit tests, gestures and precise scrolling use the same row
calculation. Top pitch is clamped only as needed to keep the legal pitch range
visible; when more than 128 rows fit, top pitch is 127 and the vertical scrollbar
thumb fills its rail. The blank region below MIDI pitch 0 rejects note
creation without extending the pattern. No project format or audio behavior changes.

`piano_resize` exercises size cycles and note/key hit tests at 1x/2x coordinates
and multiple manual zoom levels, actual window maximize/restore transitions,
and precise-scroll sensitivity before/after resize. The existing frozen tests
are unchanged. Physical mouse/trackpad and native HiDPI verification remain
outstanding. See the visual follow-up report for capture review.
