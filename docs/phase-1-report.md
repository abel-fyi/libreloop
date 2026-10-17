# Phase 1 engine port report

Continuation authorized by the user; no routine confirmation pause is introduced. The engine and original core interactions now build as C11 modules, with audio/filesystem services in platform/. UI drawing/native MIDI integration follows in Phase2. Full original functionality is the target; no UI feature or project schema has changed.

## Completed

- Vendored original miniaudio0.11.21 and original stb_truetype/image headers with embedded notices and checksum provenance; original font/factory assets retained. SDL source vendoring remains pending.
- Extracted original engine/DSP/device/timeline/project/automation/MIDI-take code and pure interaction/history/window models. Removed POSIX headers/thread-once/string extensions from engine/interaction; genuine path/directory/mapping/atomic-save/recording-worker operations remain behind platform services.
- Replaced callback mutex mailbox with bounded SPSC committed commands, eight immutable Project+plan slots and serial sample-retirement acknowledgement. Replaced shared view mutex with three owned view slots. Meter maximum publication is bounded to two strong CAS attempts under its single-writer/zero-clearing-consumer invariant. See audio-publication.md for memory ordering and topology.
- Carried all Phase0 expected PCM/project/state/export files unchanged into tests/fixtures/original. Test runner hashes the goldens before comparing, reports max/RMS/differing samples, and never regenerates expected outputs. Default acceptance is exact. Nonzero tolerance is explicit and does not relax timing/state checks.

## Verified and how

-53/53 Release CTest pass on macOS arm64, AppleClang21/CMake4.4.3/SDK15.4. Includes original core/decoder/recording-session/audio tests, all64 original fixture scenarios,14 runtime callback audits and guard positive control.
- All64 fixture scenarios have maximum and RMS sample error0; original Player/command/preview timing state and WAV bytes exact. Every one of88 original audio CHECK expressions remains literal and unchanged. Mutex-based delayed-publication test inputs were adapted to the new publication boundary at the same audio frame; expected files and assertions unchanged, documented in audio-publication.md.
- ASan+UBSan audio test passes, including concurrent callback/sample replacement, shared capture inputs, queue pressure, stop/seek/key-up and stopped-device behavior. TSan passes the same concurrent workload. LeakSanitizer is unsupported on this host; no leak-detector claim.
- Runtime dynamic guard detects the frozen original callback's pthread_mutex_trylock and rejects it as expected. New callback modes exercise sequenced/live samples, all synth engines, preview/metronome, output clipping, automation/resync and recording taps without intercepted allocation/free, locks, stdio/file calls or sleeps. The guard is test-only and not shipped.
- Engine/interaction source audit finds no SDL/raylib/GLFW/rlgl or POSIX API/header dependence. Source/reference182-file byte manifest remains unchanged. Original .llp schema/version and factory DSP source arithmetic retained.

## Unverified and remaining work

- Graphical launch, real device callbacks/latency, mapped-sample page residence, callback-stack worst cases, long-session recording, Linux/Windows and other CPUs/toolchains. Dynamic guard proves exercised API paths, not arbitrary OS scheduling or page-fault latency. Mapped original/history sharing is retained, so large-file storage behavior has not been silently weakened.
- UI calls/gestures still need their Phase2 SDL boundary and deterministic event replay. Native CoreMIDI/ALSA wrappers exist in platform/ but are not yet wired into a new GUI. No visual pass performed.
- SDL3 source archive not yet available; shell DNS is restricted. User received one command to obtain the official archive; no package/system installation attempted.

## Render measurements

Same original benchmark source,5 scenarios ×64/512 blocks ×3 repeats,3s audio and128 warmup blocks, no PCM file writes/hashing during measurement. Render-only numbers, not app CPU. Raw original90-run baseline and current30 runs retained; measurements are not interleaved/power-isolated, so small differences cannot establish a speedup or regression. No observed deadline misses in current runs.

| Scenario | Frames/block | CPU median % | p99 median ms | Max observed ms |
| --- | ---: | ---: | ---: | ---: |
| idle_monitor | 64 | 0.139 | 0.0020 | 0.1590 |
| demo_song | 64 | 0.904 | 0.0200 | 0.0430 |
| sampler_32_stretch | 64 | 8.195 | 0.6420 | 0.6870 |
| fm_32 | 64 | 5.296 | 0.0800 | 0.2760 |
| fm_analog_32 | 64 | 12.981 | 0.1850 | 0.3410 |
| idle_monitor | 512 | 0.105 | 0.0120 | 0.0280 |
| demo_song | 512 | 0.871 | 0.1000 | 0.1290 |
| sampler_32_stretch | 512 | 8.152 | 0.8910 | 0.9130 |
| fm_32 | 512 | 5.231 | 0.6310 | 0.6850 |
| fm_analog_32 | 512 | 12.945 | 1.4140 | 1.4440 |
