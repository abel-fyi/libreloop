# Phase 2 progress — interaction extraction in progress

Continuation remains authorized. No visual pass, new feature or hands-on parity acceptance is claimed. SDL source archive is still unavailable; the existing engine can be built/tested offline independently.

## Completed

- Piano brush/erase/box selection and move/resize functions extracted from original ui_piano.c into interaction/piano.c. Borrowed project/note/selection data plus one concrete preview callback; no graphics/window/audio-backend dependency.
- Editor navigation extracted from original ui_shell.c into interaction/editor_navigation.c. Inputs carry logical pointer, explicit modifiers/capture/blocked states and layout scale. Original hit-region guards, middle-pan capture, wheel conversion, zoom anchors and fractional pitch-scroll remainder preserved.
- Scroll accumulator separates original last-event wheel/scalar behavior from accumulated raw Cocoa precision and logarithmic magnification. CGFloat-compatible double inputs retain original float compound-assignment rounding. Native momentum is treated as received deltas, not synthesized inertia.

## Verified and how

Five original piano sequences were captured from the frozen original functions before extraction; repeated original output files match exactly. Port compares complete saved project, selection bytes, audition events and gesture state. Cases cover interpolated brush, original-stack erase (no peeling), group move/pitch limits, resize/minimum/extension and additive box interval rules.

Eight navigation sequences were captured from the actual original navigation function with deterministic logical window geometry1200×675, explicit frame times125ms+n/60, logical pointer positions, initial views and modifiers. Layout scales1/2 are distinct cases. Outputs include zoom/pan/scroll/remainders/pitch limits/capture/focus and scalar wheel. Cases include multiple events/poll, precise momentum, mixed modifiers/pinch, invalid magnification, piano keyboard-region zoom guard, middle drag outside window and modal cancellation. Every port output byte matches.

Original capture harnesses/builds remain outside original repository in baseline-artifacts/harness/ and interaction/. Frozen goldens copied unchanged to tests/fixtures/interaction/, with reference/hash manifests. Original files remain unchanged.

## Unverified / remaining

These are lower-level state-machine/navigation replays, not full end-to-end UI event acceptance. SDL window/event/render/clipboard/drop/DPI bridge, complete note-press and other editor/control interactions, pixel/font comparisons, whole-app CPU/GPU, native Cocoa event delivery and real mouse/trackpad hands-on checks remain. Native coalescing fixture adapters apply the frozen Cocoa formulas without opening NSWindow; they do not prove OS dispatch/momentum phase behavior.

Logical layout-scale tests do not establish all physical Retina/monitor-transition rendering. No old geometry assertions were weakened and no concept-art layout was applied. The Phase3 named-marker/extra-control conflicts remain outside this port.

## Text follow-up

Original raylib CPU LoadFontData runs without a window, so glyph reference capture is now possible despite graphical startup failure.29 physical raster sizes reached by requested sizes10..22 at densities1/1.25/1.5/2 were captured twice from the pinned raylib library, including224 Latin codepoints plus Greek/Cyrillic and missing-glyph cases. The stb-only raster helper matches every grayscale bitmap byte and integer advance/bearing/dimension exactly. This establishes CPU glyph metrics/masks, not full renderer atlas filtering or frame pixels. The original default bitmap fallback asset still needs its own carry-over when renderer integration begins. No new font or shaping/kerning was introduced.

## Atlas follow-up

The 29 original raster cases now also include twice-captured gray/alpha font atlas pixels and rectangle coordinates. The own-code basic packing preserves the original area heuristic, power-of-two sizing, wrapping threshold, padding4 and white corner. All atlas bytes/locations match the original. These are CPU references; texture filtering and complete frame pixels remain separate checks.

## Bitmap fallback and text layout

Carried the existing raylib-bundled 10px bitmap font data as an asset/helper with its zlib notice. No raylib library or API header is required. Original bitmap upload bytes and all224 glyph metrics/rectangles were captured before porting, twice identically. The original upload is intercepted only to save bytes/return its128×128 texture metadata. DrawTextEx/DrawTextCodepoint emit captured quads; actual linked MeasureTextEx/GetGlyphIndex/GetCodepointNext produce original measurements.

The own-code implementation matches160 cases across fallback and embedded TTF fonts: empty text, spaces/tabs, multiline/trailing newline, accents, Greek/Cyrillic, missing characters and malformed/truncated UTF-8; four font sizes and two spacings. Both measurement values and source/destination quads match byte-for-byte in serialized float output. This verifies geometry, not GPU sampling. Original permissive UTF-8 decoding/encoding and2px line spacing are retained.

## SDL drawing boundary

Small concrete draw interface now supports rectangles/borders, hairlines/thick-line geometry, clipping replacement, textured triangles, RGBA textures/targets and text. SDL types stay inside the SDL implementation. Font texture creation expands the verified gray/alpha atlas into white RGBA, using bilinear filtering for TTF and original nearest filtering for the default bitmap until explicitly changed by the font cache. No effects or concept-art styling were added.

The optional OFF-by-default LIBRELOOP_VALIDATE_SYSTEM_SDL CMake option allows developer checks using the installed SDL3.4.16 SDK while its source archive is unavailable. This does not adopt system/dynamic linking for shipping. A headless SDL software-renderer test verifies transforms, clip replacement, nearest texture colors, render-target restoration, thick-line coverage and bitmap text upload/drawing. This exercises SDL's approved renderer API, not the Phase5 custom CPU-rasterizer experiment. It does not verify hardware-renderer pixel parity, native display/DPI or window startup.

