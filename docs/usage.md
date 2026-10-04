# Using LibreLoop

**VIEW → Dark / Light** changes the appearance. Both themes use neutral gray surfaces with a soft blue accent (`#516389`) by default. All window
surfaces are opaque.
**VIEW → Accent color** offers the shared rainbow palette for selection and active
control accents. Theme and accent are remembered between launches. Preferences are stored in
`$XDG_CONFIG_HOME/libreloop/theme.txt` (or `~/.config/libreloop/theme.txt`).

Zooming reveals finer grid subdivisions; zooming out groups ruler numbers into
2, 4, 8 and larger bar intervals. Four-bar shading stays aligned while panning
and fades out at distant overview zooms. Editing follows the visible grid automatically.

## First session

The initial project is empty, with one unloaded Sampler, Pattern 1 and a Playlist
with 100 tracks. Load a sample from the Browser to get started. **FILE → Demo**
loads an eight-bar drum and bass example using generated sounds; press Play to hear it. The stacked PATT/SONG button highlights Pattern in orange and Song in green.
Switch to SONG to hear the Playlist, or click its ruler. Play
and Space start from the ruler marker; stopping returns to it. Editing and
transport use mouse controls, and the Browser retains navigation keys. Text fields
support normal text editing (Command+A/V on macOS, Control+A/V on Linux). **Help** lists all current keybindings in a draggable
dialog. Drag tempo up/down or use the wheel; right-click to enter fractional
BPM with a dot or comma. The knob immediately left of
LibreLoop output volume changes master pitch by up to one octave either way, using sample
playback speed. It affects playback, Browser previews and WAV export, including
notes in progress, while tempo and note timing stay fixed. Right-click opens
number entry; its Reset button restores the default. LibreLoop output volume
only controls listening level, including previews; it is independent of the
Mixer Master fader and does not affect WAV export.

- Channel Rack: left-click/drag steps to paint beats; right-click/drag to erase. Compact controls from left to
  right are mute, pan, volume, mixer destination, channel selection, then steps.
  Hold the boxed destination number and drag up/down to choose an existing
  mixer insert; 0 sends it straight to Master. Drag pan/volume knobs or use the
  wheel. Knobs and faders capture the mouse until release, even outside their
  bounds. Right-click opens exact number entry; choose Reset for defaults. Right-click a
  channel button and choose **Go to Piano Roll**. Channels with pitched notes show a
  miniature note preview in place of steps; right-click it for the same menu.
  The instrument-shaped **+** button below the channels opens the plugin menu
  and lights up when dragging a sample there to add a channel.
  Clicking a channel name opens its instrument interface. Steps and pitched
  notes belong to the same shared pattern.
- Double-click a Playlist pattern clip to open and focus its Channel Rack.
- The Playlist’s left pattern list selects the current pattern. Right-click a
  pattern for Rename, Color or Delete; Enter applies, Escape cancels, and Ctrl+A selects the name.
  Names save with the project and appear in clips and Piano Roll. The + button at
  the bottom creates and selects a blank pattern, up to eight. New projects start
  with Pattern 1 selected and an empty Playlist.

- **Edit → Undo / Redo** restores project edits. Use **Cmd+Z / Cmd+Shift+Z**
  on macOS or **Ctrl+Z / Ctrl+Shift+Z** (also **Ctrl+Y**) on Linux.
  Clip and note drags, brush strokes, automation-point drags and control drags
  each undo as one action. Samples, sampler settings, routing, names and colors
  are included; recording becomes one action after it stops. Original audio is
  retained in memory, so sample replacement and deletion can be undone even if
  the source file is unavailable. A new edit after undo discards the redo branch.
  New, Demo and Open start fresh history. History retains up to 64 states within
  a 256 MiB budget (the latest state is always retained). Undo does not delete
  saved files or recorded WAV files, or change navigation and window layout.
