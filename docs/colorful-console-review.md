# Colorful console refinement

## Completed

The user asked to remove segmented bars, preserve signal information, unify the
master/Mixer display style, add spectrum colors, color the icons, dress up the
track/piano/Mixer side panels, and correct the toolbar proportions. They then
explicitly requested menus at the very left, followed by Pitch/Vol, in one strip.

The master FFT draws every one of its 96 bins independently using the original
continuous -78..0 dB height mapping. There is no grouped maximum or segment
rounding. A green/teal/amber frequency palette gives the display color. Continuous
stereo level fills have matching dark wells, green/amber ranges and the existing
red clipping indicator. Mixer meters keep their original curves, thresholds,
release and peak-hold logic, gaining inset wells, fine scales and bright peak
markers. Mixer strip borders, header plaques, fader rails/ticks and gain readout
wells add console detail. No signal values or audio code are changed.

Icons use blue, green, amber, violet and red tints drawn from the existing atlas;
active icons retain their color and disabled states stay subdued. No Windows
artwork is copied. Track-number plaques and a narrow identity stripe derive
color from the first clip, with neutral trim on empty tracks. Piano tool,
quantum, note-length and selection areas gain recessed groups. Tall Piano windows
also show real pattern/channel details and an editing-key reference. Decorative
ribs do not introduce controls, meters or state. PATT/SONG gets two framed,
illuminated halves while the original click-to-toggle behavior is preserved.

No font, bitmap asset, new feature or dependency is introduced by the trim.

## Intentional geometry changes

User authorization: “make buttons more square and evenly placed” and Pitch/Vol
“after the FILE, EDIT, VIEW” at the “very very left”, with menus no longer above
other controls. This supersedes the preceding two-row toolbar geometry.

| Geometry | Previous | Current |
|---|---|---|
| Workspace top | 98 | 60 logical units |
| Normal transport | 54×42 | 42×42, 6-unit internal gaps |
| Normal editor/navigation buttons | 22×42 | 28×28 |
| Compact transport | 54×42 | 32×32 |
| Compact editor/navigation | 22×42 | 24×24 |
| Menus | Separate row over controls | Far-left plate, then Pitch/Vol |
| Compact menu plate | Separate upper row | 2×2 below 1180 units, within same strip |
| 1448×1086 Arrangement rect | 244,98,858,468 | 244,60,858,506 |

Pitch/Vol and the remaining toolbar controls move to new hit rectangles defined
by the concrete pure-C `studio_toolbar_layout` function. The label/readout and
button gestures retain their existing commands. A normal toolbar fits from 1180
units; at 900 units all controls fit using the compact arrangement. Spectrum is
hidden when fewer than 96 logical units remain, rather than overlapping other
controls. These are layout changes to targets, not permission to change gesture
sensitivities or modifier rules.

At the reference size, Piano and Mixer rectangles remain in place. Arrangement
and Inspector begin 38 units higher and gain that height. Their ruler/control
positions move with their containing windows; snapping, content-local zoom
anchors and input conversion are unchanged. At other window sizes the existing
reflow thresholds operate on the larger workspace. Stable Piano vertical row
height from the previous change is retained.

Only the owned studio layout expectation changes from top 98 to 60 and exact
Arrangement height 468 to 506. Bounds, minimum-size and non-overlap assertions
remain intact. Frozen original audio/interaction fixture expectations remain
unchanged. New toolbar assertions check squares, ordering, bounds and pairwise
non-overlap at 900, 1024, 1179, 1180, 1200, 1448 and 1920 widths.

## Verification

The final full CTest suite passes **121/121**. The bundle passes
`codesign --verify --deep --strict`. All 182 original/concept reference file
hashes match the frozen manifest; no original source was modified.

The new `studio_signal_draw` test inspects the actual frontend drawing function:
96 independent bins, adjacent unequal fractional amplitudes retained, frequency
colors distinct, and smooth Mixer fill response. This guards against returning
to grouped bins or segmented levels. Existing interaction and audio tests remain.

Actual SDL software-rendered captures at 1448×1086 and 900×562 are stored outside
the repositories in `../baseline-artifacts/colorful-console/`. `playing` shows
real callback-fed spectrum/meters, `idle` their quiet state, `compact` the narrow
layout, and `piano` the maximized sidebar. The project is a synthetic editable
showcase; normal startup remains the normal empty project.

Native GPU cost, physical HiDPI rendering and hands-on mouse/trackpad acceptance
remain unverified. The development bundle still uses the installed dynamic SDL
runtime and does not establish offline/static build completion or older-macOS
support. No claim of lower CPU from the new spectrum is made.

## Release size and render cost

The previous revision `42ab508` was built in a detached rewrite worktree outside
the original. Both builds use Release, AppleClang, SDK 15.4 and the same installed
SDL3 runtime. Three sequential before/after pairs use 1448×1086, scene 12,
scripted pan/zoom, 40 warmup frames and 500 measured frames. No test, build or
capture process ran concurrently with these measurements. SDL dummy video and
the software renderer were used.

| Measurement | Before | Current |
|---|---:|---:|
| Median run p50, render + flush | 5.270 ms | 5.282 ms |
| Median run p99, render + flush | 5.852 ms | 5.687 ms |
| Largest observed frame | 6.487 ms | 8.373 ms |
| Executable bytes | 3,691,088 | 3,691,184 |

The binary grows 96 bytes; added font/bitmap asset bytes are zero. Median run
p50 differs by about +0.013 ms, which is too small for a reliable performance
claim. These numbers measure the entire changed UI, including a wider spectrum,
colored trim and a taller Arrangement, not the isolated cost of segmentation.
They exclude present, events, command/state publication and pacing. Audio device
startup succeeded but audible output was not assessed. No individual trim cost,
native GPU, idle CPU, battery, physical HiDPI or latency result is inferred.
Normal event-waiting behavior is unchanged. Raw timing runs and summary are in
the artifact directory.
