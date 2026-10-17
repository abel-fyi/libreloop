# Classic desktop rendering measurements

Environment: macOS arm64, AppleClang 21, Release, SDK 15.4, installed SDL 3.4.16.
Driver: SDL dummy video / SDL software renderer. These are **render plus flush**
timings in the actual desktop, excluding present, event polling, command/state
publication and pacing. They are not native GPU, input-latency, battery or
end-to-end frame measurements. Audio device startup succeeded; process CPU
counter deltas can include its background work. Audible output was not checked.

## Method

Development-only `--smoke --render-profile 500` disables the frame limiter,
warms 40 frames and measures 500. `--profile-motion` varies the actual Arrangement
and Piano view pan/zoom each frame. The sample waveform and all four editors are
visible. Three sequential runs per case use 1448×1086. Legacy uses carried floating
windows and its font/layout scaling; classic uses the new tiled layout. Their
workloads differ, so the comparison does not isolate a renderer optimization.
Normal startup still uses the original event-waiting design and 60 Hz limiter.

| Actual desktop case | Median run p50 (ms) | Median run p99 (ms) | Largest observed frame (ms) |
|---|---:|---:|---:|
| Legacy stationary | 5.967 | 6.104 | 6.336 |
| Legacy scripted pan/zoom | 5.989 | 6.288 | 6.636 |
| Classic stationary | 4.269 | 4.387 | 4.570 |
| Classic scripted pan/zoom | 4.240 | 4.386 | 4.455 |

The largest frame in each final case is included above; no outlier was removed. No playing/recording or physical HiDPI performance is inferred here.
Raw results: `../baseline-artifacts/classic-render-profile/results.json`.

## Decorative borders

The same classic Analog scene, geometry, fonts and scripted motion were run
with the production decorative edges enabled and with development-only
`--profile-flat`. This suppresses control/panel bevel lines and fader outlines;
meaningful selection outlines remain. Three paired p50 deltas were +0.114, +0.114 and +0.082 ms; their median was +0.114 ms. These are small differences in a software workload, not reliable individual GPU bevel timings. No flat variant was adopted. Raw results: `../baseline-artifacts/classic-render-profile/edges.json`.

## Size and limits

The pre-visual SDL baseline was rebuilt from commit `415433e` in an isolated
worktree outside the original repository, using the same Release/toolchain/SDK
and developer SDL configuration. Its executable is 3,656,784 bytes. The current
executable is 3,674,352 bytes: +17,568 bytes (0.48%). The difference includes the UI changes and development capture/profiling code, and is not
attributed solely to any border. No new bitmap or font asset was added. The SDL
runtime is unchanged at 2,519,824 bytes. This is a development dynamic-SDL bundle,
not the still-unverified offline/static shipping build.

Native idle/active CPU, GPU time, power, latency, playback with visible waveforms,
physical HiDPI and other-platform results cannot be verified here. The idle wait
logic was audited and remains blocking after the original 0.15-second inactivity
threshold when no animation/capture is active. Profiling deliberately bypasses
that wait and must not be used as an idle CPU measurement.