- Playlist: the icon toolbar offers Pencil, Brush, Select, Cut (scissors) and Stretch (horizontal arrows). Hover an
  icon for its description. Pencil places one clip; drag its body to move it
  without painting more copies. Brush paints copies across a track. Click a
  clip to highlight it and choose its pattern and length as the drawing source.
  Select draws a rectangle; drag a selected clip to move the group together.
  Cut splits patterns, audio and automation at the clicked snap position, preserving
  the source position on both halves. Stretch drags either edge of an audio clip;
  its opposite edge stays anchored. Resample changes speed and pitch together;
  Stretch preserves pitch. This changes the sampler Time setting (0.25–4×), leaves
  pitch controls unchanged, and affects all clips sharing that sample. Processing
  completes after release; the waveform and clip lengths preview during the drag.
  Shift-click toggles clips in the selection with any tool; Shift-drag on empty
  space adds a rectangle to the selection. Drag a selected clip without Shift
  to move the group (including with Brush). Drag across track headers to select
  whole tracks; Shift adds those tracks to the current selection. **Delete/Supr**
  deletes selected clips while Playlist has focus; right-drag erases with any tool. Moves into occupied
  space or beyond the grid are blocked rather than overwriting clips.
  Drag a clip's right edge to resize using its selected snap grid. Each copy keeps
  its own length. Shortening crops that copy; extending restores the preserved
  source, up to the full pattern length. Rack and Piano Roll always edit the full
  source pattern. The Rack uses fixed-width steps and a bottom scrollbar; wider
  windows reveal more steps. Painting grey steps extends the pattern in whole
  bars. Existing Playlist copies retain their lengths. The + button stays beside the bottom scrollbar,
  even when the channel list needs scrolling.
  Preview lines show actual notes. Brush spaces longer clips to avoid overlaps.
  Any of 100 tracks can hold any pattern; different tracks play together.
  The narrow strip at each track header’s right edge flashes on note/audio
  starts and stays dimly lit while a voice is sounding. The Playlist marker
  sets the initial playback position; after the final clip
  edge, playback returns to the song beginning. Explicit ruler loop ranges
  still return to their selected loop start. Wheel scrolls vertically; Shift+wheel
  scrolls horizontally. Command+wheel on macOS or Ctrl+wheel on Linux zooms
  around the pointer. Middle-button drag pans freely. On macOS, two-finger
  scrolling pans both axes and pinching zooms. The bottom scrollbar also pans.
  Use the right scrollbar to scroll vertically.
  Clip resize handles highlight on hover, and the cursor updates once per frame.
- Click or drag either editor's bar ruler to position its downward start arrow.
  Playlist selects SONG playback; Piano Roll selects PAT playback. Space and
  the Play button start from that marker; stopping returns to it. Zoom keeps
  the time beneath the pointer fixed where the timeline origin allows.
  Each modified wheel notch changes the visible range by about 8%. The toolbar Follow icon
  keeps the playhead centered in Song or PAT mode, with empty space before the
  timeline origin when playback starts. Manual navigation and editing temporarily suspend following.
  Horizontal scrollbar thumbs remain at least 24 pixels wide and highlight on hover.
- Playlist and Piano Roll edits always align with the visible grid, becoming
  finer as you zoom in. At distant overview zooms they use ruler spacing.
  There is no snap selector or modifier bypass. Fractional positions and
  lengths are saved; older projects remain readable.
- Click the Piano Roll keys to audition its instrument; hold and drag along
  the keys to play successive pitches. The active key highlights and release
  stops the note.
- Piano Roll has its own Pencil, Brush and Select tools, independent of the
  Playlist. Pencil draws one note; Brush paints notes across the snap grid.
  Select draws a rectangle; drag selected notes to move them together.
  Shift-click toggles notes, Shift-drag adds a rectangle with any tool, and
  Delete/Supr removes selected notes when Piano Roll has focus. Right-drag
  erases notes with every tool. Click and drag to create a note and set its length. Its pitch
  and start remain locked during that gesture. Add notes at different pitches
  on the same step to form chords. Drag a note body to move its timing/pitch;
  length and velocity stay intact, and duplicate onset/pitch collisions are blocked.
  Dragging a note auditions it at each new pitch. The right scrollbar scrolls pitches.
  Piano Roll uses the same mouse/trackpad navigation as Playlist; vertical
  scrolling browses pitches. Drag the bottom scrollbar to pan across the pattern. Bar numbers appear above the notes. Drag an existing note's right edge to resize
  it; right-click its body to erase it. Dragging a velocity bar changes every
  note that starts at that position. C4 plays the sample at its original pitch;
  other pitches resample it. Notes stop at their duration or the sample's end.
