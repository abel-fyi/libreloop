# Architecture and scope

LibreLoop is a small, self-contained Linux and macOS DAW in C. It borrows familiar FL
Studio placement and workflows, with a simpler raylib interface. Priorities
are a coherent mouse-driven workflow and minimal code and dependencies.

## Repository map

| Path | Responsibility |
| --- | --- |
| `src/main.c` | raylib UI, gestures, transport, sampler worker orchestration |
| `src/theme.c`, `theme.h` | light/dark palettes, flat rectangle drawing, appearance preference |
| `src/engine.c`, `engine.h` | project model, synthesis, rendering and routing |
| `src/project_io.c`, `atomic_file.c` | backward-compatible project serialization and atomic project/WAV replacement |
| `src/project_assets.c`, `project_document.c` | relative references, collected audio, missing samples and unsaved document state |
| `src/recording.c` | take preparation, writer lifecycle and live waveform updates |
| `src/recording_writer.c`, `sample_storage.c` | background WAV writing and shared read-only mapped PCM |
| `src/text_fonts.c` | physical-pixel font atlases, dynamic UTF-8 glyphs and text alignment |
| `src/audio_timing.c` | pitch-curve speed integration, Audio clip duration and seek positions |
| `src/automation.c` | stable parameter targets, normalized curves, source editing and validation |
| `src/arrangement.c`, `arrangement.h` | Playlist editing, selection, timeline and snap helpers |
| `src/sampler.c` | non-destructive crop, normalize, reverse, polarity, pitch and time processing |
| `src/audio.c`, `audio.h` | miniaudio playback, live notes and sample decoding |
| `src/browser.c`, `browser.h` | folder trees, selection and saved roots |
| `src/windows.c`, `windows.h` | floating editor rectangles, stacking and mouse ownership |
| `tests/` | headless C tests |
| `samples/` | generated demo one-shots |
| `tools/` | sample generator |
| `docs/` | usage, development notes and third-party notices |

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
- Project files currently use `.hbt` and the `HOMEBEAT` version-32 header for
  compatibility. Versions 1–31 remain readable. Renaming the app did not change
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
pattern. The Piano Roll shows 25 pitches at a time and scrolls through MIDI pitches 0–127. Drum steps are one-shots; Piano Roll notes have duration gates with a
short fade at note-off. Sampler processing can change pitch and duration; held notes stop at the
processed sample's end. Mixer inserts can route to one other insert or Master; no effects,
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
