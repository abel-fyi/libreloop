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

## Physical-pixel anchoring and broader coverage

The dither and graph-dot tiles now use `draw_texture_tiled_pixels`, a concrete
draw operation implemented by each renderer. It transforms only the destination
bounds into framebuffer coordinates, then copies integer source texels to
integer output pixels, with phase anchored to target origin. UI size, fractional
translation and clipping do not change the pattern at an interior output pixel.
OS/framebuffer density is included before this step: the material stays one
texel per physical pixel rather than growing with UI scale. Icons, fonts and
knob body shapes continue to scale normally. The old atlas comment describing
logical-scale tiling is superseded.

Shared flat surfaces and source-color gradients now apply the material too.
This includes the pattern/audio/automation picker, its empty gray background,
colored arrangement clips, drag previews, menu surfaces and the status bar.
Labels, notes, curves and waveform traces are drawn afterwards and stay clear.
The shared button path avoids painting a redundant material surface beneath
its opaque button face. No hit tests or input behavior changed.

Pixel tests compare opaque tile texels exactly across 75%, 100%, 125%, 150%,
200% and 250% transforms, fractional positive/negative shifts and clipping.
The skin gradient check now samples matching physical Bayer phases rather than
assuming a logical-coordinate phase; its brightness/contrast assertions remain.
All 158 Release tests pass. The experimental CPU backend passed a syntax check;
its pixel parity and sanitizer graphics target are not verified in this build.
Native-display flicker, fractional OS compositor scaling and mixed-DPI display
transitions still need hands-on acceptance.

Pixel density is stable, but larger physical areas may require more tile
submissions than the former scaled 64-unit tiles, particularly at HiDPI. Texture
size remains 48 KiB; the executable is 5,739,136 bytes, +64 bytes from this pass's
starting build. No animated noise or idle timer was added.

Two single-run software-renderer probes used the same 1448×1086 showcase scene
12, 500 measured frames after 40 warm-up frames. Render plus flush p50/p99 were
15.3597/17.0511 ms at 100%, and 16.3063/16.7347 ms at 150%. Layout/content
visibility differs with UI scale, so this is not an isolated material-cost
comparison and cannot establish GPU overhead. Raw local data:
`/tmp/libreloop-pixel-material-profile/results.json`. Earlier before/after
measurements in this document describe the preceding material pass.