## Font cache and label fitting

Ported frozen text_fonts.c to an explicit single UI cache backed by the draw interface, with the original224-glyph initialization, Unicode growth,256/doubling capacity,10..22 requested-size cache range, physical raster-size rounding, flush-before-unload and failed-file bitmap fallback. Original width/bearing accumulation, ceil-based fit threshold, truncation without splitting UTF-8,10..15 button font selection and raster-position formula are unchanged.

Captured the actual original cache functions twice, replacing only GL texture lifecycle with CPU font data/atlas ownership. The port matches336 cache/width cases,5040 truncation results,1008 button-size results and183 physical pixel-position cases exactly. Cases include density changes1/1.25/2/0, repeated update, fallback sizes, invalid path/recovery and Unicode additions. Tests run via SDL's headless software renderer; real display transitions remain unverified.

At this checkpoint the complete58-test suite passed; the added59th font-cache comparison also passed. Audio references and existing assertions remain unchanged.

## Input frame state

Added own InputEvent/AppInput POD structs and small queries, with no SDL/raylib/window/graphics dependency. Frame begin retains original previous/current state ordering, queue reset/16-entry bounds, OS-repeat bit reset, float pointer storage, unscaled delta and last-wheel-event behavior. Physical US-position key identities preserve existing shortcut/MIDI key positions. Event timestamps are available for deterministic replay; original frame time remains the timer basis.

A temporary headless reference uses the actual pinned rcore.c query functions, GLFW callbacks and polling-reset block. Nine frames compare189 key-query cases,72 button-query cases, overflow/FIFO queues, repeated keys, same-frame press/release/re-press, lock modifiers, negative/subpixel pointer positions and offset/scale, wheel overwrite/dominant-axis ties. Repeated original captures and port output match exactly. This verifies effective state mechanics, not SDL/OS event dispatch. Focus loss must reproduce GLFW's platform-generated release events in the eventual SDL boundary; the pure focus flag does not invent releases.

## SDL event conversion

Added a platform-private SDL event adapter to the own input structs. Scancodes map to original physical US-position identities independently of localized key symbols; SDL OS-repeat flags feed the original per-frame repeat query, committed UTF-8 text feeds the16-codepoint queue. Pointer coordinates/deltas remain logical window units; motion is not accumulated from SDL relative deltas. Right/middle button ordering matches the original. Focus loss emits releases only for physically held keys/buttons, preserving the original lock-state distinction.

Synthetic SDL-event tests verify physical key mapping, localized-symbol independence, repeats/queues, multibyte committed text/overflow, button ordering, subpixel pointer motion, wheel last-event/direction semantics and focus releases. Native-scroll mode excludes duplicate SDL wheel events; the own native-scroll path still supplies both exact scalar and precise editor input. These tests do not establish real OS dispatch, keyboard-layout/IME behavior, Cocoa monitor installation or trackpad delivery. Clipboard/drop/window wait are not wired yet.

## Deferred app clicks

The original app's ui_shell.c mouse_callback queues every native press, independently of raylib's coalesced query, and main.c consumes at most one per button per UI frame. This second layer is now present as pending/frame click counts and an explicit post-poll UI update call. The earlier raw-input comparisons remain byte-identical. Added original mouse callback/query/main-loop consumption capture for five rapid press/release pairs: all five clicks survive across later updates while the raw button state remains released. Port and repeated original click traces match. The eventual event-wait loop must stay awake while pending clicks remain.

## Piano presses and scrollbar hit areas

Extracted original note hit-testing/press branch order and vertical/horizontal scrollbar press math into pure interaction functions. The last visible overlapping note wins; clipping/pattern-end visibility, logical7px resize edge, Shift toggle/additive box, shortcut-box precedence, brush versus selected-group move, right/both-button erase precedence, last-note length reuse, new-note moving state and capacity/inactive handling remain unchanged. Original grid x60/top86, width-84, height-180,25/zoom clamped8..128 and scrollbar16/corner16 constants are retained. The caller must propagate extended pattern duration to arrangement source state as before.

A reference harness invokes the actual full frozen piano() function with drawing-only calls disabled, playback disabled and toolbar widgets inactive.22 cases compare saved complete project, selection/snapshot bytes, audition events, gesture/grab/drag state, consumed input, viewport/top/pan and scrollbar state exactly. Includes pixel-exact right-edge boundary, overlapping notes, fractional clipped end, panned view, full note capacity, inactive partial bar, navigation beyond it, scrollbar thumb/page clicks and top/bottom limits. Every original capture repeated identically before port comparison. Earlier five gesture replay outputs/assertions remain unchanged. History checkpoint timing, velocity editing, keyboard audition and whole-frame event ordering remain separate work.

## Keyboard audition and velocity controls

Extended the pure piano interaction layer with original keyboard hit geometry (x4..58, black-key split x38 and half-row rule), audition release-before-press ordering/slot96+pitch%25, and explicit cancellation. Velocity press/update keeps original .48 pixel/velocity scaling,1..127 clamp,3/cw tolerance, paint/ramp snapshot choice, pattern/channel/new-press/modal cancellation, and the final update before release termination.

The same actual full-piano reference now has30 scenarios. Added original audio_key event traces, keyboard/velocity state and velocity snapshots. Original main.c velocity-update expressions execute in the harness after press. Covers white/black regions, out-of-keyboard and explicit cancel, left painting/right ramp, modal/new-press/pattern changes and release. Every output matches; prior22 scenario files and original five gesture files remain unchanged. This does not yet establish complete main-loop history/capture ordering or native audition/audio latency.

