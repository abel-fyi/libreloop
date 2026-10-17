# A shared bitmap material style

`90s1.png` and `90s2.png` show fine dither, a lighter blue-gray desktop, deep blue Browser, teal piano grid, pixel-shaded knobs and shared directional bevels. Both supplied files are byte-identical (SHA256 `29c434212c7fdd47b02f3068a8bf2a9337132d8a8190839236c435efed9f4c0a`). Their AI-generated menu/title details do not override the approved modern layout, compact gray captions or combined PATT/SONG control.

The shared theme now uses a visible two-tone surface dither and gentler blue-gray bevel colors across panels, buttons, titles, faders and scrollbars. Common 8/9/10/12-radius knobs use generated 32×32 pixel sprites, with ordered 4×4 Bayer shading, a small shared gray-blue palette and baked lower-right shadows. The pointer is drawn independently at the original continuous angle. Other knob sizes retain the geometry fallback. No bitmap changes a hit area, drag sensitivity, parameter resolution or input formula.

The existing embedded font is retained, with real atlases at each DPI and nearest sampling in studio mode to avoid a second smoothing pass. Text widths and fitting do not change; the original mode retains its sampling. The faint graph dots remain alpha 48. Notes, automation, waveforms, velocity, spectra and meters are not reduced to a global bitmap or limited signal palette. Material shading blends the desktop without reducing musical information.

Palette comparison used the OS ImageIO/CoreGraphics read-only probe saved outside the repo in `../../baseline-artifacts/90s-bitmap/palette-probe.c`. Representative reference regions averaged panel 129.7/134.7/144.5, Browser 10.9/50.1/75.3 and piano 40.8/135.3/150.6. The final capture averaged 128.8/133.6/143.6, 14.7/50.8/75.8 and 42.0/132.5/147.5 respectively. Region averages include texture and, for piano, different note content; they guide palette choices rather than establish pixel-exact identity. The final material tile uses constant alpha 12 for its two tones.

The original surface/graph tile stays 128×64 RGBA (32 KiB). One 128×32 RGBA knob atlas adds 16 KiB, generated once and explicitly freed at shutdown. Common knobs use one texture quad plus their continuous pointer instead of constructing 480 body/shadow vertices each frame. No new dependency, font file, imported bitmap artwork, audio-thread operation or idle animation is added. Initial experiments with one larger atlas and per-note/per-clip grain were removed; grain stays on shared desktop surfaces.

## Performance verification

An initial 500-frame software run before changes measured p50 17.122 ms, while later drafts measured about 34 ms. Font sampling and the flat rendering control did not explain that difference. A separate out-of-source Release build of unchanged revision `8d77616` then showed the same slower timing under current conditions. Thus the first before/after pair cannot reliably measure the change's cost.

The interleaved control/new/control sequence used the same SDL software renderer, 1448×1086, scene 12, active pan/zoom/playback, 40 warmup and 200 measured frames each. All reported audio device started:

| Build | Render CPU seconds | p50 ms | p99 ms | max ms |
| --- | ---: | ---: | ---: | ---: |
| Control A, 8d77616 | 7.650304 | 36.414834 | 53.711042 | 62.574208 |
| Bitmap materials | 6.801258 | 32.612458 | 33.412875 | 33.596625 |
| Control B, 8d77616 | 7.217374 | 34.273500 | 37.870625 | 54.960041 |

Logs, control source/build and the palette probe are under `../../baseline-artifacts/90s-bitmap/`. These whole-scene results show no added slowdown in this control sequence, but variation prevents a precise CPU-saving claim or isolated-effect estimate. Native GPU performance, physical HiDPI, battery use, trackpad feel and audio latency remain unverified. Idle event-waiting logic is unchanged.

## Completed and verified

The shared material/font changes are built and captured in synth, Arrangement, active EQ and compact 900×562 views. All 137 CTest cases pass. Rendering checks cover every cached knob radius, its limited decorative palette, shadows, faint dots and continuous curves at five logical densities. Font checks confirm unchanged text width. Original audio, interaction and project fixtures remain unchanged. Strict bundle signature verification passes. The original repository remains clean at `5e860696fe6cb6cd4c170f6ebf5378039b3e4af6`.

The final Release executable is 3,761,024 bytes, +128 from `8d77616`. The added knob atlas is 16,384 raw bytes; no font or icon atlas is enlarged by this change. Native GPU/HiDPI and physical-input verification remain unverified; software visual comparison is not a pixel-exact or hardware-performance certificate.
