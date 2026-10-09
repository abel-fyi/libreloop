# Using LibreLoop

LibreLoop uses one curated appearance: the website's violet-black background,
soft source colors, and the logo spiral's `#b39add` lavender highlight. **VIEW →
MIDI / Recording** opens input and recording settings. Appearance preferences
from older versions are no longer read.

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
  retained in memory or shared file mappings, so sample replacement and deletion can be undone even if
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
  scrolling pans both axes and pinching zooms. The top scrollbar also pans.
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
  scrolling browses pitches. Drag the top scrollbar to pan across the pattern. Bar numbers appear above the notes. Drag an existing note's right edge to resize
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
  ten built-in effect slots for the selected channel; select a slot to add Chorus.
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
  drops belong to Unsorted; Automatic Audio colors sweep from pink through violet/cyan toward lime as duration grows.
- Drop a WAV, FLAC or MP3 onto an existing Playlist audio clip or a visible Rack
  row to replace its sample. Existing notes, clip positions, crops, mixer routing
  and sampler settings stay in place; all clips sharing that sample update.
  Drop into empty Rack space or its bottom add area to create a channel.
  Up to 32 channels can be added; wheel over Rack rows to scroll. Right-click a
  channel name for Piano Roll, rename, mute or delete. Deleting removes its notes in
  every pattern and any Playlist audio clips using that channel. Mono and stereo samples retain their channel count and decode at
  48 kHz. Canonical float-stereo WAVs use shared file mappings; other formats
  decode into memory. Songs longer than a minute are supported;
  the processing frame limit is approximately 93 minutes per file.
- Browser: an expandable folder tree with **LibreLoop samples** as its first
  root. **+ Add folder** adds another root, up to eight, saved between sessions.
  Click a folder to expand/collapse it. Supported WAV/FLAC/MP3 and `.llp` files
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
  keeps the current project. Closing the app uses the same Save / Discard / Cancel
  prompt, and exits only after a successful save or an explicit Discard.
  FILE → Demo loads the built-in eight-bar example and selects Song mode. Both stop
  playback and clear previews; save your work first. New uses `project.llp`; Demo uses
  `demo.llp`, rather than the previously opened project filename.
- The three icons above the Playlist picker show Patterns (piano), Audio clips
  (waveform), or Automation. Click a source to place copies, or drag it onto the
  Playlist. Double-click a pattern to open the Channel Rack, or an audio item to
  open its sampler. Drop a Browser or Finder/file-manager audio file into the Audio
  picker to import it without placing a Playlist clip, even while viewing Patterns
  or Automation. The picker highlights during the drag, then switches to Audio
  and reveals the new item. Drag pattern or audio items onto the grid to place
  clips; a green preview shows the drop position. Audio clips list the imported Audio channels. New patterns and Audio channels
  choose an unused palette color in their own list, cycling once all 24 colors
  are used. Replacing a sample preserves its channel color. Right-click for Rename, Color or
  Delete; changes apply to the Rack channel and every Playlist copy. Deleting
  removes the channel and its clips, while leaving the source file on disk. Automation lists saved parameter curves with the same source actions.
- FILE → Save opens a file picker for a new project, defaulting to `project.llp`
  (`demo.llp` for Demo). Later saves update that chosen file. **Save As…** chooses
  another name/location. **Cmd+S** (macOS) or **Ctrl+S** (Linux) saves;
  add Shift for Save As.
- FILE → Open… chooses a `.llp` project. **Cmd+O / Ctrl+O** also opens the picker.
  Dropping a `.llp` or opening it in the Browser still works; later saves use its filename.
- FILE → Export… chooses a WAV destination and exports the entire Playlist:
  48 kHz, stereo PCM16. The next export remembers that destination.
  The built-in chooser shows the full folder path, with Up/Home navigation,
  folders first, matching file types, hidden-file toggle and a filename field.
  Double-click a folder to enter it or a file to open/select it; Enter also
  activates the selection. Click the path field to type/paste a folder and press
  Enter. Save/Export add the extension and ask before replacing an existing file.
  Cancel/Escape leaves files unchanged. The chooser is the same on Linux and
  macOS and needs no external file-manager or dialog packages.

Sample references are saved relative to the project directory. **FILE → Collect
samples and save…** writes the original audio into a companion directory beside
the project; move both together to another machine. Later saves retain those collected
references. Generated demo sounds need no external files. Projects with missing audio
still open, preserving notes, clips, source settings and sample paths. Missing channel
names show `[missing]`; right-click the Rack channel or Audio source and choose
**Relink sample…**, or drop the replacement file onto its Rack row.

