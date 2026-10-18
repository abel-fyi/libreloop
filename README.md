# LibreLoop

LibreLoop is a self-contained C11 digital audio workstation with built-in
instruments and effects, audio/MIDI recording, automation, a piano roll and an
arrangement timeline. Its SDL3 interface uses a compact, colorful desktop style
and the openly licensed PxPlus IBM VGA 8x16 font.

LibreLoop does not host external plugin formats. Instruments and effects are
part of the application. The application source is GPL-3.0-only; bundled
components, fonts and artwork retain their own documented licenses.

## Build and run

The currently verified desktop development configuration is macOS arm64 with
AppleClang, CMake and an installed SDL3 SDK. From this repository:

```sh
cmake -S . -B build-sdl-validation -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_VALIDATE_SYSTEM_SDL=ON \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/sdl3 \
  -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.4
cmake --build build-sdl-validation --target libreloop --parallel
open build-sdl-validation/LibreLoop.app
```

The explicit SDK path is a workaround verified on the development machine;
choose an installed SDK appropriate to your toolchain. The installed SDL runtime
used here requires macOS 26, so this development bundle also requires macOS 26.
See [toolchains](docs/toolchains.md) for platform status and limitations.

To build the available offline engine/interaction targets and run their tests:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For the desktop test suite, build all targets in `build-sdl-validation` and run
`ctest --test-dir build-sdl-validation --output-on-failure`.
Python 3's standard library is used for test orchestration, not app runtime.

## Current status

The desktop is a development build. Its Arrange/Pattern pages sit above a docked
Mixer; instruments and effects open as floating editors. New sessions start in
SONG mode. The Browser and Mixer
collapse, and the piano-roll velocity lane resizes and collapses. Controls share
pixel-style materials, labeled action menus and five-dot divider grips.

Engine, project format and interaction regression fixtures preserve the original
LibreLoop reference. Retain the fixtures: they are test data, including binary
render/audio results, rather than generated build debris. The old raylib source checkout, temporary
baseline builds, local measurements, history backups and supplied screenshots
were removed during workspace cleanup; committed fixtures and audit documents
remain.

The shipping desktop still needs a verified vendored/static SDL build. CMake
never downloads dependencies; `tools/vendor_sdl.py` imports a locally supplied,
checksum-pinned SDL archive. Linux/Windows runtime checks, physical audio/MIDI
hardware and native trackpad/HiDPI acceptance remain incomplete. See
[toolchains](docs/toolchains.md) and [verification status](docs/completion-blockers.md).

## Development

- [Next work and priorities](docs/roadmap.md)
- [Stereo Reverb](docs/reverb.md)
- [Tube saturation](docs/tube.md)
- [Delay](docs/delay.md)
- [Behavior specification](docs/behavior-spec.md)
- [Shared controls and velocity lane](docs/velocity-lane-and-controls.md)
- [Visual decisions](docs/visual-notes.md)
- [Architecture](docs/architecture-proposal.md)
- [Third-party licenses and provenance](docs/third-party.md)

Put future visual references in [references/screenshots](references/screenshots/README.md);
those files remain local and ignored. Local `notes.txt`, editor recovery files
and build outputs are also ignored. Earlier
design/audit documents remain as development history; the roadmap identifies
current priorities. The project and CMake application target are **libreloop**;
any local directory called `libreloop2` is only a historical workspace path.

The built-in [Dynamics multiband compressor](docs/dynamics.md) provides editable per-band transfer curves and live gain monitoring.
