# Windows/Motif tracker styling review

The October 2026 follow-up applies the user's Windows/Motif/1990s tracker
appearance direction to the existing SDL desktop. It preserves current pane
geometry, hit areas, event handling, project data and audio logic. This is a
visual refinement, not completion of every item in notes.txt.

## Completed

- Warm gray panel/control palette; teal focused window and dialog titles with
  contrasting text, raised window buttons and inset display/instrument groups.
- Beveled buttons, tabs and fader caps; concentric dial faces, without changing
  values, gesture sensitivity, wheel rules or hit boxes.
- Permanently visible raised scrollbar thumbs on Arrangement, Piano and Mixer.
- Small colored folder and sample/document icons in the browser tree. Existing
  row bounds, indentation steps, selection and drag behavior stay the same;
  text starts farther right and is fitted to the remaining width.
- Blue-teal fill under existing FM/Analog envelope segments, clearer stage
  separators and inset plot edge. The plotted points and editing formulas stay.
- No new font, bitmap skin, library or dependency. No copied Windows assets.

## Verified

Release build with AppleClang, SDK 15.4 and installed SDL3. Software-rendered
captures reviewed at 1448×1086 and 900×562, including the MIDI dialog. The
full-size populated capture uses a synthetic editable showcase; startup still
opens the normal empty project. Frozen fixture expectations are unchanged.
Drawing-only test stubs were extended for the added drawing calls. The full
CTest suite passes 119/119, including frozen audio and interaction fixtures.
The app bundle passes `codesign --verify --deep --strict`. All 182 original
source/concept reference hashes match the frozen manifest; the original is
still clean at `5e860696fe6cb6cd4c170f6ebf5378039b3e4af6`.

Artifacts are outside both repositories in
`../baseline-artifacts/vintage-workstation/`: `libreloop-smoke.png`,
`compact/libreloop-smoke.png`, `dialog/libreloop-smoke.png` and render measurements.

## Unverified and follow-up

Native GPU performance, physical HiDPI and hands-on mouse/trackpad behavior
remain unverified in this environment. The shipping offline/static SDL build
and other platform acceptance gates remain unchanged.

A broader colored Windows 2000-style toolbar icon pass is recorded for later.
Separate Mute/Solo controls, panel docking/resizing, slider replacements and the
reported piano ruler/grid alignment concern are not resolved by this appearance
pass. No interaction assertions or reference data were weakened.

## Rendering and size measurements

The pre-change revision `6d3097e` was built from a detached rewrite worktree
outside the original repository with the same Release/AppleClang/SDK 15.4 and
installed SDL3 setup as the current app. Three sequential before/after pairs
used SDL dummy video and the software renderer, 1448×1086, scene 12, 40 warmup
frames and 500 measured frames with scripted pan/zoom. Measurements were taken
without a concurrent test run. Raw runs and summary are in `render-profile.json`
and `summary.json` in the artifact directory.

| Measurement | Before | Tracker styling |
|---|---:|---:|
| Median run p50, render + flush | 4.901 ms | 5.226 ms |
| Median run p99, render + flush | 6.074 ms | 6.389 ms |
| Largest observed frame | 7.228 ms | 8.003 ms |
| Release executable bytes | 3,674,352 | 3,674,448 |
| Added bitmap/font asset bytes | — | 0 |

The combined visual pass adds 0.325 ms to median run p50 (about 6.6%) in this
software workload and 96 executable bytes. This measures all palette/chrome,
icon, knob, scrollbar and envelope changes together; individual effect costs
were not isolated. It excludes presentation, event processing, publication and
frame pacing. Background audio startup succeeded but audible output was not
checked. These measurements do not establish native GPU cost, idle CPU, battery
impact, physical HiDPI performance or input latency. The normal blocking idle
wait is unchanged. SDL's dynamic runtime is unchanged at 2,519,824 bytes.

The SDK deployment warning remains: this developer bundle links the installed
SDL runtime built for macOS 26, despite compiling app sources with SDK 15.4.
It is not evidence of support for older macOS or a completed offline/static build.
