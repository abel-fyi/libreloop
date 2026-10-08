# v0.1 release preparation

The current build identifies itself as `0.1.0-dev`. It is a development build,
not a published v0.1 release. `libreloop --version` prints the label without
opening an audio device, MIDI input or window. macOS bundles also carry numeric
`CFBundleShortVersionString` and `CFBundleVersion` metadata.

## Build reviewable archives

Build with tests enabled first:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cpack --config build/CPackConfig.cmake -B local/packages
```

CPack makes a `.tar.gz` on Linux and a `.zip` on macOS, named with the version,
platform and architecture. Linux contains `bin/` and `share/`; run the executable
with both kept together. The Mac archive contains the app bundle. Both include
factory presets, generated samples, the bundled font and dependency notices.
No release is uploaded by these commands. Mac development archives are unsigned.
Build Linux release binaries on the oldest distribution you intend to support;
a binary built on a newer system is not automatically compatible with older glibc.

For a release candidate, set `-DLIBRELOOP_VERSION_LABEL=0.1.0-rc1` when configuring.
Use `0.1.0` only for the final release. Keep the numeric CMake project version and
macOS bundle metadata in sync when moving to a later release.

## Acceptance before tagging v0.1.0

- Require Linux/macOS desktop and sanitizer CI to pass on the release commit.
- On both physical machines, create a song using samples, DX7/Custom FM and Analog;
  edit notes/clips, automate parameters, replace instruments and use undo/redo.
- Save/reopen, collect samples, move the project and its companion directory,
  reopen from a different working directory, and export. Check missing-sample
  relinking, failed/canceled saves and the close prompt.
- Test a MIDI keyboard: velocity, pitch bend, sustain, controller learn, recording,
  Stop, disconnect/reconnect, and no stuck notes after a disconnect or overflow.
- Record audio, finish takes, save/reopen and export. Test sustained playback while
  editing, plus audio-device changes and sleep/wake.
- Extract the actual archives outside the checkout and verify resources and
  preset loading. Check palette contrast, display scaling and input alignment.
- For a smooth public Mac download experience, complete signing/notarization and
  test the distributed archive. Unsigned development builds must be identified.

Automated tests do not substitute for audible hardware and physical Mac checks.
Do not mark those checks complete based only on CI or a headless smoke run.

## Initial release scope and limitations

v0.1 is an experimental Linux/macOS workstation with sampler, Custom FM,
six-operator DX7 and analog synthesis; Chorus and Equalizer; audio/MIDI recording;
automation; undo/redo; portable collected projects; presets; and stereo WAV export.

Limits are 32 channels, eight patterns, 100 arrangement tracks, 100 mixer inserts,
128 notes per channel/pattern and 64 clips per track. External plugin hosting and
FLP import are outside this release. MIDI recording follows receipt timestamps
and is not sample-accurate input scheduling. Audio latency compensation is not
implemented. WAV export ends at the arrangement boundary without an extra release
or effect tail. Polyphony headroom depends on patch complexity and hardware.
