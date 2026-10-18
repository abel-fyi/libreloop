# Multiband Compressor

A built-in three-band compressor/expander with a fourth Master processor,
inspired by the workflow in Image-Line's Maximus. Original GPL-3.0-only C code;
no Image-Line code, artwork, presets, or new dependencies are included.

Open a mixer strip's effect adder and select **Add Multiband Compressor**. The floating
editor has a level transfer curve on the left, a live monitor on the right, and
Low/Mid/High/Master selectors below. Red, amber, yellow and green identify bands.
The same bitmap text, knobs and window treatment as the rest of LibreLoop apply.

## Curves and controls

The transfer graph maps input dB horizontally to output dB vertically, from
−72 to +12 dB, using a soft-log amplitude axis with 0 dB near the center.
Both axes use the same mapping, so the diagonal is unity; flattening the curve compresses, a horizontal
segment limits the detected level, and steeper sections expand. This is an
envelope-controlled gain map, not an instantaneous audio waveshaper. Attack can
let peaks pass; this version does not promise an absolute output ceiling.

- Right-click empty graph space to add a point, up to eight per band. New points
  snap to whole dB values. Right-click an interior point to delete it.
- Drag filled points freely; endpoints keep their input position. Shift locks
  output, Ctrl locks input. Input points cannot cross or approach within 0.25 dB.
- Drag hollow segment handles vertically to bend; Ctrl makes bending finer.
  Right-click a bend to reset tension. Handles sit halfway along each segment
  in displayed coordinates. Positive/negative bows work on horizontal segments
  too; dragging up raises the curve, dragging down lowers it. Wheel over it cycles Single, S, Hold,
  Stairs and Smooth Stairs. The status bar reports this interaction.
- Copy/Paste copies the selected band's curve between bands or instances.
  Reset Curve restores a unity diagonal up to 0 dB and a horizontal 0 dB ceiling
  above it, the default for all four bands. Existing undo/redo groups a drag.
- Each band remembers its own curve, pre/post gains, attack, release, second
  release, sustain, stereo width, Peak/RMS mode and optional soft saturation.
- Mode cycles On, Comp Off (width, pre/post and saturation still work), Muted
  (detector keeps running), Off (band silent and detector reset).
- Solo isolates Low/Mid/High; soloing several keeps those bands. The Master
  always follows their sum. Master Solo is the complete summed signal.
- Width: 0% mono, 100% unchanged, 200% doubles the existing side information.
  It does not synthesize stereo from mono. Both channels share one gain envelope.
- **Band Mix** blends the compressed bands with their phase-aligned uncompressed
  sum before Master. Prefer this for parallel compression. The rack's outer Mix
  and Bypass crossfade to the original input; minimum-phase crossover phase can
  cause cancellation at intermediate outer Mix settings.
- Low/High set the crossover boundaries; Low Cut is an optional input high-pass,
  disabled at zero. Bands view also allows dragging the two boundaries.
- Saturation blends a tanh soft clipper with the processed signal; Ceiling sets
  its scale, not a brick-wall limit. It is disabled at zero and is not oversampled.

Pre/Post: ±24 dB. Attack: 0–200 ms. Release/Release 2: 0–2000 ms. Sustain:
0–1000 ms. Width: 0–200%. Saturation: 0–100%. Saturation ceiling: −24 to +12 dB.
Crossovers: Low 40–1000 Hz, High 1000–16000 Hz. Cut: Off to 200 Hz.

The gray fill beneath the curve is one live **band input-level meter**, not a
histogram or frequency display. Its right edge follows the current band level;
each filled column is capped by the transfer curve, including bent and stepped
segments. Silence leaves the graph empty. The scrolling Monitor draws gray input peaks, band-colored output
peaks and a cyan gain trace in dB. A flat gain trace at 0 dB means no gain change;
negative means reduction, positive means boost. The numeric readout shows gain.
Pause freezes the UI monitor; audio continues. The centered Speed knob below
the right graph runs from ¼× to 4×, with 1× at center. Slower shows more time,
faster spreads transients out; the newest data stays at the right edge. Speed
is a per-editor display preference and does not alter audio or presets. Bands
view shows the actual split.

Load opens the existing file chooser and creates 0 dB Ceiling, Drum Punch and
Gentle Glue factory presets if absent. 0 dB Ceiling is the default for all bands:
a unity diagonal up to 0 dB, horizontal above, without saturation or low cut.
Previously generated Neutral presets and saved project curves are preserved. Drum Punch narrows bass to mono, uses firmer curves and 65% Band Mix.
Gentle Glue uses milder curves and wider highs. Save persists all four curves
and settings. The 52 numeric parameters support existing automation/MIDI mapping;
curve topology itself is edited and saved, not automated.

## DSP and ownership

48 kHz, cascaded second-order Butterworth sections form 24 dB/octave
Linkwitz–Riley crossovers. The Low output also passes through the higher
crossover's allpass sum, so recombination has unity steady-state magnitude at
neutral settings. This is minimum phase, not sample-identical to the original
input. Filter targets update every 32 frames and coefficient interpolation
smooths crossover edits over approximately 20 ms. Pre/post gains, width and wet
mix also slew over 20 ms. Curve edits engage a 20 ms transition window with
2 ms gain smoothing, avoiding an instantaneous gain jump while dragging points.

