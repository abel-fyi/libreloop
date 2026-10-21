# Windows performance measurements

Measured on an Intel Celeron N4020 (two cores), Intel UHD Graphics 600 and
8 GB RAM, using a Release LLVM-MinGW build with pinned SDL 3.4.16. These are
local measurements with other applications running, not hardware requirements
or a clean-machine benchmark. The optimization applies to SDL rendering on
all supported operating systems.

## First rendering optimization

UI primitives repeatedly map the same colors to the fixed palette. The SDL
renderer now caches 256 exact mappings per drawing context. Texture-upload
dithering remains position-dependent and is not cached.

Three alternating runs of the original and cached renderer, 180 measured
frames after 40 warmup frames, empty classic desktop at 1448 by 1086 pixels:

| Median across runs | Original | Cached | Reduction |
| --- | ---: | ---: | ---: |
| Draw time per frame | 7.92 ms | 7.11 ms | 10.3% |
| Whole frame time | 12.80 ms | 12.02 ms | 6.1% |

These are wall-clock timings. The existing `render_cpu_seconds` field uses C
`clock()`, which measures elapsed time on Windows; it should not be interpreted
as Windows process CPU usage. Tail timings varied with competing applications;
the measurements do not establish a reduction in worst-case latency.

All four software-rendered smoke screenshots and the serialized project were
byte-identical before and after caching. Drawing and theme tests also passed.

## Audio baseline

Offline DX7 rendering at 48 kHz in 512-frame blocks, two seconds per scenario:

| Scenario | Approximate CPU use, one core | Median block time |
| --- | ---: | ---: |
| Idle, effects rack present | 1.6% | 0.095 ms |
| Eight DX7 voices | 6.2% | 0.469 ms |
| 32 DX7 voices | 14.0% | 1.911 ms |

CPU use is process CPU time divided by rendered audio duration; 100% represents
one fully occupied core. Short tests have limited CPU-clock precision.
Some 32-voice blocks exceeded their 10.67 ms duration under competing load.
This is an offline process without the audio device thread's scheduling policy;
those counts do not measure actual WASAPI dropouts. More profiling is needed
before promising a particular live-polyphony limit or further speedup.

The separate exhausted-voice-pool fix prevents a full pool from collapsing new
chords to their last note. It changes voice selection, not the cost of rendering
each DX7 voice.

## Reproduce

From PowerShell with the chosen native compiler and build tools on PATH:

```powershell
cmake --build build-windows --target benchmark_engine libreloop --parallel 2
Push-Location build-windows
.\benchmark_engine.exe fm_8 5 512
.\benchmark_engine.exe fm_32 5 512
.\libreloop.exe --smoke --visual-preview --smoke-new-project --render-profile 180
Pop-Location
```

The audio benchmark writes its capture into the working directory. Keeping the
working directory inside the build tree avoids adding captures to the checkout.
UI profiling runs without the normal frame-rate cap; it is a measurement mode.
