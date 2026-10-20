# Toolchains and build verification

No build downloads dependencies. SDL source must be imported from the pinned
local archive before claiming a shipping/offline desktop build. Headless engine
and interaction checks use the already vendored dependencies. Python3 standard
library is a test/measurement tool, not an application runtime dependency.

| Target | Configuration | Evidence |
|---|---|---|
| macOS arm64 | AppleClang21, CMake4.4.3, SDK15.4, C11 | Earlier installed-SDL desktop/tests passed on macOS26.6.2; current portable SDL input follow-up awaits native/hosted verification |
| macOS x86_64 offline core | AppleClang21 cross-target, SDK15.4, C11 | Engine/audio/platform services and test executables compile/link; execution unavailable (`Bad CPU type`); no desktop or audio-parity claim |
| macOS vendored/static | AppleClang, CMake; explicit deployment target | Pinned SDL source imported; macOS build and runtime await hosted/native verification |
| Linux | Gentoo x86_64, GCC 15.3, CMake 4.3, pinned SDL 3.4.16 static, ALSA | Desktop builds; 149 checks pass with the documented numeric portability policy; native X11/OpenGL and relocated installation smoke checks pass; see Linux validation below |
| Linux sanitizers | Gentoo x86_64, Clang 22.1.8, Debug with `-O1 -g`, AddressSanitizer and UndefinedBehaviorSanitizer | All 123 core checks pass with leak detection and halt-on-error enabled; no sanitizer findings |
| Windows x64 | MinGW-w64 UCRT C11 candidate, CMake, native WinMM/WASAPI | Native source candidates only; no Windows compiler/SDK/runtime verification |
| Windows MSVC | Requires separate C11 atomic/toolchain checks | Not claimed supported or verified; do not silently substitute mutex atomics |

### Historical macOS development verification

These SDK-specific flags and measurements describe the earlier implementation.
Use [building and platform policy](building.md) for current build commands.
The following was an installed-SDL developer build, not a shipping static build:

```sh
cmake -S . -B build-sdl-validation -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_VALIDATE_SYSTEM_SDL=ON \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/sdl3 \
  -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.4
cmake --build build-sdl-validation --parallel
ctest --test-dir build-sdl-validation --output-on-failure
open build-sdl-validation/LibreLoop.app
```

Its copied SDL library declares minimum macOS26, so the bundle plist also says26.
This host's default SDK27 fails TAPI linking; using SDK15.4 is a verified host
workaround, not a claim the development bundle runs on macOS15.4.

Prepared offline source build (unverified until SDL archive is supplied):

```sh
python3 tools/vendor_sdl.py /path/to/SDL3-3.4.16.tar.gz
cmake -S . -B build-desktop -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_BUILD_DESKTOP=ON -DBUILD_TESTING=OFF
cmake --build build-desktop --parallel
cmake --install build-desktop --prefix /path/to/staging
```

For Gentoo package prerequisites and the Linux desktop invocation, see the
Gentoo Linux section in the README. The Linux recipe imports the same pinned
SDL archive and builds it statically; no system SDL3 package is required.

### Linux validation (2026-10-20)

See [Linux validation](linux-testing.md) for commands, numeric acceptance bounds,
strict reference mode, installation checks, sanitizer setup, and device testing.

On Gentoo x86_64 (GCC 15.3, CMake 4.3.4, pinned static SDL 3.4.16), all 149
checks pass with the default cross-toolchain comparison policy. The usual
8 MiB stack limit is retained. Test portability fixes enable POSIX declarations,
declare drawing-library dependencies, discard unused drawing sections in
capture harnesses, move oversized sampler/arrangement fixtures off the stack,
and retain test assertions in optimized builds.
The many-scene visual check runs serially with a five-minute timeout to tolerate
slow hosts and competing workloads; it passed in 159 seconds under build load.

All 64 original audio scenarios were measured. Outside the previously approved
Stretch changes, the maximum PCM difference from the macOS reference was
1.79e-7, maximum RMS 1.21e-8, and 16-bit WAV differences were at most one
quantization step. Other measured differences included navigation coordinates
around 4.77e-7, waveform coordinates up to 4.58e-5, one sampler publication float
by 1.49e-8, and one font pixel by one gray level with identical metrics.
A Bezier coordinate at magnitude 97029 differed by 0.0078125 (relative 8.05e-8).
Frozen reference hashes, projects, audio transport state, and integer fields
remain exact. The eight affected comparisons pass bounded numeric/raster rules;
strict byte mode intentionally still exposes those cross-toolchain differences.

A separate Clang 22.1.8 Debug build (`-O1 -g`,
`-fsanitize=address,undefined -fno-omit-frame-pointer`) passed all 123 core
checks with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. No memory, leak, or undefined
behavior findings were reported. This instrumented build excludes SDL/desktop
code; the native display checks use the GCC desktop build.