- Mixer: Master and independent inserts with their own volume, pan and
  one light: left-click mutes/unmutes, right-click opens automation and solo actions;
  soloing a bus includes its routed sources. Master and 100 fixed inserts keep
  stable numbers; right-click an insert header for mute, solo or reset.
  Several instruments can feed the same insert; its controls affect their combined
  signal. Drag an insert's bottom-right output jack to another channel's bottom-left
  **IN** socket to route it there; right-click its output to unplug. Each insert
  has one destination; default is Master. Connections cannot create feedback
  loops. The selected insert shows its cable. Use the wheel over strips or the
  bottom scrollbar to browse inserts. The fixed-width panel on the right has
  ten empty effect slots for the selected channel; plugins are not implemented yet.
- Drop a WAV, FLAC or MP3 onto the Playlist grid to create a waveform clip
  and a reusable Audio channel. Double-click the clip to open its sampler;
  sampler changes affect every copy. Full-length clips follow trimmed sample
  duration; copies cropped in the Playlist retain their independent cap. Audio clips can move, crop, copy, mute
  and export alongside patterns, and their channels can also be sequenced.
  Moving or cropping an Audio clip during playback catches up at the current
  playhead position. Applying Sampler processing also resumes active Audio clips.
  Drag either highlighted clip edge to trim it; pulling the left edge back out
  restores the cropped beginning without changing the shared sample or pattern.
  Drag the divider beneath a track to change that track’s height (32–320 pixels).
  The Rack title filter offers **All**, **Audio** and **Unsorted**. Rack sample
  drops belong to Unsorted; Audio channel colors blend maroon to green with duration.
- Drop a WAV, FLAC or MP3 onto an existing Playlist audio clip or a visible Rack
  row to replace its sample. Existing notes, clip positions, crops, mixer routing
  and sampler settings stay in place; all clips sharing that sample update.
  Drop into empty Rack space or its bottom add area to create a channel.
  Up to 32 channels can be added; wheel over Rack rows to scroll. Right-click a
  channel name for Piano Roll, rename, mute or delete. Deleting removes its notes in
  every pattern and any Playlist audio clips using that channel. Mono and stereo samples retain their channel count and decode at
  48 kHz and are held in memory. Songs longer than a minute are supported;
  the processing frame limit is approximately 93 minutes per file.
- Browser: an expandable folder tree with **LibreLoop samples** as its first
  root. **+ Add folder** adds another root, up to eight, saved between sessions.
  Click a folder to expand/collapse it. Supported WAV/FLAC/MP3 and `.hbt` files
  appear beneath their folders; folders sort before files. Click samples to
  select and preview, or drag them onto Rack channels to load them.
  While hovering over the Browser (or after clicking it), **Up/Down** or **k/j** move through visible nodes and
  preview samples. **Right/l** expands a folder or enters its first child;
  **Left/h** collapses a folder or selects its parent. Trees replace `..`, Home,
  and filesystem-root navigation buttons; add `/` as a root if desired.
  The selected audio sample has a waveform preview at the bottom, with a moving
  playback cursor. Browser previews stop after five seconds (or at the file’s
  end, if shorter). Click the waveform to replay it from the beginning.
  Scroll the wheel to browse. Drag the Browser's right edge to change its width;
  dragging below its minimum width collapses it to a narrow strip. Drag the
  strip's right edge outward to restore it.
  Dialogs open centred and have draggable title bars.
  Added roots persist in `$XDG_CONFIG_HOME/libreloop/folders.txt`, or
  `~/.config/libreloop/folders.txt` when XDG_CONFIG_HOME is unset. Old Homebeat
  folder settings are read when no LibreLoop settings file exists.

- FILE → New starts an empty project with one unloaded Sampler and selects Pattern mode.
  New, Demo and Open ask Save / Discard / Cancel when the project has unsaved edits,
  including projects opened from the Browser or dropped from the file manager.
  A star beside the project filename marks unsaved edits. Cancelling the save chooser
  keeps the current project. This guard applies to replacement; closing the app
  still requires saving your work first.
  FILE → Demo loads the built-in eight-bar example and selects Song mode. Both stop
  playback and clear previews; save your work first. New uses `project.hbt`; Demo uses
  `demo.hbt`, rather than the previously opened project filename.
