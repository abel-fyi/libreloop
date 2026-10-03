# Architecture and scope

LibreLoop is a small, self-contained Linux and macOS DAW in C. It borrows familiar FL
Studio placement and workflows, with a simpler raylib interface. Priorities
are a coherent mouse-driven workflow and minimal code and dependencies.

## Repository map

| Path | Responsibility |
| --- | --- |
| `src/main.c` | raylib UI, gestures, transport, sampler worker orchestration |
| `src/theme.c`, `theme.h` | light/dark palettes, flat rectangle drawing, appearance preference |
| `src/engine.c`, `engine.h` | project model, synthesis, rendering, routing, save/load, export |
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
  View also toggles alpha-blended window surfaces without dimming text or controls;
  the editing grids use translucent row tints. There are no decorative backgrounds or blur passes.

- Instruments and effects ship with LibreLoop. The Sampler is the first
  instrument; future instruments belong in the Rack and effects in Mixer slots.
  Add a small internal interface when useful. External VST, CLAP and LV2 hosting
  and a public custom plugin format are outside the planned scope.
- Playlist and Piano Roll share navigation mapping: wheel scrolls vertically,
  Shift+wheel horizontally, platform modifier+wheel zooms at the pointer, and
  middle-button drag pans both axes. macOS adds precise two-finger scrolling
  and native pinch events through a small AppKit bridge.
- Editor windows are virtual windows within raylib, sharing one small window
  manager. Browser resizing and editor scrolling use the same mouse-first approach.
- Idle drawing waits for events; playback and gestures run at 60 FPS. Workers
  wake the UI when processing completes. Playlist copies reuse cached pattern
  previews; extreme zoom retains vector detail. See [performance checks](performance.md).
- Patterns contain all channels' notes. Playlist clips reference patterns or Audio channels and
  keep independent crop lengths; resizing a clip never deletes source notes.
- Dropping audio on the Playlist creates a waveform clip and an Audio Rack channel.
  Playlist waveforms cache a hierarchy of min/max peaks over 256-frame blocks.
  Drawing queries only each visible pixel’s frame range, scanning partial blocks
  directly for exact detail down to individual samples. Coarse views reuse cached
  peaks; zooming never rebuilds the cache. Sample changes invalidate it.
  Rack drops create Unsorted samplers; the title filter shows All, Audio or Unsorted.
  Both groups can be sequenced. Audio clip crop lengths are stored in seconds,
  so changing tempo changes their grid footprint without changing sample speed.
  Full-length clips follow sampler duration changes; manually cropped copies keep
  their cap. Track header activity strips flash on starts and dim while voices sound.
  Waveform envelopes are cached per channel; duration colors blend maroon to green.
- Sampler headers share channel enable, pan, volume, pitch/range and mixer routing
  with the Rack. Channel pitch changes playback speed over a saved 1–48 semitone
  range; sampler processing Pitch preserves duration. All knobs share one size
  and one cached arc atlas size bank.
- Samples store mono or interleaved stereo PCM. Decoding, playback, previews and
  export retain stereo. Sampler stretch uses one grain alignment for both channels;
  normalization uses a shared peak and trimming checks both channels. Waveform
  envelopes include both channels without summing them.
- Original sample audio is retained. Processing produces a separate sample,
  using a background worker after knob release. Start/Length and the quiet-tail Trim threshold preview the crop
  live; Pitch/Time retain the processed waveform until replacement audio is ready.
- The audio callback allocates nothing and owns its rendering state. UI edits
  enter a bounded command queue and a pending project snapshot. The callback
  tries the mailbox mutex without waiting; if busy, it continues rendering its
  current state. UI animations read a small published snapshot. Sample replacement
  and Stop wait on the UI thread for acknowledgment before old PCM can be freed.
- Mixer buses retain float headroom and unity gain is transparent. Preview,
  metronome and listening gain mix before the final device clamp to -1..1.
  PCM16 export clamps at conversion. Overloaded mixes still need gain reduction;
  there is no automatic compressor or lookahead limiter.
- Project files currently use `.hbt` and the `HOMEBEAT` version-28 header for
  compatibility. Versions 1–27 remain readable. Renaming the app did not change
  the project format. Sample paths are absolute and projects are not portable bundles.
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
recording, MIDI device I/O, external plugins, automation, undo, or FLP import. WAV export ends at
the arrangement boundary without an added tail; standard RIFF exports must
fit below 4 GiB. This is a workflow prototype,
not a production recording tool. Device changes, sleep/wake and sustained
interactive playback still need platform testing.

## UI layout

The interface places the Browser on the left, with a compact top toolbar,
Playlist, floating Channel Rack and Mixer, and a Piano Roll with keys on the
left and velocity controls below.
