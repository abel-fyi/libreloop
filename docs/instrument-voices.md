# Shared instrument voices

The Voice button beneath the instrument preset opens one channel-owned page for
Sampler, DX7, Analog and Custom FM. Sound returns to the synthesis/sample page.
The compact grouped layout follows `references/screenshots/subwindow.png`, using
Luice's existing materials, bitmap font, knobs and checkbox controls. Channel
pan, volume, playback pitch, range and routing remain in the common header;
the Voice page does not repeat them. Its compact outer height is 288 logical units.

The page implements:

- **Max**: 0 (shown as All) preserves the existing pool capacities. Values 1–128
  cap the channel's combined live/sequenced voices, including release tails;
  the oldest voice is replaced first. Drag or wheel the value; right-click for
  whole-number entry. Mono overrides this limit.
- **Mono**: last-note priority across live input and playback. Older held notes
  remain available; releasing the foreground note returns to the newest note
  still held. Sequenced note lengths supply their corresponding releases.
- **Legato**: in Mono mode, overlapping notes retain oscillator/filter/sample
  and envelope state instead of restarting the instrument. Non-overlapping
  notes start normally. For Sampler this also preserves sample position.
- **Glide**: in Mono mode, overlapping notes move linearly in semitone space
  over a constant 0–2 second interval. Zero switches immediately. Returning to
  an older held note also glides. Legato independently controls retriggering.
  Sampler Stretch retains its existing duration-preserving pitch processing.
  Note pitch is applied separately from channel/master control smoothing, so
  Glide Off adds no hidden 20 ms pitch transition. DX7 still renders in its
  existing 64-sample control blocks.

Defaults are unrestricted Poly, Legato off and Glide off. The unchanged default
DSP fixtures remain the reference. Arrangement audio clips bypass this voice
policy; channel settings affect played instruments, not clip timing.

`VoiceSettings` belongs to the channel, outside synth-specific settings. Shared
settings survive instrument replacement and move with a channel when another
channel is deleted. Optional `x-voice.*` project fields store the four values;
missing fields default to zero. Instrument-only preset files do not store these
channel settings. Max, Mono, Legato and Glide are not automation/MIDI targets in
this pass. Existing level/pitch controls keep their existing automation support.

`src/engine/voice.c` contains the shared policy with fixed voice storage and
bounded scans. No graphics/platform APIs, allocation, locking, I/O or logging
are used. `engine.c` applies it to sequenced playback and the actual mixer;
`audio_miniaudio.c` also registers keyboard/MIDI and note-preview events. Note
ordering is synchronized between the live and sequenced pools. The default
policy bypasses voice-policy processing in the sample loop.

The reference's modulation X/Y, gate/shift/swing transformations, cut groups,
root-key editor, echo/fat mode and keyboard strip are deliberately absent.
Arpeggiation remains a separate development task. No inactive decorative controls
have been added to suggest these features exist.

## Verification

`voice_test` exercises Mono priority, older-key release, held-note return,
legato/retrigger state, exact glide endpoint, voice stealing, combined live and
sequenced pools, Sampler state continuity, all three synth families through the
mixer, project round trips, channel deletion and identical rendering at 37-frame
versus whole-buffer boundaries. Callback audit mode 8 adds overlapping live
notes and releases with Mono/Legato/Glide, under the real-time allocation/lock
guard at 64/512 frames. Original expected-result fixtures are unchanged.

Development smoke scenes 29–32 show Analog, DX7, Sampler and Custom FM on the
shared page. macOS SDL captures at 1448×1086 and a 900×675 Sampler capture were
reviewed for clipping and grouping. Physical MIDI, hands-on voice transitions,
Linux/Windows behavior and perceived glide feel still need auditioning.

Rapid key-up/key-down commands in one callback and repeated key-slot reuse are
covered with an older held note throughout, both within one pool and across the
live/sequenced pools. Legato retains the foreground envelope and LFO state.
Analog slot replacement keeps a 5 ms overlap with its retiring voice using a
free slot in the fixed pool; the tail is excluded from note priority/voice-limit
counts. No extra pool is allocated. At full pool capacity there is no spare
slot for this overlap.

See [instrument fixes](instrument-fixes.md) for the current regression and
platform-verification results. These checks establish deterministic behavior,
not perceived glide quality or hardware latency.
