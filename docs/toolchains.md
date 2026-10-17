# Toolchains and build verification

No build downloads dependencies. SDL source must be imported from the pinned
local archive before claiming a shipping/offline desktop build. Headless engine
and interaction checks use the already vendored dependencies. Python3 standard
library is a test/measurement tool, not an application runtime dependency.

| Target | Configuration | Evidence |
|---|---|---|
| macOS arm64 | AppleClang21, CMake4.4.3, SDK15.4, C11 | Developer installed-SDL desktop and tests pass on macOS26.6.2 |
| macOS x86_64 offline core | AppleClang21 cross-target, SDK15.4, C11 | Engine/audio/platform services and test executables compile/link; execution unavailable (`Bad CPU type`); no desktop or audio-parity claim |
| macOS vendored/static | AppleClang, CMake; explicit deployment target | Build wiring/import verification prepared; SDL source unavailable, not built |
| Linux | C11 GCC/Clang, CMake, OS ALSA development API and SDL native window libraries | Source/recipe prepared; no Linux compiler/runtime verification on this host |
| Windows x64 | MinGW-w64 UCRT C11 candidate, CMake, native WinMM/WASAPI | Native source candidates only; no Windows compiler/SDK/runtime verification |
| Windows MSVC | Requires separate C11 atomic/toolchain checks | Not claimed supported or verified; do not silently substitute mutex atomics |

macOS development build (explicit installed SDK, not shipping static SDL):

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
