# Phase 0 report

Stopped at Phase0. No application code, dependency vendoring, engine/UI port, restyling or Phase1 work has started. Study artifacts are committed in this separate libreloop2 repository. Measurement coverage is incomplete for the explicit environment reasons below; this report does not claim full baseline/behavioral acceptance.

## Completed

- Frozen original commit `5e860696fe6cb6cd4c170f6ebf5378039b3e4af6`; initial status clean. All original/concept files hashed; builds/harnesses/measurements kept outside original.
- [Behavior inventory](behavior-spec.md), exact source-line [constants/branches](interaction-constants.md), [parameter/schema inventory](parameter-inventory.md) and [89-file/function index](source-inventory.md). Includes source-defined navigation, gestures, auto grid, automation, audio/MIDI/control recording, presets, processing, assets, history, file chooser, all built-in instruments/effects and capacity/error behavior.
- [raylib/GLFW/rlgl dependency map](raylib-dependencies.md) and [106 production APIs](raylib-api-inventory.md) with source call sites and SDL3/own-code replacements; two additional test-only APIs listed separately. Native macOS raw precision/momentum/pinch accumulation and pointer anchors recorded, including separate0.1-scaled/last-event scalar panel path.
- [Both concept images reviewed](visual-notes.md), chosen visual policy documented; nonexistent/partial counterparts and geometry/functional conflicts explicitly listed. No new views, modulation matrix, effects, quantize/snap controls or markers added.
- [Original-generated audio/render/export baselines](baseline.md):50 engine PCM fixtures at64/512 frames,16 export WAVs, archived inputs, render timing state and SHA256 manifests. Temporary harnesses link original code.90 separate original engine benchmark runs retained.
- [Real-time/shared-state and idle-loop audit](realtime-audit.md), [Windows platform assumptions](platform-assumptions.md) and [full call sites](platform-call-sites.md).
- [Architecture/module/toolchain proposal](architecture-proposal.md) ready for user review; existing Cocoa adapter and original font retention proposed. [GPL/dependency licensing inventory](third-party.md), DX7 notice retained.

## Verified and how

- Original core out-of-source Release builds using AppleClang21, CMake4.4.3, installed SDK15.4/deployment15.4.34/34 CTest pass; assertions remain active via CHECK macros. Logs retained in sibling baseline-build/.
- Every50 expected engine PCM fixture reproduced byte for byte on a second fresh-process run.16 export WAVs and feature input/source/timing files also repeated exactly. Export capture addition left previous PCM/state hashes unchanged.82 raw PCM artifacts (50 expected plus32 input copies) checked for finite samples, stereo-aligned length and hashes.
- Original unmodified benchmark measured render-only CPU/block timing across15 scenarios,64/512 blocks,3 repetitions each. Median table and raw records in baseline.md. No observed deadline misses in those short offline runs; this is not hardware latency/dropout evidence.
- Host probe reports lock-free uint_fast64,uint,int,uchar,float atomics. This proves that property only on this compiled target, not Windows/other CPUs.
- Both concept files visually inspected. Source-derived interaction/API/portability inventories checked against frozen files. Original custom point collision/OS key repeat/wheel behavior inspected through pinned upstream source, not assumed from SDL defaults.
- Final file-hash/status check confirms original remains clean at frozen commit and original/concept file bytes unchanged. Expected artifact hashes match archived manifests. Local documentation links checked; new repo contains documents/license/JSON metadata only, no app source/build files. New repo has its own logical documentation commits.

## Unverified / unavailable

- Desktop Release build failed because pinned raylib5.5 archive could not resolve/download codeload.github.com; miniaudio is also supplied by that archive. Default SDK27 failed a trivial compiler-probe link (unknown TAPI architecture); explicit SDK15.4 fixes core build. Neither original code nor installed toolchain was modified to work around this.
- Actual audio.c callback fixtures/tests (mailbox ordering, preview/metronome/listening clamp, capture taps, sample-retirement races) not executed. Shared render engine fixtures are established, but **cannot stand in for this missing callback baseline**. Before porting audio.c, obtain offline pinned dependencies and capture original callback outputs unchanged.
- Release application binary size and raylib/GLFW/OpenGL attribution; idle/active GUI CPU/GPU; waveform scrolling/zoom/playback at1x/HiDPI; GUI pixels/fonts/renderer timing. No application binary was available. Process inspection and hardware-model queries also denied. Historical performance.md numbers were not repurposed as new measurements.
- Native trackpad/mouse/pinch/momentum/direction/calibration and physical MIDI/audio/recording devices, hardware latency, extended session/race tests. Source/repeatable core tests do not prove hands-on feel.
- Linux/macOS x86_64/Windows runtime/build checks, selected SDL renderer/static builds, dependency offline-build completeness, every dependency header/license/transitive version; dependency source vendoring is later phase work.
- Cross-toolchain/CPU tolerance comparison. No arbitrary acceptance tolerance selected; same-toolchain expected PCM must remain exact.

Unavailable measurements are recorded explicitly, not estimated. Detailed reproduction procedures and gaps are in baseline.md and realtime-audit.md.

## Source and prompt conflicts to resolve before changing behavior

1. Frozen source has no count-in, copy-drag, slide-note mode, quantize command or snap bypass. It implements automatic visible grid, unquantized MIDI recording and existing clipboard functions. Confirm that this commit is the intended behavioral target; adding missing items would be new work.
2. Usage guide says new-note drag locks pitch/start and changes length, and onset/pitch collisions are blocked. Current code creates then moves notes and allows overlapping/duplicate onsets in the relevant APIs. Preserve source behavior unless user requests changes.
3. Named arrangement section markers requested visually are absent from project model/UI; original has playback-start arrows and ruler loop ranges only. No feature was invented to match art. Marker feature decision remains outside approved rewrite scope.
4. Picker ghost threshold does not gate final drop, picker lacks a universal Escape cancellation, horizontal zoom has no fixed clamp, direct resize/stretch may overlap clips. These are parity quirks, not silently fixed bugs.

## Approval and next-session gate

Review the [architecture proposal](architecture-proposal.md), including minimal SDL+Cocoa platform adapter, original-font embedding proposal and atomic callback publication direction. Approval is requested because the task explicitly requires architecture review; no dependency/plugin/skill approval rule introduced this gate.

No Phase1 follows automatically. Wait for explicit instruction after this report. Missing original callback fixtures must be established before that path is ported. Missing desktop measurements should be collected from frozen original out-of-source once offline raylib sources and measurement access are available. Every Phase1 expected fixture must originate from this frozen original, never the rewrite.
