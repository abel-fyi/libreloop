# Architecture and scope

LibreLoop is a small, self-contained Linux and macOS DAW in C. It borrows familiar FL
Studio placement and workflows, with a simpler raylib interface. Priorities
are a coherent mouse-driven workflow and minimal code and dependencies.

## Repository map

| Path | Responsibility |
| --- | --- |
| `src/main.c` | raylib UI, gestures, transport, sampler worker orchestration |
| `src/theme.c`, `theme.h` | light/dark palettes, flat rectangle drawing, appearance preference |
| `src/engine.c`, `engine.h` | project model, note scheduling, rendering and routing |
| `src/project_io.c`, `atomic_file.c` | backward-compatible project serialization and atomic project/WAV replacement |
| `src/project_assets.c`, `project_document.c` | relative references, collected audio, missing samples and unsaved document state |
| `src/recording.c` | take preparation, writer lifecycle and live waveform updates |
| `src/recording_writer.c`, `sample_storage.c` | background WAV writing and shared read-only mapped PCM |
| `src/text_fonts.c` | physical-pixel font atlases, dynamic UTF-8 glyphs and text alignment |
| `src/audio_timing.c` | pitch-curve speed integration, Audio clip duration and seek positions |
| `src/parameter.c`, `parameter.h` | stable parameter IDs, control metadata and project bindings shared by UI and automation |
| `src/automation.c` | normalized curves, source editing and validation |
| `src/arrangement.c`, `arrangement.h` | Playlist editing, selection, timeline and snap helpers |
| `src/sample.h` | PCM view and owned/shared sample lifetime contract |
| `src/sampler.c`, `sampler.h` | independent sampler settings and offline crop, normalize, reverse, polarity, pitch and time processing |
| `src/dx7.c`, `dx7.h`, `dx7_tables.h` | bounded C adaptation of the MSFA Modern six-operator engine |
| `src/analog.c`, `analog.h` | detuned oscillators, PWM, resonant filter and DC removal |
| `src/fm_synth.c`, `fm_synth.h` | three-oscillator FM settings, independent envelopes, tuning and per-note LFO |
| `src/preset.c`, `preset.h` | validated, atomic built-in device presets and relative sample references |
| `src/chorus.c`, `chorus.h` | stereo modulated-delay DSP and saved rate/depth settings |
| `src/equalizer.c`, `equalizer.h` | seven-band stereo biquads, control smoothing and response evaluation |
| `src/spectrum.c`, `spectrum.h` | UI-owned stereo FFT, logarithmic bins and level smoothing |
| `src/effects.c`, `effects.h` | instance-owned mixer slot state and ordered wet/dry processing |
| `src/sampler_voice.c` | per-note sampler DSP, interpolation and realtime tempo stretching |
| `src/audio.c`, `audio.h` | miniaudio playback, live notes and sample decoding |
| `src/browser.c`, `browser.h` | folder trees, selection and saved roots |
| `src/windows.c`, `windows.h` | floating editor rectangles, stacking and mouse ownership |
| `tests/` | headless C tests |
| `samples/` | generated demo one-shots |
| `tools/` | sample generator |
| `docs/` | usage, development notes and third-party notices |

## Built-in device baseline

The sampler is a built-in instrument, with no external plugin ABI or loader.
`sampler.h` depends only on `sample.h`. Its DSP can be used without a `Project`,
raylib, miniaudio or window. `Sampler` holds saved settings; `SamplerVoice` holds
one note's transient source position, note speed and tempo-stretch grain history.
A sequencer `Voice` wraps that DSP state with channel, lane, gain and note lifetime.
Both live audition and arrangement playback use the same sampler voice processor.

`sampler_voice_reset` resets one instance, and `sampler_voice_sample` advances it
without allocating or reading files. Processed PCM is a borrowed input during
playback. Source ownership and reference counting live in `sample_storage.c`;
`sample_process` is explicitly offline work performed on the UI's worker thread.
The engine owns note timing, channel gain/pan, routing and activity reporting.
Sampler views and worker orchestration stay in `main.c`.

`parameter.h` describes existing controls with stable IDs, names, ranges, defaults
and continuous/integer/toggle kinds. Project bindings in `parameter.c` resolve
those IDs to existing fields. UI drag capture and automation use the same ranges.
Existing parameter IDs and sampler fields are preserved; DSP voice state is never saved.
Offline sampler edits are not made realtime automation parameters by this change.

