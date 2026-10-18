# Tube saturation

Tube is a built-in stereo, tube-inspired saturator. Add it through a mixer
strip's `+` menu, then choose **Add Tube**. It uses four existing shared knobs
in a 480×220 logical-unit floating editor (minimum 440×220):

| Control | Range | Default |
|---|---|---|
| Voicing | Warm / Natural | Warm |
| Drive | 0–36 dB | +12 dB |
| Tone | 1200–20000 Hz, logarithmic, then None | None |
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

The engine runs at 48000 Hz. **Warm is the default for every newly added Tube**
and preserves the original sound. Saved instances remember their mode. Each
side has its own state. Warm follows:

1. An 18 kHz nominal one-pole input low-pass.
2. Drive gain, reduced by `1 / (1 + 0.12 * envelope)` for gentle sag.
3. An asymmetric soft-saturation stage biased by 0.2.
4. A 12 kHz nominal one-pole interstage low-pass.
5. A second soft-saturation stage, gain 1.15 and bias 0.08.
6. The variable Tone low-pass (or None to bypass it) and a 16 Hz nominal DC blocker.
7. Drive compensation, Output gain and linear dry/wet mixing.

Tone snaps to **None** when dragged or wheeled above 20000 Hz. The saved endpoint
is 22000, a bypass sentinel rather than a cutoff frequency. Raw drag/wheel motion
is accumulated so fine movements can leave the detent. Numerical values above
20000 also select bypass, including automated values. The fixed input/interstage
voicing filters remain active in Warm with Tone None.

**Natural** keeps the two asymmetric saturation stages, drive compensation and
sag, but omits those fixed input/interstage filters. It processes the nonlinear
stages at 8× sample rate, using three cascaded 2× up/down stages with original
24th-order Butterworth half-band filters and double-precision histories. Tone
and DC blocking follow downsampling. Tone None bypasses the final Tone filter
in either mode. Natural preserves much more treble; it still compresses and
changes harmonics, so it does not promise unchanged high-frequency content.
Mode changes crossfade over 20 ms. The oversampler runs only while Natural is
active or fading out; Warm histories remain live for switching back.

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
dry. There is no added block buffering or external oversampling library. Natural
adds frequency-dependent phase/group delay through its filters. The immediate
dry path is unchanged, so partial Mix can produce phase interactions; there is
no mixer delay compensation or zero-latency claim.

This is a deliberately voiced tube-inspired processor, rather than a measured
model of a named tube, amplifier or physical circuit. Its bias, sag and filter
choices are LibreLoop's sound design. Real-device listening and comparison with
hardware have not been performed.

## Integration and real-time behavior

Drive, Tone, Output and voicing use saved parameter IDs 1401–1404; Mix uses the
shared FX mix target. All five support value entry, reset, automation and MIDI learn.
Their settings and targets follow FX reordering. Removing Tube cleans up its
automation and closes its editor. Presets use the existing chooser/container
with a new device ID; optional `x-tube.*` fields save project settings. Projects
without Tube retain their prior serialized representation. `x-tube.natural`
stores 0 for Warm or 1 for Natural. Missing fields load as Warm. Tube presets
write container version 8; older Tube presets load as Warm, and other device
preset writers retain their previous versions.

Controls slew over 20 ms. Gain and filter targets are recalculated every 32
samples and interpolated every sample to avoid periodic coefficient jumps.
Tone interpolation lands exactly on its cached target at each boundary, so
None reaches a true unity filter coefficient rather than a nearly bypassed one.
Bypass/removal fades the wet contribution toward zero with the same smoothing.
The processor uses **2576 bytes** per instance on macOS arm64, inside the rack's
existing fixed-size union. It never allocates, frees, locks, waits, logs, or
accesses files in rendering. Reset clears only this bounded state. Stereo sides
share settings but have independent filters, nonlinear histories and envelopes.

The editor reuses existing surfaces, fonts, icons and knobs. No bitmap assets,
new rendering effects or separate GUI library were added. The arm64 Release executable is 5,722,112 bytes, an increase of 16,448 bytes
from the preceding build. Per-frame rendering cost and real-device CPU/latency
remain unmeasured.

## Verification and measured DSP checks

`-O3` macOS arm64 Release results from `tube_test`:

| Measurement | Test conditions | Result |
|---|---|---|
| Second harmonic | 1 kHz sine, amplitude 0.3, Drive +12 dB, Tone 6500 Hz, Output −3 dB | −24.71 dBc |
| Third harmonic | Same | −18.96 dBc |
| Ninth harmonic | Same | −69.49 dBc |
| Selected folded-harmonic reduction | 7 kHz sine, amplitude 0.5, Drive +24 dB, Tone 18 kHz, Output 0 dB | 18.63 dB relative to fundamental versus the same point-sampled model |

The last measurement sums power at 1, 6, 13 and 20 kHz and normalizes each
processor's result to its own 7 kHz fundamental. It is one reproducible test
case, not a universal alias-reduction figure or a hardware measurement.

Tests also check decreasing gain with increasing signal level, low DC offset,
exact dry bypass, Tone None persistence/detent/transition and increased high-end
response, stereo isolation, silence/reset, finite high-drive output,
smoothed parameter changes and exact 64/512-frame block determinism. Integration
checks cover project/preset roundtrips, parameter ownership, automation,
reordering, actual callback output, and agreement with offline WAV export.
The mixer test checks editor limits and add/remove behavior.

Warm matches a frozen pre-change 96,000-frame stereo render bit-for-bit on this
toolchain (SHA-256
`45a155c8e338096be47770857d1d7aa0a86a930217bc66d022cfca7b6ebf7b88`).
With amplitude 0.001, Drive/Output 0 dB and Tone None, Natural measured −0.057 dB
at 10 kHz and −0.209 dB at 19 kHz relative to 1 kHz; Warm measured −24.552 dB
at 19 kHz. A separate 19 kHz, amplitude 0.5, +24 dB Drive case measured 66.88 dB
less selected folded-harmonic power (1, 9, 10 and 20 kHz, each normalized to its
own fundamental) than the point-sampled biased-stage model. This is a selected
case, not an alias-free claim. Tests also verify Natural's Tone filtering, mode
changes across callback blocks, mode automation/MIDI and old preset loading.

A local optimized DSP-only probe processing five seconds of stereo audio used
0.0573 process CPU seconds for Warm (1.15% of one core) and 0.6371 for Natural
(12.74%). This is one-instance throughput on this machine, not a real-time
callback deadline, battery, or whole-app CPU measurement.

The full Release suite has 158 tests. Focused DSP, integration, actual callback
and mixer tests also run under AddressSanitizer and UndefinedBehaviorSanitizer.
Software screenshots are checked at 1448×1086 and 900×562. Native audio/MIDI and
pointer-device interaction, other CPUs/toolchains and subjective sound quality
still require hands-on acceptance.
