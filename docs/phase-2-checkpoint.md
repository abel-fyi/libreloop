# Phase2 checkpoint — implementation present, acceptance incomplete

## Completed

The complete carried desktop now builds as a macOS development app: Browser,
transport, six editor bodies, menus, project/preset actions, file chooser,
clipboard/drop handling and recording flows. It uses own drawing, input,
windowing and thread interfaces over SDL3. The engine has no graphics/windowing
dependency; the pure interaction models remain free of SDL and raylib. Desktop
Piano edits, navigation, controls, automation display and MIDI routing delegate
to those models. Remaining mixed UI glue is carried from the original and still
needs broader interaction replay coverage.

The relocatable LibreLoop.app contains its resources, notices and an approved
installed SDL runtime. A nondefault --visual-preview applies dark gray/cyan
colors with no geometry, font or hit-area changes. Normal launch retains the
original look for parity review. The guarded WinMM/native-clock code is a Windows
foundation, not a complete Windows port.

## Verified and how

Current host: macOS arm64, AppleClang21, SDK15.4, installed SDL3.4.16. The current developer-SDL suite passes97/97 checks; prior full offline suite
passed76/76 before adding clipboard/history, shortcut and drop captures. Expected audio/interaction data
continues to come from the untouched frozen original. The actual desktop Piano
adapter matches30 original press cases; actual desktop navigation matches eight
original wheel/precise/momentum/pinch/modifier/pan traces at scale1/2. Original
PCM hashes, callback guards, font metrics/masks, Browser and control fixtures
remain unchanged and passing.

The complete application smoke sequence runs40 frames and captures four1200x675
views using SDL dummy window/software renderer. A relocated app copy runs that
sequence; loader tracing verifies its bundled SDL library. Local signature
verification passes. Both color modes save byte-identical project data and
produce matching-size screens. Current desktop/navigation/preview checks also
pass ASan+UBSan, with leak detection disabled because it is unsupported here.

## Unverified and remaining

Native GUI/pixels, display-DPI transitions, physical mouse/trackpad/pinch feel,
real audio/MIDI devices, recording permissions/latency, idle/active hardware
CPU/GPU, input latency and battery behavior remain unverified. Actual desktop clipboard/history, shortcut ordering, rack/ruler/Piano bindings
and sample-drop handler captures now match the original. Broader whole-loop
menu/file/recording delivery and lifecycle replay coverage remains incomplete. Passing component fixtures and headless screenshots do
not establish complete hands-on acceptance.

The developer runtime requires macOS26+. It is not the final vendored/static SDL
build: the SDL source archive is absent and restricted shell downloads cannot
resolve hosts. No shipping/static decision has been adopted. Windows filesystem, recording writer/mapping and UTF-8 pathname source candidates
are now implemented. Windows build/packaging and native checks remain pending;
guarded Windows source has not been compiled or run on Windows.

The visual preview is available for review, with original geometry preserved;
it does not claim final layout/hands-on approval. Optimization and CPU-rasterizer experiments have host measurements in
experiments.md; static/shared SDL source builds and other-platform measurements
remain pending. No experimental optimization, linking choice or CPU rasterizer
has been adopted. The ongoing goal remains active through these checkpoints.