Add a future synth or effect as its own `.c`/`.h` module with instance-owned state,
explicit preparation/reset/cleanup and allocation-free processing. Instruments
receive timed notes; effects process audio. Keep the processing API independent of
project/UI types, prepare buffers outside the callback, and add parameter/state
bindings when that device exists. The chorus establishes the effect contract below; there is no external plugin loader.

## Decisions

- Dark and Light share the same control geometry and behavior. The View
  menu selects the palette and saves it in the user's LibreLoop configuration.
  Surfaces use plain rectangles, without gradients or shadows. A shared small
  circle mask and a monochrome icon atlas smooth controls; the atlas is drawn
  at four times its display size, reduced with exact area coverage, and aligned
  to display pixels. It rebuilds when the window scale changes. Knob value arcs
  are cached alpha masks; hover animation returns to event waiting once settled.
  Windows are opaque by default. View also toggles alpha-blended window surfaces without dimming text or controls;
  the editing grids use translucent row tints. There are no decorative backgrounds or blur passes.

- Instruments and effects ship with LibreLoop. The Sampler is the first
  instrument; future instruments belong in the Rack and effects in Mixer slots.
  Add a small internal interface when useful. External VST, CLAP and LV2 hosting
  and a public custom plugin format are outside the planned scope.
- Playlist and Piano Roll share adaptive timeline rendering: power-of-two subdivisions,
  ruler labels at 1/2/4/8/etc. bar intervals, and alternating four-bar shading.
  Lines disappear at distant zooms, followed by the shading; editing snap is independent.
- Playlist and Piano Roll share navigation mapping: wheel scrolls vertically,
  Shift+wheel horizontally, platform modifier+wheel zooms at the pointer, and
  middle-button drag pans both axes. macOS adds precise two-finger scrolling
  and native pinch events through a small AppKit bridge.
- Editor windows are virtual windows within raylib, sharing one small window
  manager. Browser resizing and editor scrolling use the same mouse-first approach.
- Idle drawing waits for events; playback and gestures run at 60 FPS. Workers
  wake the UI when processing completes. Playlist copies reuse cached pattern
  previews; extreme zoom retains vector detail. See [performance checks](performance.md).
- Patterns contain all channels' notes. Playlist clips reference patterns, Audio channels or automation sources and
  keep independent crop lengths; resizing a clip never deletes source notes.
- Dropping audio on the Playlist creates a waveform clip and an Audio Rack channel.
  Playlist waveforms cache a hierarchy of min/max peaks over 256-frame blocks.
  Arrangement, source-list, Browser and Sampler views share a filled envelope
  renderer with antialiased edges. Display peaks use source-aligned power-of-two
  bins, interpolating neighboring bins and zoom levels to avoid shimmer. Coarse
  views read cached tree nodes directly; exact range queries remain available
  for analysis. Zooming never rebuilds the cache. Sample changes invalidate it.
  Live sampler previews restrict queries to the trimmed region before applying
  reverse, polarity and normalization.
  Rack drops create Unsorted samplers; the title filter shows All, Audio or Unsorted.
  Both groups can be sequenced. Audio clip crop lengths are stored in seconds,
  so changing tempo changes their grid footprint without changing sample speed.
  Full-length clips follow sampler duration changes; manually cropped copies keep
  their cap. Track header activity strips flash on starts and dim while voices sound.
  Waveform envelopes are cached per channel; duration colors blend maroon to green.
- Sampler headers share channel enable, pan, volume, pitch/range and mixer routing
  with the Rack. Channel pitch changes playback speed over a saved 1–48 semitone
  range; sampler processing Pitch preserves duration. Regular knobs share one size;
  the title-bar Swing knob is smaller. Both reuse the same cached arc atlas.
- Samples store mono or interleaved stereo PCM. Decoding, playback, previews and
  export retain stereo. Sampler stretch uses one grain alignment for both channels;
  normalization uses a shared peak and trimming checks both channels.
  Sampler and Playlist live envelope previews share cached source bounds and peaks;
  crop edits preview full-length clip duration without publishing temporary PCM to audio. Waveform
  envelopes include both channels without summing them.
