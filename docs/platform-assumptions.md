# Windows portability inventory

[Full call-site ledger](platform-call-sites.md) records original file/line for OS, POSIX, path and compile-time assumptions. This proposal does not select Windows MIDI/toolchain before Phase 4 approval.

| Assumption / original files | Proposed minimal replacement or handling |
| --- | --- |
| pthread mutex/thread/join in audio, sampler worker, recording writer, ALSA MIDI; pthread_once in DX7 | Callback locks removed in Phase 1. Workers use small platform thread lifecycle (SDL threads or OS APIs hidden behind interface); explicit startup DX7 preparation removes pthread from DSP. No dependency in interaction/engine headers. |
| clock_gettime(CLOCK_MONOTONIC), nanosleep (audio.c/midi_input.c/recording_writer.c) | Platform monotonic clock and worker sleep; ordinary C portable clocks where adequate. Callback clock guarantee verified per backend. Windows QPC proposal, no universal memory/clock wrapper. |
| open/fstat/mmap(MAP_SHARED)/munmap and POSIX fd lifecycle (sample_storage.c) | Platform read-only sample mapping using CreateFileW/CreateFileMappingW/MapViewOfFile, retained refcount/lifetime and growing-recording semantics; source/tail and44-byte float WAV alignment validated. No POSIX in sample type. Realtime residence requires explicit policy. |
| mkstemp/fchmod/fdopen/fileno/fsync/unlink/rename-overwrite (atomic_file.c) | Small atomic-file platform implementation; unique sibling temporary, flush and replace existing file, original failure preservation. Windows replacement/open-handle behavior differs; use wide paths and OS replacement, keep portable FILE stream facade if suitable. |
| mkstemps/pwrite/lseek/write/EINTR/ftruncate/fsync/close/unlink (recording_writer.c) | Platform recording-file handle/write-at/commit helper or worker implementation; preserve checkpoints/chunking/captured-audio recovery. No libc-name macro swaps that change semantics. |
| mkdtemp/rmdir/unlink/realpath (project_assets.c) | Platform temp-directory/resource/path operations preserving rollback and immutable companion assets. |
| dirent/opendir/readdir/closedir/stat/S_ISDIR/S_ISREG/access/getcwd/realpath (file_chooser.c) | Platform directory metadata/iteration/path resolution. Keep own chooser model, hidden toggle/sorting/extension filtering. Windows hidden attributes vs dotfile policy must be proposed; avoid native dialog. |
| strdup/strnlen/strcasecmp/strncasecmp, strings.h | Small portable implementations or toolchain equivalents; maintain byte/case comparison semantics. C11 itself does not provide these; MSVC names/extensions differ. |
| PATH_MAX/limits.h, fixed Project paths1024, name48/device128; slash root/parent parsing | App-defined bounded UTF-8 buffers, OS conversion at boundary; format capacities unchanged. Handle drive-root, UNC, separators, absolute-path recognition, case rules, symlinks/reparse points. Do not change project schema just to use Windows MAX_PATH. |
| reference[0]=='/', strrchr('/',...), realpath before save/load and relative path common-prefix arithmetic (assets/preset/chooser/app) | Concrete path helper supporting old forward-slash relative .llp references plus Windows volume roots and cross-volume absolute references; approve behavior if incompatible old paths cannot resolve. Preserve serialized portable references. |
| HOME/XDG_CONFIG_HOME/.config/libreloop and old homebeat folders (browser/app_midi/chooser) | Propose Windows known-folder config locations with existing settings migration; filesystem text encoding and per-purpose locations kept. No new persistence library. |
| cwd-relative recordings/, default saves and Finder launch cwd='/' special handling | Platform initial working directory/resource root policy; retain project-side files separately from bundle/install resources. Windows writable directory proposal before implementation. |
| main/ui include unistd.h and pthread.h through ui_internal.h | Remove from interaction/UI-private header; platform service handle types only. |
| __APPLE__ vs else=ALSA throughout MIDI/CMake | Explicit APPLE/Linux/Windows branches; never accidentally compile ALSA on Windows. MIDI backend deferred WinMM vs MIDI Services tradeoffs/approval Phase4. |
| CoreMIDI/CFString NSWindow/NSEvent/Cocoa/Objective-C blocks/retain-release | Keep in macOS platform translation units only, preserve serial MIDI source and gesture input contracts. |
| glfw/raylib/rlgl texture/window/input handles in UI and Browser | SDL boundary; own POD rect/vector/color/input state and draw interface; no graphical headers in engine/interaction. |
| CMake m, -Wall/-Wextra/-Wpedantic, CMAKE_DL_LIBS, Apple bundle resources/plist, Linux install desktop/icons, FetchContent network | Rewrite platform-conditional flags/libraries/install layouts with vendored sources only. C11 compiler support assessed; no unity build. MSVC atomics/C extensions need explicit validation; MinGW alternative not approved yet. |
| _Static_assert sizeof(float/int/unsigned)==4, IEEE32/little-endian map checks, unsigned24-bit DX tables/phase arithmetic | Keep format scalar mappings and overflow-safe arithmetic; validate Windows ABI/endian/atomics and signed-shift assumptions. Raw golden PCM is little-endian binary32, project format text ABI-independent. |
| Unix permissions, access tests and atomic rename guarantees | Windows ACL/read-only/share modes require functional tests; authorization/atomic failure behavior stays transactional. |
| /proc/self/smaps_rollup, sys/resource.h/getrusage and ru_maxrss units (tools/benchmark_engine.c) | Measurement harness only; Windows process/thread timing/memory APIs. Do not ship benchmark machinery in engine. |
| Linux tools check_gui/run_benchmarks invoke X11/PulseAudio and Python helpers | External validation tooling requires platform adaptations; not new shipped runtime dependencies. |

