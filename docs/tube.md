# Tube saturation

Tube is a built-in stereo, tube-inspired saturator. Add it through a mixer
strip's `+` menu, then choose **Add Tube**. It uses four existing shared knobs
in a 480×220 logical-unit floating editor (minimum 440×220):

| Control | Range | Default |
|---|---|---|
| Drive | 0–36 dB | +12 dB |
| Tone | 1200–18000 Hz, logarithmic | 6500 Hz |
| Mix | 0–100% | 100% |
| Output | −24–+12 dB | −3 dB |

Low drive gives gentle coloration; increasing it progressively compresses and
saturates peaks. Tone rolls off brightness after saturation, while Mix retains
an adjustable amount of the original signal. Drive has a fixed compensation
curve of −0.5 dB per added drive dB, before the independent Output trim. This
moderates level jumps without attempting program-dependent loudness matching.
Start with the defaults, reduce Mix for subtle coloration, or increase Drive
for a more compressed overdrive.

## Signal path

The engine runs at 48000 Hz. Each side has its own state and follows:

1. An 18 kHz nominal one-pole input low-pass.
2. Drive gain, reduced by `1 / (1 + 0.12 * envelope)` for gentle sag.
3. An asymmetric soft-saturation stage biased by 0.2.
4. A 12 kHz nominal one-pole interstage low-pass.
5. A second soft-saturation stage, gain 1.15 and bias 0.08.
6. The variable Tone low-pass and a 16 Hz nominal DC blocker.
7. Drive compensation, Output gain and linear dry/wet mixing.

The envelope follows the first stage's absolute output, with approximately
20 ms attack and 150 ms release. Positive bias introduces even harmonics as well
as the odd harmonics of soft clipping. Filtering restrains high-order brightness;
DC blocking removes the offset produced by asymmetric saturation.

Each nonlinear stage uses the normalized curve
`f(x) = (tanh(x+b) - tanh(b)) / (1-tanh(b)^2)`.
Its antiderivative is evaluated in double precision with a stable `log(cosh)`
identity, and adjacent values are differenced to obtain first-order
antiderivative antialiasing. Nearly equal inputs use the curve at their midpoint
to avoid cancellation. Exact zero input/history returns exact silence.

The method follows the mathematical approach described by
[Parker, Zavalishin and Le Bivic, DAFx 2016](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf).
The implementation is original GPL-3.0-only C; no external source or dependency
was imported. Antiderivative antialiasing reduces aliasing and also attenuates
high frequencies; it does not eliminate aliasing. Each first-order stage has
approximately half a sample of small-signal averaging delay, in addition to the
filters' phase response. The dry path remains immediate and bypass stays unity
dry. There is no added block buffering or oversampling library.

This is a deliberately voiced tube-inspired processor, rather than a measured
model of a named tube, amplifier or physical circuit. Its bias, sag and filter
choices are LibreLoop's sound design. Real-device listening and comparison with
hardware have not been performed.

## Integration and real-time behavior

Drive, Tone and Output use saved parameter IDs 1401–1403; Mix uses the shared
FX mix target. All four support value entry, reset, automation and MIDI learn.
Their settings and targets follow FX reordering. Removing Tube cleans up its
automation and closes its editor. Presets use the existing chooser/container
with a new device ID; optional `x-tube.*` fields save project settings. Projects
without Tube retain their prior serialized representation.

Controls slew over 20 ms. Gain and filter targets are recalculated every 32
samples and interpolated every sample to avoid periodic coefficient jumps.
Bypass/removal fades the wet contribution toward zero with the same smoothing.
The processor uses **160 bytes** per instance on macOS arm64, inside the rack's
existing fixed-size union. It never allocates, frees, locks, waits, logs, or
accesses files in rendering. Reset clears only this bounded state. Stereo sides
share settings but have independent filters, nonlinear histories and envelopes.

The editor reuses existing surfaces, fonts, icons and knobs. No bitmap assets,
new rendering effects or separate GUI library were added. Incremental binary
size, per-frame rendering cost and real-device CPU/latency remain unmeasured.

## Verification and measured DSP checks

`-O3` macOS arm64 Release results from `tube_test`:

| Measurement | Test conditions | Result |
|---|---|---|
| Second harmonic | 1 kHz sine, amplitude 0.3, default controls | −24.71 dBc |
| Third harmonic | Same | −18.96 dBc |
| Ninth harmonic | Same | −69.49 dBc |
| Selected folded-harmonic reduction | 7 kHz sine, amplitude 0.5, Drive +24 dB, Tone 18 kHz, Output 0 dB | 18.63 dB relative to fundamental versus the same point-sampled model |

The last measurement sums power at 1, 6, 13 and 20 kHz and normalizes each
processor's result to its own 7 kHz fundamental. It is one reproducible test
case, not a universal alias-reduction figure or a hardware measurement.

Tests also check decreasing gain with increasing signal level, low DC offset,
exact dry bypass, stereo isolation, silence/reset, finite high-drive output,
smoothed parameter changes and exact 64/512-frame block determinism. Integration
checks cover project/preset roundtrips, parameter ownership, automation,
reordering, actual callback output, and agreement with offline WAV export.
The mixer test checks editor limits and add/remove behavior.

The full Release suite has 151 tests. Focused DSP, integration, actual callback
and mixer tests also run under AddressSanitizer and UndefinedBehaviorSanitizer.
Software screenshots are checked at 1448×1086 and 900×562. Native audio/MIDI and
pointer-device interaction, other CPUs/toolchains and subjective sound quality
still require hands-on acceptance.
