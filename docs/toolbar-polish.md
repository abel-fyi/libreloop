# Toolbar and elapsed-time polish

The first roadmap pass restores FILE/EDIT/VIEW/HELP text beside the existing
colored icons. The four 68x20 controls form two columns at the far left; menus
open below their actual control rather than the former fixed coordinates.
Full-width controls keep the filename above them. Compact controls shift right
by 56 units to preserve room for the labels. The global 60-unit workspace start
and all editor content geometry remain unchanged.

The clock's alternate view now reads minutes:seconds:centiseconds, starting at
00:00:00. Minutes are not capped at 99. Values derive from integer audio sample
positions, with correct second/minute rollover and no floating-point timing
accumulator. Main digits stay 16 units and centiseconds are 11, bottom-aligned;
this is the explicit exception to uniform text size requested in the owner's
notes. Very long times scale proportionally to fit. Count-in still shows its
step count. Click still switches between musical and elapsed-time views; legacy
parity mode retains its original seconds/milliseconds format.

`icon_in_button` reuses the existing unpadded 16px sprites, fitting them into
transport/record-type/follow/typing controls with a common interior margin.
The 32px transport images are direct 2x enlargements rather than enlargements
of an already padded cell. Existing icon rendering and atlas data remain
available for carried controls/fixtures; no new artwork or dependency.

Screenshots exposed overlapping captions in the compact readouts. Clock and
tempo now use independent 44-unit-high wells there, keeping digits and caption
rows separate. Pitch/Vol captions are also spaced apart. These intentional
layout changes affect only toolbar controls, not piano/arrangement zoom, scroll,
snapping, note gestures, engine commands or project data.

Verification: new frontend draw-command tests check smaller centiseconds, shared
baseline, long-time bounds and fitted icon geometry. Formatting tests cover
44.1/48 kHz centisecond boundaries, minute rollover, >99 minutes, zero rate and
UINT64_MAX frames. Layout/text-fit assertions cover labeled menus, nonoverlap
and readout room. The 141-test suite passed; targeted layout/readout/startup
checks were repeated after fixing compact spacing. Software screenshots at
1448x1086 and 900x562 are in `../baseline-artifacts/toolbar-polish/`. Native
mouse/HiDPI appearance remains unverified. App signature verification passed.

Release executable: 3,778,256 bytes, 192 bytes above the preceding local build.
This pass adds no textures/assets. Per-frame cost was not reliably measured;
no CPU or GPU saving is claimed. The existing sample/tempo/audio logic is
unchanged.
