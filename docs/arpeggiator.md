# Shared instrument arpeggiator

Open an instrument, click **Voice**, then choose an Arpeggiator direction.
Sampler, DX7, Analog and Custom FM use the same controls and note generator.
The controls follow the Arpeggiator group in
[FL Studio's channel settings](https://www.image-line.com/fl-studio-learning-content/fl-studio-online-manual/html/chansettings_misc.htm)
and `references/screenshots/subwindow.png`; the implementation is LibreLoop's.

| Control | Behavior |
|---|---|
| Direction | Off, Up, Down, Up-Down, Bounce, Random |
| Time | 0.125–16 sixteenth-note steps; follows BPM |
| Gate | 1–100% of the interval |
| Range | 1–4 octaves, including the original octave |
| Repeat | 1–8 passes through a note group before changing octave |
| Slide | Preserve the sounding instrument state between connected arp notes; Glide sets pitch-transition time |
| Chord | Auto (sustain), Auto, None (octaves), Major, Minor, Major 7, Minor 7, Dominant 7, Sus 2, Sus 4, Diminished, Augmented |

Defaults are Off, one step, 80% gate, one octave, one repeat, Auto (sustain),
Slide off. **Auto (sustain)** leaves a single held pitch un-retriggered at Range
1. Hold a chord, increase Range, or choose **Auto** to hear repeated notes.
Finished Sampler one-shots are not restarted by Auto (sustain).

Time reuses Delay's logarithmic rhythm magnets, including triplets. Ctrl gives
fine unsnapped adjustment. Right-click allows numeric entry/reset. Range and
Repeat support drag, wheel and right-click whole-number entry. Up-Down visits
each endpoint once; Bounce repeats both endpoints. Random is repeatable after a
reset, without using a global random-number generator.

Auto modes sort held pitches and combine simultaneous live/sequenced notes for
the channel, retaining the latest velocity for duplicate pitches. Templates use
the most recently pressed source as their root. None transposes that root in
octaves. Notes above MIDI 127 are skipped. These ordering and clock rules are
LibreLoop's documented choices, not a claim of sample-identical FL Studio output.

The clock starts on the first held note, advances in musical steps, and preserves
its phase through BPM changes. Successive notes need not lie on integer sample
boundaries. Gate introduces genuine release gaps; use 100% gate for connected
Slide notes. Glide Off changes pitch immediately while Slide can still preserve
the envelope. Ordinary Mono/Legato and voice limits continue to apply.

Keyboard/MIDI, piano-key/note auditions and sequenced Pattern notes feed the same
engine. The arp also runs while the transport is stopped; routine UI updates do
not restart it. Releasing the last source releases its generated voice. Off
restores still-held live keys to their original input slots, so later key-up
continues to work. Transport stop, panic, seek/loop reset, source replacement
and channel-structure changes clear the relevant held state. Space preserves
mixer effect tails; Stop All Audio also clears those effects.

Piano-roll notes are not rewritten. Arrangement audio clips and Browser sample
file auditions bypass arpeggiation. Keyboard recording retains the played source
notes, so project playback applies the arp once rather than recording generated
notes and arpeggiating them again.

## Storage and real-time boundary

`src/engine/arpeggiator.c` owns held-note metadata and clock state, with fixed
128-entry source tables per Player. Generated notes use the existing sequencer
voice pool; live key slots are never borrowed for generated notes. On source
capacity exhaustion the oldest held entry is replaced. Generated voice pressure
steals a released or generated voice; it never takes an arrangement-audio voice.
There is no allocation, lock, I/O, platform/graphics API or global PRNG in this
path. With all arps off and no pending state, the render loop bypasses it.

`Project.arp[]` stores channel settings. They survive instrument replacement,
move with channel deletion, participate in undo, and use optional `x-arp.*`
fields. Missing fields use defaults. Default-only projects omit these fields,
keeping original fixtures unchanged. Instrument-only DSP presets do not store
channel arp settings. Arp automation/MIDI-learn targets are not added in this pass.

## Verification

The arpeggiator test checks chord order, both up/down variants, range/repeat,
templates, deterministic random, gates, sustain, finished one-shots, Slide,
Off/key-up, all four instruments, fractional/triplet block replay, persistence,
validation and channel deletion. Magnetic-knob tests cover Time's shared detents.
Callback audit mode 11 exercises live keys, preview notes, tempo changes and
repeated paused transport updates at 64/512 frames under the real-time guard.

The Release suite contains 172 checks. Engine/voice/audio tests and actual
callback capture mode 11 also run under ASan/UBSan. On this host, the sanitizer
runtime and the dyld-interposition guard abort when combined, including unchanged
control mode 8; sanitizer callback captures and Release guard audits are verified
separately. Native screenshots at 1448×1086 and 900×675 are reviewed for layout.
Physical MIDI, hands-on timing/feel, Linux and Windows remain unverified.
