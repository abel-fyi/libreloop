# Third-party components and licenses

## Current build policy

The Phase0 sections below are historical inventory notes. The current application
is C11 with pinned miniaudio/stb headers already present. The reference desktop
build imports verified SDL 3.4.16 source; an installed SDL >= 3.4 SDK is also an
explicit supported build configuration (`LIBRELOOP_USE_SYSTEM_SDL=ON`). CMake
never downloads libraries. Source preparation and tests use Python only.

The application-owned Cocoa/Objective-C input adapter has been removed in favor
of SDL wheel and pinch events. SDL may compile its own native-language backends;
that does not add Objective-C or C++ application sources. Native MIDI and file
adapters remain C. No PortMidi/RtMidi dependency has been added.

See [building and platform policy](building.md) for the current requirements and
[toolchain evidence](toolchains.md) for what has actually been verified.

## Current dependency inventory

| Component | Source/version policy | Notices |
|---|---|---|
| SDL3 | Reference source 3.4.16, archive verified by importer; installed SDK >= 3.4 is explicit | [SDL notice](licenses/SDL3.txt), imported upstream source license |
| miniaudio | 0.11.21; header bytes pinned in `third_party/manifest.json` | [Upstream notice](../third_party/miniaudio/LICENSE.txt) |
| stb image/font headers | Exact header bytes pinned in `third_party/manifest.json` | Embedded header notices retained |
| Fonts, presets, sample assets, adapted DSP/helpers | Existing project assets and original-source provenance | Detailed inventory and retained notices below |

## Historical inventory and migration notes


Rewrite source remains GPL-3.0-only; full project license in ../LICENSE. Phase0 contains documents only, no vendored application dependency source or ported DSP yet. The embedded/documentary original excerpts retain their original GPL notices by attribution to frozen reference. DX7 excerpt is Apache-2.0; notice is retained in [licenses/msfa-Apache-2.0.txt](licenses/msfa-Apache-2.0.txt).

| Component | Original provenance / rewrite plan | License / status |
| --- | --- | --- |
| SDL3 | Approved new dependency; pin exact version/revision and archive checksum before vendoring; source under third_party/SDL, static where platform allows | zlib; no source vendored yet. Developer SDK3.4.16 dynamic runtime is copied into the macOS test bundle with [license](licenses/SDL3.txt); this is not the shipping/static choice. Source archive SHA256 recorded from installed Homebrew metadata: `7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68` (archive unavailable, not independently verified). |
| miniaudio | Original0.11.21 bundled with raylib5.5 archive; initially preserve version for audio baseline | Public domain or MIT No Attribution; full header/notice must be carried. Archive unavailable locally. |
| stb_truetype | Approved; prefer original raylib5.5 bundled version for glyph parity, pin bytes/revision | Public domain or MIT, keep header notices. No source copied yet. |
| Existing image/audio decoder headers | Original raylib external headers used by actual decoding/atlas/icon paths; select only needed components (stb_image, WAV/FLAC/MP3 decoder headers and any transitive headers) | Existing bundled components allowed with their individual notices; exact versions/hash and transitive inventory remain gated on original archive. No SDL_image/SDL_ttf. |
| Liberation Sans Regular2.1.5 | Original assets/fonts/LiberationSans-Regular.ttf, existing UI font; may embed exact bytes later | SIL OFL1.1, Google/Red Hat notice in original assets/fonts/LICENSE; no substitute font approved. |
| Adapted DX7/MSFA core | Original dx7.c/h/tables; adapted Dexed MSFA Modern commit2e182b3db85c09083ab13c8b9b00565ce7d9ff85, Google2012–2013, Pascal Gauthier2017 | Apache2.0 notice preserved; original C adaptation/change descriptions accompany port. No JUCE/C++ runtime. |
| DX7 factory parameter banks | Original dx7_factory.h and factory presets; Yamaha ROM1A parameter values, no sampled audio/firmware | Preserve provenance text from original docs/third-party.md. Parameter-bank distribution/license provenance remains as in original, not independently reverified in Phase0. |
| Demo WAVs/presets/branding | Original generated sounds/project assets | Preserve original project licensing and attribution; no third-party sample pack introduced. |
| raylib5.5, bundled GLFW, rlgl, default bitmap font data | Baseline dependency only; exclude libraries/API headers from rewrite. Default bitmap font data may need extraction as existing bundled asset for fallback-size parity | zlib/libpng notices required for any carried font/helper data; no raylib library retained. |
| CoreMIDI/CoreAudio/Cocoa, ALSA and later Win32/WASAPI | System-provided APIs behind platform interfaces | CoreMIDI/CoreAudio/Cocoa not vendored; ALSA userspace LGPL2.1-or-later supplied by system. Windows MIDI/backend/toolchain choice deferred Phase4. |