## Original palette

Carried the original theme unchanged onto own DrawColor/DrawRect types and drawing calls. Palette bytes, sRGB foreground formula and fader marks remain original, including raylib's255-valued WHITE constant and its integer tint/blend arithmetic. Adapted existing original contrast assertions pass; the original fader-mark coverage condition also passes through a headless SDL renderer. No concept-art palette or styling is active. Hardware framebuffer/rasterization parity remains unverified.

## Browser filesystem port

Original browser tree selection/navigation/expansion/collapse/favorite persistence now uses a small native directory/config boundary. Interaction code has no raylib, POSIX header or realpath call. Preserved folders-first bytewise basename ordering, both-separator filename/parent quirks, case-insensitive supported suffixes, directory depth64/tree10000/favorites8 limits, XDG/HOME config and homebeat fallback on current Unix targets. Native list construction preserves unfiltered entries and separate opendir-based directory classification, including broken audio symlinks. Ordinary portable fopen/fread/fwrite remain UI-side.

All existing original browser assertions pass with only its directory API name mapped to the boundary. A separately linked original-browser/raylib capture repeats identically and compares complete tree/favorite/selection state across expand/collapse/navigation/add/duplicate/reload, with uppercase/hidden files, broken/valid symlinks and Unix backslash names. Only the canonical temporary root prefix is normalized. Windows config/directory implementation, OS drops and UI sample audition/loading remain unverified/not wired. Concurrent directory mutation and allocation failure are outside these stable-tree comparisons; the new native list bounds its allocated capacity instead of relying on the original's undefined overrun if directory entries appear between its two scans. Original MakeDirectory's normal return convention, including ignored mkdir errors, is retained.

## Current verification checkpoint

Fresh default offline Release build:61/61 tests passed. Complete optional installed-SDL developer build:66/66 tests passed.11 focused new interaction/text/palette/browser checks passed ASan+UBSan with leak detection disabled (LeakSanitizer unsupported here). The installed SDL dylib declares macOS26.0 minimum; linking it for developer checks with SDK/deployment15.4 does not verify older macOS runtime compatibility. No SDL shipping/linking choice or Phase2 completion is implied. Original182 source/concept SHA256 values and all Phase1 original audio fixtures remain unchanged. See phase-2-checkpoint.md for completed/verified/unverified separation.

## Window/event-wait and Cocoa boundary

Added an own opaque window interface over SDL creation/presentation/geometry, event polling or blocking/timeout wait, worker wake, clipboard ownership and grouped file drops. Original1200×675/minimum900×506/resizable defaults, Mac-only high-density flag, logical layout-scale and framebuffer-width raster-scale formulas are retained. Deferred app clicks are consumed after the event batch. Logical resize alone sets the original resize flag; density events still wake/redraw. Drop groups publish only on completion, and later complete groups replace earlier unconsumed ones as with the original GLFW callback. Mac drop coordinates update the pointer before delivery.

The original target-NSWindow Cocoa monitor is compiled against the SDL window property. It returns native events unchanged, routes raw precise/nonprecise scroll through the already-verified own scalar/native formulas, and routes magnification through the exact logarithmic accumulator. A user event wakes SDL waiting when magnification produces no SDL input event. This uses SDL plus existing OS Cocoa APIs only. Worker wake is explicitly excluded from the audio callback.

SDL dummy-window/software-renderer tests pass for default/resized geometry, deferred clicks, clipboard copies, grouped/last-group drops, bounded wait/worker wake, close-request consumption and presentation. Existing original input/navigation replays still pass. Native hidden-window smoke failed before window creation with “The video driver did not add any displays”; diagnostic/result saved outside original in baseline-artifacts/gui/. Cocoa monitor installation/dispatch, real trackpad momentum/pinch, actual display/DPI and idle CPU/GPU remain unverified. Blocking wait is implemented; complete original idle-decision/frame pacing still belongs to app integration.

## Exact color/pixel references

Added twice-captured original circles/knob-arcs/cable RGBA assets and complete original palette plus512 deterministic color triples/3072 fade cases. The original pixel comparison caught an adapter mistake: WHITE had been mapped to245 (RAYWHITE), whereas pinned raylib WHITE is255. Corrected foreground fallback, color helper initialization, original contrast-test tint and circle/cable generation to255. Existing contrast checks still pass; the stronger original byte/float comparisons now pass too. No original fixture was replaced with rewrite output.

Widget pixel generation preserves original64×64 circle/1536×288 arc/1×16 cable sizes, antialiasing formulas and level/bank layout. These are carried original startup assets, not a custom CPU renderer experiment or a visual restyle. Shader sampling/hardware rendering remains separate verification.

## Renderer context and piano drawing

Added a concrete UI render context owning font/widget texture caches and preserving original label raster-position formulas. Original integer scissor arguments, Cocoa DPI multiplication and GL-to-SDL vertical-origin conversion are verified against45 twice-captured pinned original cases (main framebuffer/generic/FBO, fractional DPI and outside bounds). SDL tests verify cache upload, editor translation, clipping, labels and original circle/arc drawing.

Added draw-only piano content: keyboard, note grid, phrase shading, notes/resize markers, selection box, velocity stems/ramp, scrollbars, ruler/start/loop markers and playhead. Original widths/top/padding, arithmetic, integer-versus-float draw calls, palette and alpha values are retained. Editing/audition remain in the pure interaction layer; keyboard geometry and edge-hover masks are shared through own PODs. A headless SDL test checks representative original palette pixels and verifies the complete Project and selection remain unchanged during drawing. Original30 press/control and five gesture replays still pass. Toolbar/tool icons, corner zoom control, editor chrome and complete app wiring remain separate work; hardware frame-pixel parity remains unverified.

