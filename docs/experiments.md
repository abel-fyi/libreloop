# Phase5 experiment protocol and current evidence

No candidate is adopted. Normal CMake builds retain their original options.
These measurements use the geometry-preserving dark-gray/cyan review preview,
not a newly approved final layout. Native parity and final layout review remain
separate acceptance work.

## Optimization matrix

Explicit experiment flags in cmake/experiments.cmake select engine/audio O2/O3,
UI/glue Os and thin LTO on/off. The actual compiler command lines confirm the
candidate flag is last. LTO capability checking propagates SDK15.4 into its
nested project; the initial probe silently used SDK27 and failed on TAPI, so
that probe failure is not an LTO-performance result.

Four candidates (O2/Os, O3/Os, each LTO off/on) passed all89 developer-SDL tests,
including unchanged original PCM hashes, project/state/export checks and
interaction fixtures. No assertion or reference was changed. Each engine
measurement renders approximately3 seconds at512-frame blocks after128 warmup
blocks, three processes per scenario. Eleven scenarios cover idle, song,
sampler voices/stretch/chorus/EQ, DX7 and analog. These are warmed offline render
measurements, not device callback deadlines or native playing-app CPU.

The UI harness renders the actual Piano editor/chrome/painter with128 notes,
scripted pan/span changes, a moving playhead and visible stereo waveform, using
SDL's offscreen software renderer. It flushes every frame, excludes40 warmup
frames and measures300 frames per run, three repetitions at framebuffer density
1/2. Fonts, icon atlas and waveform peaks are prepared outside timed frames.
It measures these render components; it does not include the complete desktop
service/event/presentation path, audio I/O or physical HiDPI displays.

Raw build/test logs, engine repetitions, rendering repetitions and toolchain
metadata remain in sibling baseline-artifacts/experiments/optimization.
Tools: experiment_optimization.py builds/tests/captures; collect_optimization_ui.py
collects the normal Release baseline and candidate render results. The compiler
is AppleClang21, CMake4.4.3, SDK15.4, macOS26.6.2 arm64, installed SDL3.4.16.
CPU model identification was denied by sysctl and is reported unavailable.

## Remaining experiments and unavailable measurements

Custom CPU rasterizer, dirty rectangles/peak-cache strategy and static/dynamic
SDL comparison remain pending. SDL source/static archive is unavailable; shell
network DNS remains restricted. Installed SDL is used only for developer
validation and these experiments. Static SDL is not replaced by a chosen
shipping dynamic dependency.

Linux and Windows builds/runs are unavailable here. Native hardware renderer,
GPU load, battery impact, mouse/trackpad smoothness, input latency, device audio
latency and physical-DPI behavior are unverified. Software component timing is
not presented as a native GPU or full-app performance measurement.

## Optimization results on this host

Bytes are unstripped executable file sizes (SDL runtime remains a separate
2,519,824-byte library in every bundle). Render times are medians of the three
run p50/p99 values, not pooled latency distributions. All entries use the preview
palette and the same scene/geometry. Values from different rendering hardware or
platforms cannot be inferred from this offscreen software result.

| Engine / UI / LTO | Executable bytes | 1x p50/p99 ms | 2x p50/p99 ms |
|---|---:|---:|---:|
| O3-O3-lto0-baseline | 3,638,800 | 9.376 / 10.027 | 36.936 / 39.394 |
| O2-Os-lto0 | 3,446,016 | 9.361 / 9.991 | 36.879 / 39.345 |
| O3-Os-lto0 | 3,510,016 | 9.359 / 10.007 | 36.783 / 39.294 |
| O2-Os-lto1 | 3,236,864 | 9.400 / 10.363 | 36.890 / 39.575 |
| O3-Os-lto1 | 3,236,672 | 9.362 / 9.985 | 36.906 / 39.296 |

Selected warmed offline engine CPU seconds for approximately3.008 rendered
audio seconds,512-frame blocks (median of three processes):