- Original sample audio is retained. Processing produces a separate sample,
  using a background worker after knob release. Start/Length and the two-sided Trim threshold preview the crop
  live; Pitch/Time retain the processed waveform until replacement audio is ready.
- The audio callback allocates nothing and owns its rendering state. UI edits
  enter a bounded command queue and a pending project snapshot. The callback
  tries the mailbox mutex without waiting; if busy, it continues rendering its
  current state. UI animations read a small published snapshot. Sample replacement
  and Stop wait on the UI thread for acknowledgment before old PCM can be freed.
  Song Audio clip edits and sample replacements reconcile active Audio voices
  at the current transport frame; unrelated pattern and live voices keep playing.
- Mixer buses retain float headroom and unity gain is transparent. Preview,
  metronome and listening gain mix before the final device clamp to -1..1.
  PCM16 export clamps at conversion. Overloaded mixes still need gain reduction;
  there is no automatic compressor or lookahead limiter.
- Project files currently use `.hbt` and the `HOMEBEAT` version-39 header for
  compatibility. Versions 1–38 remain readable. Renaming the app did not change
  the project format. Sample references are relative to the project directory, with old absolute paths
  still readable. Collect samples and save writes original PCM into a unique companion
  directory, and subsequent saves retain those references. Missing audio opens
  with empty playback while preserving its path, clip duration and editing state; the UI
  marks missing channels and offers relinking.
- Browser roots are written under the `libreloop` configuration directory;
  legacy `homebeat` roots are read when no new configuration exists.

## Deliberate limits

Up to 32 channels, eight patterns, 100 fixed Playlist tracks, 100 fixed mixer
inserts and 128 note slots per channel per pattern, with up to 64 clips per
Playlist track. Timeline and pattern lengths have no fixed bar count; zoom and
horizontal navigation reveal additional empty bars. Drawing farther right
extends the source pattern; existing Playlist copies keep their own crop lengths.
Unused Rack steps stay grey until painted.
Note starts and lengths use the selected snap grid; notes stay inside their
pattern. The Piano Roll defaults to 25 visible pitches; its corner zoom control adjusts
the view from 8 to all 128 MIDI pitches. Keyboard rendering, hit testing, note
editing and scrolling share that row scale. Drum steps are one-shots; Piano Roll notes have duration gates with a
short fade at note-off. Sampler processing can change pitch and duration; held notes stop at the
processed sample's end. Mixer inserts can route to one other insert or Master, with built-in Chorus and Equalizer slots; no
MIDI device I/O, external plugins, or FLP import. WAV export ends at
the arrangement boundary without an added tail; standard RIFF exports must
fit below 4 GiB. This is a workflow prototype,
not a production recording tool. Device changes, sleep/wake and sustained
interactive playback still need platform testing.

## UI layout

The interface places the Browser on the left, with a compact top toolbar,
Playlist, floating Channel Rack and Mixer, and a Piano Roll with keys on the
left and velocity controls below.

## Automation

Automation clips use a third Playlist source range and step-based length/offsets.
Each source stores a parameter ID, owner index, slot, name, palette color and
normalized points in nondecreasing time order. Neighboring points may share
a time for a vertical jump; evaluation uses the last point at that time. Dragging
preserves point identity and clamps at neighbors. Channel deletion remaps targets and removes curves
for the deleted owner. Parameter IDs are saved explicitly; UI addresses are
only used to discover a target when opening a control menu. Unresolved future
plugin IDs are preserved without attempting to process them. `automation_evaluate`
is the shared Song-position lookup for future built-in processors; parameter
resolution and realtime application will be added with each actual processor.

Automation line segments reuse the Mixer cable alpha texture for smooth edges
in Playlist clips, picker thumbnails and floating drag previews.

The renderer binds supported targets once per buffer, collects enabled automation
clips, evaluates their curves per sample, and recomputes only affected channel/bus
controls. This shares the playback/export path, with no callback allocation or
project copying. Automation does not rewrite saved manual values. Sample rebuilding
operations remain separate from this realtime parameter path.

Full-length Audio clip duration integrates channel pitch/range and Master pitch
automation over Song time. Linear semitone segments yield exponential playback
speed, integrated and inverted analytically, with integer pitch-range boundaries
split explicitly. Playback seeks and waveform pixel ranges use this same source
position mapping. Explicit crop caps remain timeline boundaries. The renderer
computes clip durations once per buffer rather than once per sequencer tick; full
Audio voices finish at the sample end instead of an obsolete duration gate.