The user's latest instruction explicitly authorizes continuing through subsequent stages without per-phase stopping. Work continues; this does not turn unavailable hardware measurements into verified results or authorize adding unsupported features/dependencies.

## Waveforms and original icons

Ported draw-only waveform source mapping, envelope sampling and three-quad feathering, keeping1/layout-scale columns and.5/raster-scale feathering. Five actual original geometry captures match every position/UV for mono/stereo, layout/raster densities1/2/4/1.25, tempo fit/pitch automation, empty bounds and deterministic preview callbacks. Live sampler-preview service integration remains pending; callback geometry is verified separately from that service. Source/audio colors use original HSV conversion;144 original inverse-HSV cases now supplement the exact palette/color comparisons without changing prior references.

Carried original20 icon shapes and three raster variants, exact high-resolution area coverage/rounding/packing, nearest filtering and pixel-aligned placement. Five original synthetic-source packing cases match every RGBA byte at densities1/1.25/1.5/2/3. Original GPU source masks cannot be captured here; SDL source-mask rasterization and hardware pixel parity remain unverified. No new image-resize dependency was needed. Window cursor flushing maps only original EW/NS/hand requests; all other requests retain original default-arrow behavior, including resize-all.

## Ruler controls and app-time ordering

Extracted original playback-start/loop ruler state and main marker-drag branch into pure C, including strict.3s/6-unit double-click tests, editor/pattern/button scope, nearest-edge anchor, start updates, song/reset/stop-armed behavior and cancel/final-release ordering. Original state traces match exactly; the original reversed-time-difference quirk is retained rather than corrected. Press/drag step arguments are the original already-snapped/clamped caller values.

Added original piano scrollbar continuation math, and original piano toolbar/corner zoom drawing using the carried icons. App composition still needs its original capture/update ordering; standalone widgets do not establish whole-app feel.

Cocoa now keeps precision/pinch in its own pending bucket until consumption, as in the original monitor; only its ordinary scalar wheel resets before OS polling. Window frame timestamps are taken after blocking poll completes so UI timers do not inherit pre-wait time. Removed obsolete macOS window-operation declarations from the otherwise portable interaction navigation header. Original navigation/raw-input replays and dummy-window tests remain passing; real native dispatch remains unavailable.

The latest full SDK integration run passed72/72 tests before the added waveform/icon/ruler references; those new references also pass independently. Work continues under the user's automatic-stage continuation instruction. No phase-completion or native hardware acceptance is asserted by this checkpoint.

## Rack painting and sampler service

Pure Rack filtering, row mapping, melodic detection and paint interpolation now match actual original function traces and complete saved project bytes. Limits,16-unit step width,32-unit grid top,28-unit rows, boundary inequalities, erase semantics and duration extension remain original. Arrangement source-duration propagation remains the app caller's responsibility as in the piano extraction.

Moved sampler worker/queue/update/flush into an app service with borrowed project/sample generations and a small opaque SDL thread boundary. Worker processes only its copied input/settings, marks done atomically and issues a non-audio wake. UI publication occurs before old PCM release. Original recording/editing gates, one-worker scheduling, epoch/generation/settings rejection, rollback and flush discard/reprocess behavior are retained. A twice-captured original-service harness matches every publication/state line and processed PCM byte through those cases. Audio publication is a concrete callback to the existing audio service; no graphics/native thread types enter DSP code.

## Control motion and automation display

Original knob/fader capture and motion now live in pure interaction code: descriptor bounds/kinds, BPM.25-per-unit sensitivity, normal range/160, logarithmic ratio, reversed width, integer fractional accumulator, gain-law faders and fixed-DX coarse alias interaction are preserved. Original capture/main/knob arithmetic traces match normal/scaled/integer/log/fader/alias sequences, including negative half-wheel rounding.

Automation's UI display follows the original two-pass active/held selection with duplicate targets, lane mute/solo, source offsets and integer pitch-range rounding.33 actual original display/value cases match exactly. Rendering consumes this display snapshot; automation never needs an SDL or graphics type to choose shown values.

Draw-only knob styles use the original pre-interaction shown-value snapshot, circle/arc banks, origins/rotations, pointer/unity marks and tints.40 intercepted original draw-command cases match every source/destination/origin/rotation/color field. Rotated texture support keeps original CPU origin/rotation arithmetic; SDL tests verify a four-color texture rotated90 degrees around its explicit origin. Render-target source storage remains SDL top-origin; app cache callers omit the old compensating GL framebuffer flip. Hardware sampling parity is unverified.

## Continued execution and editor composition

The user explicitly requested unsupervised continuation through the last stage while away. An active ongoing goal now carries the remaining rewrite work; phase boundaries are not stopping points. Existing no-new-feature/dependency and original-reference rules still apply. Native/hardware results remain unverified when this environment cannot supply them.

Original editor chrome is now a read-only painter over the existing Windows model, with pure title/chrome hit information. The integer thin-outline path retains pinned DrawRectangleLines .5/modelview-scale endpoint extension rather than substituting filled borders. A headless test composes the original760×430 Piano editor at its original320,128 rectangle with title/chrome, toolbar and full content; existing window assertions still pass. Other editor bodies and complete app event/service composition remain pending.

