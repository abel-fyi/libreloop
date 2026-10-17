# raylib, GLFW and rlgl dependency map

Frozen original only; no raylib will be carried into the rewrite. [Complete API table](raylib-api-inventory.md) lists every callable graphics/window/helper API and all source locations. Types/constants (`Color`, `Vector2/3`, `Rectangle`, `Camera2D`, `Font/GlyphInfo`, `Texture2D`, `Image`, `RenderTexture2D`, FilePathList, KEY_*, MOUSE_*, TEXTURE_*, FLAG_* and RL_QUADS) also must leave interaction/engine interfaces.

SDL alternatives below are proposed boundaries, not tested implementations. SDL version must be pinned when vendored. SDL3 is C; statically linked where permitted, no SDL_ttf/SDL_image satellites. Font and existing decoder/image headers may be carried with notices.

## Effective input behavior to preserve

raylib 5.5 GLFW `MouseScrollCallback` overwrites the current wheel vector with each event, so the last event wins per poll, rather than summing. `KeyCallback` sets one repeat bit per frame. `PollInputEvents` resets state and waits using glfwWaitEvents when enabled. Reproduce these alongside LibreLoop's separate press queue. [Pinned backend source](https://raw.githubusercontent.com/raysan5/raylib/5.5/src/platforms/rcore_desktop_glfw.c).

