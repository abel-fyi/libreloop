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
accumulator. All digits now use the shared UI font at the normal text size,
including centiseconds. Long values use the common text-fit path. Count-in shows its
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

Verification: frontend draw-command tests check fitted icon geometry. Tests of
the retired custom numeral renderer have been removed. Formatting tests cover
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

PATT/SONG follow-up: clicking anywhere in the combined control toggles the
mode/page, including its currently active half. Recording still blocks mode
changes. Desktop rebuilt; existing Pattern-page and workspace tests passed.

Shared-font follow-up: removed 84 lines of dedicated bitmap/segmented numeral
drawing code and its two public declarations. Tempo and position now call the
existing label/text-fit functions. The Release build and four targeted transport,
startup and frontend checks passed; a native macOS elapsed-time capture was
reviewed. No binary-size or rendering-time saving is claimed from line count.
