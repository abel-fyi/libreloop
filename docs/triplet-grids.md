# Explicit third grids

Piano Roll and Arrangement share the same grid menu and snap intervals. The
existing eighth-triplet and sixteenth-triplet entries are now labeled `1/3 beat`
and `1/6 beat` so their musical meaning is visible. Their stored UI choice IDs
and intervals remain unchanged. Appended choices provide `1/3 step`, `1/6 step`,
`1/3 bar` and `1/6 bar`.

A beat is four steps and a bar is sixteen, so the six triplet intervals in
logical steps are 4/3, 2/3, 1/3, 1/6, 16/3 and 8/3 respectively. Snap stays at
the selected interval at every zoom. Existing distant-zoom line suppression,
modifier rules and Auto/Straight grids are unchanged.

`grid_choice_test` checks the explicit intervals and exercises actual Piano
Roll note creation/dragging and Arrangement clip placement for every choice at
three zoom levels. Full suite: 145 passed. Five focused grid/FX/routing/pitch
tests passed after the final cached MIDI-learn target remapping fix. Release
build and strict app signature verification passed. Software menu captures:
`../baseline-artifacts/triplet-grids/`. Native device/trackpad/HiDPI behavior and
performance remain unverified.

Menu order refinement: Auto/None remain first; all fixed intervals now appear
in strictly decreasing step duration, with thirds between their adjacent
straight divisions. A menu-row-to-choice mapping preserves existing choice IDs.
Tests assert unique coverage of every choice and strict numeric ordering.
