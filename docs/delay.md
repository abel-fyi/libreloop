# Built-in Delay

Add Delay from a mixer strip's `+` menu. New instances use a compact horizontal
editor inspired by the owner's local `fruity-delay.png` reference. Seven knobs
are grouped into Input, Feedback, Time and Dry. The editor opens at 740×220 logical
units, with a 620×220 minimum; resizing does not add a second row of controls.
Existing shared knob drawing, value menus, automation, MIDI learn, presets and
window behavior remain inside LibreLoop. A separate Luice library is not planned.

## Controls and signal flow

| Section | Control | LibreLoop behavior |
|---|---|---|
| Input | Pan | Centered stereo balance, −1 to +1; attenuates the other side when panning Left or Right. |
| Input | Volume | Gain entering the delay line, 0–2. Default 35%; does not change dry level. |
| Feedback | Normal / Invert / P.pong | Normal preserves sides. Invert swaps the wet output sides while keeping feedback independent. Ping-pong also crosses feedback, alternating successive echoes. |
| Feedback | Volume | Recirculation, 0–100%, default 35%. At unity and with filtering bypassed, an isolated echo does not decay. Continuous input can accumulate and clip. |
| Feedback | Cut | One-pole low-pass in the wet/feedback path, 100–20000 Hz; 20000 bypasses filtering. Default 8000 Hz. |
| Time | Time | Continuous tempo-relative duration, ⅛–16 sixteenth-note steps; default 3 steps. |
| Time | Offset | Centered stereo timing offset. Negative delays Left, positive delays Right. Magnitude adds up to 50% of base time to that side. |
| Dry | Volume | Independent original-signal gain, 0–2, default unity. Set to zero for a wet send. |

Input balance uses `min(1, 1-pan)` on Left and `min(1, 1+pan)` on Right.
Base delay is `15000 / BPM * steps` milliseconds, bounded to 1–4000 ms. Each side,
including stereo offset, is capped at four seconds. The status bar shows the
Time knob's step value and its effective base milliseconds. Arbitrary fractions
between rhythmic marks are available and remain relative to the project tempo.

The new editor replaces the earlier sync/division buttons and wet/dry knob.
Shared rack mix remains a saved/automatable control; new instances set it to 1.
The processor interpolates from unity dry at rack mix 0 to the configured dry
plus echoes at rack mix 1. Thus bypass is unity dry even with Dry Volume at zero.
The former wet-output gain parameter remains available to existing automation.
All seven visible continuous controls have stable automation/MIDI targets that
follow FX reordering. The three feedback modes are saved discrete choices.
Export retains the song's existing length, without appending an effect tail.

## Magnetic Time knob

Time uses the shared knob implementation with a small interaction-layer snapping
policy, independent of SDL/drawing. Its logarithmic range gives small subdivisions
usable space. Detents, in steps, are:

`⅛, ⅙, ¼, ⅓, ½, ⅔, ¾, 1, 4/3, 1½, 2, 8/3, 3, 4, 16/3, 6, 8, 32/3, 12, 16`.

A raw value snaps to the nearest mark within a log-distance of 0.035. Dragging
keeps an unsnapped accumulator, so the pointer can leave a detent without getting
stuck. A normal drag spans the logarithmic range in 160 logical pixels; holding
Ctrl slows it tenfold and bypasses snapping. Wheel sensitivity spans the range in
80 units, or 800 with Ctrl. Wheel motion also keeps an unsnapped accumulator,
including sub-unit trackpad deltas. Other controls retain their existing interaction math.
The right-click value menu accepts an exact step value without magnetic rounding.

## Reference and implementation choices

[Image-Line's Fruity Delay 2 manual](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins/Fruity%20Delay%202.htm)
documents the seven controls, stereo feedback modes, tempo-relative timing,
fractions and fine adjustment. Its exact detent thresholds, parameter tapers,
offset range and filter implementation are not established by the screenshot.
LibreLoop's magnetic policy, balance law, one-pole filter and 50% offset range are
explicit implementation choices, not measured reproductions of that processor.
No source code, artwork, logo or font from the referenced plugin is bundled.
The panel uses LibreLoop's existing bitmap font, gray surfaces and cyan selection.

## Processing, storage and compatibility

The processor runs at the engine's fixed 48000 Hz rate. Time/offset changes
crossfade fixed read taps over 20 ms; continuous gains, balance and routing
matrices use 20 ms one-pole smoothing. Filter coefficients update every 32 samples.
A further time change waits until the active crossfade finishes. Reset invalidates
history with constant-size metadata work, without clearing the PCM ring.

Each prepared delay position occupies **1,536,128 bytes** on this macOS arm64
build. Its four-second stereo ring is allocated/page-touched before publication
on the UI/control thread, or before offline rendering on the export thread.
A lock-free atomic pointer publishes initialized storage. The callback alone
mutates DSP state; no allocation, freeing, locks, waits, logs or file I/O were
added there. Cached buffers remain owned by their bus/slot until rack destruction;
editing more positions reserves more buffers. Missing storage passes dry with a
status warning during playback; startup/export fail if preparation fails.

Optional `x-delay.*` project fields store the additional parameters and processing
model. Projects without Delay retain their previous serialized representation.
Earlier delay projects and version-6 presets retain their old DSP settings/editor.
New Delay presets use container version 7; other preset kinds keep version 6.
Loading a preset with a different editor layout grows the existing window to its
required minimum so the controls remain accessible.

## Verification

The macOS arm64 Release suite passes 149 cases, including the unchanged original
comparisons under the existing approved Stretch contract. Focused tests cover
exact routing/echo timing, full feedback, offset direction, input balance/gain,
dry level, old/new settings and preset roundtrips, automation, callback echoes,
block determinism through time/tempo/offset/mode changes, magnetic detents and
escape/fine/wheel behavior. The core, integration, callback, mixer and magnetic
knob tests are also checked with AddressSanitizer and UndefinedBehaviorSanitizer.

The Time ring adds 25 procedural dots per paint and no bitmap assets. Its isolated
per-frame cost and binary-size delta have not been measured; the earlier build
artifacts were removed during the requested cleanup.

Software screenshots at 1448×1086 and 900×562 were inspected against the reference.
The rebuilt app passes strict bundle signature verification. Native
mouse/trackpad feel, real-device listening, comparisons against actual Fruity
Delay 2 audio, CPU/latency measurements and other platforms remain unverified.