Save/load and export report errors in the status bar. Project saves and WAV exports
write unique temporary files and replace existing destinations only after success.
A failed write preserves the previous file.
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
Projects save relative references to those files. Saving, exporting, replacing a
project or closing the app finishes the current take first. Background workers write
the audio; live previews, unprocessed playback and undo share read-only file mappings
instead of holding full copies of long takes in RAM. Waveform caches still grow with
the take, and sampler processing can allocate full output buffers. Buffer overrun,
device failure, mapping/memory exhaustion or disk errors finish the successfully
captured portion and report the error. Each take supports about 93 minutes at 48 kHz.

The Playlist’s left pattern picker shows note previews; click to select a pattern,
right-click for Rename, Color or Delete, and scroll the list when necessary. Right-click a Playlist
track header to rename that track. The Mixer effects area provides Chorus slots with rate, depth, mix and bypass controls.

The top bar groups FILE, EDIT, VIEW and HELP on the left. HELP → Keybindings opens
the shortcut list. The arrow past a vertical marker toggles Follow playhead;
the keyboard icon toggles typing notes. Hover either icon for its description.

Dropdown and context menus support Up/Down or k/j to move keyboard focus,
Enter to select, and Escape to dismiss. Left/Right or h/l also move between
choices, including Save/Discard/Cancel. Menus have one shared mouse/keyboard highlight; moving the mouse transfers
focus to its row, and keyboard navigation moves that same focus. The unsaved-changes
prompt initially focuses Cancel. Text-entry dialogs retain their normal typing bindings.

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
brighter fills, then matching deeper fills. Titles and previews choose black or white for contrast; selected sources and
Arrangement clips use white outlines, matching note selections. Each stored row shares OKLCH lightness; chroma is kept soft so selection highlights stand out. Pattern, channel, audio and
automation color pickers use this palette. Source colors are displayed at up to
25% saturation so the stronger selection accent stands out; stored project RGB
values are retained.

Source colors, piano key identities and semantic colors (such as recording red)
retain consistent meanings throughout the interface.

Tempo, pitch range and Mixer routing number controls show the vertical resize
cursor while hovering or dragging, like Mixer faders. Knobs retain the regular
cursor. Arrangement clip and track geometry retain fractional positions while
panning; all text remains aligned to physical display pixels. Font atlases extend
for UTF-8 names using the glyphs available in the bundled font.

Waveforms share a continuous filled style in Arrangement clips, source previews,
the Browser and Sampler. Display bins follow the source audio and blend smoothly
while panning or zooming, without changing the audio. Pattern previews use display
resolution and filtered textures for fractional movement.

### Built-in Chorus

Open the Mixer, select an insert or Master, and select one of its ten effect slots
on the right. Click **+ Chorus** to add it. Occupied slots have a dot beside their
number. Slots process in numeric order, before the selected track's fader.

**Rate** controls modulation speed (.05–5 Hz), **Depth** controls delay variation
(0–8 ms), and **Mix** blends dry and delayed sound. New instances start at .513 Hz,
1.85 ms depth and 50% mix for a slow, wide vintage sound. Try .863 Hz at the
same depth for a faster shimmer. Opposed stereo modulation and a filtered wet
path give warmth without added hiss; this is inspired by vintage BBD choruses,
rather than an exact circuit emulation. Existing chorus settings remain saved,
but use the new sound. **Bypass** smoothly returns to dry; **x** removes the effect.
Right-click any of the three knobs to enter a value, reset, or create automation.
Settings are included in undo, project saves, recording and WAV export. Old
projects load without effects; newly saved projects use format version 37.

### Built-in FM Synth

