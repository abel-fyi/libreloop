# Building and platform policy

LibreLoop's application-owned runtime sources are C11. SDL3 owns windowing,
rendering, desktop input and clipboard delivery; miniaudio owns audio-device
access and decoding. The pinned stb headers provide image/font support.
Dependencies may use native platform languages internally. Python is only a
development tool; it is not required to run the application.

Small native **C** adapters remain for MIDI, durable saves, file mapping and
filesystem services. They are behind platform interfaces. A new dependency must
remove enough maintenance to justify its build requirements and preserve the
features we use; a C API alone does not make an implementation C. No new MIDI
library has been selected or added.

## Requirements

- A C11 compiler and CMake 3.20 or newer, plus the build tool CMake selects.
- SDL 3.4 or newer; the reproducible reference build uses SDL **3.4.16**.
- Linux desktop builds need ALSA development files for MIDI and the development
  dependencies of the SDL backends being enabled. See the README's Linux notes.
- macOS desktop builds need Xcode Command Line Tools and its SDK.
- Python 3 is needed to import the pinned SDL archive or run tests.

miniaudio 0.11.21 and stb headers are already in `third_party/`; their exact bytes
and notices are recorded in `third_party/manifest.json`. No library is downloaded
by CMake. SDL's local source import verifies its archive and records provenance.

## Reference desktop build: Linux and macOS

Once per fresh checkout, obtain the pinned SDL release archive and import it:

```sh
curl --fail --location --output SDL3-3.4.16.tar.gz \
  https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
python3 tools/vendor_sdl.py SDL3-3.4.16.tar.gz
```

Then use the same build commands on both systems:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

The default builds the desktop with pinned static SDL; tests are opt-in. Missing
SDL fails with preparation instructions, rather than silently building only the
core. The source archive and imported tree are local build inputs; importing is
unnecessary when a prepared source distribution already contains the tree.

Run `./build/libreloop` on Linux or `open build/LibreLoop.app` on macOS.

A native macOS build defaults its deployment target to the host macOS version.
CMake selects the installed SDK. To target an older OS, set
`-DCMAKE_OSX_DEPLOYMENT_TARGET=...` explicitly and verify that the SDK and all
linked dependencies support it. A successful build on a newer machine is not
proof of older-OS compatibility. Cross-compilation also requires an explicit
SDK/toolchain and deployment target.

## Installed SDL SDK / distribution builds

Packagers and developers can use an installed SDL >= 3.4 SDK:

```sh
cmake -S . -B build-system -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_USE_SYSTEM_SDL=ON
cmake --build build-system --parallel 2
```

Set `-DCMAKE_PREFIX_PATH=/path/to/sdk-prefix` if the SDK is not on CMake's search
path. The SDK must provide its SDL3 CMake package. Shared SDL is selected when
available, otherwise static SDL. `LIBRELOOP_VALIDATE_SYSTEM_SDL` remains an alias
for existing developer recipes. The pinned source build is the CI reference;
system SDKs require validation against the selected version. On Linux, shared
system SDL stays a system dependency and is not copied into the install tree;
the system loader must be able to find it.

On Linux, stage an installation without root access:

```sh
cmake --install build --prefix "$PWD/stage"
./stage/bin/libreloop
```

The layout is `bin/` plus `share/libreloop/`. macOS installs a self-contained
`.app` bundle. The installation tests exercise these final layouts, including
paths with spaces. Creating distro packages is a separate downstream task.

## Tests and core-only builds

Enable the test suite explicitly:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --parallel 2 --timeout 180
```

For core development without SDL, ALSA or a GUI SDK:

```sh
cmake -S . -B build-core -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_BUILD_DESKTOP=OFF -DBUILD_TESTING=ON
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure --parallel 2 --timeout 180
```

Core tests still require Python. A core library build with `BUILD_TESTING=OFF`
does not. See [Linux validation](linux-testing.md) for sanitizers and numeric
reference policy.

## Platform verification

Linux x86_64 is verified locally on Gentoo. CI covers Linux x86_64 and macOS
Apple Silicon core/desktop builds using the same pinned SDL source, plus
generic Linux ARM64 core builds. Hosted
results remain pending until the workflow actually runs. Asahi desktop/device behavior is a
future native validation target; a generic ARM64 CI run cannot establish it. Windows sources remain present, but Windows
builds and runtime behavior are not currently verified.

Input now uses SDL's normalized fractional wheel and pinch events on every
platform, replacing the app-owned Cocoa monitor. Fractional wheel units do not
expose Cocoa's old raw precise-scroll distinction, so Mac trackpad feel may
change. Pinch scales are converted to the existing zoom model; invalid scales
are ignored. Real Mac trackpad/momentum, Wayland, mixed-DPI displays and physical
MIDI/audio devices require native sessions in addition to automated checks.
