# Linux validation

Use the pinned SDL source import and development dependencies described in the
[README](../README.md). Optimized test executables retain libc assertions. The normal test build
exercises the desktop as well as
engine, file services, recording, MIDI logic, and UI interactions:

```sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_BUILD_DESKTOP=ON -DBUILD_TESTING=ON
cmake --build build-linux --parallel 2
ctest --test-dir build-linux --output-on-failure --parallel 2 --timeout 180
```

Two build jobs suit small machines; increase the limit if you have spare RAM
and CPU cores. CTest saves failure details in
`build-linux/Testing/Temporary/LastTest.log`. The desktop scene check launches
dozens of software-rendered windows and may take several minutes on small
machines. CTest runs that check by itself to avoid competing with other tests.

`installed_linux_desktop` installs to a temporary prefix containing spaces,
moves the install tree, clears `LD_LIBRARY_PATH`, and launches from an unrelated
working directory. It checks installed resources, renders four views, and saves
a project. The temporary install is removed afterward.

## Numeric reference policy

Frozen references were captured on macOS. CTest defaults to bounded comparisons
for explicitly selected numeric/raster artifacts because compilers and math
libraries can round differently. Reference SHA256 hashes, file shapes, discrete
integer fields, event order, serialized projects and audio transport state remain exact.
The allowances are:

| Artifact | Bound |
|---|---|
| Float PCM | Peak absolute error <= 1e-6 and RMS <= 1e-7; same frame/channel counts; finite samples |
| 16-bit WAV | Identical format and duration; at most one quantization step per sample |
| Text coordinates | Absolute error <= 1e-4 or relative error <= 2e-7; text structure and integer fields exact |
| Binary Bezier coordinates | Absolute error <= 1e-5 or relative error <= 2e-7 |
| Font rasters | At most one gray level, affecting <= 0.01% of bytes (minimum one byte); metrics and atlas geometry exact |

Only the named PCM, coordinate and font artifacts use these rules. Unknown
files remain byte exact. Comparator regression tests check that larger errors,
noise, changed integer fields, NaNs, lengths, WAV format and duration fail.
Previously approved Stretch contract changes remain separately identified in
the audio comparator. No frozen reference files are rewritten.

To require the original byte-identical numeric comparisons:

```sh
cmake -S . -B build-linux -DLIBRELOOP_STRICT_FIXTURES=ON
ctest --test-dir build-linux --output-on-failure
```

Set that option back to `OFF` for cross-toolchain checks. Standalone comparator
scripts are strict by default; CTest sets `LIBRELOOP_PORTABLE_FIXTURES=1` only on
the eight tests that need numeric portability. `fixture-results.json` records
all 64 audio reference scenarios, their peak/RMS differences and approved
Stretch changes.

## Memory and undefined behavior checks

Clang can run the core tests without SDL or desktop dependencies:

```sh
cmake -S . -B build-linux-sanitize -DCMAKE_C_COMPILER=clang \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS_DEBUG="-O1 -g" \
  -DLIBRELOOP_BUILD_DESKTOP=OFF -DBUILD_TESTING=ON \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-linux-sanitize --parallel 2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build-linux-sanitize --output-on-failure \
    --parallel 2 --timeout 300
```

## Native display and device checks

Most automated rendering tests intentionally use SDL's dummy/software drivers.
From a terminal in an X11 desktop session, this exercises the native X11/OpenGL
path and puts captures in a temporary directory:

```sh
exe="$PWD/build-linux/libreloop"
capture_dir=$(mktemp -d)
(cd "$capture_dir" && SDL_VIDEODRIVER=x11 SDL_RENDER_DRIVER=opengl \
  "$exe" --smoke --visual-preview)
```

The captures remain in `$capture_dir` for inspection. A Wayland session needs
its own native-driver check; X11 success does not establish Wayland support.

Before a release, also exercise real audio output and recording, device changes,
MIDI note/controller input and reconnect, long playback, clipboard/drop from
other applications, and resizing across displays with different DPI settings.
Automated callback tests do not measure physical device latency or xruns.
ALSA MIDI requires `/dev/snd/seq`; without it, MIDI parsing and recording logic
can still be tested, but physical MIDI input cannot.

The GitHub workflow runs core tests on Linux x86_64/ARM64 and macOS, pinned-SDL
desktop builds on Linux/macOS, a virtual X11 smoke run and Linux Clang sanitizers. Those hosted jobs need
their own successful runs before being treated as verified configurations.