Original documentation says dependencies fetched at build time, not vendored (except adapted DX7). Rewrite must deliberately invert that: all approved dependency sources/notices/checksums stored under third_party/, no network build step. Static SDL system library prerequisites and per-target toolchain recipes must be recorded once actually built. Do not call this repository offline-buildable yet: no application build exists in Phase0.

Primary API references used for mapping: SDL3 official wiki and pinned raylib5.5/GLFW source URLs in raylib-dependencies.md. Documentation references are not shipped dependencies. Temporary Python capture/index scripts remain outside rewrite repo; they are measurement tools, not runtime dependencies.

## Phase1 vendoring update

`third_party/manifest.json` pins exact header/license bytes extracted from the verified raylib5.5 source archive. miniaudio0.11.21, stb_truetype, stb_image and stb_image_write sources are now vendored; their embedded licenses are retained, with miniaudio's full standalone notice. Original Liberation Sans/factory/sample/branding assets and their notices are copied. The Phase0 table's “not yet copied” entries describe the initial study checkpoint, not current vendoring status. SDL remains pending source archive; no raylib/GLFW library or header is a rewrite build dependency.

The default10px bitmap font data, basic font atlas packing, text quad/measurement math and UTF-8 helpers are now carried from the pinned raylib5.5 rtext.c under the retained zlib notice in docs/licenses/raylib.txt. They are adapted own-code helpers/assets; no raylib/GLFW/rlgl library or API header is linked or included in the rewrite. No new font or text shaping library was introduced.

The Fade/ColorToHSV/ColorAlphaBlend color helpers are adapted from the same pinned raylib5.5 rtextures.c with its retained zlib notice. They preserve original rounding/tint arithmetic for the unchanged original UI palette; no raylib build dependency is introduced.

The application-owned text_legacy.c path/extension/format helpers adapt pinned
rcore.c/rtext.c behavior (selector limits, ASCII case handling, rotating format
buffers and truncation). The same retained zlib notice applies. They introduce
no raylib header/library dependency; original-derived captures check their
effective values and buffer lifetimes.

platform/frame_wait.c adapts the pinned rcore.c WaitTime branches under the
original LibreLoop build's disabled busy-wait configuration. Its original
raylib zlib notice remains in docs/licenses/raylib.txt; it uses only OS-provided
sleep APIs and introduces no raylib dependency.

Browser-facing directory list/construction behavior and filename/parent logic are adapted from pinned raylib5.5 rcore.c with the retained zlib notice. Native filesystem access lives in the platform boundary; no raylib API header/library is required by Browser.

ColorFromHSV and icon circle tessellation/outline geometry are adapted from the same pinned raylib5.5 rtextures.c/rshapes.c under the retained zlib notice. Icon definitions and coverage packing are carried from original GPL ui_widgets.c. No additional dependency was introduced.

## Prepared SDL source import and offline selection

`tools/vendor_sdl.py` imports only a local SDL3.4.16 archive matching the recorded
SHA256 above. It validates archive paths/links, requires source CMake and license
files, records source-file hashes, and refuses to replace an existing checkout.
It performs no download. CMake chooses vendored static SDL by default for desktop
builds, with shared SDL only behind an explicit experiment; installed SDL remains
an explicit developer-validation choice. A missing/unverified source checkout
causes an explicitly requested offline desktop configure to fail clearly.

Verified here: wrong archive rejected before a destination is created; missing
source configure fails with the required dependency message; the installed-SDK
build still passes97 tests and its packaged desktop smoke. No SDL source has
been imported and no actual vendored/static/shared-from-source build has been
verified. This closes build wiring, not the missing-source requirement. The
macOS packager now copies a runtime only for a shared SDL target; static builds
are prepared to package assets without a copied SDL library.

