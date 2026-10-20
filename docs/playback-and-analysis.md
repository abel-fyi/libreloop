# Playback, editor pages and analysis

The toolbar PATT / SONG control selects the playback source. It does not change
editor pages. Arrangement / Pattern tabs in the editor title bar select and
focus a page without changing playback, position or transport reset state.
Views remain switchable during recording; playback mode changes remain blocked.
Editing a ruler on the inactive page changes that page's start/loop markers
without switching playback or seeking the active source. Existing per-page
scroll, zoom and selection state is retained.

## Live Master routing

The metronome and browser previews enter Master before its effects, fader, pan,
meters and analyzer tap. Master EQ therefore sees and processes the click;
Master volume and mute affect it too. The click uses the engine's current
sample position, including loop wrapping, rather than a second block clock.
It is generated only by the live audio callback, so offline WAV exports exclude
it. Recording the Master bus includes the click when enabled; recording input
or insert buses does not add it.

## Spectrum updates

The callback publishes contiguous stereo history through three owned slots per
analyzer. Each snapshot retains a 4096-frame FFT lookback plus the newest audio
block (bounded to 16384 frames total). It performs bounded copies and atomic
exchanges; there is no FFT, allocation or waiting in this publication path.
The consumer and producer never access the same slot concurrently. A stalled UI
skips old publications. Bus changes clear the selected-bus history and tag
snapshots to reject stale bus data.

Audio callbacks can arrive far less often than video frames. On the tested
PulseAudio device, the default callback is 3600 frames at 48 kHz: 75 ms, or about
13.3 callbacks per second. Jumping to each block's final FFT window makes the
graph step at that rate even when the rest of the UI runs smoothly.

The UI now advances its FFT window through that already rendered audio using
monotonic playback time, so it can analyze a distinct window on every display
frame between callbacks. It does not interpolate graph heights or force smaller
audio buffers. When no further samples are available it holds the last window.
Attack and decay use elapsed audio samples, and 64-bit sample positions support
long idle sessions. The FFT's cached Hann window and stereo power calibration
remain unchanged. Its 4096 samples span about 85 ms at 48 kHz.

The EQ response curve no longer flushes SDL's renderer after every mesh segment;
SDL preserves drawing order while batching those primitives.

The development render profiler also reports complete frame p50/p99 times and
fresh Master/EQ analysis updates per second. For example:

```sh
./build-linux/libreloop --smoke --visual-preview --smoke-scene 11 \
  --smoke-playing --render-profile 200
```

This profile disables the 60 fps limiter. Use a native renderer to measure the
actual desktop path; software/dummy renderer results describe a different
workload. In one native X11/OpenGL pair, fresh EQ updates increased from 13.560
to 53.838 per second during compilation load; a final run after compilation
measured 63.849 updates per second. These are local observations,
not a hardware-independent frame rate guarantee.

Regression checks cover Master effects/fader/mute and analyzer routing for the
click, callback block-size independence, a stalled analyzer reader, distinct 60 Hz windows between 75 ms callbacks,
long idle sessions, calibrated
spectral peaks and decay, and independent playback/page/ruler state.
