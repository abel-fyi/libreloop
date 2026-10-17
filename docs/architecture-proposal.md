# Architecture proposal — Phase0 reference

This records the Phase0 proposal. Subsequent user continuation authorized incremental extraction; current implementation/verification status is in phase-1-report.md and phase-2-progress.md. This document does not imply hands-on parity acceptance or approval of the visual/Windows/experiment phases. Preserve C11, GPL-3.0-only, CMake, existing LLP schema and built-in device DSP. Extract concrete existing functions incrementally. No raylib, plugin host, generic widget/event framework, unity build or first-pass CPU renderer.

## Proposed layout

```text
libreloop2/
  CMakeLists.txt
  cmake/                 platform flags, resource packaging, offline checks
  src/
    engine/              project types, sequencing/render, playback plan, parameters
      instruments/       sampler voice, custom FM, adapted DX7, analog
      effects/           chorus, EQ, fixed effect racks
      project/           LLP/preset schema, asset policy, document dirty state
      midi/              parser, mappings, timestamped takes (no OS input handles)
    interaction/         POD input/geometry + navigation, grid, hit tests, gestures,
                         selection/clipboard, history boundaries, window model
    ui/                  existing concrete editor drawing, font/atlas/peak caches
    app/                 transport/document/recording orchestration, service lifetime
    platform/            small window/input/draw/audio/MIDI/file/thread interfaces
      sdl/               SDL3 window/input, renderer implementation
      macos_input.m      minimal Cocoa precise scroll/magnification adapter
      midi_macos.c       CoreMIDI
      midi_linux.c       ALSA
      files_posix.c      filesystem/mapping/atomic save
      files_win32.c      later OS implementation, no POSIX leakage into engine
      audio_miniaudio.c  backend/device lifecycle, callback adapter
  third_party/           pinned SDL3, miniaudio, stb_truetype + existing carried headers
  assets/                original font, generated samples, factory presets/notices
  tests/                 Phase1 frozen PCM regression, Phase2 deterministic replays
  docs/                  behavior/architecture/notices/baselines/approval record
```

Names are proposed groupings, not a requirement to split every existing file. engine.c DSP and editing math can first remain small separate files under these modules. Current original CMake engine target includes chooser/OS files; move these actual dependencies out rather than rename the existing mixed target “clean engine.” Peak cache math is portable and can be shared with drawing without graphics types.

## Concrete interfaces and dependencies

Engine headers expose Project/Player/Sample/ParameterTarget, prepared plans and render entrypoints; no SDL/raylib/window/texture/font/cursor types. Timeline, grid and note/clip edits use own float coordinates and structs. Interaction accepts app InputFrame/Event PODs (timestamp, logical pointer, delta, key/button/modifier, scroll precision/axes/pinch units, viewport/DPI); boundary retains the original per-frame polling semantics, press counters, repeat bits and scalar/native wheel paths. No polymorphic event dispatch bus. Replay clock, geometry and input are explicit, not read from OS inside gestures.

UI asks concrete interaction functions for state changes and draws through small rect/line/text/texture/clip functions. Existing window/editor functions remain direct calls. Draw caches belong to UI; engine has no texture references. Layout comes from own rects, allowing Phase3 approved geometry updates without changing assertions. Separate interaction updates from drawing incrementally, preserving original frame order; do not rewrite the state machine while changing API names.

App owns editable Project, source samples, history, worker jobs, document and recording sessions. Engine audio runtime owns callback state. UI commands/state are a small concrete interface: transport state/seek/mode/loop, project+prepared-plan publication, sample replace, preview, note/key and stop. Ordered events stay ordered; continuous state may coalesce only where matching original semantics. Callback publishes compact playhead/voice activity/meters and acknowledgement; app does not mutate render state.

Audio publication proposal: preallocated bounded state slots and ordered command storage, atomic ownership/acknowledgement and retired PCM epochs. Single UI producer -> single playback callback supports SPSC command queue; MIDI input first reaches UI, so it is not another audio queue producer. State slot ownership prevents a UI write while audio reads. Callback never allocates/locks/waits/frees; capacity pressure handled outside audio callback with visible failure/lifecycle policy. Exact scheme and memory orders must be demonstrated in Phase1 tests rather than hidden in a generic messaging abstraction. Sample lifetime includes previews, live/sequence voices and pending commands. Device stopped/offline rendering uses explicit sole-owner transition, not concurrent queue consumption.

