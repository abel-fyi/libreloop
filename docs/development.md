# Development

## Build on Linux

Use a C11 compiler, CMake 3.20 or newer, a build tool such as Make or Ninja,
pthreads, and X11/OpenGL development libraries. CMake fetches raylib 5.5 with
an SHA-256-verified archive; GLFW and miniaudio come from that archive. No system
raylib installation, raygui, or plugin framework is required.

On Debian/Ubuntu, the desktop build dependencies can be installed with:

```sh
sudo apt-get install build-essential cmake pkg-config libx11-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

See the [raylib Linux build instructions](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux)
for other distributions. Audio uses miniaudio's runtime-selected system backends;
raylib's audio module is disabled.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/libreloop
```

The samples path is currently set to the checkout's `samples/` directory at
build time. Run from a writable directory: project saves, WAV exports, and
smoke screenshots are written into the launch directory. Installation and
portable release packaging are not implemented yet.

## X11 and Wayland

The default backend is X11; it can run on Wayland through XWayland. For a build
with native Wayland and X11 support, install Wayland development packages
(including wayland-scanner, wayland-client/cursor/egl, and xkbcommon), then use:

```sh
cmake -S . -B build -DGLFW_BUILD_WAYLAND=ON -DGLFW_BUILD_X11=ON
cmake --build build --parallel
```

## Tests

The desktop build has six CTest tests: engine, arrangement, sampler, windows,
decoder, and browser. They exercise rendering, project validation and backward
compatibility, crop boundaries, selection and movement, sampler processing,
window ownership and stacking, sample decoding, and Browser persistence.
They do not open an audio device or require a display.

Core-only builds need no network, graphics dependencies, or audio library:

```sh
cmake -S . -B build-core -DBUILD_GUI=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

For address and undefined-behavior sanitizers:

```sh
cmake -S . -B build-asan -DBUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

Use `-DBUILD_TESTING=OFF` to omit test executables. GitHub Actions builds and
tests the desktop configuration and a sanitized core configuration on Linux.

## Visual checks

`./build/libreloop --smoke` writes `libreloop-smoke.png` and three
`libreloop-view-*.png` screenshots, then exits. It requires a display;
Xvfb works for a headless run. For interactive changes, check stacking,
resizing, capture during drags, hover help, and the affected playback behavior.
Use an isolated configuration and audio output when automating GUI checks.

## Samples and local artifacts

See [sample generation](../samples/README.md). Bundled WAVs are small generated
fixtures, not imported sample packs. Put private projects, exports, screenshots,
and references in `local/`; that directory is ignored. Build directories and
root-level runtime outputs are also ignored. Dependency source is fetched into
the build tree rather than copied into this repository.
