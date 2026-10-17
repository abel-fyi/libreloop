> Current direction: the user selected **2.png alone**, authorizing a close
> replication of its classic gray DAW layout with LibreLoop's existing features.
> The earlier blend and geometry-preserving preview described below are historical.
> See [the current layout record](phase-3-layout-proposal.md).

# Visual direction and concept audit

Phase0 audit: reviewed the actual `../ai-screenshots/1.png` and `2.png` directly.
They are AI concept art, not original application screenshots. At that checkpoint
no visual assets, font, layout or styling were changed. The later nondefault
palette preview is documented below; the complete review candidate is in
[phase-3-layout-proposal.md](phase-3-layout-proposal.md). No final adoption or
hands-on parity approval is claimed.

## Image 1

Near-black blue background, cyan borders/text/selection, saturated color-coded sources, yellow/green meters. Very dense simultaneous Browser/Arrangement/Piano/Mixer and right synth column. Transport has compact rectangular buttons and LCD-ish monospaced cyan readouts, line-framed panels, top piano toolbar, velocity lane. Arrangement includes labeled section blocks. Synth shows three oscillator tabs/noise, filter, three envelopes/two LFOs, modulation matrix; mixer includes named effect list and send/group tabs. Typography appears condensed/monospaced and often extremely small. Cyan is overused for labels/borders as well as active states; a literal copy would weaken selection hierarchy.

Artifacts: duplicate track number05 on Pad/Vox, inconsistent spacing/border thickness, inconsistent ruler progression (Piano measures13..21 against arrangement1..61), implausibly dense tiny text, icon meanings unclear, mixer names/assignment not consistently equivalent to Playlist lanes, rigid narrow synth column cannot expose existing DX7 pages cleanly. Some waveform-like curves appear mixed with MIDI notes without source distinction. Readout values/CPU percentages are illustrative, not measurements or existing data bindings.

## Image 2

Medium gray flat surfaces, dark green-gray recessed fields, fine borders, pastel track/clip colors, turquoise notes/envelope traces, green/yellow meters. More generous control spacing and readable main labels, less ornament, broad arrangement rows. Browser contains tree plus lower item list. Top transport has bar/beat/tick, tempo/signature, snapping and extra switches, CPU and engine status. Right panel has two oscillators/noise, filter, amplitude envelope and LFO; lower Mixer has larger faders. Piano tools/quantize/snap/scale/length sidebar consumes significant grid width.

Artifacts: low-contrast labels on gray, some tiny units, inconsistent control/title sizes, ambiguous track/clip/mixer associations, master-looking Playlist row actually contains automation; fixed dock layout would replace existing floating/pinned editors. Left piano sidebar contradicts the width-preserving toolbar direction. Synth diagram/control grouping does not map directly to the existing three engines. Ruler/velocity label scale and transport granularity differ from original.

## Current visual policy: image 2 alone

The user superseded the blend and approved a close replication of image 2,
including layout and aesthetic changes. The SDL app now defaults to cool gray
panels, darker inset fields, mint active states, pastel clip/strip identities,
fine borders and restrained bevels. It uses the existing Liberation Sans, with
larger classic captions. No glow, blur, shadow or heavy gradient was added.

At the reference size, Browser occupies the left column, Arrangement and Piano
occupy the center, instrument controls occupy the right column and Mixer spans
the bottom. The existing clip-source picker occupies the lower Browser area.
Piano tools occupy a left column, as in image 2. Actual auto-grid and remembered
note-length values are displayed; unsupported selectors are omitted. Smaller
windows use the documented compact startup arrangement; editor move/resize and
view actions remain available. See [the layout record](phase-3-layout-proposal.md)
for exact geometry, changes and verification.

The concept's named markers, extra views, invented diagnostics, configurable
signature and unsupported synth/effect controls are not added. Real notes,
samples and automation are rendered from the project; artwork is never used as
an application background. The illustrative showcase is a real editable
project made with carried instruments, samples and automation.

## Concept elements with no direct counterpart

