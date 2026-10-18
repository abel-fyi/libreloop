# Polyphonic Analog and dedicated synth identities

LibreLoop exposes Sampler, DX7, Analog and Custom FM as separate instrument
choices. DX7 retains the existing adapted six-operator core and its notices.
Custom FM retains the three-operator engine. New Analog patches use the voice
described here. Existing Analog patches retain the original DSP model and appear
as Analog (Legacy); opening a project does not upgrade its sound.

The stable saved synth tag and FMSettings container remain shared internally.
Instrument identity comes from the synth family and Analog model fields. This
keeps existing projects and native DX7 parameter IDs readable without duplicating
the sequencer, note allocation or channel/mixer interfaces. The engine remains
independent of graphics and windowing APIs.

## Analog voice

The voice is original GPL-3.0-only C in `src/engine/analog.c` and
`src/engine/fm_synth.c`. It is inspired by the oscillator/mixer/ladder/envelope
signal path; it has not been calibrated against a particular Moog circuit or
VK-1 recording.

- Three independent oscillators have continuously morphable saw/pulse/triangle/
  sine shapes, semitone tuning, fine tuning and mixer levels. Saw and pulse use
  polynomial edge correction. Each note owns oscillator phases, drift state,
  filter state and envelopes. Drift uses deterministic voice-local randomness.
- Mixer levels sum before drive; increasing drive changes the operating point
  of the nonlinear filter. There is no automatic normalization of oscillator
  levels. Noise, pulse width and per-voice PWM remain available.
- A four-stage ladder uses nonlinear current differences at every stage. Its
  trapezoidal integrators and present-sample resonance feedback form four
  simultaneous equations. Three fixed Newton iterations solve their cyclic
  bidiagonal Jacobian; there is no unbounded convergence loop. The bounded
  rational tanh approximation has a matching analytic derivative.
- Oscillators and ladder run at 192 kHz. Two normalized, Hamming-windowed 19-tap
  half-band FIR stages decimate to the engine's 48 kHz rate. Unconsumed FIR
  outputs are skipped without changing the generated audition WAV bytes.
- High resonance can self-oscillate with the mixer muted. A deterministic
  thermal-scale initial perturbation starts that unstable equilibrium. A DC
  blocker removes offset. Resonance has no automatic bass/loudness compensation.
- Amp and filter ADSRs are separate and exponential. Attack reaches its peak at
  its nominal time; decay is approximately 99% toward sustain at its nominal
  time. Release reaches approximately −80 dB relative to its starting value at
  the nominal time, then continues to a finite silence threshold. Graphs show
  the new envelope shapes; editing retains the established handle math.
- Cutoff, resonance, drive, oscillator settings and other continuous timbre
  controls are smoothed. New parameter IDs occupy 2201–2215 and use the existing
  automation/MIDI interface. Audio rendering adds no allocation, locks or I/O.

Chorus is off in all three new factory patches: Warm Brass, Round Bass and Soft
Pad. Core stereo handling is unchanged; voices are panned/mixed by the DAW.

## Projects, presets and replacement

New settings are optional `x-analog.*` project fields. Missing fields are zero,
which selects the original model; all-zero extension fields are omitted. Modern
Analog presets use native preset version 9 and append model/oscillator settings.
Earlier preset versions remain readable. Other device preset encodings are
unchanged.

The chooser and factory lists keep synth families distinct. Each family has its
own preset directory. Loading another synth family's preset is rejected with a direction to use
Replace instrument first. Explicit replacement preserves notes, sample data,
channel controls and routing; incompatible synth automation and MIDI links are
removed. Shared channel volume/pan controls retain their IDs and behavior.

## Verification

All 161 Release tests pass, including frozen legacy audio fixtures and the new
64/512-frame callback cases under the real-time allocation/lock guard. The new
voice test also passes AddressSanitizer and UndefinedBehaviorSanitizer.

The new test covers simultaneous notes with independent voice state, exact
4096-frame versus 37-frame-block output (also with automated drive), parameter influence measured as sample differences, separate envelope state,
release completion, parameter/MIDI ownership, project/preset round trips,
replacement and finite output at extreme frequency/cutoff/resonance/drive.
Muted-mixer self-oscillation is checked independently.

A temporary 81-case numerical comparison used 20/1000/20000 Hz cutoff,
0/6/24 dB drive, 0/0.5/0.95 resonance and 40/440/10000 Hz oscillator frequency.
Three Newton iterations versus eight produced maximum sample error
0.00000739097595 and RMS error 0.000000248151066. This checks the numerical solve,
not fidelity to a hardware instrument.

Software captures cover the chooser, dedicated DX7 editor and all four new
Analog pages. Native MIDI/audio hardware, listening comparisons with VK-1 or a
physical synth, and Linux/Windows execution remain unverified. Hardware-perfect
DX7 emulation is not newly claimed; its established core is preserved.

## DSP cost

Apple M4, Apple Clang 21.0.0, macOS 15.4 SDK, Release `-O3 -DNDEBUG`.
`analog_modern_test --benchmark` renders one audio second at 48 kHz, with chorus
off. Three repetitions give the following median **CPU seconds per audio
second**, excluding the UI, audio driver, effects and project synchronization:

| Voices | Legacy Analog | New Analog | New Analog, share of one CPU core |
| --- | ---: | ---: | ---: |
| 1 | 0.002506 | 0.016841 | 1.68% |
| 4 | 0.010161 | 0.067213 | 6.72% |
| 16 | 0.042168 | 0.268181 | 26.82% |

New-Analog ranges were 0.016673–0.017456, 0.066796–0.067899 and
0.267589–0.269170 respectively. These are offline DSP measurements, not live
callback deadline or end-to-end latency guarantees. Full-session performance
with heavy effects and higher polyphony has not been measured.

## Auditions

Run `build-sdl-validation/analog_modern_test --renders` from a scratch directory.
It writes two-second, stereo 48 kHz PCM16 chords using the real sequencer/render
path. Build-directory copies are in `build-sdl-validation/analog-auditions/`.

| File | Patch | Peak amplitude | SHA256 |
| --- | --- | ---: | --- |
| analog-0.wav | Warm Brass | 0.19128418 | a3795238d7f93e77834d6dccb72425dd384f27a90d2a53ccc7e6cace93b7afbc |
| analog-1.wav | Round Bass | 0.17373657 | 4b99cd47ad6572817a47326534d1203741fea9639ccab23b3c76a4c5302db361 |
| analog-2.wav | Soft Pad | 0.13134766 | c734f6e97711146b33e6ba37deab01aadbdc5eadc83750fefd6fcca4dfd0a2ed |

These are candidate sound references for listening, separate from the untouched
legacy reference fixtures.

## Design references

[Blamsoft's VK-1 description](https://blamsoft.com/vst/vk-1-viking-synthesizer/)
provides the listening/design direction: oscillator variation, nonlinear ladder
filters and envelope character. The new engine is independently implemented.

[Huovilainen, DAFx 2004](https://www.dafx.de/paper-archive/2004/P_061.PDF)
analyzes the ladder's nonlinear differential equations.
[Zavalishin, The Art of VA Filter Design](https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf)
discusses trapezoidal integration and nonlinear zero-delay feedback. These are
mathematical references, with no source code imported from them and no new
runtime or build dependency.
