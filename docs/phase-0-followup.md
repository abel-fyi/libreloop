# Phase 0 follow-up after continuation authorization

The user authorized continued work on2026-10-10 and asked to avoid routine questions. The architecture proposal and frozen-source parity policy are taken as accepted for proceeding with engine work. Specific visual/layout/Windows/experiment decisions still require their concrete review when reached.

## Completed and verified

The exact raylib5.5 archive was recovered read-only from `/Users/a/libreloop/build/_deps/raylib-subbuild/raylib-populate-prefix/src/archive.tar`. SHA256 matches original CMake pin `aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e`. Copied/extracted only in this workspace baseline area. Other checkout code was not used as the reference. Frozen original source bytes remain unchanged.

Original desktop Release builds out-of-source with cached archive override and SDK15.4. All39 CTest pass, including decoder/recording-session/audio/Browser/theme (theme render mode is not included). Actual callback fixture harness now invokes original audio.c callback:7 scenarios ×64/512 blocks,96000 frames each. Fresh-process repeat matches PCM and frame/command acknowledgement/preview state text exactly; recording mode also captures three post-fader take streams. Includes queued keyboard/note events, previews, metronome, gain/clamp, all three synth engines, busy UI mailbox, tempo/pitch edits, loop/automation/audio resync and capture bus routing. Device lifecycle is a miniaudio state test double, not physical-device evidence. Expected outputs remain original-owned and are archived under baseline-artifacts/audio/callback/. Existing50 engine/16 export goldens unchanged.

Release executable size is4,317,960 bytes (unstripped Mach-O). Link-map attribution and Mach-O dependency list retained outside original; linked raylib/GLFW symbol sizes by section in baseline-metadata/original-size.json. Symbol contributions exclude dead-stripped objects and separate zero-fill BSS; alignment/shared strings/import tables/debug/symbol/signing metadata cannot be perfectly assigned by library. OpenGL links the OS framework dynamically: framework implementation bytes are not embedded in this binary. Do not equate static archive sizes with linked contribution.

## Unverified

Graphical launch in this session aborts during GLFW/Cocoa initialization before a window is available. No original smoke images, GUI CPU/GPU, scroll/zoom/playback waveforms or actual trackpad feel measured. Source/fixture tests do not substitute for these. The failed smoke log and build/test logs remain in baseline directories. Hardware latency/audio/MIDI devices and Linux/Windows runtime checks remain unverified.

SDL3 source is still needed for offline vendoring; the user was given a direct official3.4.16 download command. Engine work can proceed with recovered miniaudio/stb sources while this archive is obtained.
