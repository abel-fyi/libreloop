# Completion audit and external blockers

> Historical design record. The preview and legacy layout flags described below
> were removed; launch and smoke now use the single current desktop layout.

The complete user objective is not achieved. This audit preserves the shipping,
feel, visual and platform requirements; it does not redefine success as the
current developer build or passing component tests.

Current reviewable work: engine/desktop port, original-derived audio and
interaction fixtures, own chooser/project/preset/asset services, real-time guards,
native platform candidates, macOS packaging, visual preview/layout proposal and
unadopted optimization/CPU-renderer experiments. Latest checkpoints are113/113
developer-SDL and91/91 SDL-free tests, plus targeted sanitizer checks. These
counts do not prove complete native or cross-platform parity.

| Unfinished requirement | Current authoritative evidence | Resource/state needed |
|---|---|---|
| Offline vendored/static SDL desktop | third_party/SDL source absent; pinned archive absent; shell download DNS fails | Approved SDL3.4.16 archive, followed by checksum import, offline static build and packaging checks |
| Native original/rewrite feel and baseline | Fresh native-window probe fails before creation: no displays; tests use dummy/software renderer | Native display/session and original/rewrite hands-on mouse, trackpad, pinch and DPI checks |
| Physical audio/MIDI/recording/deadlines | Fixtures/guards/stress pass; physical devices unverified | Native device access and latency/recording/reconnect verification |
| Native idle/active/GPU/battery/input-latency measurements | Source wait design audited; component/software timings only | Native runnable original/rewrite and appropriate hardware measurement |
| Final visual adoption | Concrete phase-3-layout-proposal.md and nondefault palette preview; default remains original | Parity confirmation and one-time layout review required by the user's Phase3 instruction |
| Linux/Windows verification | Native source candidates prepared; no Linux execution or Windows compiler/runtime available | Appropriate build/runtime environments; compiler/atomics/device/DPI/drop/path checks |
| Other-CPU audio tolerance evidence | arm64 exact reference checks; x86_64 cross-build succeeds but execution fails | Executable alternate architecture/toolchain and original cross-target renders |
| Static/shared SDL and hardware experiment comparisons | Software/optimization/CPU candidates measured; source SDL missing | Vendored source and native platform availability; adoption remains user-controlled |

The same missing-source/native-environment conditions have persisted across
multiple consecutive goal turns. Independent original-derived service/replay
work and the concrete review proposal were completed while those constraints
persisted. The remaining acceptance evidence cannot be manufactured here.
The objective stays unfinished; a blocked goal is not a completion claim.

## Run the current developer app

```sh
open /Users/a/libreloop-rewrite/libreloop2/build-sdl-validation/LibreLoop.app
```

Palette review:

```sh
open /Users/a/libreloop-rewrite/libreloop2/build-sdl-validation/LibreLoop.app --args --visual-preview
```

The locally signed bundle includes assets/notices and its shared SDL runtime.
It is a developer build with a macOS26 minimum, not the required vendored/static
shipping result. See toolchains.md for build/install details and native limits.

## Resume the source-dependent work

Provide the approved local SDL archive at any workspace path, then use:

```sh
python3 tools/vendor_sdl.py /path/to/SDL3-3.4.16.tar.gz
```

The importer expects SHA256
`7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68`.
Official archive:
https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
Do not replace the frozen original, fixtures or reference hashes to unblock a
build. Real native/platform evidence and visual review remain separate gates.
