# Phaser and Flanger

Add either effect from a Mixer strip's **+** menu. Both use the existing compact
knob editor, file chooser, preset format, automation/MIDI mapping and bypass.
Default Mix is 50%, which combines dry and shifted signals to create notches.
Neither effect adds a delay to the direct path or requires compensation latency.
They are original GPL-3.0-only implementations, with no new dependencies or assets.

## Phaser

| Control | Range | Default |
|---|---|---|
| Rate | 0.01–10 Hz, logarithmic | 0.35 Hz |
| Center | 80–8000 Hz, logarithmic | 900 Hz |
| Depth | 0–4 octaves | 2 octaves |
| Feedback | −90…+90% | +35% |
| Stereo | 0–100% LFO spread | 50% |
| Stages | 2–12, integer | 6 |
| Mix | 0–100% | 50% |

The wet path is a cascade of first-order allpass sections. Combining it with dry
creates the phaser notches; 100% wet with zero feedback has an allpass magnitude
response at a static frequency, rather than the same notched response as 50% Mix.
Center is the logarithmic sweep center; Depth varies it by ±that many octaves.
The break frequency is bounded to 20–18000 Hz. Stages changes notch density.
Signed feedback changes resonance and voicing. Stereo changes LFO phase spread,
not a hard pan or synthesized input: at zero spread identical input channels
produce exactly identical output channels.

**Warm Sweep** is the default. **Deep Phase** is slower, eight-stage and centered;
**Stereo Motion** uses twelve stages, negative feedback and full phase spread.
Load creates these presets on first use without overwriting existing files.

## Flanger

| Control | Range | Default |
|---|---|---|
| Rate | 0.01–10 Hz, logarithmic | 0.25 Hz |
| Delay | 0.2–12 ms, logarithmic | 2.5 ms |
| Depth | 0–100% | 80% |
| Feedback | −95…+95% | +55% |
| Stereo | 0–100% LFO spread | 25% |
| Invert wet | Normal / inverted | Normal |
| Mix | 0–100% | 50% |

A short continuously modulated delay produces the comb-filter sweep. Depth is
relative to Delay, keeping the minimum delay at 0.05 ms or more; at maximum
settings the delay reaches 23.95 ms. Signed feedback controls resonance. Invert
Wet reverses the delayed path's polarity and changes the comb pattern. The
polarity transition is smoothed. Zero spread keeps centered input centered.

**Classic Jet** is the default. **Slow Sweep** uses a wider, slower motion with
higher feedback. **Hollow Flange** uses inverted wet and negative feedback with
a centered image. These are available through the existing Load chooser.

## Implementation and real-time ownership

Both run at the engine's 48 kHz rate. Saved controls, wet mix and polarity slew
with a 20 ms time constant. LFO phases advance per sample and wrap at 2π. Ear
phases are symmetric about the common phase, starting at the same instantaneous
sweep value. Spread 100% separates the LFOs by 180°; zero is identical modulation.
No random generators, device clocks, allocation, locks, logging, or file I/O are
used during processing.

Phaser's allpass section uses `y = a*x + state`, `state = x - a*y`, with
`a = (tan(π*f/48000)-1)/(tan(π*f/48000)+1)`. Its coefficient follows the sweep
with a 32-sample time constant. All twelve sections run continuously; a smoothed
stage count interpolates neighboring output taps when changed. Phaser occupies
168 runtime bytes on the verified arm64 ABI, fitting inside the rack's existing
union. This is a general phaser design, not a model of a particular pedal.

Flanger has a 2048-frame stereo ring (16,448 total runtime bytes on this ABI).
Central four-point Lagrange interpolation reads its fractional delay. Page
reservation/allocation happens through `effects_prepare` on control/export
threads; an atomic pointer publishes the prepared state. Reset clears metadata
and uses valid-age checks instead of clearing or reading uninitialized PCM.
State is cached until rack destruction, including after moving a Flanger to a
new slot. It is allocated only for slots that have used a Flanger.

Feedback in both effects includes a 20 Hz DC blocker on the feedback branch;
the direct and wet output paths are not low-passed. The network input is scaled
by `1-abs(feedback)` to bound resonance gain. This is part of their voicing:
turning feedback up does not simply add uncontrolled output gain. No hidden
limiter or saturation is used. Existing Space behavior lets histories ring;
Stop All Audio clears them. Removing an effect fades its wet contribution.

Stable saved effect IDs append Phaser and Flanger. Numeric parameters occupy
1701–1706 and 1801–1806, with existing shared Mix. Optional project fields
`x-phaser.settings` and `x-flanger.settings` are omitted unless used. Defaults
initialize missing fields. Reordering carries settings, automation/MIDI targets
and editor identity; DSP history resets on the affected bus as before. Parameter
address lookup uses the concrete settings layout. Automation takes sparse
copies of only affected settings, avoiding large callback-stack arrays.

Algorithm background: Julius O. Smith's
[Phasing with First-Order Allpass Filters](https://www.dsprelated.com/freebooks/pasp/Phasing_First_Order_Allpass_Filters.html).
No code was imported from that reference.

## Verified and unverified

Release macOS arm64 build and complete regression suite: **157 tests passed**.
Focused ASan/UBSan runs cover modulation DSP, integration, and the real callback.
Additional tests check actual Mixer add/open/remove controls and automation cleanup.
Software-rendered editors were inspected at 1448×1086 and 900×562. Neither
editor introduces textures, animation effects or bitmap assets.

Checks include exact silence and initial bypass, centered stereo at zero spread,
nonzero stereo modulation at default spread, analytic notch positions, exact
static-delay timing, wet inversion, reset without leaked history, 64/512-frame
replay agreement, smoothed edits, and finite/decaying high-feedback behavior for
both signs at minimum and maximum sweep centers/delay. Prepared engine rendering
and actual callbacks match standalone DSP bit-for-bit on this toolchain;
WAV export matches within one int16 unit. Project/preset roundtrips, all parameter
targets, automation and FX/MIDI reordering are exercised. Static analysis and
runtime callback guards remain in the existing regression suite.

The controlled static tests measured Phaser's 900 Hz six-stage notch at −76.47 dB
and its 100% wet allpass magnitude within 0.001 dB of unity. With a static 10 ms
Flanger delay, the 50 Hz notch was −102.95 dB and the 100 Hz passband was within
0.001 dB of unity. These are software fixture measurements, not hardware models
or listening-quality scores.

Native-device audition, input feel, latency/deadlines, idle/active CPU/GPU,
other platforms and other CPUs remain unverified. No whole-app performance
improvement is claimed. The existing system-SDL deployment-version linker
warning remains. There is no tempo-sync mode in this initial pair of effects.

Final focused sanitizer coverage includes **four passing tests**: modulation,
modulation integration, Mixer slots and audio callback. Extra callback assertions
verify that Space preserves both processors' histories and Stop All Audio clears
them. Strict app signature verification and `git diff --check` pass.

The Release executable is 5,705,664 bytes versus 5,589,472 bytes before this pair,
a measured increase of 116,192 bytes for the complete integration. This is not
an isolated per-effect size measurement. Per-frame drawing cost and native CPU
cost have not been measured reliably; no performance estimate is presented as
a measurement.
