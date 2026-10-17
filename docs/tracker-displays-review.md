# Tracker display and proportion review

The user asked for more 1990s visual detail, including the master spectrum,
fonts and filled synth LFOs, and delegated screenshot review. A separate request
asked that piano-roll resizing/maximizing preserve vertical zoom.

## Completed

The studio transport uses original procedural seven-segment numerals for tempo
and bar/step, within the existing buttons/readout rectangles. General UI text
still uses the existing font. The spectrum has a dark phosphor grid with
segmented green/amber bars; the stereo display has green/amber/red cells and
L/R labels. Each spectrum column takes the maximum of its corresponding
contiguous existing FFT bins. The original -78..0 dB mapping remains. The FFT,
audio meter state, smoothing, timing and audio callback are unchanged. Quiet
cells are dim and unlit, with no synthetic spectrum activity.

The Analog overview has a filled two-cycle waveform thumbnail beside its shape
selector. Analog Motion shows a read-only 500 ms waveform derived from the
existing LFO rate and shape. Custom FM's existing editable Motion graph gains
filled color and a gridded display, keeping the existing sampled curve, plot
bounds, speed/amount handle and drag formulas. The curve/hint contrast was raised
after screenshot review. These static shape illustrations neither represent a
live phase shared across notes nor add an animation that prevents idle waiting.

The screenshot review found oscillator values touching the lower group edge;
its decorative border extends four pixels lower within the existing gap. Knobs,
input rectangles and panel layout remain in place. Synth knob captions now use
a 12-pixel rendered size at `y + 12`, with 11-pixel values at `y + 25`, instead
of 13-pixel captions at `y + 17` and 12-pixel values at `y + 31`. This clears
the group edge without moving or resizing the controls. Transport numeral
size and spacing fit their original fields at compact and full window sizes.
No new font, bitmap artwork or dependency is used.

The separately committed piano change is documented in
`piano-resize-behavior.md`. Resizing reveals more pitches at the same row height;
manual vertical zoom continues to work. Blank space below MIDI note 0 rejects
new-note clicks without extending the pattern. Frozen legacy behavior and
fixture expectations remain unchanged.

## Verified

The final full CTest suite passes **120/120**, including the frozen audio and
interaction fixtures and new resize test. The bundle passes `codesign --verify
--deep --strict`. All 182 original/concept reference file hashes still match
the frozen manifest. No fixture expectation was changed.

The actual SDL desktop was built in Release using AppleClang, SDK 15.4 and the
installed SDL3 developer runtime. Software-rendered app captures were reviewed
at 1448×1086 and 900×562: idle, real playback, Analog Motion, Custom FM Motion,
and maximized Piano. The playback capture shows actual callback-derived spectrum
and meter values; it is a visual check, not an audible correctness measurement.

Artifacts live outside both repositories at
`../baseline-artifacts/tracker-displays/`. Subdirectories `idle`, `playing`,
`lfo`, `custom-lfo`, `compact` and `piano-maximized` contain the captures. Their
projects are editable synthetic review projects, not normal startup content.

Developer-only `--smoke-playing` and `--smoke-piano-maximized` are accepted only
with `--smoke`. Scene 13 covers Custom FM Motion. Visual state comparison was
extended to 14 scenes × 4 captures per style (56 each), retaining byte-identical
project assertions between legacy and studio rendering. The new `piano_resize`
test covers size cycles, actual maximize/restore, hit tests, zoom levels,
1x/2x coordinate conversion, precise-scroll sensitivity and invalid low pitches.

## Unverified

Native GPU timings, physical HiDPI rendering, audible playback, and hands-on
mouse/trackpad acceptance remain unverified here. Synthetic 2x input checks do
not establish native display behavior. Shipping offline/static SDL and other
platform acceptance gates remain outstanding; the installed SDL runtime still
requires macOS 26. No broader completion claim is made by this visual pass.

## Size and frame cost

Revision `3d25cbd` (the previous Windows/Motif styling) was rebuilt from a
separate detached rewrite worktree outside the original. Both builds use the
same Release toolchain, SDK 15.4 and developer SDL runtime. Measurements include
this follow-up's piano fix and display drawing together; the default small
piano pane retains its previous row height. SDL dummy video/software rendering
was used at 1448×1086 with scripted pan/zoom, 40 warmup frames and 500 measured
frames, three sequential before/after pairs per scene. No test/build/capture
process ran concurrently with these measurements.

The figures are render + flush only, excluding presentation, event processing,
state publication and pacing. They are not native GPU or input-latency results.
Audio device startup succeeded; audible output was not checked. Timings are not
individual per-effect measurements and small differences should not be treated
as portable performance guarantees.

| Scene | Build | Median run p50 | Median run p99 | Largest frame |
|---|---|---:|---:|---:|
| Overview, miniature LFO | before | 5.276 ms | 5.608 ms | 9.316 ms |
| Overview, miniature LFO | after | 5.200 ms | 6.009 ms | 7.121 ms |
| Analog Motion, full LFO | before | 4.629 ms | 5.865 ms | 8.220 ms |
| Analog Motion, full LFO | after | 4.704 ms | 5.653 ms | 11.178 ms |

The executable grows from 3,674,448 to 3,691,088 bytes (+16,640 bytes,
0.45%). This includes drawing, piano behavior changes and development
capture flags; it is not attributed solely to an individual effect. Added bitmap
and font asset bytes: **0**. The bundled dynamic SDL runtime remains unchanged.
Raw timings and summaries are in the artifact directory. Normal blocking idle
wait behavior remains unchanged; these active-render profiles do not measure
idle CPU, physical HiDPI or battery use.