- The three icons above the Playlist picker show Patterns (piano), Audio clips
  (waveform), or Automation. Click a source to place copies, or drag it onto the
  Playlist. Double-click a pattern to open the Channel Rack, or an audio item to
  open its sampler. Drop a Browser or Finder/file-manager audio file into the Audio
  picker to import it without placing a Playlist clip, even while viewing Patterns
  or Automation. The picker highlights during the drag, then switches to Audio
  and reveals the new item. Drag pattern or audio items onto the grid to place
  clips; a green preview shows the drop position. Audio clips list the imported Audio channels. New patterns and Audio channels
  choose an unused palette color in their own list, cycling once all eight colors
  are used. Replacing a sample preserves its channel color. Right-click for Rename, Color or
  Delete; changes apply to the Rack channel and every Playlist copy. Deleting
  removes the channel and its clips, while leaving the source file on disk. Automation lists saved parameter curves with the same source actions.
- FILE → Save opens a file picker for a new project, defaulting to `project.hbt`
  (`demo.hbt` for Demo). Later saves update that chosen file. **Save As…** chooses
  another name/location. **Cmd+S** (macOS) or **Ctrl+S** (Linux) saves;
  add Shift for Save As.
- FILE → Open… chooses a `.hbt` project. **Cmd+O / Ctrl+O** also opens the picker.
  Dropping a `.hbt` or opening it in the Browser still works; later saves use its filename.
- FILE → Export… chooses a WAV destination and exports the entire Playlist:
  48 kHz, stereo PCM16. The next export remembers that destination.
  The built-in chooser shows the full folder path, with Up/Home navigation,
  folders first, matching file types, hidden-file toggle and a filename field.
  Double-click a folder to enter it or a file to open/select it; Enter also
  activates the selection. Click the path field to type/paste a folder and press
  Enter. Save/Export add the extension and ask before replacing an existing file.
  Cancel/Escape leaves files unchanged. The chooser is the same on Linux and
  macOS and needs no external file-manager or dialog packages.

Sample paths are stored as absolute paths; keep imported files available when
reopening projects. Generated demo sounds need no external files. Save/load
and export report errors in the status bar. Save and export replace their
existing target files. Project writes use a temporary file and rename.
Dialogs open centred and remain draggable. Hover controls to see their function
and relevant optional keys in the bottom helper.

Project format version 29 saves polyphonic notes, durations, pattern names,
insert settings, routing, source lengths and individual clip lengths, master pitch, fractional BPM, insert outputs, pattern
count, channel names/count, fractional note/clip timing, 100-track clips, sampler processing, stereo width, mute/solo states, global swing, boosted mixer gains, audio-device choices, track and insert names, pattern and Audio channel colors and reserved effect-slot settings. Versions
1–28 still load; older projects keep their previous one-bar clip lengths.

The Channel Rack Swing knob delays alternate sixteenth steps (up to half a step).
It affects pattern/song playback and WAV exports; live audition stays immediate.

## Editor windows

- Drag a title bar to move a window, or click a window to bring it forward.
  Double-click its title bar to maximize/restore.
- Drag the bottom-right corner to resize it. The square maximizes/restores;
  the minus minimizes it; the cross hides it. Restore it with its toolbar icon.
- Right-click a title bar for Stay on top, Maximize/Restore, or Hide.
- Toolbar order is Rack, Piano Roll, Playlist, Mixer. Clicking brings a window
  forward; clicking it again while it is in front hides it.
- Stay-on-top windows retain their own stacking priority. Window positions
  currently reset on launch.

## Instruments and keyboard audition

- Click a Rack instrument name to open its movable sampler window, with
  waveform, volume and pan. Click the waveform to preview it, then click again
  to stop; the top-bar pause/stop controls and Space also stop all previews. A playback line
  moves through it. The + beside the bottom Rack scrollbar opens a movable instrument picker
  containing Sampler. Selecting it adds an empty channel; drag a Browser
  sample onto its window to load it. Empty samplers persist when saved. A ghost row beneath the final channel
  shows where a dropped sample will be added; the Rack scrollbar browses channels.
  Future instrument types will open their
  own interface here. Right-click a channel and choose Go to Piano Roll to
  assign that editor; selecting another Rack channel keeps its notes unchanged.
