# Synth crop, patterns and continuous meters

Completed: Analog overview slider layout, larger blue envelope, full clip/card
gradients, six-color continuous meter ramp, and fitted piano octave labels.
The layout and remaining non-counterparts are recorded in `visual-notes.md`.

The actual 1448×1086 SDL software-rendered showcase was inspected, including
numeric contrast, pattern previews, envelope guides and octave labels.
Capture: `../baseline-artifacts/synth-crop-pass/after/libreloop-smoke.png`.
The preserved prior bundle is revision `2eba217` in `before/LibreLoop.app`.

Three paired runs used scene 12, moving views, playback, 40 warmup frames and
500 measured frames each. SDL dummy video/software renderer; no concurrent
build/tests during measurements. These measure UI render/flush CPU time, not
present, event handling, pacing, total app CPU, latency or GPU time. Combined
layout/effect differences are included; individual effect cost is not isolated.

| Version | Median run p50 ms | Median run p99 ms | Median run max ms |
| --- | ---: | ---: | ---: |
| before | 9.386 | 9.842 | 10.282 |
| after | 9.273 | 9.633 | 9.889 |

After-run worst frame was 21.260 ms (one run); other maxima were 9.889 and
9.883 ms. The data does not establish a CPU improvement. Executable size rose
from 3,691,232 to 3,691,440 bytes (+208). No new bitmap/font/dependency was added.
Raw results are in the artifact directory's `render-profile.json`.

Verification: the release developer build and ad-hoc bundle signature verify.
All 122 CTest checks pass, including unchanged original audio/interaction
fixtures, font fitting at two simulated raster densities, independent spectrum
bins and continuous meter geometry. Added software pixel checks assert many
intermediate meter colors, green/red endpoints, empty zero-level bars and no
fill beyond the requested level. Original repository working tree remains clean.

Unverified: physical HiDPI, native GPU cost, hands-on new slider feel and audible
playback. The fitted-label test uses constructed glyph metrics; actual screenshot
inspection covers the embedded font at the captured density. The software
capture does not establish mouse/trackpad parity. This remains the dynamic
installed-SDL developer build, with the existing macOS SDK/SDL deployment
warning; offline static packaging and Windows validation remain separate work.