A relocated temporary installation under a path containing spaces launched from
an unrelated working directory without `LD_LIBRARY_PATH`, rendered four views,
and saved a project. Installed fonts, presets, samples, and license notices were
checked. A separate native X11/OpenGL smoke run on the actual display also
rendered four views and saved a project. The wider automated desktop suite uses
dummy/software rendering. Native Wayland, real audio output/capture latency and
physical MIDI remain unverified here; `/dev/snd/seq` is unavailable on this host.

GitHub jobs are prepared for Linux x86_64/ARM64 and macOS core tests, pinned-SDL
Linux/macOS desktops, Linux X11 and Linux Clang sanitizers. Hosted results are not claimed until those jobs run.

### Portable C11 build follow-up

All application-owned runtime sources are now C; the Cocoa input adapter is
removed. SDL wheel/pinch events route through the same C adapter, with gesture
window isolation covered by tests. High-pixel-density requests and drag/drop
coordinates use SDL on all desktop backends; scissor scaling follows measured
pixel/logical geometry. Native macOS trackpad/momentum behavior needs testing
after this change, rather than assuming the old raw-Cocoa behavior is retained.

The desktop is the default CMake target and tests are opt-in. Missing SDL now
fails with preparation instructions. Core-only builds do not find ALSA, SDL or
Python unless tests explicitly require Python. `build_contract` checks those
configuration rules, missing-archive diagnostics, C-only source and pinned
header bytes. The normal Linux suite passes all 149 checks; the separate
Clang ASan/UBSan core suite passes all 123 with leak checking enabled.

Both pinned-source SDL and `LIBRELOOP_USE_SYSTEM_SDL=ON` were built on Gentoo.
For SDK validation, SDL 3.4.16 was actually installed into an isolated prefix
and discovered through its installed CMake package (static target). That desktop
rendered four views, saved a project, and passed the relocated Linux install test.
This validates an installed static SDK, not arbitrary distro/shared SDK versions.
The post-change native X11/OpenGL smoke check also passed. Current commands and
support boundaries are in [building and platform policy](building.md); older
SDK-specific macOS recipes elsewhere in this document describe historical runs.

### Platform installation notes

Native macOS builds now default their deployment target to the host version;
installed-SDL builds no longer hard-code macOS 26. Select and validate older
targets explicitly. The SDK-specific notes below are earlier verification history.

On macOS, additionally set the SDK/deployment target explicitly. The static
bundle packages assets/notices without copying a shared SDL runtime. Its minimum
is the selected deployment target and still requires real compatibility testing.
On Linux/Windows the install tree is bin/ plus share/libreloop containing fonts,
samples, presets, branding and notices. Shared SDL is an explicit unadopted
experiment; Linux locates that library via an origin-relative install RPATH and
Windows keeps its DLL beside the executable. A final Windows compiler-runtime
inventory and package test are still required.

The installed macOS developer bundle is verified after CMake's install-time
runtime-path edits: the final bundle is signed again, passes strict/deep signature
verification, and runs its four-view/project smoke capture from a temporary
staging path containing spaces. The regression test also checks installed assets,
notices and the absence of Homebrew runtime references. This is a dummy/software
launch check; Finder display, device access and shipping compatibility remain
unverified. Linux shared-library installation includes both the real library
filename and its loader SONAME; that path has not been exercised on Linux.

Windows native candidates use UTF-16 filename APIs, native threads (no
winpthreads), read-only file mapping, durable sibling saves, WinMM's dedicated
message thread, WASAPI and an8MiB UI stack for original by-value Project copies.
DPI/pointer coordinates normalize at the boundary. These source choices have not
been compiled or exercised on Windows. Several inherited test/measurement
harnesses still use POSIX temporary/process APIs and need Windows test adapters;
BUILD_TESTING=OFF above is a source-build recipe, not proof Windows tests pass.

Remaining platform verification: real audio latency/recording permissions,
MIDI devices/reconnect, DPI transitions, OS drop/clipboard, precision touchpads,
idle/active CPU/GPU, battery/input latency and physical feel. No estimates are
reported as measurements.

Verified x86_64 cross-build on the arm64 host:

```sh
cmake -S . -B build-macos-x86-validation -DCMAKE_BUILD_TYPE=Release \
  -DLIBRELOOP_BUILD_DESKTOP=OFF -DLIBRELOOP_VALIDATE_SYSTEM_SDL=OFF \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 \
  -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.4
cmake --build build-macos-x86-validation --parallel
```

The outputs are verified Mach-O x86_64 executables. The host cannot spawn
x86_64 executables, so CTest/audio renders were not run for this target. No
cross-CPU tolerance is selected from these compile-only results. The installed
SDL development runtime is arm64; it was excluded from this cross-build, so
the result does not establish an x86_64 desktop package.