Complete SDK suite passed80/80 tests before adding the editor composition test; that test and window assertions pass.10 focused new renderer/control/rack/automation/sampler checks passed ASan+UBSan with leak detection disabled (unsupported here). ThreadSanitizer passed the new original sampler-service state/PCM comparison. Headless piano content image was captured outside original for development inspection; it is not a hardware/parity screenshot.

### Piano frame coordination

`interaction/piano_frame.c` carries the piano branches from original `main.c`
into a graphics-free frame coordinator, followed by the original `ui_piano.c`
press priority. Existing note captures clear before audition cancellation;
scrollbars update before gestures; gestures retain their final release-frame
sample; moving notes update next; velocity edits update last. A new press clears
old captures before it can start another operation. Modal cancellation, focus
loss and editor visibility retain the original distinctions.

Verified: twelve eight-frame sequences were captured twice against the frozen
original (main-loop blocks copied verbatim into a temporary external harness,
with original piano code and drawing disabled). Both original runs are byte
identical. The rewrite matches each frame's notes, selection, gesture snapshots,
scrollbars, key/velocity state, preview/audition commands and final project
exactly. These fixtures are additional original references, not replacements
for the Phase 0 audio fixtures. This verifies piano coordination, not the full
application event/update/drawing order or native input latency.

### Portable MIDI routing and typing audition

`interaction/midi_session.c` carries original `app_midi.c` note/controller
routing, sustain, retriggers, slot stealing, MIDI Learn, pitch bend, parameter
scaling, recording updates and the complete 37-key physical typing map. Its
borrowed project/take state and audio command callbacks contain no native MIDI,
windowing or drawing dependency. Device enumeration, reconnect and settings
persistence remain application/platform work. The existing CoreMIDI backend now
has a build target; ALSA retains its original OS-provided library requirement.

Verified: eight original-derived MIDI sequences, including 40-event slot
exhaustion, were captured twice using an external harness with original routing
and take-update blocks and original typing/panic functions. Original runs are
byte identical; the rewrite matches held notes, sustain, key highlights, learn
status, full take state, command order, parameter values and saved projects.
This deliberately preserves the original typing release's current-channel
argument while decrementing the channel recorded at key press. It is a retained
quirk, not a correction. CoreMIDI compiles on macOS; actual devices, reconnect,
settings persistence, input-overflow handling and native delivery are unverified
by these routing tests.

Complete current suites: **68/68** offline checks and **83/83** installed-SDL
validation checks pass, including unchanged original audio fixtures and callback
real-time audits. These are component/integration checks; the full desktop app
and Phases 3–5 remain pending.

The new piano-frame and MIDI-session reference comparisons also pass under
AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection remains disabled
because it is unsupported in this environment.


### Complete carried desktop frontend

The original main loop, six editor bodies, widgets, menus, browser UI, project
and preset actions, MIDI device management and recording flows now compile as
`libreloop2_desktop` against the own draw/input/window/thread boundary. SDL and
OS headers live in `platform/frontend_host.c`; there is no raylib/GLFW/OpenGL
link dependency. This is an integration checkpoint, not completed Phase2.
The original mixed UI routines are carried incrementally; their remaining
interaction branches must be connected to the pure models and checked.

The host retains original60Hz frame accounting, event polling after drawing,
configured full native sleep on active frames and blocking event wait when the main
loop's original .15-second idle rule enables it. Native precise navigation uses
the existing Cocoa monitor. HiDPI camera coordinates scale by framebuffer/window
width; preview targets retain pixel coordinates and omit the old GL-only
vertical flip. Device/window icon calls remain at the platform boundary.
Resource paths currently target source-tree assets; relocatable packaging and
vendored SDL source/static linking remain pending.

Correction: this checkpoint originally described a95% sleep/5% busy tail. That
mistakenly followed raylib's default config.h instead of the original CMake's
CUSTOMIZE_BUILD/EXTERNAL_CONFIG_FLAGS, which disables both busy-wait options.
The later frame-wait correction below restores the configured original behavior.

Verified: desktop compiles on the installed SDL SDK. The complete original smoke
sequence exits successfully after40 frames and produces four1200x675 PNGs using
SDL dummy window/software renderer. The Piano capture was inspected and contains
its original editor geometry, toolbar, keys, grid, notes and velocity area;
Browser, Arrangement and transport remain present behind it. The app's MIDI
routing now delegates to the already verified portable MidiSession model.
Screenshots live outside the original and are integration evidence, not original
GPU pixel/parity or native hardware measurements. Native GUI/display, all UI
flows under scripted input, physical trackpad and real audio/MIDI devices are
still unverified. No visual restyling has occurred.

The desktop Piano note press, gesture drag and key audition now delegate to the
pure Piano model. A capture compiles the actual desktop `ui_piano.c` with drawing
calls disabled and matches all30 unchanged original piano-press fixtures. This
checks the binding as well as the model. The full desktop smoke and that binding
capture also pass ASan+UBSan (leak detection unsupported).

Pinned helper checks additionally compare22 Bézier points and ten filename cases
against two identical original runs. The host retains original powf-based
Bézier arithmetic, keeps leading-dot filenames intact, and treats both path
separators like the original UI. Circle drawing shares the previously carried
pinned36-segment geometry with the icon painter. These fixes preserve the
reference; they do not replace fixtures or alter interaction assertions.

Desktop editor navigation now delegates to the pure navigation model, including
wheel, precise scroll, momentum, pinch and middle-drag behavior. Eight unchanged
original navigation traces (144 frames, scale1/2) also pass through the actual
desktop adapter and its original state defaults. Knob capture, drag arithmetic,
wheel changes and automation display now use their existing pure models; UI
status text and draw-time snapshot order remain as carried from the original.


