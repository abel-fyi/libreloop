# Stereo Reverb

Add **Reverb** through a mixer strip's `+` menu. It is an original algorithmic
stereo reverb, voiced for a dense, moving tail and bright percussion. The single
row of shared knobs opens at 760×220 logical units, with a 640×220 minimum.

## Controls and presets

| Control | Range | Default |
|---|---|---|
| Size | 0–100% | 55% |
| Decay | 0.15–20 seconds, logarithmic | 2.4 s |
| Damping | 500–20000 Hz, logarithmic; maximum is Open | 18000 Hz |
| Pre-delay | 0–250 ms | 18 ms |
| Width | 0–200%; 0 is mono | 100% |
| Modulation | 0–100% | 30% |
| Gate | Off at zero, otherwise 0–1000 ms wet gate hold | Off |
| Mix | 0–100% linear dry/wet | 25% |

Damping affects the feedback, so lower settings shorten high-frequency decay.
Open bypasses this damping filter. Size changes the network's time scale; Decay
sets a nominal low-loss T60 rather than a guaranteed measured tail length.
Interpolation, damping, early reflections and modulation affect the measured
result. Width changes only wet mid/side balance and leaves its mid component
intact. Modulation gently varies delay lengths to keep a static ringing pattern
from dominating. Mix zero/bypass keeps the immediate dry signal unchanged.

Click **Load** to access four factory presets, created in the existing local
Reverb preset directory on first use without overwriting existing files:

- **Bright Hall:** the natural default.
- **Bright Snare Gate:** 38% size, 4 s decay, Open damping, 8 ms pre-delay,
  130% width, 20% modulation, 260 ms gate, 50% mix.
- **Small Room:** 18% size, 0.65 s decay and restrained modulation.
- **Wide Space:** 85% size, 8 s decay and wider, more modulated ambience.

For the requested bright gated-snare sound, start with Bright Snare Gate. The
wet gate retriggers while the high-passed input exceeds −48 dBFS. Its hold is
measured from the **last** above-threshold sample, with pre-delay included in
the countdown. It uses a 2 ms opening ramp and a smooth release around 10 ms.
Sustained input keeps it open. The dry signal is unaffected and the internal
reverb continues decaying while the gate is closed. This is an input-driven
gate; its timing depends on the source tail and level. Very short holds can
close before useful reverberation has developed.

## Algorithm and voicing

The engine runs at 48000 Hz. Its wet signal path is:

1. A 20 Hz input DC blocker, then an interpolated stereo pre-delay.
2. Four serial allpass diffusers per side, with different channel delays.
3. Eight modulated feedback delay lines, mixed by a normalized 8×8
   Walsh-Hadamard matrix, with per-line damping and decay gains.
4. Different orthogonal stereo pickup vectors and two output allpasses per side.
5. Twelve short stereo early-reflection taps, added to the late field.
6. Wet-only mid/side width, the optional gate, and dry/wet mixing.

The base feedback lengths are `1493,1601,1747,1867,1999,2137,2281,2437` samples.
Size scales these by `0.5 + 1.5*size`. Feedback gains use
`10^(-3 * length * scale / (48000 * decay))`.
The normalized matrix preserves vector energy before the losses. Each line's
modulation has its own oscillator, from 0.071 to 0.197 Hz, at up to ±12 samples.
Double-precision oscillator state is periodically normalized. Central four-point
Lagrange interpolation reduces the high-frequency loss of modulated delay reads
compared with linear interpolation; it still has frequency-dependent loss.

Diffuser delays are `{143,211,347,557,421,683}` samples on Left and
`{163,239,373,601,449,709}` on Right. Input allpass gain is 0.72; output gain is
0.6. Early-reflection times are `{5,9,14,22,35,51}` ms and
`{7,12,18,29,43,61}` ms before the same size scale and pre-delay. Their differing
time/polarity patterns help spread a mono source into stereo.

