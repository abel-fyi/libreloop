# Performance checks

Measured on 2026-10-02 with a Release build on a two-core Intel Celeron N4020,
Intel UHD Graphics 600, and Gentoo Linux. CPU percentages use **100% = one core**.

The test UI ran in a hidden native X11 window using the Intel GPU, with audio
sent to an isolated PulseAudio null sink. CPU and RAM were sampled for six
seconds after warm-up. These figures exclude desktop-compositor presentation
and do not measure GPU power or end-to-end audio latency. They are comparisons
on this machine, not guarantees for every system.

## Desktop CPU and memory

| Scenario | Before | After |
| --- | ---: | ---: |
| idle | 24.0% | 2.0% |
| pattern playback | 28.0% | 23.3% |
| empty song playback | 32.3% | 24.5% |
| sampler idle | 28.7% | 2.2% |

Normal resident memory stayed around 59 MiB before and
59 MiB after. Memory apportioned between processes (PSS)
stayed around 26 MiB.

## Dense Playlist drawing

The synthetic stress scene fills all 100 tracks with 64 one-bar clips each;
the pattern has 16 notes per channel. Continuous redraw was forced in the
optimized run to measure preview caching independently of idle event waiting.
Playback was stopped. A stopped interactive app can also sleep between events.

| Channels | CPU before | CPU after | FPS before | FPS after |
| --- | ---: | ---: | ---: | ---: |
| 8 | 57.0% | 27.3% | 57.7 | 57.5 |
| 32 | 93.8% | 27.5% | 34.3 | 59.2 |

Dense-scene resident memory stayed around 64 MiB.

## Aero themes and TrueType text

After adding the bundled TTF font and cached rounded surfaces, the same native
Intel GPU checks measured 1.8% idle CPU, 26.0% Pattern playback and 25.7% empty
Song playback. Normal resident memory was about 60 MiB. The forced-redraw dense
scene measured 29.8% CPU with 8 channels and 29.7% with 32, at 57.8 and 59.3 FPS
respectively; resident memory was about 65 MiB. These runs used Dark Aero.

An initial version drew rounded surfaces from multiple shapes and used about
41% CPU in the dense scene. Caching tiny procedural surfaces reduced that cost.
Surfaces are generated once per color/style, reused at different sizes, and
released between frames when the palette changes. No blur or postprocessing
passes are used. Light Aero uses the same drawing path with a different palette.
An ASan/UBSan GUI check covers both palettes, menu dismissal, recoloring cached
clip previews, editing after a switch, resizing, repeated switches, normal
closure and loading the saved theme on restart.

## Audio renderer

Offline rendering used 48 kHz stereo, 512-frame buffers, and 20 seconds of audio
per run. Values are medians of three runs; CPU is relative to the audio duration.
These measure engine work alone, not the complete audio callback.

| Scenario | CPU before | CPU after |
| --- | ---: | ---: |
| idle | 0.962% | 0.009% |
| pattern playback | 2.263% | 0.499% |
| song, 4 populated tracks | 7.215% | 1.864% |
| song, 100 populated tracks | 15.024% | 10.990% |
| pattern, 32 channels with 128 notes each | 6.276% | 4.682% |
| 128 live voices | 5.241% | 5.147% |

## Changes and verification

- Pattern previews are cached in small transparent textures and reused across
  clips. Edits, source length, channel count, scale, zoom, and color changes
  refresh the cache. Extreme zoom uses vector drawing to preserve detail.
  Previews fit within clip rows instead of overflowing with many channels.
- Idle frames wait for input events. Playback, drags and Sampler playback cursors
  continue at 60 FPS. Sampler workers post an event on completion to refresh
  processed audio and the waveform without further user input.
- Audio state reaches the callback before the UI can wait. Mixing enumerates
  active voices, skips idle live rendering, and skips empty clip timing work.
- The frame limiter sleeps without raylib's partial busy-wait loop.