| Element shown | Image | Existing source comparison / decision |
| --- | --- | --- |
| Hamburger menu; Options/Tools menu, Track/Clip menu | 1 / 2 | Existing FILE/EDIT/VIEW/HELP only; preserve actions, do not add menu features just to match art. |
| Browser Project/MIDI/FX tabs; Files/Devices tabs | 1 / 2 | Existing Browser folder tree plus Playlist source picker; MIDI own settings dialog, no Devices Browser. |
| Predefined categorized instrument/preset/effect library tree | Both | Browser lists actual filesystem folders/audio/projects. Built-in instrument/factory menus and own preset chooser exist separately; do not invent a synthesized library. |
| Browser lower Name/Type preset browser table | 2 | No preset-table equivalent. The existing clip-source picker now occupies this lower area; sample audition remains in the upper Browser. |
| Named Intro/Verse/Build/Drop/Break/Outro/Hook section markers | Both | No named timeline marker data/editor; only start arrow/ruler loops. Do not add. |
| Signature readout/edit (4/4), beat/tick numerical transport, editable loop-length LCD | Both | Original fixed16-step bars and bar:sixteenth readout; loops are ruler-defined, no signature control. Retain available transport data. |
| Previous/next/jump transport arrows | Both | Stop twice rewinds; no corresponding next-location transport buttons. |
| CPU/MEM meters/percentages, engine Running/sample-rate/buffer readout | Both | No GUI counterparts. Do not invent diagnostics to populate these fields. |
| SYNC/INT/MIDI/CLK switches | 1 | No clock sync/external MIDI transport; MIDI notes/CC/record only. |
| Snap dropdown/checkbox, Quantize, scale Chromatic/C Major, note length selector | Both | Existing automatic visible grid and remembered note length, no selectors/commands; adding changes feel and feature scope. |
| Piano Score, Drum Edit, Step Seq/Step Editor, separate Automation editor tabs | Both | Existing Piano/Rack steps/Playlist curves are distinct editors, not these tabbed new views. |
| Unmapped piano icons (text, lock, diamond/octagonal tools) | 1 | Only Pencil/Brush/Select plus row zoom exist; no invented actions for uncertain symbols. |
| Generic SYNTH/FX/MATRIX/GLOBAL tab hierarchy | 1 | Existing custom/DX7/analog engine-specific pages plus separate Mixer effects. Global maps partly to DX7 Global, not a universal module. |
| Third independently editable oscillator, Noise tab, OSC mix1/2/3/N topology | 1 | Analog has two shared waveform/detuned oscillators, sub/noise; DX7 has6 operators, Custom FM3 operators. Preserve these concrete engines. |
| Multiple freely assignable envelopes and two independent LFOs | 1 | Existing engine-specific sound/body/strike/filter envelopes and per-note motion; no independent Env3/LFO2 routing UI. |
| Modulation matrix Source/Destination/Amount, aftertouch routings | 1 | No arbitrary matrix or aftertouch processing. MIDI CC learn exists, not equivalent. |
| Filter selectable Low Pass24, Drive | 1 / 2 | Analog fixed filter/resonance/env depth exists, no exposed type/drive control. EQ shapes exist in separate EQ editor. |
| Generic LFO Destination/Fade/Amount modulation routing | Both | Specific pitch/vibrato/volume/tremolo/PWM/native DX7 sensitivities exist; no arbitrary target selector. |
| Independent oscillator Oct/Semi fields per osc | 2 | Shared carrier ratio/detune/mix/sub controls; no independent per-osc octave/semitone controls. |
| Right Instrument/Mixer/Editor and Synth/FX/Routing docking tabs | 2 | Original floating, pinned, resizable editors; docking is a layout/interaction decision, not accepted by art alone. |
| Clip FX / Send FX / Groups / Grouping / Sends panels | Both | No clip-effect rack, multiple sends or grouping. One outgoing insert cable plus Mixer slots must remain understandable. |
| Compressor, Tape Saturation, Limiter named rack slots | 1 | Built-in Chorus and7-band EQ only. Width exists as bus control, not effect slot. No extra DSP. |
| Separate Master OUT/output mixer strip | Both | Mixer Master and top listening volume already separate; no additional bus/device routing functionality. |
| Per-Playlist-track dedicated M/S buttons and record-arm buttons | Both | Lane mute/solo light and context interactions exist; recording arm belongs to Mixer. New buttons/hit areas change interaction and need layout approval. |
| Browser/arrangement illustrated instrument/track names and demo content | Both | Demo names/data are not project specifications. Do not create sample packs/features to match illustrations. |