### Relocatable macOS development bundle

The developer target now produces `LibreLoop.app` with samples, presets, font,
branding/icon, GPL text and third-party notices. It copies the approved installed
SDL runtime into Contents/Frameworks and changes the executable's load command
to @executable_path/../Frameworks. The original microphone usage description and
HiDPI plist flag are retained, with a distinct rewrite bundle identifier.

Verified: local signature verification succeeds; a copy outside the build tree
completes the40-frame headless smoke sequence. dyld's load trace confirms SDL is
loaded from that copied bundle, not Homebrew. Current file-byte measurements are
in sibling baseline-artifacts/gui/desktop-bundle-size.json. This is a developer
bundle for macOS26+, not the final static/vendored-SDL distribution. Native
Finder launch, permissions, devices and display behavior remain unverified.

Latest fresh checks after bundle/preview and native-clock/header foundations:
70/70 offline and88/88 installed-SDL checks pass. Three current full-desktop,
navigation-binding and visual-state checks pass ASan+UBSan. The frozen original
and both concept images retain all182 recorded file hashes; original working
tree is clean and Phase0 audio fixture files are unchanged.

### Rack/ruler desktop bindings and conditional-state corrections

The actual desktop rack filter/channel/melodic/paint handlers now call the pure
Rack model, and ruler press/drag calls use the pure Ruler model. Original Rack
and ruler traces also pass through these desktop bindings, with the existing
fixtures unchanged.

Binding review found two omissions: the pure ruler drag had omitted main.c's
paused-transport reset request, and the Piano binding had updated arrangement
source_steps even when no pattern extension occurred. Both now retain the exact
original conditions; the Rack binding does likewise. A trimmed arrangement
source remains trimmed during edits inside a pattern; extension updates its
source duration. These are parity corrections, not original behavior changes.

New additional fixtures were captured twice from frozen original code outside
its repository: actual Piano calls with a shortened arrangement source, actual
Rack paint/extension behavior, and the verbatim original main.c marker-update
branch with reset cleared before each drag. Original runs are byte identical.
The desktop bindings match them. All earlier audio/interaction goldens and
assertions remain unchanged. The original abbreviated ruler capture did not
exercise the cleared-reset case; the new capture uses the full original branch.

Verified now:76/76 offline and94/94 installed-SDL checks pass. Six focused
binding/application checks pass ASan+UBSan with unsupported leak detection
excluded. Native hardware/trackpad checks and broader full-app keyboard/menu,
history/file/drop/recording ordering remain separate acceptance work.

### Clipboard/history and keyboard dispatch

A new capture exercises actual desktop edit_selection, piano_shift/octave,
history_checkpoint and undo_redo handlers against frozen original function
bodies. Twenty successive full saved projects, selection arrays, history
count/cursor, clipboard and status traces match exactly. The sequence covers
select/copy/paste/cut, shifts/octaves, undo/redo, Browser/capture/take guards,
clip paste and invalidated clip sources. Original expected outputs were captured
twice identically outside the original repository; project fixture compression
is lossless standard-library gzip, with hashes checked on expanded bytes. No
existing reference or comparison assertion is weakened.

The original main.c shortcut branch order now lives in the pure Shortcuts model.
Forty controlled contexts (macOS/non-macOS modifier rules, simultaneous keys,
repeat, modal/capture/sample-drag/Browser guards, editor/tool precedence, note
nudges and Space transport) match two repeated captures of the original main
branches. The desktop dispatches those concrete actions in the same order.
Tests control key current/previous/repeat states, context and frame timestamps;
they do not claim native key delivery or every project/menu side effect.

Verified:96/96 current developer-SDL checks pass; the new clipboard/history,
shortcut and full desktop smoke checks pass ASan+UBSan with unsupported leak
detection disabled. The original/concept references retain all182 hashes.
Broad file/drop/recording interactions and native hands-on feel remain pending.

### Actual sample-drop handler replay

Twelve original-source scenarios now compare the actual desktop drop_sample and
import_sample flows: empty Playlist placement, Audio-list import, invalid ruler/
editor targets, Audio-clip replacement, occupied placement, channel limit,
Rack replacement/new space, sampler replacement, FM-to-sampler automation
cleanup and loader failure. The injected loader supplies deterministic stereo
PCM; this isolates handler behavior from OS file lookup/codec/device delivery,
which have separate decoder/audio/platform tests. It is not a native OS-drop or
physical recording test.

Original captures were run twice identically outside the reference repository.
The rewrite matches all full saved projects, processed PCM bytes, arrangement
selections, generation/frame/duration fields, target/editor/filter changes,
status and audio-publication command counts. The occupied case confirms zero
loader and publication calls before rejection. Existing fixtures remain unchanged;
new PCM/project fixture compression is lossless and hashes check expanded bytes.

Verified now:97/97 developer-SDL checks pass. The full twelve-case drop comparison
also passes ASan+UBSan with unsupported leak detection excluded. Broader native
file/drop delivery, real recording/device lifecycle, default/static SDL packaging
and hands-on parity remain outstanding.

### Recording lifecycle replay

Sixteen captures compare actual desktop recording_start/start_audio/finish and
recording-session prepare/start/finish logic against the frozen original bodies.
They use controlled device responses and writer completion, deterministic WAV
files and real WAV mapping/waveform processing. Cases cover disabled masks,
missing MIDI input, MIDI held notes, automation-only takes, unarmed/full Rack,
directory/sampler/prepare/device/worker failures, empty takes, committed frames
with failed mapping, device errors, combined Audio/Notes/Automation and active
Song cursor origin. This is handler/lifecycle parity, not physical device or
actual writer-thread scheduling validation; those have separate writer/callback
checks and native hardware remains unverified.

