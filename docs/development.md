# Development

## Build on Linux

Use a C11 compiler, CMake 3.20 or newer, a build tool such as Make or Ninja,
pthreads, ALSA development headers, and X11/OpenGL development libraries. CMake fetches raylib 5.5 with
an SHA-256-verified archive; GLFW and miniaudio come from that archive. No system
raylib installation, raygui, or plugin framework is required.

On Debian/Ubuntu, the desktop build dependencies can be installed with:

```sh
sudo apt-get install build-essential cmake pkg-config libx11-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev libasound2-dev
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

Development builds fall back to the checkout's `samples/` and `assets/fonts/`.
Install the Linux executable, samples, font, desktop entry and license notices with:

```sh
cmake --install build --prefix "$HOME/.local"
```

Installed resources resolve relative to the executable under `../share/libreloop`,
so the installation can be moved independently of the checkout. Run from a writable
directory for recording files and smoke screenshots. Project saves and exports use
the location selected in the file chooser.

## Build on macOS

Install Apple's Command Line Tools (`xcode-select --install`) and CMake
(`brew install cmake` if you use Homebrew), then use the same configure,
build, test and launch commands above. CMake builds the bundled graphics
dependencies and links the system CoreAudio frameworks; no system raylib
installation is needed. Build natively on either Apple Silicon or Intel.

The macOS build produces `build/LibreLoop.app`, with its generated demo samples,
font and license notices inside the bundle. Launch it with `open build/LibreLoop.app`.
For command-line smoke checks use `build/LibreLoop.app/Contents/MacOS/LibreLoop --smoke`.
The bundle can be moved independently of the checkout. It is unsigned; signing
and notarization are still required for a public macOS release. Saved sample references are relative to the project directory; Collect samples
and save creates a companion directory for moving projects.

Command+A/V select all and paste in text fields on macOS; Control+A/V remain
Linux shortcuts. Both Mac Delete and forward Delete remove focused selections.
Linux and Retina rendering use the actual GLFW framebuffer-to-window ratio for
font and icon atlases. Text positions align to physical pixels, while input
and editor layout remain in logical window coordinates.
A small AppKit event monitor supplies precise trackpad scroll and pinch events;
GLFW continues to handle mouse wheel events. Linux uses GLFW scroll events only.
Finder launches from `/` use `~/Music/LibreLoop` for default saves and exports;
terminal launches keep their working directory.

If the default SDK fails to link a simple C program, select a compatible installed
SDK explicitly with `-DCMAKE_OSX_SYSROOT=/path/to/MacOSX.sdk`. This is a local
Command Line Tools mismatch; do not hardcode one developer's SDK in the project.

CI builds and tests macOS and Linux. Interactive editing, audible playback,
device disconnect/reconnect, and sleep/wake still require manual verification.

## X11 and Wayland

The default backend is X11; it can run on Wayland through XWayland. For a build
with native Wayland and X11 support, install Wayland development packages
(including wayland-scanner, wayland-client/cursor/egl, and xkbcommon), then use:

```sh
cmake -S . -B build -DGLFW_BUILD_WAYLAND=ON -DGLFW_BUILD_X11=ON
cmake --build build --parallel
```

## Tests

The desktop build has 36 headless CTest tests (31 in core-only builds). In addition
to engine, editing, audio and persistence checks, tests cover atomic write failures,
relative/collected assets, document close decisions, background recording and mapped
sample ownership through playback and undo.
They exercise rendering, project validation and backward
compatibility, crop boundaries, selection and movement, sampler processing,
window ownership and stacking, sample decoding, and Browser persistence.
The audio test invokes the callback without a device and checks unity gain,
continued playback under UI lock contention, output bounds, ordered controls
and safe sample replacement. CTest tests do not open an audio device or require a display.
Owned `Sample` values must be released with `sample_free`, and cloned with
`sample_clone`; mapped samples share read-only storage with reference counting.
The audio callback borrows samples and does not release storage.

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
tests the desktop configuration on Linux and macOS, plus a sanitized core
configuration on Linux.

## Visual checks

`./build/libreloop --smoke` writes `libreloop-smoke.png` and three
`libreloop-view-*.png` screenshots, then exits. Smoke checks force opaque windows regardless of saved appearance preferences. It requires a display;
Xvfb works for a headless run. Smoke scenes use the populated demo project.
Linux CI also runs the isolated GUI regression script:

```sh
python3 tools/check_gui.py --build build --dpi 144
```

It requires Xvfb, libX11 and libXtst, creates a temporary configuration, and checks
physical-pixel text rendering at 100/125/150/200%, complete compact captions
across fractional scales, dynamic UTF-8 glyphs, populated
smoke screenshots, and close/cancel/discard/save in the running app. It does not
require `xvfb-run` or `xdotool`. CI repeats the actual app input checks with X11
monitor DPI settings of 96, 120, 144 and 192, including actual window resize
events from 900×506 through 1920×1080 and the 154–157% layout range. Linux keeps raylib's high-DPI window
flag off to avoid its separate mouse/scissor scaling conflicting with the UI's
2D cameras; font density still follows the actual framebuffer. For interactive changes, check stacking,
resizing, capture during drags, hover help, and the affected playback behavior.
Use an isolated configuration and audio output when automating GUI checks.

## Samples and local artifacts

See [sample generation](../samples/README.md). Bundled WAVs are small generated
fixtures, not imported sample packs. Put private projects, exports, screenshots,
and references in `local/`; that directory is ignored. Build directories and
root-level runtime outputs are also ignored. Dependency source is fetched into
the build tree rather than copied into this repository.

## macOS release checks

Before distributing a release, verify these workflows on a physical Mac:

- Finder launch, save/reopen a project, WAV export, and an app moved out of the checkout.
- Command+A/V in rename/path/number fields; shortcuts must not audition notes.
- Delete selected notes/clips; delete text in dialogs without changing the project.
- Mouse and trackpad editing, drag/drop samples, editor resizing and scrolling.
- Retina and external-display rendering, including moving the window between displays.
- Sustained playback while editing and processing samples; listen for dropouts.
- Microphone permission, selected input recording, live waveform growth, multiple armed inserts, and Stop/save/reopen of takes.
- Playback after sleep/wake and audio-device disconnection/reconnection.
- Intel and Apple Silicon builds on the supported macOS versions.
- Signing and notarization for public distribution.

The callback continues playback when its UI mailbox is busy, applying pending
edits on a later buffer. Automated callback and rendering checks do not establish
dropout-free physical output; sustained interactive playback and device lifecycle
checks remain necessary on both platforms.

The in-app file chooser uses portable C directory enumeration and the existing
raylib UI. There are no GTK, Zenity, KDialog or native file-panel dependencies.
Chooser tests cover navigation, filtering, filename validation and overwrite
confirmation; opening the chooser does not create or modify files.

## Reproducible engine benchmarks

Use a Release build; the optional benchmark target is excluded from ordinary builds:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target benchmark_engine --parallel
python3 tools/run_benchmarks.py --build build --seconds 10 --repeats 3
```