Partial counterparts are listed deliberately: a familiar visual label is not enough to infer equivalent DAW semantics. Existing Analog2 oscillators and envelope/LFO controls should appear, even though the prompt warns about “extra oscillators/envelopes/LFOs”; only nonexistent parts are omitted.

## Layout conflicts and approval gates

Any docking, simultaneous fixed panels, right instrument column, larger typography or wider track labels changes editor geometry. In particular Piano visible grid/ruler/keyboard widths, velocity lane, source picker width, track header edges, knob/fader rectangles, ruler start/loop hit strip, scrollbar travel and transport positions are interaction changes. Even a top toolbar can reduce grid height if enlarged. Phase 3 must propose full layout and get one-time approval; subsequent deviations affecting hit areas/anchors/modifier sensitivity require approval. Logical drag thresholds and snap math remain unchanged. Replays update only approved geometry fixtures and document coordinate changes separately.

Do not copy duplicated track numbering or gray-on-gray labels. Color meaning/hover/selection/recording distinction must remain readable. Preserve sample preview, automation points, clip crops and separate PAT/SONG playback rather than sacrificing a functional panel for the art.

## Cost and verification

The Phase-0 measurement status and earlier charcoal/cyan preview are superseded.
Current measured software timings, same-toolchain binary comparison, new-asset
size and hardware limitations are in [classic-render-measurements.md](classic-render-measurements.md).
Normal launch uses the classic layout; `--legacy-ui` is the comparison option.
No new font or bitmap asset was added. Native-display/HiDPI readability and
hands-on pointer acceptance remain unverified.

## October 2026: Windows/Motif and tracker refinement

The user's subsequent direction is a 1990s tracker workstation with Windows
95/2000 and Motif character. This supersedes the flat gray finish, while keeping
the current DAW layout. Warm gray raised faces, two-edge bevels, inset instrument
groups/displays, teal focused titles, pastel channel colors and small colored
folder/sample icons are drawn procedurally with the existing font. Knobs gain
concentric highlight/shadow faces; sliders retain their original controls and
geometry. Scrollbar thumbs are visibly raised even without hover. Existing
FM/Analog envelope graphs gain blue-teal area fill and stage separators using
the existing plotted points and formulas. No bitmap skin or dependency is added.

The user also requested more colored Windows 2000-style button icons, optionally
later. This pass adds browser folder/sample icons only; the wider toolbar icon
redesign remains follow-up work. Use original artwork/procedural symbols inspired
by that period, with existing functionality and bottom-bar descriptions.

Geometry, ruler math, snapping, hit targets and input semantics are unchanged in
this refinement. Separate Mute/Solo buttons, slider replacements, dropdown
behavior and dockable/resizable tiling from notes.txt remain separate interaction
work; their mention here does not mean they have been implemented. The reported
piano ruler/grid alignment issue also remains to investigate.

Capture and verification: see docs/vintage-workstation-review.md.

## Tracker display follow-up

Following the user's request for more 1990s eye candy and a screenshot-led
proportion review, numeric transport readouts now use original procedural
seven-segment digits. General captions retain the licensed embedded font and
unchanged text metrics. The master spectrum uses grouped frequency-bin peaks
and green/amber segmented cells over a dark grid; idle cells remain unlit. The
existing L/R levels receive matching segmented cells and labels. There is no
random animation or invented audio signal.

The Analog overview now includes a two-cycle shape thumbnail beside the existing
LFO selector. Analog Motion has a read-only filled 500 ms LFO plot; Custom FM's
existing editable Motion plot gains matching fill, grid and stronger contrast
without changing its plot bounds or drag formulas. These are illustrations of
the existing per-note LFO, not a live global phase scope or new modulation source.
Oscillator group borders extend four pixels lower to clear existing value labels;
knob centers and hit boxes remain in place. Knob captions/values use compact
12/11-pixel typography and tighter vertical spacing to clear the group borders.
Numeric text and display
spacing are adjusted within existing transport bounds. No pane arrangement or
control hit targets were resized during this display pass.