Click the instrument-shaped **+** row in the Channel Rack and choose **FM Synth**.
It shares channel volume, pan, pitch, mixer routing, note gates and automation with
Sampler. New channels start with the dry Bright EP patch. Sound, Body, Attack and
Motion tabs expose independent tuning, modulation envelopes and a graphical LFO.
See [FM electric piano and modulation](#fm-electric-piano-and-modulation).

### Device presets

Sampler, FM Synth and Chorus each have **Load** and **Save** preset buttons.
These open the in-app file chooser for `.llpreset` files. Presets store the
instrument/effect settings; channel volume, pan and routing stay with the project.
A preset must match the device it is loaded into.

Sampler presets reference the original sample file rather than embedding audio.
Keep that audio available; when moving presets, preserve its relative location.
Presets made from generated sources without a file path save only settings and
apply to the receiving Sampler's current audio. Missing sample files and invalid
presets report an error and leave the device unchanged.

### Equalizer and live spectrum

Select an empty Mixer effect slot and click **+ Equalizer**. **Open Equalizer**
opens its resizable editor. Seven fixed bands cover 20 Hz–20 kHz. Select a numbered band and use its shape
menu to choose Bell, Low Shelf, High Shelf, Low Cut, High Cut, or Off. New EQs
start with a low shelf, five bells and a high shelf. Drag a numbered point horizontally for frequency and
vertically for gain (±18 dB). Right-click a dot and choose **Reset** to restore
that band’s default frequency, gain, Q and shape. Wheel over a point adjusts Q; larger Q narrows the
bells and adds resonance to shelves/cuts. Cuts have a fixed 12 dB/octave slope
and use frequency/Q rather than gain. Off skips the band. Select a band to use its frequency, gain
and Q knobs. Frequency knobs follow the logarithmic graph scale for even drag
and wheel movement, while still showing Hz. Right-click the knobs for exact
values or automation.

The colored spectrum shows the selected bus's post-effect, post-fader stereo
power on a logarithmic frequency axis; its level range is -78 to 0 dBFS. The
response curve and point positions use the separate ±18 dB EQ gain scale.
**Mix**, **Bypass**, and **Load/Save** presets work like other effects. EQ settings
are included in project saves, undo, recording and WAV export. Older four-band
projects and presets retain their original shapes and disable the extra bands.

The top bar shows the listening output's frequency spectrum beside a stereo
level meter. Red marks indicate an overloaded Master output. The analyzer is
visual only and does not change the sound.

Knobs, mixer faders and numeric value controls use the same right-click menu:
**Reset** restores that control's default, **Enter value** accepts an exact value,
and supported controls also offer **Create automation**. No reset shortcut is
required. Click a channel's miniature note preview in the Channel Rack to open
its Piano Roll; right-click retains the channel options menu.

The file chooser remembers its last folder separately for opening projects,
saving projects, exporting WAVs, choosing samples, loading presets and saving
presets. Locations persist across app restarts; browsing and cancelling also
remembers the folder. Unavailable folders fall back to the normal starting path.
Saving an already saved project still writes directly to that project's file.

Arrangement and Piano Roll horizontal scrollbars sit above their timeline rulers.
The small arrow button at the right end controls vertical zoom: hover for the
vertical resize cursor, then drag up for taller rows or down to show more tracks
or notes. Arrangement zoom preserves custom track heights and the top visible
track. Piano Roll keys, notes and pitch scrolling follow the same row scale.

### FM electric piano and modulation

New FM Synth channels start with **Bright EP**, a dry electric piano with a
long-lived Body tone and a separate short, bright Attack. **Bright EP** resets
the synth to that factory patch. No chorus is added by the synth or preset.

The panel has four tabs:

- **Sound:** pitch/fine tuning of the audible oscillator, velocity brightness,
  routing, and the volume ADSR envelope.
- **Body:** independent semitone pitch, fine tuning, harmonic ratio and FM amount,
  plus its own attack, decay, sustain and release. This shapes the lasting tone.
- **Attack:** independent pitch, fine tuning and FM amount, plus a separate
  envelope for the bright strike. Increase amount for a harder attack; shorten
  decay to keep it out of the tail.
- **Motion:** per-note LFO speed, pitch motion in cents, volume motion and fade-in.
  Choose Sine, Triangle, Saw or Square from the waveform menu. Select Pitch or
  Volume and drag the graph horizontally for speed, vertically for amount.

Drag envelope handles horizontally to change stage time; the middle handle also
changes sustain vertically. The graph gives short times extra space, and the knobs
show milliseconds/seconds. Knobs support exact entry, Reset and automation. Pitch
ratios of 1x, 2x and 0.5x mean the played pitch, one octave up and one octave down;
Fine is in cents. Body additionally has a semitone control and integer harmonic ratio.

**Body + Attack** routes both modulation layers into the audible oscillator.
**Attack into Body** stacks them for a different, more complex tone. Routing and
LFO waveform also support right-click automation. **Play C4 / Release C4** auditions
without leaving the synth. Settings, graphs, tuning and routing survive project
save/load, presets and undo, and playback/export share the same DSP.

This is a three-oscillator FM instrument inspired by a bright digital electric
piano; it does not reproduce Roland's MK-80 preset or implement a complete DX7.
Older projects and presets retain their two-oscillator sound: the added Attack
layer is muted, carrier tuning is neutral, and original automation ranges remain.

Default preset folders sit under LibreLoop's configuration directory, normally
`~/.config/libreloop/presets/`, with a folder for Sampler, FM Synth, Chorus and
Equalizer. `XDG_CONFIG_HOME` is respected. The FM folder receives
`Bright Electric Piano.llpreset` on first use; existing user presets are preserved.
Load/Save still remember the last folder chosen. Older projects and presets
retain their previous tone with the new modulation controls initially neutral.

Song playback also animates sounding notes in the Channel Rack and Piano Roll.
The Piano Roll playhead follows the selected pattern's playing Playlist instance,
including clip offsets, and Follow playhead works in Song mode as well.
Piano Roll note placement starts at two steps and remembers the length of the
last note clicked or resized. Hold and drag after placing a pencil
note to move it in time and pitch; drag an existing note's right edge to resize.
Brush placement uses that remembered length too. Movement follows the current grid.

### Copy and paste

Focus the Piano Roll or Arrangement, select items, then use **Edit → Copy**,
**Cut**, **Paste** or **Select all**. The standard shortcuts are Cmd+C/X/V/A on
macOS and Ctrl+C/X/V/A on Linux. Text dialogs retain their text clipboard.

Paste places the group's earliest note or clip at that editor's start marker.
Notes keep pitches, velocities, lengths and relative timing, and can be pasted
into another channel or pattern. Clips keep their sources, trims, offsets and
relative track positions; choose a track header for the destination, or retain
the copied group's original tracks. Pasted items become selected. Cut and Paste
support undo. If space is occupied or capacity is insufficient, the whole paste
is rejected without changing the project. Clip copies stay within the current
project; changing its source list requires copying again.

In Piano Roll Pencil mode, Cmd+Up/Down (macOS) or Ctrl+Up/Down (Linux) moves the
selected notes up/down exactly one octave. Cmd/Ctrl+K goes up and Cmd/Ctrl+J
goes down. The same actions appear in Edit as **Octave up/down**. They preserve
timing, lengths and velocities, support undo, and reject a shift if any selected
note would leave MIDI pitches 0–127 or collide with an existing note.

Right-click a Channel Rack name and choose **Replace instrument…** to switch
between Sampler and FM Synth while retaining notes, channel settings, mixer routing
and sample settings. This changes the shared channel across every pattern. The
inactive sampler's audio is retained so switching back can restore it; automation
specific to FM is removed when switching to Sampler, and Undo can restore the change.

In Arrangement, right-click a pattern clip's title (or its entry in the pattern
picker), choose **Replace instrument…**, then choose the channel and instrument type.
Patterns containing several channels list their instruments individually. Right-drag
on clip bodies continues to erase clips. Audio clips require a Sampler and cannot be
switched to FM Synth; use their sample replacement action instead.