Two original runs match byte-for-byte. The rewrite matches started/finished
projects, processed PCM, state/status and publication/service counts. Empty
files are removed; failed mapping retains committed duration and path; successful
samples are published with the original generation/application bookkeeping.
No count-in is invented: the frozen source has no count-in state as recorded in
Phase0. Fixture compression is lossless and original hashes check expanded data.

Verified:98/98 developer-SDL checks pass, and the complete sixteen-case lifecycle
comparison passes ASan+UBSan with unsupported leak detection excluded. Existing
audio and interaction fixtures remain unchanged.

### Popup text editing extraction and installed app check

The popup/chooser text handler now delegates to graphics-independent
interaction/text_edit.c using the application's input queue. The original branch
order and UTF-8 helpers remain unchanged, including whole-string selection,
control-character paste termination, strict byte-capacity boundaries and no
Backspace repeat branch. Twenty-four frozen-original cases check both the pure
handler and its actual desktop adapter. Two original captures were identical;
fixtures record original source hashes and use pinned raylib5.5 UTF-8 behavior.

Verified:101/101 developer-SDL tests,82/82 SDL-free offline tests, and both new
text-edit comparisons under ASan+UBSan (unsupported leak detection excluded).
The installed macOS bundle also passes strict signature/resource/runtime-path
checks and launches four headless views from a staging path containing spaces.
CMake install-time edits are followed by final signing. See toolchains.md for
prepared but unverified Linux/Windows and vendored-static installation paths.
Native display, clipboard/IME delivery, devices and physical feel remain
unverified. The original tree and carried audio fixtures remain unchanged.

### Visible text preserved across API renaming

A source-literal audit found eight accidental substitutions from the graphics
API conversion: three Color menu labels, three Color status strings and two
Fade in instrument labels. They now match the frozen original. A frozen inventory
checks all1,051 original non-include UI string occurrences across the carried
frontend and extracted interaction handlers. Its only exclusions are the three
macOS resource-path formats whose directory boundary changed. An external
negative-control copy confirms the check rejects the accidental Fade in rename.
This inventory supplements behavior/render tests; it does not establish menu
placement or native-device acceptance. All102 developer-SDL checks pass after
restoring the strings, including unchanged audio/interaction references and the
installed bundle smoke/signature check.

### Numeric and rename validation

Numeric Apply and ASCII-space name trimming now live beside text editing in the
portable interaction layer. The original64-byte copy, comma replacement,
strtof/trailing-whitespace parsing, finite/inclusive-range/integer conditions,
failed-target preservation and rename-buffer mutations are retained. Two original
runs of200 numeric and17 name cases match exactly; rewrite comparisons and the
103-test developer desktop suite pass. Popup-value and both text-edit comparisons
also pass ASan+UBSan with unsupported leak detection excluded. These are handler
checks, not native popup/clipboard/IME or full dialog acceptance.

### Menu focus, keyboard repeat and scrolling

Menu begin/end navigation now uses a concrete portable MenuNavigation state and
logical pointer/key flags. No rendering/window header is used by its model. The
original preceding-frame bounds/count, pointer-before-key ordering, opposing-key
cancellation, one-row edge scrolling/wrapping, open-frame suppression, initial
Enter/keypad Enter handling and128-bound count/clamp remain unchanged. Buttons
still register their existing geometry during drawing; no widget framework or
layout change was introduced.

Thirty-two original controlled begin/end captures were identical across two
runs. Both the pure model and actual desktop adapters match them. Full verification
passes105/105 developer-SDL checks and86/86 SDL-free checks; both new comparisons
pass ASan+UBSan with unsupported leak detection excluded. Pointer bounds use
logical units and the handlers have no clock/DPI-dependent branch. Menu item
activation, full popup lifecycle/native delivery and hands-on feel remain broader
acceptance work. The frozen original and audio fixtures remain unchanged.

### Own file chooser dialog flow

The actual carried file-picker drawing/input/sync/submit/commit/text handlers now
have48 original-derived contexts (24 three-frame scripts at application layout
scales1/2). Real chooser directory enumeration, filtering, canonicalization,
extension/overwrite validation and remembered-folder persistence are exercised.
Inputs fix logical geometry, pointer, key/character flags and timestamps. Only
temporary filesystem-root spelling is normalized; project I/O success/failure is
controlled and rendering/button hover descriptions are disabled.

Cases cover file/directory selection and double-click, the strict .35 boundary
and original negative-time quirk, keyboard folder entry, filename/path entry,
invalid paths, overwrite/Back/cancel, hidden files, fractional scrolling,
centered-thumb drag/release, Up/Home, empty folders and open/save/collect/relink/
preset/export dispatch. Initial save field/select-all and per-action remembered
purposes follow the original launch handlers. Two original runs are identical;
actual rewrite handlers match all144 frames and remembered-folder outputs.

Verified:106/106 developer-SDL and87/87 SDL-free checks pass; the finalized chooser
comparison passes ASan+UBSan with unsupported leak detection excluded. This does
not establish full dialogs with real project I/O/rendering, framebuffer DPI,
native events/clipboard/IME or hands-on feel. A host probe also shows x86_64
executables cannot run here (Bad CPU type); cross-architecture audio execution
is unavailable, and no cross-CPU tolerance is invented.