raylib scalar wheel selects x when abs(x)>abs(y), otherwise y; raw mouse delta is current-minus-previous position, while mouse position applies internal scale/offset. SDL adapter must retain frame-level snapshots for these polls, not use SDL relative deltas indiscriminately. [Pinned core source](https://raw.githubusercontent.com/raysan5/raylib/5.5/src/rcore.c).

LibreLoop native precision handling, accumulated Cocoa magnification, momentum, modifier conversions and pointer anchors are in behavior-spec.md. Only editors use the native accumulated struct; Rack/Browser/Mixer/menus/knobs use scalar raylib wheel. Multi-event replay must test both paths. Pinned GLFW Cocoa scrollWheel multiplies precise scrollingDeltaX/Y by 0.1 before forwarding nonzero deltas; it does not invert direction. The scalar panels therefore receive the last precise event scaled by 0.1, while native editor navigation receives the full accumulated raw deltas. Preserve both paths. [Pinned Cocoa backend](https://raw.githubusercontent.com/raysan5/raylib/5.5/src/external/glfw/src/cocoa_window.m).

SDL wheel carries floating axes/direction and pointer coordinates; its general event struct alone does not establish raw Cocoa precision/phase parity. Treat OS direction normalization explicitly and avoid changing the original's effective signed values. [SDL wheel reference](https://wiki.libsdl.org/SDL3/SDL_MouseWheelEvent).

Smallest proposal: retain a target-NSWindow Cocoa scroll/magnify monitor in platform/macos_input.m, using SDL's native window property instead of glfwGetCocoaWindow. Return native events unchanged; editor adapter suppresses corresponding duplicate SDL wheel contribution. Preserve raylib-compatible scalar path separately. No touch-centroid reinterpretation, synthesized inertia or extra framework. Cocoa is OS-provided and already used by the original; architecture approval includes this proposal. Verify selected SDL backend's wheel path before implementation; hardware validation remains required.

Use SDL scancodes to preserve physical typing piano positions; translate to app key enums. Key-down/up/repeat events populate separate held/pressed/released/repeat bits. Repeat comes from OS, with no application-defined fixed delay/rate. Pinned core defaults cap key/character queues at16 each, with512 key states and8 mouse buttons; retain the character cap per event poll. Only menus and Shift-note arrows consume repeat; Browser/text-backspace/chooser use initial press. SDL_TEXT_INPUT supplies UTF-8 decoded into original codepoint queue; retain original command-key suppression and byte-capacity/backspace behavior. Do not add insertion-cursor semantics. Button presses queue counts independently while held/released maintain original frame transitions. Cursor change once per frame after all requests.

## Drawing, text and pixel behavior

Use concrete draw functions for filled/outlined rectangles, line segments/triangle geometry, glyphs, textures and clip rectangles. Thin integer `DrawLine/DrawRectangle` and floating `DrawLineEx/DrawRectangleRec` are different paths; preserve truncation at original integer argument conversions. SDL default line/outline rasterization is not assumed identical. Implement original thin border geometry and segment quads, compare pixels at 1x/Retina, including clipped notes and hit-highlight edges.

Camera2D used only for scale/translation: apply app logical origin/scale at draw boundary. No generic camera framework. Scissors are physical-window inputs with raylib DPI adjustment; repeated BeginScissorMode **replaces** current clip, not a nesting stack. Port each overwrite/restore call exactly. Rounded/geometric icon procedures are currently rasterized into cached texture atlases; preserve atlas coverage and point filtering, bilinear circle/arc/cable masks and pattern texture clamp/bilinear. Render target textures require orientation handling (original negative height/y flip); SDL target orientation must not double-flip.

rlgl quad streams for waveforms/cables/smooth lines become triangles with same vertices, colors and UVs through SDL_RenderGeometry, which accepts textured triangles on main thread. [SDL geometry reference](https://wiki.libsdl.org/SDL3/SDL_RenderGeometry). Keep the source waveform hierarchy/bin interpolation. Physical-pixel edge feather=.5/font_density; display sampling step=1/layout_scale. Preview cache width cap2048; vector fallback retains clipped detail. No CPU rasterizer in Phase 0/1/2.

`text_fonts.c` is authoritative, not standard SDL font defaults: retain Liberation Sans Regular 2.1.5, dynamic codepoint sets, sizes10..22, raster size round(size*density), bitmap advance/offset widths and fallback behavior. Width tracks negative left bearing, right ink bound and pen advance; UI applies ceil. No kerning introduced. Atlas changes flush outstanding geometry before old texture destruction. Glyph positions round at physical density using original text_pixel_position. Sizes outside10..22 currently use raylib's default bitmap font; titles such as small “PAN”/transport labels may rely on this. Carry the existing default font data with notices if needed; substituting Liberation Sans at these sizes would change original look/metrics and needs explicit decision. Reproduce UTF-8 replacement behavior and glyph fallback. Text metrics affect truncation, toolbar centering, ruler labels and minimum legibility, not just appearance.

SDL cannot load arbitrary PNG directly without own decoder. Carry only the already bundled approved-by-origin stb_image header for current logo/icon use (with license), or convert existing assets offline to a simple embedded format in a later phase. No SDL_image dependency. Atlas generation can use CPU shapes and stb_truetype and/or renderer targets; renderer readback used only at initialization/density rebuild, not every frame.

## Window, frame timing and idle

1200×675 resizable, minimum900×506, close request intercepted for own save prompt, Escape disabled as exit shortcut. SDL_WINDOW_HIGH_PIXEL_DENSITY applies macOS; use window/pixel sizes and original app layout_scale separately. Do not equate monitor content scale to framebuffer density. Linux original intentionally stays in consistent window coordinates. Resize/focus changes refresh state/raster density. GetTime uses monotonic seconds; frame duration includes prior wait/frame limiter behavior. No new smoothing of double-click or animation timers.

`main.c` publishes audio before event waiting. After .15s since movement/navigation/key/button/resize/job completion and when no animation reason, block. Active/audio/capture/meter/spectrum/playhead/preview/window-drag reasons are enumerated in realtime-audit.md. Worker/MIDI completion posts a wake event. SetTargetFPS60 and partial busy wait disabled in original CMake. SDL adapter uses blocking wait with an animation timeout, not a permanent60FPS render loop; SDL wait belongs on main thread and removes an event when returning one. Process that event, then poll remaining events. [SDL wait reference](https://wiki.libsdl.org/SDL3/SDL_WaitEventTimeout). Exact frame pacing and wake behavior need original desktop execution once raylib sources are available.

The recovered original build confirms CUSTOMIZE_BUILD=ON and both
SUPPORT_BUSY_WAIT_LOOP and SUPPORT_PARTIALBUSY_WAIT_LOOP=OFF. Generated
EXTERNAL_CONFIG_FLAGS omit both defines, overriding config.h's default partial
busy wait. Active-frame waits therefore use the entire remaining target time:
macOS usleep(seconds*1000000.0), Windows Sleep(unsigned-long milliseconds), and
Linux nanosleep with original separate seconds/nanoseconds conversion and retry.
The own platform_frame_wait helper carries those configured branches, and the
frontend has no explicit busy-spin tail. Sixteen macOS native argument/call-count
captures match the frozen WaitTime body. Hardware scheduling/input latency and
Linux/Windows execution remain unverified; no CPU savings estimate is a measured
result. The SDL blocking event wait remains the idle mechanism.

TextFormat uses4 rotating1024-byte buffers and marks truncation with three dots; preserve return lifetime across nested formatting calls. [Pinned text implementation](https://raw.githubusercontent.com/raysan5/raylib/5.5/src/rtext.c).

The carried path/format helpers now delegate to graphics-independent
interaction/text_legacy.c. Extension matching preserves GetFileExtension's
whole-path last-dot search (a dot at byte0 is excluded), ASCII case conversion,
16-byte file-extension copy,1024-byte selector buffer and128-result split limit.
At the split limit the last pointer can refer to the still-empty buffer tail;
the original-derived fixture retains that result. TextFormat clears each buffer
before use, adds `...` at bytes1020..1022 when required output is>=1024, and wraps
after four calls. Captures compare all1024 bytes, not just visible strings.
Filename stems retain leading dots and return empty for a null path. The port's
bounded path-sized stem buffer is retained; captures cover valid original
basenames<=255 bytes and do not emulate the original unchecked256-byte strcpy
overflow for invalid longer basenames. Both helpers and actual frontend bindings
match the pinned library. This is string/API parity, not native filesystem or
clipboard/drop-event acceptance.

Clipboard failure/availability is observable during paste: frozen GLFW's Cocoa
getter returns NULL when no string type exists or retrieval fails, but an
available empty string is non-NULL. The original text-input branch leaves
selected text intact for NULL and clears it for an empty string. SDL3's
GetClipboardText normalizes unavailable/failed reads to an allocated empty
string, so the boundary reads the advertised UTF-8 clipboard data directly and
preserves NULL through the frontend wrapper. An empty advertised payload remains
an owned empty string. Dummy-driver tests cover absent, empty, UTF-8, binary-only,
failed-provider and repeated ownership cases, connected to the original-derived
text-edit model. Actual OS clipboard/IME delivery remains unverified.
Implementation evidence: [SDL3.4.16 clipboard](https://raw.githubusercontent.com/libsdl-org/SDL/release-3.4.16/src/video/SDL_clipboard.c)
and [Cocoa backend](https://raw.githubusercontent.com/libsdl-org/SDL/release-3.4.16/src/video/cocoa/SDL_cocoaclipboard.m).

## File utilities and non-render APIs

raylib file/path helpers in Browser/UI are platform operations, not engine dependencies. Replace with small directory enumeration/path/resource helpers, ordinary C formatting/color/UTF-8 arithmetic where appropriate. Browser sorting is case-sensitive, chooser sorting case-insensitive: preserve both. No new filesystem library. Clipboard uses SDL_GetClipboardText then SDL_free at boundary, copying into app buffer. OS drops use SDL drop events and ownership rules, preserving original processing order and pointer target. Smoke screenshots use SDL renderer readback plus existing encoder or external temporary harness, not a shipped new dependency. `GetFileNameWithoutExt` is deprecated raylib spelling but still used; own basename logic must preserve stem behavior.

Pinned rcore.c filesystem quirks also affect effective error handling:
`MakeDirectory` rejects null/empty paths with1, then returns0 for nonempty paths
even if native mkdir calls fail. It treats slash/backslash as separators,
normalizes them to slash while walking, and skips the character after a colon.
Do not replace its ignored-error convention with a successful-creation check:
recording/preset callers then reach different later checks and status messages.
The frontend now reuses the existing native Browser directory helper for this
behavior. `DirectoryExists` is an opendir/close test (an unreadable directory is
false), whereas `FileExists` is access(F_OK), including directories/nonregular
paths (existence does not imply readable or regular). The own equivalents are
platform_browser_directory and platform_path_exists; Windows uses native UTF-16
operations and remains unverified. Original-derived filesystem captures compare
both these helpers and actual frontend bindings, including nested creation,
blocked parents, backslashes/colon, trailing/repeated separators, absolute paths,
and an unreadable directory. No raylib header/library is introduced.

## Outstanding checks

Original GUI dependency source archive could not be fetched. All local call sites are inventoried; upstream runtime behavior above was inspected through pinned source URLs, but graphical execution is unavailable. Unverified: raylib default-font exact glyph data; GLFW precise scalar wheel calibration on hardware; rasterization comparisons; monitor transitions; key repeat on Linux; renderer timing/idle GPU. These are explicit Phase 2 acceptance gates, not claims that SDL defaults preserve feel.