The detector uses the larger absolute L/R amplitude for Peak, or the mean L/R
power with a one-pole Sustain averaging time for RMS. Attack/release smooth the
amplitude envelope with exp(−1/(sample_rate*time)). The dB transfer curve sets
linked gain; Release 2 optionally smooths its recovery. Sustain is an exponential
averaging time, not an exact sliding RMS window. Gain changes are bounded to
−72…+24 dB per dynamics stage. The curve uses signed quadratic bows, optional S interpolation,
hold, and stepped variants. It is not claimed to reproduce Maximus's proprietary
curve law or detector response exactly.

Runtime state is allocated and initialized on the control/export thread through
`effects_prepare`, then published with a lock-free atomic pointer. Each prepared
slot caches its state until rack destruction. No callback allocation, file I/O,
locks, waiting, or logging. Telemetry is a separate 1024-entry atomic ring with
input/output peaks and applied gain per band, published every 256 samples.
Approximate resolution is 0.38 dB, range −72…+24 dB, up to about 5.46 seconds of history (about 1.36 seconds visible at 1×).
The UI reads telemetry without modifying DSP state. Pause affects only drawing.

Project settings use an optional `x-dynamics.settings` field, emitted only if a
Dynamics effect exists. Existing projects receive neutral defaults. New settings
follow effects through reordering with automation/MIDI targets. Each fixed
project slot reserves 736 bytes of settings (743,360 bytes per Project). Runtime
state is prepared only for used slots. Automated settings use at most 32 sparse
copies per render call, rather than another project-sized array. Project
installation now borrows its caller's scratch project rather than copying it by
value; sample ownership/publication order stays unchanged.

## Scope and verification

The reference workflow and public controls were checked against
[Image-Line's Maximus manual](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins/Maximus_Controls.htm).
The implementation provides the core multiband curve-compression workflow.
It does **not** reproduce Maximus's full feature set: look-ahead/group delay and
host delay compensation, linear-phase filters, selectable crossover slopes,
oversampling, alternative saturation models, attack/release curve-number
controls, Master Mid sidechain mode, heatmap/FFT spectrum, adjustable monitor
speed, independent curve-state files and A/B comparison banks are not implemented.
There are no inactive controls pretending to implement these features.

Tests cover neutral crossover magnitude at eight frequencies (within 0.05 dB),
centered stereo and linked gain on unequal channels, mono low-band solo,
compression response, telemetry, silence/reset, exact initial dry bypass,
curve insertion/deletion/bending and bounds, deterministic replay, registry and
parameter ownership, project/preset roundtrips, engine/rack equivalence, export
PCM, automation and effect/MIDI target reordering. Actual-device listening and
comparison against Maximus, native input feel, other CPUs/platforms, and
full-load performance remain hands-on verification items. Measurements and
review captures are kept outside the repository.

Verified on this macOS arm64 checkout: Release build, all **155 CTest tests**,
and focused ASan/UBSan runs of Dynamics, Dynamics Integration and Audio. The
editor was visually inspected in software-rendered 1448×1086 and 900×562 app
captures. Curves remain editable and controls fit the smaller viewport; compact
layout moves Copy/Paste to the footer. Strict app code-signature verification
passes. These captures do not verify native input or real-device sound.

Before the monitor-history extension, a single-instance offline throughput probe
with the Release `-O3 -DNDEBUG`
compiler settings processed ten seconds of a 48 kHz stereo sine (right at 70%
of left) in 0.101 / 0.104 / 0.101 process CPU seconds for Neutral / Drum Punch /
Gentle Glue. That corresponds to approximately 1.01 / 1.04 / 1.01% of one core
for this controlled probe; it is not a live callback deadline or whole-app CPU
measurement. Runtime storage is **18,888 bytes per prepared instance** on this
ABI. The complete app executable is **5,589,472 bytes**; its size delta and the
per-frame UI rendering cost have not been isolated. No image assets were added.

Display regression checks cover the centered ¼×/1×/4× speed mapping, a fixed
right-hand newest sample, extended history capacity, and meter-fill bounds for
all five curve shapes. Audio tests remain unchanged by the display settings.

## Reference layout pass

The locally supplied `references/screenshots/maximus.png` was reviewed directly.
The fixed 820×480 editor retains the curve/history above the controls and uses
three lower groups matching the reference's organization:

- Left: Low/Mid/High/Master, four explicit band modes, Pre/Post above
  saturation/ceiling, and a Solo checkbox with stereo Width immediately right.
- Middle: Attack, Release and Hold above Release 2 and the RMS checkbox.
  Unchecked RMS means peak detection.
- Right: Low/High crossover frequencies above Band Mix and Low Cut.

The existing sampler checkbox implementation is reused for Solo, band mode
selection and RMS. Knobs retain the shared compact size. Curve reset/copy/paste,
wet/dry and presets occupy the footer. Readouts use short Hz/kHz formatting.
The add/replace menus are wide enough for the complete effect name.

Band Mix is the former LMH Mix: it has a defined parallel-compression role and
is retained with a clearer label and tooltip. No LMH Delay control is implemented
or added. Hold labels the existing sustain/hold time. No parameters, DSP, saved
IDs, presets or project fields were removed or changed in this layout pass.
The internal C identifiers and serialized `x-dynamics` fields retain their names.

Differences from Maximus are deliberate: LibreLoop's compact vintage materials
and knob sizes remain; unsupported threshold, envelope-curve selectors,
crossover-slope/linear-phase choices and LMH Delay are not drawn. The existing
editable transfer curve supplies the compression threshold/shape workflow.
Software captures were inspected at 900×562 and 1448×1086. Native interaction
and HiDPI legibility remain hands-on checks.