## Recording

Mixer inputs use miniaudio capture devices at the engine sample rate. Armed
tracks that share an input device share one capture stream. Preallocated stereo
SPSC rings pass capture audio to the playback callback and each armed bus's
post-fader tap to a background writer. The callback does no allocation, disk I/O or
blocking synchronization. Inputs follow normal mixer routing and level controls;
unused armed buses are included in the routing graph. Recording suppresses Song
looping, so empty arrangements and long takes keep a continuous cursor.

A worker per take drains its SPSC ring into a uniquely named float WAV with bounded
scratch buffers. The UI maps committed frames read-only and updates only new waveform
peak blocks. Finished playback, sampler identity processing and undo share mapped PCM;
reopening a canonical 48 kHz float-stereo WAV also maps it. Sampler transformations still
create heap-backed output. Recording previews remain separate from playback until
finalization, using the acknowledged replacement path.

Each writer periodically updates its header and finalizes the successfully committed
frames on Stop or error. Device, buffer, mapping and disk errors are reported. Takes
are bounded by `SAMPLE_MAX_FRAMES` (about 93 minutes at 48 kHz) and standard RIFF size.
Capture device opening/closing remains on the UI thread; hardware latency compensation
and channel-pair selection remain future work.

Undo history captures completed editing gestures as project snapshots. Original
sample PCM is retained in reference-counted buffers shared between snapshots;
processed PCM is rebuilt only for changed samples/settings when restoring.
Derived audio durations do not create undo entries. The UI restores buffers
through the acknowledged audio mailbox before freeing retired PCM. History is
bounded to 64 states and 256 MiB, retaining at least the current state. New/Open
clear history; saved project and recording files are not deleted by undo.

Save/Open/Export share a modal in-app chooser. Its filesystem model enumerates
regular files and folders with portable POSIX C APIs, filters by extension, and
validates the canonical destination before returning it to the UI. Existing
Save/Export destinations require an explicit Replace action; cancellation does
not call the save/load/export routines. Project and WAV writes remain in the
existing engine code, separate from chooser navigation.

Tempo-fitted samplers retain PCM and clip offsets in reference seconds. BPM edits
change a voice’s playback rate with a 20 ms slew, without sample rebuilding.
Stretch uses overlapping, stereo-aligned grains per voice to preserve pitch.
The audio thread owns this state; its playback path allocates no memory.

Window close uses the same Save/Discard/Cancel decision as project replacement.
Failed saves keep the pending action available, and canceled choosers cancel it.
Project and WAV replacement uses unique sibling temporary files, checks write/flush/close
errors, syncs file data, and renames only after success.

Text uses font atlases rasterized at the actual framebuffer density on both platforms.
Encountered UTF-8 codepoints extend each size's atlas, within the bundled font's glyph
coverage. Rebuilds flush queued drawing before retiring textures. Arrangement labels
align to physical pixels while clip geometry retains fractional movement.

## Chorus and mixer effect runtime

Each Master/insert has ten ordered built-in effect slots. The initial device is
Chorus: a vintage-inspired stereo modulated delay, .05–5 Hz rate, 0–8 ms depth,
and wet/dry mix. Opposed triangle modulation sweeps around 3.5 ms; at depths
above 1.85 ms the center moves outward to keep the minimum delay at 1.65 ms.
One-pole 7 kHz filters before and after the delay soften the wet path. Rate, depth and wet amount slew over 20 ms;
bypass/removal fades toward dry while the delay continues running. There is no
feedback or hidden output gain, and dry-path compensation latency is zero.

`Chorus` owns a fixed stereo delay buffer and transient LFO/smoothing state.
`EffectRack` prepares bounded storage outside playback (~8 MiB for all 101 buses
and ten slots), so adding slots during playback needs no allocation. Unused slots
perform no DSP. A generation reset clears individual used slots lazily. The
playback callback owns a rack shared by song/live rendering; it is borrowed by
`Player` and freed after stopping the audio device. Export owns a separate rack.
Standalone engine clients attach a prepared rack to `Player.effects` after reset
when effects are needed. A null rack retains the dry rendering API.

