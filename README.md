# LibreLoop

LibreLoop is a digital audio workstation I’m building for myself and sharing
under the GPL-3.0-only license. It is written in C11, runs locally, and has
built-in instruments and effects instead of external plugin formats.

The desktop has an arrangement timeline, pattern and piano-roll editors,
automation, audio and MIDI recording, a sample browser, and a mixer. It is
actively developed; expect rough edges and incomplete platform support.

## Build

LibreLoop's application runtime is C11, with SDL3 for desktop services and
miniaudio for audio. You need a C11 compiler, CMake 3.20+, and the platform
requirements below. Python 3 is used for source preparation and tests.

Prepare the pinned SDL source once per checkout (CMake never downloads it):

```sh
curl --fail --location --output SDL3-3.4.16.tar.gz \
  https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
python3 tools/vendor_sdl.py SDL3-3.4.16.tar.gz
```

The same commands build the desktop on Linux and macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

Run `./build/libreloop` on Linux or `open build/LibreLoop.app` on macOS.
Tests are enabled with `-DBUILD_TESTING=ON`. An installed SDL SDK can be selected
with `-DLIBRELOOP_USE_SYSTEM_SDL=ON` instead of importing source.

### Linux / Gentoo

Install ALSA development files and the headers for SDL's window backends. For
X11, SDL needs Xlib, Xcursor, Xi, Xrandr, XScreenSaver and Xxf86vm headers.
Wayland builds also need Wayland client/protocol, xkbcommon and EGL development
files. Enable the `seq` USE flag on `media-libs/alsa-lib` for MIDI input.
If XScreenSaver support is unavailable, `-DSDL_X11_XSCRNSAVER=OFF` disables it.
Audio uses miniaudio's available host backends; MIDI currently uses ALSA.

### macOS

Install Xcode Command Line Tools (`xcode-select --install`). CMake selects the
SDK; native builds default to the host macOS deployment target. An older target
must be selected and verified explicitly. SDL handles desktop input, including
pinch gestures; LibreLoop has no Objective-C runtime sources.

See [building and platform policy](docs/building.md) for dependencies, system-SDL
builds, installation, core-only builds and verification scope. See
[Linux validation](docs/linux-testing.md) for tests and sanitizer commands.

## Project notes

- [Website](https://libreloop.org)
- [Behavior and controls](docs/behavior-spec.md)
- [Third-party licenses](docs/third-party.md)
- [Development roadmap](docs/roadmap.md)

The application is GPL-3.0-only; bundled components, fonts, and artwork keep
their own documented licenses.