Separately, the user explicitly requested stable piano vertical zoom on resize.
That intentional interaction change is documented in `piano-resize-behavior.md`.
Review, captures and costs: `tracker-displays-review.md`.

## Colorful console refinement: continuous signal displays

The user rejected segmented displays because visual information should be
preserved. The master spectrum now draws all 96 existing FFT bins independently
with the original continuous height mapping. It is wider when space permits,
with a green/teal/amber frequency palette. The top-bar stereo levels and Mixer
meters use continuous fills; Mixer wells have the same dark inset treatment,
faint scale lines and clear peak markers. Existing smoothing, gain curves,
thresholds and peak-hold timing are unchanged. No CPU-saving claim is made.

The user explicitly requested square, evenly placed buttons and a single toolbar
with menus at the very left followed by Pitch/Vol. The workspace top moves from
98 to 60 logical units, returning 38 units to the editors. At normal widths the
transport buttons are 42×42, editor/navigation buttons 28×28; compact widths use
32×32 and 24×24. Below 1180 units the far-left menu group is a two-by-two block,
within the same 60-unit toolbar height. All controls remain accessible. Project
name moves into the left menu plate at normal widths and the right-hand spare
space when compact. See `colorful-console-review.md` for geometry changes.

Blue/green/amber/violet/red procedural icon tints add distinct color families;
active icons retain color and disabled states stay subdued. The PATT/SONG selector
now has recessed illuminated halves rather than flat fills painted over its
button border. Track headers have recessed source-colored number plates, trim
and an identity stripe derived from their first clip. Empty tracks use neutral
trim. Mixer channel strips have framed headers, fader rails/ticks and inset gain
readouts. Piano tools/readouts have inset grouping; taller piano windows show
actual pattern/channel details and existing editing-key reminders. Decorative
ribs add period character without adding fake controls. No new font or bitmap.

## Wider master L/R and gradient shading

The user requested a longer master L/R display, taking space from the adjacent
spectrum, and then requested gradients on patterns and windows. At 1448 logical
units the monitor pair keeps its right edge at 1380: the spectrum changes from
256 to 192 units, L/R from 40 to 104. At intermediate widths the transfer scales
down, with the spectrum outer width kept at least 100 whenever space permits.
No FFT bin grouping, amplitude segmentation or signal-processing change occurs.

Pattern/audio/automation picker cards gain vertical color shading. Arrangement
clips retain their base fill and note/waveform/automation preview geometry, with
a gradient on the existing 16-unit caption band and a fine top highlight. Window
header bands and the visible piano sidebar have restrained vertical shading;
title bars/dialog titles have horizontal shading. Existing bounds, labels and hit areas are preserved. Shading uses six
colored vertices (two triangles) per rectangle through the existing draw API,
with no new texture, font, dependency, blur, shadow or time-based animation.
The existing developer `--profile-flat` also suppresses these gradients.

Captures and measured costs are documented in `gradient-console-review.md`.


## Cropped synth reference and continuous meter color (October 16)

The user replaced `ai-screenshots/2.png` with a cropped synth reference
(SHA-256 `be55e51510d29e4cf9a55f86110f5ffa7aa98f8026e54a25d6ad567ec20dde01`).
Its compact gray slider rows, inset values and large blue-filled envelope guide
the Analog overview. All nine existing oscillator parameters remain available,
plus cutoff/resonance/envelope amount, ADSR and four per-note LFO parameters.
Velocity appears once in the oscillator section. The existing focused tabs
remain available in short windows. FM/DX7 retain their engine-specific controls.

No independent second-oscillator waveform/octave/semitone, noise-mode selector,
filter type/drive, arbitrary LFO destination or new routing was added: the crop
contains controls the current Analog engine does not implement.

The requested slider layout intentionally changes Analog overview hit targets.
Rails drag horizontally, with the original relative 160-unit sensitivity and
parameter ranges; there is no jump on press. Value boxes drag vertically.
Logarithmic pitch/cutoff and envelope-time mapping, wheel changes, right-click
value/automation access and capture/undo infrastructure are reused. Envelope
handles retain their original formulas and 14-unit hit regions in the larger
plot. Hands-on slider feel still needs user verification.

