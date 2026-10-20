# LibreLoop

LibreLoop is a digital audio workstation I’m building for myself and sharing
under the GPL-3.0-only license. It is written in C11, runs locally, and has
built-in instruments and effects instead of external plugin formats.

The desktop has an arrangement timeline, pattern and piano-roll editors,
automation, audio and MIDI recording, a sample browser, and a mixer. It is
actively developed; expect rough edges and incomplete platform support.

## Build on Gentoo Linux

You need GCC or Clang, CMake, Python 3, ALSA development files, and the
development headers for the SDL window backends you want to use. For X11, SDL
needs Xlib, Xcursor, Xi, Xrandr, XScreenSaver, and Xxf86vm headers. Wayland
builds also need Wayland client/protocol, xkbcommon, and EGL development files.
Enable the `seq` USE flag on `media-libs/alsa-lib` for ALSA MIDI input.

LibreLoop builds against SDL 3.4 or newer. CMake does not download dependencies.
To build SDL from the pinned source archive, download SDL 3.4.16 from the
[official release](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.16)
and import it into the checkout. The importer verifies the archive checksum:

```sh
python3 tools/vendor_sdl.py /path/to/SDL3-3.4.16.tar.gz
```

Then configure and build the desktop:

```sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_BUILD_DESKTOP=ON -DBUILD_TESTING=OFF
cmake --build build-linux --parallel
./build-linux/libreloop
```

If SDL’s X11 setup reports a missing XScreenSaver dependency and you do not need
that integration, configure with `-DSDL_X11_XSCRNSAVER=OFF`.

The Linux desktop has compiled with GCC 15 on Gentoo. Audio and MIDI use ALSA;
actual device access depends on the host setup. Other distributions and BSDs
have not yet been verified.

## Build on macOS

The current development recipe uses AppleClang, CMake, and an installed SDL3 SDK:

```sh
cmake -S . -B build-sdl-validation -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_VALIDATE_SYSTEM_SDL=ON \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/sdl3 \
  -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.4
cmake --build build-sdl-validation --target libreloop --parallel
open build-sdl-validation/LibreLoop.app
```

The installed SDL runtime used for this development build requires macOS 26.
This is a developer setup, not a shipping package. See
[platform notes](docs/toolchains.md) for current verification and limitations.

## Project notes

- [Website](https://libreloop.org)
- [Behavior and controls](docs/behavior-spec.md)
- [Third-party licenses](docs/third-party.md)
- [Development roadmap](docs/roadmap.md)

The application is GPL-3.0-only; bundled components, fonts, and artwork keep
their own documented licenses.
