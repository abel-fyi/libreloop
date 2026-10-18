# Shared desktop materials

The Multiband Compressor now calls `ui_graph_paper` for both the transfer-curve
and history/band display backgrounds, the same helper used by the sampler and
EQ. Graph dots, base color and future paper changes come from the shared theme.
Curve traces, handles and monitoring data remain above the background.

Panel, button and title material uses the existing ordered 4×4 Bayer pattern.
Black/white overlay alpha increased from 12 to 18 out of 255: a modest contrast
increase without changing density or adding another overlay. Graph-dot opacity
and knob shading are unchanged, preserving plot and control legibility.

## Implementation and cost

The skin generates two nearest-filtered RGBA atlases once: 128×64 for material
and graph dots (32 KiB pixel payload), and 128×32 for knobs (16 KiB). These
48 KiB already existed; this pass adds no texture storage or embedded images.
Driver texture overhead is not included. There is no per-frame noise generation,
blur, shader effect, animation timer or audio-thread work. Stronger material
contrast changes pixel values in the same cached tile and adds no submissions.

The compressor's fixed 820×480 layout adds two background fills and 36 tiled
texture submissions at 100% UI scale (12 for the curve, 24 for history). SDL may
batch these; they are not necessarily 36 GPU draw calls. Cost varies with
viewport, UI scale and renderer.

## Local measurement

Before: commit b623cc0, copied app bundle outside the repository. After: this
material pass. Release macOS arm64 / AppleClang / SDK 15.4 / developer SDL3.
Dummy video, software renderer, 1448×1086, same Multiband Compressor scene 19,
500 unpaced measured frames after 40 warm-up frames, three sequential runs each.
The existing profiler measures render plus flush, excluding present, event
polling and frame pacing. Audio device startup succeeded; listening was not
checked. The first runs overlapped brief build/capture work, so these are local
observations, not an isolated benchmark of the texture operations.

| Whole scene | Median of run p50 (ms) | Median of run p99 (ms) | Largest frame (ms) |
|---|---:|---:|---:|
| Before | 16.6605 | 17.7732 | 19.5868 |
| After | 16.6810 | 19.2040 | 27.1491 |

The p50 difference is +0.0205 ms, but tail timings vary more. This does not
establish a precise isolated effect cost or native GPU overhead. Raw records:
`/tmp/libreloop-material-profile/before.json` and `after.json` (local temporary
artifacts, not repository assets). Both executables are 5,739,072 bytes; no
binary-size increase was observed in this build.

All 158 Release tests pass, including shared skin checks and scale/startup
captures. Compressor and UI Scale screenshots were inspected. Native GPU time,
HiDPI appearance, power consumption and hands-on dialog dragging remain
unverified. The existing idle event-wait design is unchanged.