Continuous meters now interpolate emerald, lime, yellow, amber, orange and red.
The existing amber and zero-dB thresholds anchor the ramp; values above zero dB
remain red. Master and mixer share the same drawing helper, with fine face
highlights. There are no cells, grouped samples, new timers or peak changes.
Each fill uses up to five six-vertex gradient strips plus one over-zero rectangle
and two hairlines. This adds drawing work; it does not save signal-processing CPU.

Picker shading strength rises from .10 to .45; arrangement gradients cover the
full clip rather than just its title band. Preview and hit geometry stay intact.
Piano octave labels draw after all keys and fit their visible glyph ink to each
clipped white-key region. Key widths, row heights and input geometry stay intact.
Font fitting has coverage at simulated raster densities 1 and 2 and very short
clipped key heights; physical HiDPI appearance remains unverified.

See `synth-crop-review.md` for captures, measurements and verification limits.


### Envelope display follow-up

The user found the previous envelope treatment visually unchanged. The shared
Analog/Custom FM envelope renderer now uses a navy display, stronger blue area
fill, cyan contour and outlined square drag handles. Five horizontal amplitude
lines and vertical guides at the actual attack/decay handles and release start
replace the faint equal-width guides. Fixed start/release junction marks are
visual references, not new controls. Existing envelope formulas, plot bounds,
three draggable handles and 14-unit hit regions are unchanged. Legacy rendering
is retained. This applies to volume and modulation/filter envelope plots.

Actual app capture: `../baseline-artifacts/envelope-display-review/libreloop-smoke.png`.
The release executable remains 3,691,440 bytes (no size increase). There are no
new assets or dependencies. The display adds grid/guide lines, outlined handles
and a contour pass over the fill/grid. Isolated effect CPU/GPU cost has not been
measured reliably; native GPU and physical HiDPI appearance remain unverified.


### Grid and navigation controls

Requested interactive grid dropdowns replace the Piano sidebar's read-only
Auto grid value and occupy unused Arrangement toolbar space (x=136, y=24,
124×20). The Arrangement button clears the unchanged horizontal scrollbar.
Rack zoom controls occupy the existing footer reserve at x=106..196. These are
explicitly requested new hit targets; no grid/ruler/keyboard dimensions change.
Tempo keeps a dark face on hover with a cyan border. Inactive seven-segment
strokes are omitted. See `interaction-refinements.md` for behavior, validation
and limitations.

### Six-op envelope renderer omission corrected

The user confirmed launching with `--visual-preview`; this flag correctly
selects the studio renderer. The earlier envelope work covered Analog and
Custom FM, but missed `dx7_envelope_graph`, which the default Six-op FM uses.
That separate renderer now has the same navy well, blue area fill, horizontal
amplitude grid, vertical stage guides, cyan contour and outlined square handles.
All six operator envelopes and the global pitch envelope use it. These remain
DX7 rate/level stages; they are not relabeled or converted to ADSR.

Original point coordinates, rate/level formulas and 14-unit hit regions remain
unchanged. Legacy drawing remains unchanged. The actual frontend regression test
now checks all four handles in all six operators and the pitch envelope, checks
that capture does not mutate settings, compares drag results between legacy and
studio, and asserts the blue fill and grid/guide draw calls occur.

Actual software captures are in
`../baseline-artifacts/six-op-envelope-review/{operator,pitch}/libreloop-smoke.png`.
The release executable grew from 3,691,968 to 3,708,384 bytes (+16,416, including
binary layout/alignment). No assets, font or dependency were added. Per-frame
CPU/GPU cost of this isolated effect has not been measured reliably. Native
HiDPI and hands-on input remain unverified. The release build and bundle
signature verify; the full 125-test regression suite passes.

### Approved docked workspace

The user approved persistent Arrangement/Mixer/Piano/Rack regions and a shared
instrument/effect inspector, plus replacing mixer slot-number buttons with named
rows. Startup proportions are retained; resizing now operates through dividers.
This is an intentional interaction-layout change. Empty slots open the existing
effect picker; filled slots open the right editor. Classic bevels, readable labels
and small divider grips fit the current visual direction without new assets.
See `docked-workspace.md` for controls, state persistence, limits and verification.

### Shared Rack/Piano region and compact effect stack