The runner writes raw results and medians to `local/performance/engine.json`. Each
scenario gets a separate process, 128 warmup blocks, 48 kHz stereo output, and
512-frame or 64-frame buffers. Engine CPU is process CPU seconds divided by
rendered audio seconds: 100% uses an entire realtime core budget. Block latency
uses the monotonic wall clock, so scheduling interruptions can affect the maximum.
These are offline engine measurements, not hardware dropout or audible latency
measurements. Sample stress cases deliberately share one read-only PCM buffer;
their memory figures do not represent importing 32 distinct long recordings.

`build/benchmark_engine fm_32 3 512 verify` additionally hashes the rendered PCM
for differential checks. Hashing is excluded from ordinary timing runs. Native
GUI benchmarks require a separate measurement because they include graphics,
analyzers, the audio callback and device threads.

MIDI uses the system CoreMIDI framework on macOS and ALSA sequencer on Linux.
The ALSA sequencer must be available to the running user for Linux MIDI input.
Core-only tests do not require MIDI hardware or ALSA headers.

## Release preparation

See [the v0.1 release guide](release.md) for version labels, CPack archives,
physical-device acceptance and the current limits. The local build is marked
`0.1.0-dev`; creating an archive does not publish a release.

Palette changes can be measured without starting the DAW:

```sh
python3 tools/audit_palette.py --output docs/palette-measurements.md
python3 tools/sync_website_palette.py ~/libreloop-website
```

The audit checks labels, selected states, composited directional rings, spectrum
bars, clip artwork and perceptual separation. The reference report lists actual
ratios and saturation. Inspect the smoke screenshots as well, since
pixel size and layout still affect readability.