The engine runs each bus's chain before its fader/pan/width and routes its output
onward. Record taps and meters therefore include effects. Rate, depth and mix
use stable `(parameter, bus, slot)` automation targets (1025–1027), sampled at the
same song position as other controls. Version 33 stores slot types and chorus
settings, while earlier versions open with empty slots. Existing mix/bypass fields
are retained. Undo includes settings; delay buffers and LFO state are runtime only.

## FM instrument and device presets

`FMSettings` stores harmonic ratio, modulation depth and ADSR settings. `FMVoice`
owns oscillator phases, envelope and smoothing state; its processing API depends
on neither Project nor UI. The sequencer wraps sampler/FM state in a tagged union,
keeps note gates, and mixes either instrument through the same channel routing.
FM uses three sine oscillators with four substeps per output sample. Modulation is
limited at high pitches to reduce aliasing; this is not an alias-free oscillator.
Ratio, depth, sustain and pitch changes slew over 20 ms. Note-off starts a release
from the current envelope level. Rack steps gate for one step; Piano Roll notes
use their entered duration. Parameters 1101–1128 bind the FM controls to existing automation. Version 34
introduced instrument types and FM settings; version 38 adds layered modulation. Older
projects retain sampler channels. Oscillator/envelope state is never serialized.

Sampler, FM, Chorus and Equalizer share versioned `.llpreset` files. Presets store device
settings, excluding channel routing, channel gain and runtime state. Sampler
presets reference original audio relative to the preset directory; they do not
embed PCM. Generated sources without a file reference produce settings-only
presets. Loading validates the file and device type first, and prepares sampler
PCM before replacing current settings. Missing audio or invalid presets leave the
device unchanged. Writes use the same atomic replacement helper as projects.

## Equalizer and spectrum

Equalizer slots use seven fixed stereo biquads with selectable bell, shelf, cut
or Off shapes, with
frequency, ±18 dB gain and Q per band. Controls slew over 20 ms; coefficients
update every 64 samples, while wet/bypass changes slew per sample. Shape changes crossfade between two
filter states over 20 ms; Off bands skip filtering. Low/high cuts use a fixed
12 dB/octave slope. Slot state
shares storage with Chorus through a tagged union. The same mixer chain serves
playback, recording and export. IDs 1201–1221 bind frequency/gain/Q automation. Version 36 and preset version 2
store seven bands and their shapes. Version-35 projects and version-1 EQ presets
retain their four original bands and stable automation IDs, with extra bands Off.
Presets retain settings and mix only.

Two bounded SPSC rings pass listening output and one selected post-fader bus to
the UI. A full analyzer ring drops visual samples without blocking playback.
The UI runs 4096-point Hann-windowed stereo FFTs with 1024-frame hops and groups
power into 96 logarithmic display bins. Left/right power is averaged rather than
summing audio, preserving opposite-phase content. Spectrum and meters share the
existing animation/event-wait lifecycle. No FFT, allocation or file I/O runs in
the audio callback. Monitor-only mixer taps preserve transport loops; recording
taps retain continuous recording behavior.

FM modulation has a separate exponentially decaying tone envelope and velocity
response, plus a per-note sine LFO for vibrato and tremolo. These states belong
to each voice and require no callback allocation. IDs 1107–1112 bind the new
controls; version 37 appends their settings after the existing FM fields and
before EQ data. Preset version 3 stores the same controls. Earlier files retain
neutral defaults: full tone sustain, no velocity brightness response, vibrato
or tremolo. The Electric Piano factory patch is bundled and seeded into the
user's device preset folder without overwriting an existing file.

## Layered FM and format compatibility

Version 38 appends FM carrier tuning, independent Body tuning/envelope controls,
a second Attack modulator with its own tuning/envelope, parallel/stacked routing,
and LFO waveform/fade-in. Preset version 4 saves these same controls. Older files
initialize the added fields neutrally through `fm_legacy`; their Attack amount is
zero, and original parameter IDs and normalized automation ranges are retained.
Custom FM uses `fm_epiano`, with a short high-ratio strike and a slower Body tone.

Parameter IDs 1101–1128 resolve through `fm_parameter_pointer`, shared by UI lookup
and renderer binding. Sequenced notes initialize from the current automated FM
settings so an automated Attack amount is effective on the first note sample.
Oscillator tuning, depth and LFO amounts slew; note-off releases each modulation
envelope from its current level. A shared sideband budget reduces high-note aliasing;
this remains an approximation rather than an alias-free oscillator.

