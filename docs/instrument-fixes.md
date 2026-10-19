# Instrument playback and display corrections

## Completed

- The Voice page contains Polyphony and Portamento only, at 288 logical units
  tall. Pan, volume and pitch remain in the common header. Duplicate Levels and
  Pitch panels have been removed. Arpeggiation remains deferred.
- Shared note glide is a separate ratio from channel/master tuning. Its 0–2 s
  ramp controls the note transition without a second 20 ms synth slew; existing
  channel/master control smoothing is retained. DX7 keeps its 64-sample block
  processing, so pitch changes can wait for the next control block.
- A key-up followed by key-down before another audio sample cannot restart the
  envelope while another key stays held. Foreground state is preserved before a
  live key slot is overwritten, including overlap across playback/live pools.
- Replacing an existing Analog voice in a key slot retains its old DSP output
  under a 240-sample (5 ms) fade. This avoids cutting a nonzero waveform to zero
  when the replacement has a slow attack. It uses an available slot in the
  fixed pool; it cannot create another slot when the pool is full. This fixes a
  specific discontinuity rather than changing oscillator/filter tone or adding
  a blanket fade/filter to every attack.
- Browser and ordinary Sampler note auditions enter Master after the insert
  sums and before Master effects, gain, pan, width and meters/recording taps.
  Master automation applies through the existing mixer. Browser auditions and
  the ordinary note-preview path still go directly to Master; normal instrument
  playback and shared-policy note previews retain their channel insert routing.
  The separate metronome monitor path is unchanged.
- Motion plots show an integer number of complete waveform cycles, at least
  two, including at the minimum LFO rate. Pitch modulation amount is shown as
  plain cents (`100 ct`), never scientific notation.
- Studio ADSR plots use one linear seconds scale across attack, decay and
  release. The illustrative sustain interval is explicitly described in hover
  help. Analog exponential contours remain curved; their nominal decay time
  represents settling, not an instantaneous arrival at the sustain asymptote.
  DX7 stage durations are computed from its actual rate/level equations at C4,
  velocity 100, including rate scaling and hold behavior. Their vertical axis
  remains the native level parameter. The same display frame, guides, fill,
  handles and readable time-axis labels are used for both models.

## Geometry fixture changes

Studio envelope positions now come from seconds rather than logarithmic thirds
or one fixed slot per DX7 stage. A captured seconds span and stage start are
held throughout dragging. ADSR dragging converts horizontal distance to seconds;
DX7 dragging chooses the native rate whose stage duration best matches that
horizontal distance. Vertical sustain/level controls retain their parameter
meaning and 14×14 logical hit boxes. The legacy/debug view retains its original
geometry and formulas.

`studio_fm_graph_test` now checks the actual drawn width ratio of a 10 ms decay
and 1 s attack, captured parameter-group ownership, unchanged DX7 parameters when
dragging to the same point, level direction and unrelated-parameter preservation.
DX7 fill counts increase from eight to ten triangles because the held sustain
interval is now a separate segment. This is a geometry update, not a weakened
sound or interaction assertion. Original expected-result files are unchanged.

## Verification

- `voice_test`: rapid same-buffer release/repress while holding an anchor,
  repeated slot reuse, cross-pool continuity, immediate Glide Off, existing
  block-size determinism, and the first output sample at Analog slot reuse
  compared with the continuing old voice. The retiring overlap ends at 240
  samples.
- `dx7_test`: preview timings agree with real envelope stage advances within
  two 64-frame control blocks, including pitch envelopes. Slowest-rate preview
  calculation remains bounded rather than rendering minutes of audio.
- `preview_routing_test`: the actual callback produces different audio through
  a Master phaser, exact half output at half Master gain, zero with Master mute,
  and identical wet renders at whole-buffer/37-frame sizes, for Browser and
  ordinary Sampler auditions. Master analyzer taps include those frames.
- Callback audit mode 8 includes same-buffer key-up/key-down over a held key.
  Modes 9 and 10 cover Master preview effects and Analog voice retirement at
  64/512 frames under the real-time allocation/lock guard.
- SDL app captures review the compact Voice page, Analog and DX7 envelope plots,
  and the complete two-cycle Motion preview at 1448×1086.

Physical MIDI, hands-on rapid playing, perceived click removal and glide quality,
Linux/Windows execution, hardware latency and native HiDPI behavior remain
unverified in this pass. The automated sample-continuity check does not establish
that every possible audible click has been eliminated.

The macOS Release build passes **169/169** tests. The four focused tests
(`voice`, `dx7`, `studio_fm_graph`, `preview_routing`) also pass AddressSanitizer
and UBSan. Original frozen audio/state/project/export comparisons pass without
changing expected files or increasing tolerances. The compact Voice page was
also reviewed at 900×675.

The 64- and 512-frame guarded callback captures match exactly for each new case:

| Mode | SHA-256 of stereo float PCM |
| --- | --- |
| 8, rapid legato | `3107fc7bf425e48b8ff7427907b5590d007b43f5c8f20994853f50550b26442f` |
| 9, Master previews | `7c4994c1c85a95daffd850ab15a67ee2ce8b960a788c66aa190cdbe9793a5a03` |
| 10, Analog retrigger | `3732fbe3a18807d607bc70c7dbb50716ae9ca60ad6f664da9e70ea58daeea4cb` |