### Six-operator FM and analog engines

FM Synth now starts with **DX7 E.PIANO 1**. Its engine menu selects **Six-op FM**,
**Analog**, or **Custom FM** (the earlier three-operator engine). **Factory** opens
nine sounds: the original DX7 E.PIANO 1, BASS 1, MARIMBA and TUB BELLS patches,
plus MK80-inspired EP, Juno-inspired Pad, Juno-inspired Bass, Chime EP and
Better Chime. The inspired sounds are not exact Roland emulations. Better Chime
uses a tine modulator anchored near 5.5 kHz at middle C, with 25% key tracking,
reduced attack velocity
sensitivity and no attack rate scaling. Its carrier and body follow the played
note fully; the tine layer shifts three semitones per keyboard octave. Load/Save supports
all engines. Old projects and presets retain their original Custom FM engine.

Six-op FM provides six independently tuned sine operators and all 32 DX7 routing
algorithms. Select Op 1–6 for output, coarse/fine tuning, detune, velocity response,
rate scaling and frequency tuning mode. **Ratio** specifies a frequency ratio;
**Hz at C4** specifies the frequency at middle C. Each operator has **Tracking**:
0% keeps its frequency fixed, 50% moves half an octave per keyboard octave, and
100% follows the keyboard normally. Middle C (C4) is the tuning anchor. Switching
tuning modes starts at 100% for Ratio or 0% for Hz; adjust Tracking afterward.
Tracking supports exact values, reset, automation, and project/preset saves.
Older DX presets keep their original full tracking or fixed frequencies. **Envelope** shows four rates and four levels;
higher rates are faster. Drag a graph point to change its rate and target level.
**Keyboard** exposes breakpoint, left/right scaling and modulation sensitivity.
**Global** contains algorithm, feedback, transpose, oscillator sync, LFO and pitch
envelope controls. The factory BASS 1 patch intentionally transposes down an octave.
Knobs share Reset, exact entry and automation. Engine selection is structural and
is not an automation target; changing engines retires existing voices, with the
new engine used by subsequent notes. Oscillator sync controls phase reuse.