True capture/input/take/analyzer SPSC topology may retain original ring structures behind audio boundary. CoreMIDI serialization must be proved before SPSC use there. DSP scratch/effect storage allocated before playback, DX7 tables initialized explicitly during engine startup. Verify atomic lock freedom per supported architecture; unsupported primitive fallback cannot introduce audio mutexes.

Portable ordinary C file/format routines can stay in project modules. Only genuine OS operations move behind concrete helpers: sample mapping, atomic replace, directory enumeration/canonical paths, initial resource/config locations, recording file writes, thread lifecycle/clock where necessary. Do not wrap malloc/memcpy/ordinary fopen indiscriminately. Engine file policy stays separate from realtime render contract.

## Platform proposals for approval

SDL3 renderer, statically linked where allowed, miniaudio current original version0.11.21 first, pinned stb_truetype compatible with original font loader. Retain existing font (SIL OFL), original image/audio decoder headers/notices and adapted DX7 (Apache2). No SDL satellite libraries. Vendor archives/licenses/checksums; offline CMake has no FetchContent/network path. Exact dependency revisions selected and recorded when source archives are available, never inferred from installed shared SDL.

macOS: keep small Cocoa target-window scroll/magnify monitor; obtain NSWindow via SDL property, preserve summed raw precision/pinch and emulate the separate scalar GLFW path. macOS audio uses miniaudio CoreAudio; MIDI keeps CoreMIDI. Linux: miniaudio supported backend configuration and ALSA MIDI; SDL owns window/input/renderer. No extra dependencies beyond approved list/existing bundled components. Windows remains designed for portability but MIDI/backend/toolchain choices wait for Phase4 proposals and explicit approval; WASAPI is planned, no ASIO SDK.

Original font is a packaged TTF, not currently embedded in C source. User requests existing embedded font: propose embedding the exact existing font bytes during rewrite packaging, preserving license/metrics and avoiding a new font. No bytes copied into new repo in Phase0.

## Proposed toolchain verification matrix

| Target | Proposed toolchain/build | Status |
| --- | --- | --- |
| macOS arm64 | AppleClang C11 + matching CLT SDK + CMake>=3.20; Objective-C adapter and system frameworks | Original core verified AppleClang21/CMake4.4.3/SDK15.4, deployment15.4. Default SDK27 fails linker. Rewrite SDL/static build unverified. |
| macOS x86_64 | Same toolchain family, separate architecture artifacts (universal packaging later if required) | Unverified; do not claim support based on arm64. |
| Linux x86_64/aarch64 | GCC or Clang C11, CMake, system ALSA development API, platform libraries required by vendored SDL | Proposed, not verified here; exact minimum compiler/SDL prerequisites must be recorded after builds. |
| Windows x64 | MSVC C11 or MinGW-w64, selected in Phase4; lock-free atomics and math parity verified | Neither selected nor tested. No current supported-toolchain claim. |

## Phase gates

Before Phase1: preserve all Phase0 fixture files/harness settings unchanged inside test assets, verify hashes, obtain actual callback original fixtures/tests and missing desktop baseline when feasible, confirm source discrepancies. Same toolchain exact PCM hash + Player timing; other CPUs/toolchains report max abs/RMS sample error, differing sample count and exact timing state. Tolerance is selected from original cross-toolchain evidence, not invented to pass rewrite failures.

Phase2 original look/geometry and OS-independent interactions; replay both frozen original and rewrite where possible, timestamp/viewport/DPI/modifiers/initial state explicit. Test many-event wheel overwrite, native summed momentum, pinch logarithms, zero-boundary zoom, region guards, event wait/wake and scalar panels. Hands-on Mac mouse/trackpad acceptance remains user-controlled.

Phase3 only after parity acceptance; full layout approval first; restyle with unchanged DSP/behavior assertions. No named arrangement sections/snap-selector/new views inferred from art. Phase4 platform proposals approved before Windows work. Phase5 experiments remain optional and never automatically adopted.