Six CTest tests and four ASan/UBSan core tests pass. An additional differential
check compares the optimized renderer to the previous implementation across
32 randomized Pattern/Song/live scenarios and four buffer sizes: PCM and Player
state match bit for bit. GUI checks cover cache edits, idle input, delayed worker
completion, Sampler crop previews, Pitch/Time behavior, and smooth playhead/Follow.
An ASan/UBSan GUI run also covers cached edits, worker completion, extreme zoom
and its vector fallback, and normal window closure with resource cleanup.

Source audio and processed audio remain separate for non-destructive edits;
long samples and large stretch settings can still consume significant memory.
At the time of those measurements the audio callback trylock could emit silence
during concurrent UI updates. The callback mailbox described below replaces that
behavior.

## Mixer meters and stereo width

The mixer now sums routed channels into stereo buses before applying width and
collecting post-fader peaks. The callback also combines live and sequenced voices
before bus processing. There are no callback allocations or additional locks.

An offline 20-second audio run on the same Celeron measured 0.84% of one CPU core
for the default pattern, 5.33% for 32 dense channels, 12.50% for 100 populated
Playlist lanes, and 6.78% for 128 sustained voices. The 128-voice 99th-percentile
512-frame render took 0.79 ms against a 10.67 ms audio-block deadline. These are
single-run engine measurements, including meter collection, rather than whole
application CPU measurements. Raw results: `local/performance/mixer-engine.json`.

Visible meters animate during audio and their decay, then return to the existing
idle event-wait behavior. Hidden meters do not keep the UI animating.

## Flat appearance

Rounded/glossy surfaces and shadows have since been removed. The theme module
now draws opaque rectangles, retains dark/light palettes,
and no longer creates or caches surface textures. The Aero measurements above
describe the previous implementation; the flat appearance has not yet been
benchmarked separately.


## macOS audio correctness (2026-10-03)

On an Apple M4, the previous callback could return about 9.19 ms of silence when
its UI mutex was busy. A real CoreAudio/miniaudio harness observed 22 silent
buffers out of 787 while playing one stereo song with roughly 60 UI updates per
second. The no-update control had zero failures. Short GUI runs also observed
occasional silent buffers. Those rates are workload-specific, not a prediction
for every session.

The callback now owns render state and reads pending UI commands without waiting.
A busy mailbox postpones edits while audio continues. Sample replacements are
acknowledged before the UI frees retired PCM. Unity-gain float mixing replaces
unconditional saturation; clipping occurs only at device/PCM16 output boundaries.

The same real-device harness, muted after analyzing its computed output, measured:

| Scenario | CPU, one core | Callback p99 | Stalled buffers | Output samples above full scale |
| --- | ---: | ---: | ---: | ---: |
| Stereo song with UI updates | 2.37% | 0.310 ms | 0 / 787 | 0 |
| 32 overlapping stereo voices | 5.39% | 0.586 ms | 0 / 785 | 0 |
| 32 voices with listening boost | 5.39% | 0.560 ms | 0 / 783 | 0 |
| 100 overlapping stereo voices | 9.99% | 1.115 ms | 0 / 781 | 0 |

These were approximately 7.2-second runs using 441-frame callbacks at 48 kHz
(9.188 ms of audio per callback); the native hardware rate was 44.1 kHz. The
longest callback was 1.153 ms, with none exceeding its audio duration. CPU includes
diagnostic overhead and UI updates, but excludes GUI drawing. Lock attempts did
occasionally fail; playback still advanced normally. The first 30 seconds of a
169-second stereo WAV rendered with zero sample error at unity gain, compared
with 0.004971 RMS sample error before the change.

The correlated 32/100-voice mixes intentionally exceed available output headroom.
The final clamp bounds their output; it does not make overloaded mixes distortion
free. Reduce mix gain when meters show overload. These tests analyze computed
float output, not a physical audio loopback or hardware scheduling latency.
Local diagnostics are under `local/mac-test/audio-diagnostics-after.*`.