- The toolbar keyboard icon enables polyphonic audition of the selected channel.
  Z/X/C/V/B/N/M and the three following keys are white notes from C3 to E4;
  S/D/G/H/J/L and the key after L are the black notes. Q/W/E/R/T/Y/U/I/O/P
  and the two following keys are white notes from C4 to G5, with
  2/3/5/6/7/9/0 and the key after 0 as black notes. Positions follow a US
  keyboard: the final lower/upper white keys are -/+ on a Spanish keyboard.
  Release stops the note with a short fade; samples also stop at their end.
  Browser focus, dialogs, Ctrl/Alt shortcuts and loss of app focus suppress
  audition. This plays notes without recording them into a pattern.

Sampler processing is reversible and saved with the project. Normalize sets
the processed peak to 100%; Reverse plays the cropped sample backwards;
Polarity flips its sign. Pitch shifts up to an octave without changing duration.
Time sets 0.25–4 times the cropped duration: Resample changes playback speed
and pitch like vinyl, while Stretch preserves pitch. Start trims a percentage
from the source beginning; Length retains a percentage of what remains.
Trim removes quiet audio from both ends of that range; turning it up raises
the silence threshold. Audio present in either stereo channel is preserved.
The Sampler, Audio list and Playlist share a live source-envelope preview while
dragging Start, Length or Trim; Reverse, Polarity and Normalize update immediately
in all three views. Full-length Playlist clips preview the new duration too;
clips with a manually set length keep that length. Pitch and Time retain the
last processed waveform until the new audio is ready.
This lightweight crop envelope preview is
marked Preview until the exact processed waveform is ready; pitch/stretch
detail requires processing. Playback and WAV export use the processed audio.
Knob processing starts on release in a background worker,
and the original sample file is never modified. Stretch/pitch use a compact
WSOLA implementation, so complex material and extreme settings may have
audible artifacts.

Mixer meters show stereo levels with half-second peak holds. Width runs from mono (0), through unchanged (1), to wider (2). Mixer record-arm lights toggle red; the top-bar Record button records all armed tracks. Rack lights use left-click mute and right-click automation/solo actions. Playlist track lights retain right-click solo.

The metronome icon before Tempo toggles beat clicks during Pattern or Song playback, with an accent on the first beat of each bar. It is off by default and excluded from WAV export.

Knobs use a full-circle value sweep, except Swing, which keeps its horseshoe.
Volume knobs mark unity (1 / 0 dB) with a fixed dot at three-quarters of the turn;
the remaining quarter allows up to 1.25 (+1.94 dB). This applies to Channel Rack,
sampler channel volume and listening output volume.
Knobs show their value with an edge dot and a filling outer arc. Knob size stays fixed; drag up/down to adjust or right-click for value entry and supported automation actions.

The Playlist, Rack and Piano Roll share the same compact ruler. Left-click or
drag sets the playback start. Right-drag creates a red loop region. Once a loop
exists, right-click or right-drag to the left of its midpoint adjusts its left
edge; to the right adjusts its right edge. Double-click anywhere on the ruler
with either button to clear the loop. Rack and Piano Roll share the current pattern's
start and loop, while Playlist uses Song positions. These playback selections
reset when opening a project and do not restrict WAV export. Press Stop once
to stop audio; press it again while stopped to reset the playback start to the
beginning. Playing or setting a ruler marker starts a fresh Stop sequence.

Typing keyboard notes highlight their Piano Roll keys and matching notes.
Chords remain highlighted until their keys are released.

Pan knobs show deviation from center: warm yellow left, orange-red right.
Mixer stereo width is neutral at the center, blue toward the left (wider),
and purple toward the right (mono). Its saved multiplier remains 0 for mono,
1 unchanged, and 2 wider. Swing has an orange horseshoe indicator.

Mixer faders start at unity (0 dB), with up to +6.02 dB of boost above the
marked unity line. Unity sits a quarter of the travel below the top on faders
and meters, leaving room for boost and peaks. The tall meter to the left of Master follows the selected
track and shows its peak in dBFS. Meter colors change to orange at -12 dBFS
and red above 0 dBFS. Master shows the unclipped mix and also reports overload
introduced by listening-volume boost. Audio below full scale plays transparently;
the device output and PCM16 export clamp peaks outside -1..1. Reduce gain when
meters turn red to avoid clipping distortion. Saved projects
retain their gain settings. The unlabeled orange knob at the right of the
Channel Rack title bar controls Swing.