The user superseded the previous layout with one editor region that switches
between Rack and Piano Roll, an optional right-side split, compact always-on-top
effect popups and no Arrangement maximize/Mixer minimize chrome. Titles identify
effect instances and the shared region has an explicit Back to Rack action.
These are approved interaction/layout changes. Multiple named effect slots can
open multiple existing Chorus/EQ editors; there are no new synthesis/effect
features to match concept art. See `flexible-workspace.md` for exact behavior,
validation, analyzer limitations and costs.

## Approved classic desktop refinement (2026-10-16)

The user requested convincing Windows/X11 vintage control styling and uniform
buttons. This pass uses neutral system-gray faces, proper directional two-edge
bevels, neutral hover, pressed content offsets and original multicolour indexed
pixel sprites. Geometry and interaction handling are retained. No Microsoft or
unverified icon-theme assets were bundled. Implementation, actual-app capture,
measurements and pending feature discussion are recorded in
[vintage-desktop-pass.md](vintage-desktop-pass.md).

## Approved minimal Arrangement / Pattern workspace (2026-10-17)

The user approved removing the default step sequencer and third split. The two
main pages now alternate above the docked Mixer. Pattern combines a compact
channel list with Piano Roll, explicit C4 hit entry, reference ghost notes and a
toolbar above the canvas. Instrument/effect editors float and have close only.
The new layout, geometry changes, defaults and preserved view-state behavior are
specified in [pattern-workspace.md](pattern-workspace.md).

## Recording count-in control (2026-10-17)

A visible `IN <steps>` toggle sits below the metronome, sharing its previous
rectangle in studio mode. The transport readout shows remaining sixteenth steps
while counting in. MIDI / Recording also exposes the toggle and a numeric steps
button. No decorative bitmap/effect was added. See
[keyboard-recording.md](keyboard-recording.md) for behavior and timing limits.

## 90s.png shared skin pass (2026-10-17)

The new explicit reference is `../../90s.png` (SHA-256
`1e91e630fe631edaf70389d750783384bb23f8da46e004e2500864e9872eef16`).
Its steel-gray materials, cobalt captions, cyan graph paper, metal knobs,
shaded push buttons and vivid Mixer identities now drive shared draw primitives.
Fine ordered grain and dotted envelope backgrounds are generated once in a
small texture; curves, meters and individual FFT bins remain continuous.
See [90s-reference-skin.md](90s-reference-skin.md) for the implementation,
measurements, renderer checks and explicit remaining fidelity work.

This checkpoint changes rendering and small-caption sizing. It retains current
hit rectangles, drag/knob formulas, Arrangement/Pattern page switching, floating
editors and saved clip colours. It does not certify exact image parity: reference
title sizes, grouping, dropdown arrow cells and Synth proportions still differ.
The simultaneous Arrangement/Piano layout in the AI image conflicts with the
later approved page workflow and has not been reinstated. No new functionality
is inferred from its ambiguous icons or extra decorations.

## Caption and control proportions (2026-10-17)

The user explicitly retained Arrange/Pattern switching and identified the stacked
Arrangement/Piano layout in `90s.png` as an artifact of the old input screenshot.
That conflict is resolved; keep the modern workspace and floating editors.

The next skin checkpoint adds shared raised dropdown arrow cells, 32-unit editor
captions with real 24px font atlases, wider default floating instruments, a
brighter shaded Record lamp, shaped piano keys, and consistent dark display
wells. The caption changes translate editor content by 14 logical units; drawing,
hit testing, drops, context anchors, envelope drags and navigation use the same
content frame. Existing replay state assertions are unchanged, with only the
new geometry supplied separately. See [90s-caption-controls.md](90s-caption-controls.md)
for geometry changes, tests, captures, limits and remaining work.

## Six-op control and plot proportions (2026-10-17)

The six-op editor now uses the reference's row proportions at its 376×654
panel size: larger metal knobs and labels above a 152-unit dotted envelope,
with evenly spaced 34-unit footer buttons. A concrete pure layout distributes
extra height among rows and plot; compact and legacy windows retain their carried
positions. Shared caption fitting prevents long global-control labels from
running into neighbouring controls. No parameter, DSP, envelope or automation
formula changes. See [90s-synth-proportions.md](90s-synth-proportions.md) for exact
coordinates, the geometry-only assertions, real captures, measurements and limits.

