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

## Transparency toggle

The updated build was compared with transparency off and on in the same dense
32-channel scene. Six alternating runs (three per setting) used a hidden native
Intel GPU window and forced continuous redraw. CPU was sampled after a
three-second warm-up for six seconds. Median one-core CPU was 30.7% off and 29.7%
on; resident memory was 64.8 MiB for both. Average FPS was 57.4 off and 57.7 on,
including startup in the ten-second frame count. The observed differences are
small relative to run-to-run variation; this test found no measurable CPU
penalty from the toggle. It does not establish equal GPU power consumption.

The existing surface draw calls use alpha blending when enabled; no additional
render passes are added. Text, controls, waveforms and editing grids remain
opaque. ASan/UBSan GUI checks cover switching alpha, retaining it across palette
changes and restarts, old one-value preferences, recoloring previews, editing,
resizing, repeated switches and normal resource cleanup.

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
The existing audio callback trylock can still emit silence during a concurrent
UI update. That synchronization design is outside these CPU optimizations.

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
now draws plain rectangles, retains dark/light palettes and optional transparency,
and no longer creates or caches surface textures. The Aero measurements above
describe the previous implementation; the flat appearance has not yet been
benchmarked separately.
