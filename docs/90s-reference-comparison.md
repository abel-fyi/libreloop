# Reference comparison and final detail pass

Direct inspection uses `../90s.png` and real application captures in `../../baseline-artifacts/90s-final/`. This pass keeps the approved modern page workflow and the later user changes instead of restoring the AI image's obsolete stacked layout.

| Difference found | Correction |
| --- | --- |
| Synth stretched to 802 units | Stock tool is 376×646: the reference's 376-unit width and 622-unit body, plus the requested 24-unit gray title. Only saved stock 376×802 geometry migrates; custom sizing remains. |
| Preset and operator tabs too short | Spacious preset/engine wells are 28 high; operator tabs 30 high. Compact and legacy sizes stay carried. |
| Extra boxes around synth rows | Removed those outlines; controls sit directly on the steel panel, as in the reference. |
| Knobs/faders/buttons lacked cast depth | Knob fan adds a hard shadow offset (2,3), radius +1; raised buttons/faders use 2-unit lower/right shadows. Pressed buttons lose the raised shadow. No blur. |
| Track headers had extra plates/lines | One flat shaded row, light top/dark bottom divider and left edge; names and mute controls retain their positions/hit semantics. |
| Position numerals crowded the display | Full-size digits 22→18 high; compact 16→14; larger inset. Tempo and count-in timing unchanged. |
| Dots competed with content | Tile alpha 105→48, reused by Arrangement, notes, velocity and synth/EQ graphs. Curves, note data and continuous meters stay above it. |

Reference body row geometry remains the same concrete six-op layout; its 640-unit virtual content frame includes the original 18-unit internal title coordinates. With the new 6-unit translation it produces a 24-unit physical title and the same 622-unit body. Operator knobs have 24-unit diameter. Real sound parameters determine the envelope instead of tracing AI artwork. Visual dot checks now require faint-but-visible contrast and an upper bound, reflecting the explicit opacity change; interaction/audio reference assertions remain unchanged.

Intentional differences are gray smaller titles, icon action controls, modern Arrangement/Pattern pages, retained original embedded font, original project/control names, and continuous spectra/meters. The image is not a pixel-exact final acceptance certificate: its fonts/icons contain generated details, and layout exceptions were explicitly requested. No nonexistent synth controls or views were added.

All 137 tests pass after integration, including original audio/project/interaction fixtures. Checks also cover stock-size migration versus custom sizing, real font fit at five densities, visible knob/button shadows, faint dots with a bright continuous curve, and revised Mixer geometry. The bundle passes strict codesign verification; the original repo is clean at its pinned commit. Final executable: 3,760,896 bytes, +32 from the action-icon checkpoint. No new texture or dependency is added in this detail pass; each knob adds 120 vertices to its existing batch (480 total), buttons add two small shadow rectangles. No idle animation or audio-thread work is introduced.

The final sequential SDL software-renderer profile at 1448×1086 used scene 12, pan/zoom and playback, 40 warmup then 500 measured frames: p50 17.423500 ms, p99 17.829416 ms, max 18.290834 ms; rendering CPU 8.904570 seconds. The audio device reported started. The scene includes the merged page toolbar, all materials and waveform/automation content; this does not isolate shadow/dot cost or prove a CPU saving. Native GPU cost, battery, physical HiDPI, real trackpad feel and audio latency remain unverified. Production idle blocking-wait logic is unchanged.