Shared dialog fields, chooser rows and folder glyphs are documented in [90s-dialog-controls.md](90s-dialog-controls.md). The latest direction requests smaller gray captions and dotted Arrangement/velocity surfaces, superseding the reference blue title bars.

Latest direction: compact gray 24-unit captions and one-pixel borders supersede the blue/32-unit art styling. Arrangement, note and velocity backgrounds now share dotted paper; see [90s-dotted-gray.md](90s-dotted-gray.md).

Icon action controls and the filled-width Mixer are recorded in [90s-icon-actions.md](90s-icon-actions.md). Labels remain for data and parameters; actions retain hover descriptions.

Direct comparison corrections, faint dots, shadows, compact synth body and simpler track rows: [90s-reference-comparison.md](90s-reference-comparison.md).

The later user request merges top-level view/playback selection into PATT/SONG and removes duplicate Arrange/Pattern buttons; [patt-song-pages.md](patt-song-pages.md) supersedes earlier toolbar notes.

The later 90s1/90s2 direction uses lighter blue-gray dithered materials, pixel-shaded cached knobs and native-density text sampled without extra smoothing. It preserves the current layout and gray captions. Details, memory and controlled software timing: [90s-bitmap-materials.md](90s-bitmap-materials.md).

2026-10-17: the user authorized a free bitmap-style replacement font and uniform
text size. Studio now uses 16-unit DotGothic16, with adjusted instrument/Mixer
proportions and calibrated, continuous displays sharing the meter palette.
See `uniform-bitmap-controls.md` and `third-party.md`. Sampler minimum height
is now 420 to protect its waveform when resizing.

2026-10-17 follow-up: approved resizable piano velocity lane, clickable seconds
readout, text alongside action icons, square sampler/record-arm keys, and
simpler muted-green spectrum traces replace the previous rainbow styling.
Top frequency/dB labels were removed at the user's request. See
`velocity-lane-and-controls.md`; PxPlus IBM VGA font import remains pending
because its actual asset is unavailable in this environment.

2026-10-17 font follow-up: network permission was enabled, the official archive
was downloaded and verified, and PxPlus IBM VGA 8x16 now replaces DotGothic16.
The exact unmodified font, full CC BY-SA 4.0 license and publisher readme are
vendored/packaged. The common text size stays 16 logical units, matching its
native 8x16 pixel design. This supersedes all earlier pending-font notes.

Dropdowns retain their original action names with colored icons first. The
File menu is widened to 272 logical units so Save, Save As..., and Collect
samples and save... are readable at the common font size; Edit and Help also
have labeled rows rather than the former 44-unit icon-only strips. No action
mapping, keyboard navigation or project save semantics changed. Actual font
fit tests cover the longest File entry and the native 8-unit advance at all
five tested raster densities. Software captures are in
`../baseline-artifacts/vga-font/` (including `file-menu/`).

The top-bar spectrum is now 120 units wide beside the 104-unit L/R meter
(116 units at the narrowest full toolbar). It no longer expands across spare
toolbar width. The geometry fixture records the explicitly requested new width;
all signal-value assertions remain unchanged. The latest font/menu build passed
all 140 tests and strict app signature verification; native HiDPI appearance
still needs a hands-on check.

Shared-grip follow-up: Browser, Mixer and collapsible velocity lane now use
one five-dot horizontal/vertical grip renderer. Notes and velocity stems gain
progressive velocity brightness with a distinct selection hue. See
`velocity-lane-and-controls.md` for collapse gestures, reuse policy and tests.

Divider color correction: the shared grip renderer now owns the whole rail
fill as well as its dots. Browser, Mixer and velocity rails use control gray
(-8 RGB idle, +8 hovered/dragged) and the same border-colored five dots; Browser
caption colors cannot alter them. Independent cyan Browser fill and velocity
edge decoration were removed. Render tests verify both orientations and both
states at five DPI scales, including deliberately different caption colors.
Release build, four relevant theme/workspace/velocity tests and strict app
signature verification passed. Capture: `../baseline-artifacts/shared-grips/uniform/`.