The Mixer starts hidden; open it from the toolbar. Its compact side panel has
an Input dropdown for recording and a short control legend. None records only
the track's internal audio. Device names refresh when opening the menu; input
choices are saved per track. External Output choices are hidden until external
routing is implemented. Internal cable routing continues to work. The effects
area shows one empty-state message until effects are available.

To record a microphone, select its Input on a Mixer insert, click that insert's
record-arm light so it turns red, then click the red Record button in the top bar.
macOS may request microphone permission on the first take. Each armed track
creates a separate Audio channel and a clip on an empty Playlist track. The clip
and its waveform grow live. Selected inputs also feed the armed track and its
normal Mixer routing while recording. Capture is post-fader: pan, width, mute,
gain and upstream routing affect the take; the listening-volume knob does not.
New take channels play back directly through Master, avoiding a second pass
through the recorded insert's gain.

Recording begins at the Song cursor when already playing Song mode, or at the
Playlist start marker otherwise. It switches to Song mode and continues beyond
the existing arrangement end without looping. Click Record again to finish the
takes while playback continues; Stop or Space finishes them and stops playback.
Tempo and recording-arm choices stay fixed during a take. WAV files are written
as 48 kHz stereo float audio in `recordings/` under the launch working directory.
Projects reference those files by absolute path. Saving, exporting, replacing a
project or closing the app finishes the current take first. Buffer overrun, device
failure, memory exhaustion or a disk write error ends recording with a status
message and retains the successfully captured audio. Takes currently remain in
memory as well as on disk; very long sessions depend on available RAM.

The Playlist’s left pattern picker shows note previews; click to select a pattern,
right-click for Rename, Color or Delete, and scroll the list when necessary. Right-click a Playlist
track header to rename that track. The Mixer effects area has no interactive placeholder controls.

The top bar groups FILE, VIEW and HELP on the left. HELP → Keybindings opens
the shortcut list. The arrow past a vertical marker toggles Follow playhead;
the keyboard icon toggles typing notes. Hover either icon for its description.

Mixer inserts show their saved names beneath their numbers. Right-click the name
to rename it; right-click the number for the existing mixer actions. Longer names
are shortened to fit the strip and shown in full in the hover helper.

Deleting a pattern removes its Playlist clips. Deleting the final pattern leaves
one blank pattern selected so the Rack always has a pattern to edit.

Patterns receive distinct default colors. Right-click a pattern and choose Color
to select another palette color; the list and its Playlist clips share that saved color.

If the start marker is at or beyond the song/pattern end, playback starts at the
beginning immediately while the marker stays where you placed it. Explicit loop
selections still define their own playback region.

Sampler channel controls at the top right share the Rack enable, pan, volume
and mixer destination. Channel Pitch changes playback speed; drag its Range
number to choose 1–48 semitones (default 2). The separate processing Pitch
retains duration. Trim removes quiet audio from both ends using an adjustable threshold;
0 leaves it intact, a tiny turn removes silence, and higher values cut further
into the quiet decay (threshold range −90 to −30 dBFS). Original
audio stays intact, so reducing Trim restores the tail.

Track height: drag the separator inside a Playlist track header. Only the header
separator highlights and resizes the track; timeline grid lines remain visual.

Automation: right-click a channel volume, pan, pitch/range, Swing, Mixer fader,
Mixer pan/stereo-width or mute light and choose **Create automation**. The menu
also retains exact value entry and Reset; mute lights keep Solo in their menu.
An automation clip starts at the Song marker (the current bar during Song playback),
or spans the selected Song loop. Otherwise it uses the current pattern length.
It appears on the first free Playlist track and in the Automation picker tab.

Click inside its body to add a point, drag a point to change its position/value,
and right-click an interior point to remove it. Horizontal movement follows the visible grid;
points stop at their neighbors, allowing vertical jumps without crossing. The
outermost points can move beyond the clip edges, expanding that copy
to follow them. Moving left stops at the beginning of the song.
Drag the title strip to move a clip, drag an edge to shorten or extend it, or right-click the
title to erase a copy. Resize the track header for a taller editing area.
The Automation list supports selection, anchored drag previews, drag/drop, Rename,
Color and Delete, like the other source lists. Copies share their curve.