## Classic desktop sprites (2026-10-16)

`src/ui/classic_icons.h` contains original LibreLoop pixel artwork under
GPL-3.0-only, not third-party Windows icons. No Chicago95/ChiDoors/Microsoft
assets or new icon rendering dependency were imported. The user-supplied
`../icons` screenshot is a visual reference only. See `vintage-desktop-pass.md`
for provenance limitations and rendering/storage measurements.

## Bitmap-style studio font (2026-10-17)

The user authorized replacing the UI font with a freely licensed bitmap-style
font. Studio mode now uses Fontworks' DotGothic16 Regular, based on a 16×16
bitmap design, under SIL Open Font License 1.1. Fontworks' upstream project:
https://github.com/fontworks-fonts/DotGothic16 . The unchanged local TTF was
recovered from the out-of-source reference dependency cache; this adds no raylib
code or runtime dependency. Liberation Sans remains available for legacy parity
fixtures. This supersedes earlier notes retaining Liberation as the studio font.

- Asset: `assets/fonts/DotGothic16-Regular.ttf` (2,026,816 bytes).
- SHA256: `92524cf71473014b9988fbcb63ac5cfe045e55944918b021a41f991ca0b6e329`.
- Copyright: 2020 The DotGothic16 Project Authors.
- Complete license: `assets/fonts/OFL-DotGothic16.txt`, copied with the font into
  the app bundle. No font modification or network build step.

### Selected replacement, not imported

The user subsequently selected PxPlus IBM VGA 8x16 from VileR's Oldschool PC
Font Pack. The publisher documents CC BY-SA 4.0 at
https://int10h.org/oldschool-pc-fonts/readme/ . Actual import is pending: no
local font was found and shell DNS resolution prevents the official archive
download. DotGothic16/OFL remains the active font. Do not mistake this selection
for a bundled asset or claim that its notices have already shipped.

## Active studio font: PxPlus IBM VGA 8x16 (2026-10-17)

Network access was enabled and the selected font has now been imported. This
supersedes the pending-import notes above. Studio uses the unmodified pixel
outline font from VileR's Ultimate Oldschool PC Font Pack v2.2, released under
Creative Commons Attribution-ShareAlike 4.0 International. Copyright/attribution:
outline version by VileR, 2020; pack (c) 2016–2020 VileR / INT10h.org.

- Publisher/source: https://int10h.org/oldschool-pc-fonts/ .
- Archive: `oldschool_pc_font_pack_v2.2_linux.zip` from its official download page.
- Archive MD5 `e4e114151d7d9d4baf7b0f69b6645043` matches publisher's listing.
- Archive SHA256 `b30dc3ecc9931ad2dd8be7517dd01813c8834a1911b582ab7643191b41a3d759`.
- Exact member: `ttf - Px (pixel outline)/PxPlus_IBM_VGA_8x16.ttf`.
- Asset: `assets/fonts/PxPlus_IBM_VGA_8x16.ttf`, 70,640 bytes; version v2.2-2020-11.
- Font SHA256 `c99fb740581471c2bd239a200c9f7a6c12f60a7099b0f0af795e405281bc2c5f`.
- Unchanged full license and publisher readme are packaged as
  `LICENSE-PxPlus-IBM-VGA.txt` and `README-Oldschool-PC-Fonts.txt` in the same
  directory. Font itself is unmodified; its CC BY-SA license remains separate
  from LibreLoop's GPL-3.0-only source license.

The now-unused DotGothic16 asset/license were removed; Liberation remains for
original parity fixtures. The new font retains studio's common 16-unit size
and nearest atlas sampling. No new runtime or build dependency, no build-time
network fetching and no installed-system-font requirement.

## Polyphonic Analog design references

The new ladder/oscillator implementation is original GPL-3.0-only C. Its design
references are Huovilainen's DAFx-2004 ladder analysis and Zavalishin's *The Art
of VA Filter Design*. VK-1 supplies a listening/design reference. No code or
binary assets were imported from these references, and no new dependency was
added. See [Analog design and verification](analog-synth.md) for source links,
implementation scope and measurements. Existing DX7 notices remain unchanged.