Size and pre-delay changes crossfade old/new read taps for 20 ms, including
early reflections. Continuous gains, damping, width, modulation and wet level
are smoothed. Decay/damping targets update every 32 samples with per-sample
interpolation. A further size/pre-delay change starts after the active crossfade.
There is no nonlinear clipper, saturation, bit reduction or forced dark voicing.

The owner's references were Valhalla's
[Supermassive](https://valhalladsp.com/shop/reverb/valhalla-supermassive/) and
[VintageVerb](https://valhalladsp.com/shop/reverb/valhalla-vintage-verb/).
Valhalla describes Supermassive as FDN-based and VintageVerb provides multiple
algorithms, including nonlinear/gated sounds. LibreLoop's topology and tuning
are its own implementation. No Valhalla code, presets or artwork were imported,
and sound equivalence has not been measured. The fractional-delay method is
explained in Julius O. Smith's
[Physical Audio Signal Processing](https://www.dsprelated.com/freebooks/pasp/Lagrange_Interpolation.html).
No dependency was added.

## Ownership, persistence and real-time behavior

Each prepared instance occupies **508,480 bytes** on macOS arm64. Fixed rings
hold feedback lines, diffusion and pre-delay/early-reflection history. The UI
control thread allocates and page-touches storage before publishing a project
snapshot; offline export prepares its own rack before rendering. A lock-free
atomic pointer publishes initialized instances. The audio callback exclusively
updates their state, with no allocation, freeing, locks, waits, file I/O or logs.

Reset only invalidates history/metadata and never clears the full PCM storage
on audio. Valid-history counts protect unwritten data, including all four
interpolation taps. Buffers are cached per bus/slot until rack destruction,
following the existing Delay policy. Moving through additional positions reserves
more storage. Preparation failure is reported; unprepared effects pass dry in
an existing session, while startup/export fail rather than render partially.

All seven reverb controls and shared Mix have stable automation/MIDI targets.
IDs 1501–1507 identify the reverb controls. Settings and targets follow chain
reordering; removal cleans up automation and closes the editor. Optional
`x-reverb.*` fields persist settings; projects without Reverb keep their prior
serialized representation. Presets use the existing container and chooser.
As with existing effects, export ends at the current song length and does not
append an additional reverb tail automatically.

## Verification

The macOS arm64 Release suite has 153 tests. Focused reverb DSP, integration,
actual callback and mixer checks also run under AddressSanitizer and
UndefinedBehaviorSanitizer. Software editor captures are inspected at 1448×1086
and 900×562, including Bright Snare Gate.

For a stereo unit impulse, size 55%, decay 2.4 s, Open damping, zero pre-delay,
no modulation and unity width/mix, the Release DSP test measures:

| Metric | Result |
|---|---|
| L/R correlation, 100–500 ms | −0.002 |
| Samples above combined magnitude 1e−6, 100–500 ms | 100% |
| Decay estimate from residual-energy slope, 300–1300 ms | 2.259 s |
| Peak absolute output sample | 0.157 |
| Late-tail difference-energy / signal-energy ratio, Open vs 3 kHz damping | 0.672 vs 0.015 |

These are controlled software measurements, not listening scores. The test also
checks finite stable output through a 30-second maximum-decay/modulation case,
falling tail energy, exact dry bypass, mono width, gate closure, pre-delay,
silence after reset, instance isolation and exact 64/512-frame replay results
through parameter changes. Integration checks compare prepared realtime rendering
and WAV export, preserve history through repeated preparation, and exercise
project/factory-preset roundtrips, automation, reordering and actual callback
samples/tails. Mixer tests cover add/open/minimum geometry/remove behavior.

Native-device listening and CPU/latency, pointer-device interaction, other
platforms/toolchains and comparisons with Valhalla remain unverified. The UI
adds no bitmap assets or new rendering effects; incremental binary size and
per-frame rendering cost have not been measured.