Automated knobs, faders and mute lights follow playback visually without changing
the saved manual values. Automation runs in Song mode and WAV exports. Values interpolate linearly; mute
uses a halfway threshold. Muted/non-solo Playlist tracks do not contribute curves.
Before the first automation, the saved manual value applies. After a clip ends,
its final value holds until another automation takes over; overlapping clips for
the same control use the later Playlist track, then the later slot. Projects
save up to 32 automation sources with 64 points each.

Sampler channel controls are automatable. Sampler processing Pitch/Time,
Start/Length/Trim, Normalize/Reverse/Polarity and Stretch still rebuild a sample
after edits and are not yet realtime automation targets. Tempo, listening output
volume, routing, device choices and the inactive effect-slot controls also retain
their current manual behavior. Future instruments/effects can use the same
saved parameter target and normalized-curve interface.

Pitch automation changes sample speed and duration together. Full-length Audio
clips and their waveforms follow the complete channel pitch/range and Master
pitch curves: lowering pitch takes longer to finish, raising it finishes sooner.
Playback, seeking and WAV export use the same timing. Explicitly cropped clips
keep their chosen playback boundary. Muted automation tracks release pitch back
to the manual value.

Playlist pinch zoom and modifier+wheel zoom also work over the Track headers,
anchored at the left edge of the visible timeline.

Hold Command on macOS or Control on Linux and left-drag in the Playlist or
Piano Roll to temporarily box-select, even over existing clips, automation
points or notes. Your Pencil/Brush tool stays selected. Add Shift to preserve
the previous selection. Release the mouse to finish selection.

The sampler’s **Fit to tempo** button locks the current audio duration to the musical grid. Subsequent BPM changes adjust playback duration: **Resample** changes pitch, while **Stretch** preserves it. Trimmed and split copies remain aligned; the pitch and Time knobs keep their values. Enable it at the tempo where the clip already has the desired length. The setting is saved with the project and supports undo/redo. Turning it off restores the duration set by the Time knob, independent of tempo.

Fit to tempo changes playback rate smoothly while playing, without rebuilding the
sample. Clip geometry and waveforms remain fixed at their reference tempo.
Stretch preserves pitch using real-time grain alignment; large tempo changes
can still change the texture of transients.

The interface keeps its normal text and control sizes in small windows. Editors
show less content rather than shrinking all controls; Help scrolls when needed,
and file dialogs show fewer rows. Browser rows share a continuous background,
with selection and hover cues. The initial layout shows Arrangement and Channel
Rack; the Mixer opens on demand.

Sampler **Pitch** changes playback speed using its Range. The separate
**Pitch shift** control under Sample processing changes processed pitch.
**Route** selects the Mixer destination (0 = Master). Resample/Stretch opens a
two-choice menu. Sample region groups Start, Length and Trim above the waveform.
Hovering even a short Arrangement clip shows its full source name and duration.

Clip colors stay the same in Light and Dark themes. Clip titles, note previews,
waveforms and automation curves use a contrasting foreground based on the clip
color, including the source list and drag previews.

The shared color picker has twelve evenly spaced rainbow hues in two rows:
light fills with dark artwork, then matching dark fills with light artwork.
Each row shares perceptual lightness and chroma. Pattern, audio, automation and
accent color pickers all use this palette; saved project colors are retained.

Light mode derives its surfaces, text, rulers and grid from the inverted dark
brightness hierarchy. Clip colors, the chosen accent, piano key identities and
semantic colors (such as recording red) retain their meaning in both themes.

Tempo, pitch range and Mixer routing number controls show the vertical resize
cursor while hovering or dragging, like Mixer faders. Knobs retain the regular
cursor. Arrangement labels and track surfaces retain fractional positions while
panning; stationary text remains aligned to display pixels.

Waveforms share a continuous filled style in Arrangement clips, source previews,
the Browser and Sampler. Display bins follow the source audio and blend smoothly
while panning or zooming, without changing the audio. Pattern previews use display
resolution and filtered textures for fractional movement.
