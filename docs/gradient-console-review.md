# Master monitor proportions and gradient shading

The user requested a longer master L/R display, taking space from the adjacent
spectrum, and then asked for gradient shading on patterns and windows.

At the 1448-unit reference width, the spectrum shrinks from 256 to 192 units and
L/R grows from 40 to 104, retaining the monitor pair's right edge at 1380. The
level fill now uses its actual width for the signal, amber range, clipping
indicator and hover description. Intermediate widths transfer only the available
space. All 96 independent FFT bins and continuous level values remain intact.
The existing toolbar layout check asserts the new reference dimensions and
continues checking bounds and pairwise non-overlap at all supported test widths.

Picker cards get vertical color shading, Arrangement clip caption bands get a
16-unit shaded header/top highlight, and editor/dialog title bars get horizontal
shading. Window header bands and the visible Piano sidebar get subtle vertical
shading. Preview geometry, clipping, selection outlines, fonts, note/waveform
content and interaction rectangles are retained. No whole-window gradient is
painted behind opaque controls. Source shading keeps the base color at the
lowest-contrast end, preserving its original foreground contrast.

Each shaded rectangle uses six colored vertices (two interpolated triangles)
through the existing draw interface. No bitmap asset, new font, dependency,
blur, shadow or animation is added. Normal blocking idle waiting is unchanged.
The developer `--profile-flat` suppresses these gradients along with its existing
trim suppression; it is not an isolated per-gradient benchmark.

Full-window shading was initially measured and omitted: it added about 4 ms to
render time in the SDL software driver, largely behind opaque controls. The
final pass confines shading to visible regions. The rejected measurement is
retained as `rejected-full-window-*` in the artifact directory.

Actual app captures live outside the repositories in
`../baseline-artifacts/gradient-console/`, including `playing` and maximized
`piano`. They use the synthetic editable showcase; normal startup is unchanged.
The prior app bundle was copied before rebuilding, preserving revision
`31aa9ba` as the comparison executable with its original bundled resources.

Final verification: **121/121 CTest checks pass**, including independent FFT
bin/continuous meter drawing and the widened-monitor layout assertion. The
bundle passes `codesign --verify --deep --strict`. All 182 frozen reference
file hashes match; original audio/interaction fixtures are unchanged.

Native GPU cost, physical HiDPI, hands-on input feel and audible playback remain
unverified. This is still the installed-dynamic-SDL developer build; offline
shipping and other-platform acceptance gates remain separate work.

## Measured size and render cost

Release, AppleClang, SDK 15.4, installed SDL3 runtime. SDL dummy video/software
renderer; 1448×1086, scene 12, scripted pan/zoom, 40 warmup frames, 500 measured
frames, three sequential before/after pairs with no concurrent test/build/capture
run. Values are render + flush, excluding present, input, state publication and
frame pacing. They compare the entire gradient/L/R change, not each effect in
isolation. Audio device startup succeeded; audible output was not assessed.

| Measurement | Before | Current |
|---|---:|---:|
| Median run p50 | 5.250 ms | 6.643 ms |
| Median run p99 | 5.501 ms | 6.954 ms |
| Largest observed frame | 6.708 ms | 7.487 ms |
| Executable bytes | 3,691,184 | 3,691,232 |

Added asset bytes: **0**. The executable grows 48 bytes. The median-run p50
increase is about **1.394 ms** in this software-renderer workload; no native GPU,
battery, idle CPU or latency conclusion is inferred. Idle still blocks when no
animation/capture requires rendering. Raw runs and summaries are beside the app
captures outside the repositories.