System APIs are proposals within the approved platform boundary, not adopted dependencies. macOS/Linux build recipes can be validated locally only where toolchain and offline archives are present. Windows paths, audio/MIDI devices, DPI, OS drag/drop, precision touchpad and hardware latency remain unverified here.

## Windows implementation candidate

With the user's instruction to continue through all stages, the proposed
baseline MIDI implementation is WinMM, with a dedicated OS message window/thread
so the existing MIDI ring has exactly one producer. WinMM uses the OS Winmm
library; its device IDs are enumeration indices, so the existing unique-name
reconnect fallback remains necessary. Windows MIDI Services is a later alternative
with a more involved API/deployment surface, not adopted here. The
[midiInOpen documentation](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-midiinopen)
specifies window/thread callback delivery and the Winmm library; the
[midiInClose documentation](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-midiinclose)
describes reset/close requirements.

Audio remains miniaudio WASAPI, with no ASIO SDK. The initial toolchain candidate
is 64-bit MinGW-w64 with UCRT and C11 atomics; MSVC requires separate atomic and
build verification. No Windows compiler/SDK/runtime is available in this host,
so these are source candidates, not verified Windows support. Filesystem,
recording writer/mapping, UTF-8 paths, packaging, DPI, native drag/drop, precision
touchpad, device latency and MIDI hotplug remain Windows verification work.

Implemented source foundation (unverified on Windows): the private editor header
is now `editor_windows.h` so it cannot shadow the OS windows.h header. Native
audio/MIDI time uses CLOCK_MONOTONIC unchanged on POSIX and QueryPerformanceCounter
on Windows. The WinMM candidate receives short MIDI and device-change messages
on one dedicated hidden-window thread; UI-side shutdown joins it before ring
flush/reopen. Windows audio excludes DirectSound and WinMM audio backends so
miniaudio's WASAPI path remains the candidate. The filesystem/recording services
are still POSIX implementations, so this is not yet a Windows-build claim.

Host verification: all70 offline tests pass after the header/clock extraction,
including original audio fixtures and callback guards. The macOS desktop and
CoreMIDI target build after adding the guarded Windows source. No Windows branch
was compiled or executed; native lifecycle, device notification, Unicode names,
latency and shutdown need Windows validation.

Windows source services now include UTF-8/UTF-16 file opens, native directory and
canonical-path APIs, drive/UNC path rules, cross-drive asset references, atomic
sibling saves, read-only WAV mapping and a native recording writer thread. Writer
PCM/header serialization and4096-frame drain/5ms idle/RATE checkpoint behavior
remain the original algorithm; Win32 replaces positional writes, truncate, sync
and thread operations. Disk work stays off the audio callback. Only pathname
operations needing Unicode translation are wrapped; ordinary stream operations
remain standard C and POSIX retains its original calls.

The Windows coordinate candidate normalizes SDL's physical-pixel window/pointer
coordinates by content scale to preserve logical UI units. Initial/minimum
window dimensions scale correspondingly; pointer deltas normalize at the boundary,
and native drop coordinates update the pointer. macOS/Linux rules are unchanged.
SDL's [high-DPI documentation](https://wiki.libsdl.org/SDL3/README-highdpi) explains
that Windows uses physical window coordinates and content scale, unlike macOS.
Native DPI transitions, fractional-size rounding, pointer sensitivity and
precision-touchpad behavior still require Windows tests; no hardware claim is
made by these guarded branches.

Windows candidate build configuration uses native threads instead of a
winpthreads dependency, WASAPI's system COM/UUID libraries, and an8MiB desktop
stack for the original by-value Project operations. Its file services are selected
at CMake configure time. Current verification remains macOS only:71/71 offline
checks and89/89 installed-SDL checks pass. Windows path tests exercise drive and
UNC roots, cross-volume assets, root parents and bounds on the host; they do not
exercise Win32 filesystem calls. Native Windows build, saved-project relocation,
file permissions/atomic replacement, recording durability, mapped-view lifetime,
Unicode paths, DPI transitions, device audio/MIDI and physical touchpad behavior
remain unverified. The development SDL SDK option is still not the final
vendored/static distribution.

Six focused host checks (atomic save, project assets, recording writer/session,
desktop smoke and Windows lexical paths) also pass ASan+UBSan after these changes.
Leak detection remains disabled because it is unsupported here. All182 original
and concept reference hashes remain unchanged; original working tree is clean
and original audio golden files have not changed.

The desktop project file action now uses platform_absolute_path through the host
boundary, rather than testing only a leading slash. This preserves the POSIX
condition and recognizes Windows drive/UNC destinations when preparing LibreLoop's
own file chooser. Native Windows dialog/path flows still need Windows execution.
