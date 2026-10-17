# Built-in Delay

Add Delay from a mixer strip's `+` effect menu, on an insert or Master. Its
floating editor uses the existing controls and can be stacked with other effect
editors. It participates in the same nine-slot chains, bypass, drag reordering,
project history, project saving and preset chooser as the other built-in effects.
No dependency or external plugin format was added.

## Controls

- Time: 1–4000 ms in free mode. Tempo sync replaces this knob with the effective
  millisecond readout; the saved free time remains available when switching back.
- Division: click to cycle through 1/32, 1/16 triplet, 1/16, 1/8 triplet, 1/8,
  dotted 1/8, 1/4, dotted 1/4 and 1/2. Values are ordered by increasing duration.
  Tempo sync uses `60000 / BPM * quarter-note beats`, bounded to 1–4000 ms.
- Feedback: 0–95%. Tone: a one-pole low-pass at 100–20000 Hz in the wet/feedback
  path; 20000 Hz bypasses filtering. Ping-pong blends independent stereo
  feedback (0) into cross-channel feedback (1).
- Mix: linear dry/wet, 0–100%. Output: wet gain, 0–2; bypass remains unity dry
  regardless of this control. Default mix is 35%, feedback 35%, tone 8000 Hz,
  independent stereo, unity wet output and tempo-synced dotted eighths.

Free time, feedback, tone, ping-pong, wet output and the shared effect mix have
stable automation/MIDI targets. Sync and division are saved mode choices.
Targets follow FX reordering and delay automation is removed with its effect.
As with existing effects, export keeps the song's current length rather than
appending an extra effect tail.

## Processing and ownership

The processor uses the engine's fixed 48000 Hz sample rate. Time changes crossfade
between two fixed read taps over 20 ms; other continuous controls approach their
new values with a 20 ms one-pole slew. Filter coefficients update every 32 samples.
Changes received during a tap crossfade start a subsequent crossfade when the
current transition finishes. Reset invalidates history with constant-size metadata
work; it never clears the full PCM ring in the callback. Bypass/removal fades wet
level toward zero using the same control smoothing.

Each prepared delay position owns **1,536,096 bytes** on this macOS arm64 build,
including a stereo four-second float ring. Allocation and page touching happen
on the UI/control thread before its project snapshot is published, or on the
export thread before rendering. A lock-free atomic pointer publishes initialized
storage. The callback exclusively mutates DSP state and never allocates, frees,
locks, waits, logs or reads files for this effect. Existing command acknowledgments
and device shutdown protect rack destruction.

Prepared buffers are cached per bus/slot until the rack is destroyed, including
positions from which a delay was removed or moved. Repeated edits at that position
reuse its buffer; moving through more positions reserves additional buffers.
No delay buffers are allocated for a fresh project without delays. If allocation
fails, startup/export fails; an already running app reports the shortage in its
status line and an unprepared delay passes dry. This cache policy favors simple,
nonblocking audio ownership over reclaiming removed buffers during playback.

Project delay settings use optional `x-delay.*` fields, emitted only when a Delay
exists. Loading projects without those fields initializes default delay settings;
projects without Delay retain their prior serialized representation. Delay
presets use the existing version-6 preset container with a new built-in device ID.

## Verification

`delay_test` checks exact impulse/feedback/ping-pong timing, tempo divisions,
reset without stale history, maximum ring wrap, finite maximum-feedback output,
time-change continuity and exact determinism across 64/512-sample blocks.
`delay_integration_test` checks registry/parameter ownership, project and preset
roundtrips, repeated preparation without losing history, bus resets, unity bypass,
automation, target reordering and PCM agreement with WAV export. `audio_test`
checks exact echoes through the actual callback and preserves history across an
unchanged project update. `mixer_slots_test` checks adding/opening/removing Delay,
minimum editor geometry and automation cleanup.

The macOS arm64 Release build passes all 148 CTest cases, including the frozen
reference comparisons under their existing approved Stretch contract. The delay
core, integration, actual callback and mixer-slot tests also pass AddressSanitizer
and UndefinedBehaviorSanitizer. The rebuilt app passes strict bundle signature
verification.

Software-rendered editor captures were inspected at 1448×1086. Real-device
listening, native input, CPU/latency measurements and other platforms have not
been verified for Delay yet.