### Filesystem helper behavior restored at the frontend boundary

An API audit found the frontend had substituted metadata/creation-success tests
for three pinned raylib behaviors. It now reuses the existing native Browser
MakeDirectory/opendir helpers and an existence-only F_OK operation: nonempty
MakeDirectory returns0 even after mkdir failure; DirectoryExists reflects whether
a directory can be opened; FileExists also accepts directories/nonregular paths.
This restores the original recording/preset error-check order. Backslash/colon
walking and partial nested creation remain as in the frozen reference. No new
filesystem dependency or duplicate directory implementation is retained.

Two original raylib5.5 captures match return values and created-directory state
for null/empty/existing/nested/blocked/backslash/colon/trailing/repeated/dot/absolute
paths, plus metadata behavior for a file, directory, missing path and unreadable
directory. Both native helpers and actual frontend bindings match. Full108/108
developer-SDL and88/88 SDL-free checks pass; both new comparisons pass ASan+UBSan
with unsupported leak detection excluded. The x86_64 offline targets still
compile/link; execution remains unavailable. Windows UTF-16 helper source is
prepared but uncompiled/unverified. Original files and audio fixtures are unchanged.

### Path extension and format-buffer parity

The frontend path/format wrappers now use graphics-independent text_legacy.c.
The original whole-path extension search, leading-dot rule,16-byte extension copy,
1024-byte selector buffer,128-result split behavior, ASCII case conversion and
null-selector handling are preserved. Four rotating TextFormat buffers are cleared
and mark truncation with three dots exactly as in the pinned implementation.
Null filename stems now return the original empty string. Valid original filename
cases match; the port's existing bounded path storage is retained rather than
emulating undefined overflow on an invalid basename longer than255 bytes.

Two original raylib5.5 runs produced identical path results and complete format
buffers. The model and actual frontend bindings match24 names across13 selectors,
split-limit cases, null stems, formatting length boundaries and four-buffer
lifetime/aliasing. All110 developer-SDL and89 SDL-free checks pass, and both new
comparisons pass ASan+UBSan with unsupported leak detection excluded. The fixed
constant ellipsis copy also passes both exact-buffer comparisons. The desktop is
rebuilt and its signature verifies. Native clipboard/drop events and cross-platform
runtime behavior remain unverified; original audio fixtures are unchanged.

### Clipboard availability preserves selected-text behavior

The SDL boundary now preserves NULL for unavailable/nontext/failed clipboard
reads and an owned empty string for an advertised empty UTF-8 payload. The frontend
no longer coerces NULL to an empty string. That distinction matters to the frozen
text-input branch: null paste retains selected text, while empty paste clears it.
Direct UTF-8 data retrieval avoids SDL_GetClipboardText's empty fallback; source
links and platform limits are recorded in raylib-dependencies.md.

The dummy/software test connects actual platform/frontend reads to the existing
original-derived text-edit model. Absent, empty, UTF-8, binary-only and failed
providers plus twenty ownership/replacement cycles pass. Full111/111 developer-SDL
checks and the new ASan+UBSan check pass (unsupported leak detection excluded).
No native OS clipboard was modified by these dummy-driver tests. Native clipboard,
IME, Linux/Windows delivery and failure timing remain unverified.

### Configured original frame pacing restored

The original CMake and recovered generated flags disable both raylib busy-wait
options; config.h's default partial busy wait is not the effective baseline.
The frontend's incorrectly introduced95% sleep/5% spin has been removed. A small
native platform_frame_wait carries full-duration macOS usleep, Windows Sleep and
Linux nanosleep conversion/retry branches. Polling still follows drawing/limiting,
frame accounting remains unchanged, and idle still uses SDL's blocking event wait.

Sixteen macOS duration cases capture the actual original WaitTime body with the
configured options disabled, including negative/zero and microsecond boundaries.
Two original runs are identical, and the port matches call counts and arguments.
Full112/112 developer-SDL and90/90 SDL-free checks pass; the new comparison passes
ASan+UBSan with unsupported leak detection excluded. This proves configured source
and native-argument parity, not real scheduling/input latency or measured CPU/GPU
savings. Linux/Windows sleep execution remains unverified. Earlier pacing text is
corrected explicitly above; original audio references are unchanged.

### Real project/preset/asset services through carried app handlers

Twelve original-derived integration contexts now exercise actual app_project.c
load/save/collect/export/relink/preset handlers, real project/asset/preset I/O,
mapped stereo F32 WAV input, sample processing and history. They include valid,
missing and corrupt loads, failed saving, relink name retention, sampler preset
success/missing-source failure and the original confirmation-commit/save-before-
quit branch with success/failure. Publication/stop/update callbacks are counted;
hardware callbacks, decoder fallback and worker scheduling are excluded here.

Two original runs are identical with only mkdtemp's suffix input fixed (real
mkdir retained) and temporary-root spelling normalized in captured project paths.
All path components, fields and sample values remain checked. Generated project,
preset, collected WAV and export files are compared unchanged. Current/saved
projects, original/processed PCM, generation counters and document/status/history
state match. Lossless fixture compression records hashes of expanded data.

Verified:113/113 developer-SDL and91/91 SDL-free checks pass; all twelve cases pass
ASan+UBSan with unsupported leak detection excluded. This supplements chooser
input/rendering and separate decoder/callback tests; it does not establish a
complete native dialog/render/device lifecycle. The required static SDL source
archive remains absent and shell download DNS still fails. No shipping/static
claim is made; original files/audio references remain unchanged.