The FM editor separates Sound, Body, Attack and Motion. Graph gestures hold mouse
capture until release and become a single undo entry. Envelope time axes use a log
mapping per stage to make millisecond attacks editable; Motion graph gestures edit
speed and the selected pitch/volume amount. No audio buffers or effect slots are
created by opening the editor or applying the factory FM preset.

## Multi-engine synthesis

`FMSettings` keeps earlier fields and appends engine selection, analog settings
and 145 native DX7 voice parameters plus six continuous key-tracking values. Engine 0 preserves Custom FM, engine 1 uses
six-operator MSFA Modern synthesis, and engine 2 uses analog-style synthesis.
Version 39 appended native settings before EQ data. Version 40 and preset
version 6 append six tracking percentages to each native voice block; older
files derive 100% for ratio operators and 0% for fixed-Hz operators. Older files before version 39 initialize the engine extension neutrally and keep
engine 0.
New channels use the original E.PIANO 1 voice. Nine factory files are bundled and
seeded into the device preset directory without overwriting user files.

DX7 state has six four-stage logarithmic envelopes, keyboard/rate/velocity scaling,
ratio/fixed oscillator tuning, pitch EG, feedback and 32 algorithm routing tables.
It renders 64-frame blocks with gain interpolation. Lookup tables initialize once
outside playback and then stay immutable. Callback processing uses fixed voice
buffers, with no allocation, file I/O or C++ runtime. Factory reference fixtures
come from independent Dexed/MSFA renders across pitch, velocity and release.
A uniform headroom trim follows the reference core; reference PCM fixtures compare
before that trim. Native operator velocity replaces the mixer's generic per-note velocity scaling;
channel and bus gain/pan still use the ordinary path. Retired slots preserve
operator phases when oscillator sync is disabled and the channel remains the same.

Analog uses PolyBLEP saw/pulse oscillators, a second detuned oscillator, sub/noise,
PWM and a four-pole TPT filter with bounded nonlinear feedback. Per-voice settings
slew over 20 ms; stereo chorus uses the existing fixed-buffer processor. The
runtime union is bounded by its largest engine. UI voice publication now copies
small channel/gain/source-position views instead of complete oscillator/delay state.

Native parameter IDs 2001–2145 bind directly to voice settings, with integer
quantization for automation, including held curve values. The native pointer lookup
uses the field offset rather than scanning every parameter. Engine selection is
structural and does not resolve as an automation target. Editor graph coordinates
are explicitly local on the first click and transformed once on subsequent frames.

DX key tracking uses IDs 2146–2151 (Op 6 through Op 1), with continuous 0–100%
values. Frequencies interpolate in logarithmic pitch around MIDI 60 (middle C).
Ratio tuning interpolates native note pitch toward its middle-C pitch; Hz tuning
adds the scaled keyboard interval to its anchor frequency. At 100% ratio and 0%
Hz, original DX tuning is preserved exactly. Tracking updates at the existing
64-frame DSP boundary without resetting oscillator phase or allocating memory.

## Native MIDI input and recording

`midi_input.c` adapts CoreMIDI on macOS and the ALSA sequencer on Linux to a
bounded SPSC event queue. Native callbacks/ALSA polling only enqueue MIDI events
and wake GLFW; they never edit projects or touch DSP. A dedicated Linux thread
polls input and port announcements. Port discovery uses a separate ALSA client.
Queue overflow or disconnect releases held notes rather than leaving stuck voices.
`midi.c` contains MIDI 1 running-status parsing, controller bindings and the
UI-owned take builder, tested without hardware or platform dependencies.

The UI drains events, routes notes into reserved live slots 64–95 with actual
velocity, handles sustain and applies controller values through `parameter_write`.
The callback retains sole ownership of voices. Recording uses monotonic input
receipt timestamps with the take's fixed BPM; it is not sample-accurate MIDI
scheduling. Captured pattern/automation lanes are muted until recording finishes.
The audio transport's MIDI-recording flag disables song wrap without starting
capture devices. Song-mode pattern selection does not restart the sequencer.

Project version 41 stores up to 32 channel/CC parameter bindings before EQ data;
older projects initialize them empty. Machine-specific input and recording choices
live beside the browser settings in `midi.txt`. Take creation and controller curve
compaction occur on the UI thread, with no allocations in the audio callback.