| Candidate | Sampler32 | DX7 FM32 | DX7 FM128 |
|---|---:|---:|---:|
| Release baseline | 0.049940 | 0.156816 | 0.618090 |
| O2-Os-lto0 | 0.049094 | 0.156506 | 0.619535 |
| O3-Os-lto0 | 0.049736 | 0.156930 | 0.617817 |
| O2-Os-lto1 | 0.048944 | 0.148739 | 0.590286 |
| O3-Os-lto1 | 0.049480 | 0.149357 | 0.594482 |

On these measurements, LTO candidates reduce executable size and improve these
FM workloads; the software render medians are close across configurations. This
is an inference from this small host dataset, not a hardware/platform-wide
recommendation. All89 checks pass for each candidate. No flags are adopted.
Raw measurements for all eleven engine scenarios are retained externally.

## CPU rasterizer candidate

The OFF-by-default LIBRELOOP_EXPERIMENT_CPU_RASTERIZER option selects an own CPU
RGBA backend behind the same draw interface. It handles rectangles, borders,
lines, triangles, nearest/linear textures, text through the unchanged font code,
clipping and render targets. The final framebuffer is compared with the last
presented image and only its changed bounding rectangle is uploaded to an SDL
streaming texture. Existing waveform peak caches are reused. CPU commands are
immediate; flush does not force an intermediate upload. Idle event waiting
remains the original app loop. No new dependency or shipping choice is added.

Correctness: all90 CPU-candidate checks pass, including unchanged original audio
and interaction fixtures. A dedicated check verifies clipping, alpha continuity
across a shared triangle edge, target textures and zero upload for a repainted
but unchanged final frame; a2x2 edit uploads four pixels. The normal SDL path
still passes all89 checks. These are not proof of native GPU pixel equivalence.

Measured with200 frames after40 warmups, three runs at each density/mode. This
uses the preview Piano/chrome with128 notes, changing pan/span, moving playhead
and visible stereo waveform. Both are offscreen, on the same SDL software
renderer; CPU presentation includes dirty detection/upload and SDL's surface
blit, while render-only excludes those. It is not a hardware-GPU comparison.

| Backend | Executable bytes | 1x render p50/p99 ms | 1x present p50/p99 ms | 2x render p50/p99 ms | 2x present p50/p99 ms |
|---|---:|---:|---:|---:|---:|
| sdl | 3,638,848 | 9.361 / 9.961 | 9.376 / 10.021 | 36.771 / 39.338 | 36.783 / 38.992 |
| cpu | 3,654,496 | 1.687 / 1.817 | 3.199 / 3.408 | 5.841 / 6.233 | 13.276 / 14.523 |

Pixel comparison of four actual40-frame app smoke scenes at1200x675:

| View | Different pixels | Percent | Max channel error | Mean absolute RGBA error |
|---|---:|---:|---:|---:|
| libreloop-smoke.png | 19,188 | 2.369% | 231 | 0.303311 |
| libreloop-view-1.png | 19,824 | 2.447% | 231 | 0.388510 |
| libreloop-view-2.png | 36,097 | 4.456% | 231 | 0.193485 |
| libreloop-view-3.png | 29,053 | 3.587% | 231 | 0.150534 |

The CPU Piano capture was inspected: layout, controls, keys, notes and velocity
remain present. Its differing edge/sampling pixels mean fidelity is not exact.
These screenshots and raw metrics remain in sibling
baseline-artifacts/experiments/cpu-rasterizer. Maximum error is substantial at
some edges, despite low mean error. No acceptance threshold is invented.

This candidate is faster than SDL's software renderer in this workload; that
does not establish an advantage over Metal/Direct3D/OpenGL hardware rendering.
Pixel fidelity, peak/dirty-region strategies for full arrangement workloads,
GPU/battery cost, input latency and scrolling smoothness require native tests.
Linux/Windows results are unavailable. The CPU candidate remains unadopted.

The CPU candidate's dedicated raster/dirty-upload check, full desktop smoke and
visual-project-state comparison also pass ASan+UBSan; leak detection remains
unsupported and disabled. All182 frozen source/concept hashes remain unchanged,
and the original audio fixture files are unchanged. This remains an experiment,
not completed native-platform acceptance or a shipping renderer decision.