Analog provides detuned oscillators with saw, pulse, triangle or sine shapes,
sub oscillator, noise, pulse width/PWM, a resonant 24 dB/octave filter and stereo
chorus. The Amplitude and Filter tabs have independent envelopes. Motion controls
vibrato, tremolo and the LFO used by PWM. Continuous analog controls slew to avoid
hard parameter jumps. Saw and pulse oscillators use PolyBLEP correction; a fixed
low-frequency DC blocker prevents pulse-width offsets consuming output headroom.

Six-op FM is a C adaptation of Dexed's MSFA Modern engine. Factory voice renders
match that reference, rather than claiming a bit-exact emulation of the original
DX7's converters or Dexed's separate Mark I engine. Per-note native LFOs, normal
LibreLoop mixer gain and project timing are retained. Patch data and DSP notices
are listed in [third-party notices](third-party.md).

Dragging a selected note’s right edge resizes every selected note by the same
amount. Dragging a selected Playlist clip’s left or right resize edge adjusts the
whole selection, preserving different lengths and relative spacing. The group
stops when any member reaches its minimum size or a pattern/source boundary.

Piano Roll right-click erases the topmost note at the pointer. Holding or dragging
the eraser does not repeatedly delete notes hidden underneath that note; release
and click again to erase the next overlapping note.

In the Piano Roll velocity lane, left-drag paints velocities across note starts;
fast strokes interpolate between cursor positions. Right-drag previews a straight
velocity ramp from the press position to the cursor. Notes outside the current
ramp retain their original values. Notes starting together share the edited value.
Each stroke is one undo action.

With the Piano Roll focused, Shift+Left/Right moves selected notes by one
sixteenth-note step; Shift+Up/Down moves them by one semitone. Hold an arrow
to repeat. These work with any Piano Roll tool and preserve the selection, note
lengths, velocities and spacing. Moving right can extend the pattern. Boundaries
block the whole move; each nudge supports undo/redo.

Notes may share both pitch and start time when moved or pasted into a stack;
they retain separate lengths, velocities and playback voices. The brush skips
occupied starts to avoid accumulating notes while held. Piano Roll notes show
selection highlights; playing or auditioning highlights the horizontal pitch row
and piano key instead of changing the note fill.

### MIDI keyboards and recording

Open **View → MIDI / Recording** (or right-click Record) to select a USB,
Bluetooth or virtual MIDI input provided by the OS. Refresh lists newly connected
inputs; the selected input reconnects when it becomes available. Input choice and
recording toggles are remembered on this machine. Select an instrument in the
Channel Rack, Piano Roll or instrument editor to play it. Note velocity, sustain
pedal and pitch wheel are supported. Live input supports 32 simultaneous MIDI
voices in addition to computer-keyboard audition.

The three buttons beside Record independently enable **Audio** (wave), **Notes**
(piano), and **Automation** (curve). Audio records red-armed mixer tracks as before.
Notes creates a new pattern and a Playlist clip at the recording origin; notes
and their lengths appear while playing. Automation records learned MIDI CCs,
pitch bend and mouse-dragged automatable controls into separate clips. Stop or
Record finishes the take. MIDI takes use the selected instrument and a fixed
recording tempo; their lanes are temporarily muted to prevent monitoring twice.
Recording runs past the existing song end instead of looping.

Right-click an automatable control → **MIDI Learn**, then move an absolute MIDI
CC knob or fader. **Clear MIDI link** removes its assignments. Links are stored
with the project and update when channels are removed. Escape cancels learning.
Changing the input or disconnecting releases held notes. Recorded notes and
curves can be edited afterward and recording is undoable. A project has eight
patterns and 128 notes per channel/pattern; recording stops and keeps the take
if that capacity is reached. Long controller curves are compacted within the
existing 64-point automation limit. No external plugin hosting is involved.

Mixer faders have independent fill/mark colors, keeping unselected marks visible.
Selected controls choose a contrasting foreground for their accent surface,
including hover states.

Pan and stereo-width knobs show lime on the left and pink on the right, even at
center. Their stronger value arcs show the actual setting. Knob pointers are white with
a thin violet rim so their position remains clear. Lime channel lamps mean enabled, amber means solo, and hollow lamps mean muted.
Amber meters signal higher levels and coral red marks clipping. The measured palette contrast and
saturation are documented in [palette-measurements.md](palette-measurements.md);
run `python3 tools/audit_palette.py` to check future color changes.
