# Using LibreLoop

## First session

The initial project has generated kick, snare, hat and pitched tone samples,
one selected Pattern 1 and an empty Playlist with 100 tracks. Click Play to hear the current
pattern. The stacked PATT/SONG button highlights Pattern in orange and Song in green.
Switch to SONG to hear the Playlist, or click its ruler. Play
and Space start from the ruler marker; stopping returns to it. Editing and
transport use mouse controls, and the Browser retains navigation keys. Text fields
support normal text editing. **Help** lists all current keybindings in a draggable
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
  Clicking a channel name opens its instrument interface. Steps and pitched
  notes belong to the same shared pattern.
- Double-click a Playlist pattern clip to open and focus its Channel Rack.
- Pattern selector in the toolbar: click the pattern name to open the dropdown
  and choose a pattern. While hovering over the selector, Up/Down or k/j
  changes patterns and opens the dropdown; the bottom helper shows these keys. Choose
  **Rename selected pattern**, or right-click the selector to rename.
  Enter applies, Escape cancels, and Ctrl+A selects the name. Names are saved
  with the project and appear in clips and Piano Roll.
  New projects start with only Pattern 1 selected; **+ New pattern** creates a
  blank pattern, up to eight. The top-bar selector always identifies the current
  pattern, including when no Playlist clips are selected. All patterns share
  the same Rack channels and samples.
- Playlist: the icon toolbar offers Pencil, Brush and Select. Hover an
  icon for its description. Pencil places one clip; drag its body to move it
  without painting more copies. Brush paints copies across a track. Click a
  clip to highlight it and choose its pattern and length as the drawing source.
  Select draws a rectangle; drag a selected clip to move the group together.
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
  bars. Existing Playlist copies retain their lengths. The + button stays beneath
  the visible channels, even when the channel list needs scrolling.
  Preview lines show actual notes. Brush spaces longer clips to avoid overlaps.
  Any of 100 tracks can hold any pattern; different tracks play together.
  Playback loops through the final clip edge. Scroll the wheel over the grid
  to zoom around the ruler start marker; drag the bottom scrollbar to pan.
  Use the right scrollbar to scroll vertically.
  Clip resize handles highlight on hover, and the cursor updates once per frame.
- Click or drag either editor's bar ruler to position its downward start arrow.
  Playlist selects SONG playback; Piano Roll selects PAT playback. Space and
  the Play button start from that marker; stopping returns to it. Zoom keeps
  the marker at the same screen position where the timeline origin allows,
  or brings it into view if offscreen.
  Each wheel notch changes the visible range by about 8%, so large projects
  zoom faster while the ruler marker stays anchored. The toolbar Follow button
  keeps the smooth moving playhead in view in Song or PAT mode.
  Horizontal scrollbar thumbs remain at least 24 pixels wide and highlight on hover.
- Snap menus in Playlist and Piano Roll are independent: Auto, Bar, Beat,
  1/2 beat, 1/3 beat, Step (1/4 beat), 1/6 beat, 1/2 step, 1/3 step,
  and 1/4 step. Auto uses a finer grid as you zoom in. Fractional positions
  and lengths are saved; older projects remain readable. Grid divisions follow
  the selected snap, including triplets; dense views show spaced multiples.
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
  Wheel over notes to zoom around the ruler start marker; drag the bottom
  scrollbar to pan across the pattern. Bar numbers appear above the notes. Drag an existing note's right edge to resize
  it; right-click its body to erase it. Dragging a velocity bar changes every
  note that starts at that position. C4 plays the sample at its original pitch;
  other pitches resample it. Notes stop at their duration or the sample's end.
- Mixer: Master and independent inserts with their own volume, pan and
  labelled **M** (mute) and **S** (solo) buttons. Multiple inserts can be soloed;
  soloing a bus includes its routed sources. Master and 100 fixed inserts keep
  stable numbers; right-click an insert header for mute, solo or reset.
  Several instruments can feed the same insert; its controls affect their combined
  signal. Drag an insert's bottom-right output jack to another channel's bottom-left
  **IN** socket to route it there; right-click its output to unplug. Each insert
  has one destination; default is Master. Connections cannot create feedback
  loops. The selected insert shows its cable. Use the wheel over strips or the
  bottom scrollbar to browse inserts. The fixed-width panel on the right has
  ten empty effect slots for the selected channel; plugins are not implemented yet.
- Drop a WAV, FLAC or MP3 onto a visible Rack row to replace its sample.
  Drop into empty Rack space or its bottom add area to create a channel.
  Up to 32 channels can be added; wheel over Rack rows to scroll. Right-click a
  channel name for Piano Roll, mute or delete. Deleting removes its notes in
  every pattern. Samples decode to mono at
  48 kHz, maximum 60 seconds.
- Browser: an expandable folder tree with **LibreLoop samples** as its first
  root. **+ Add folder** adds another root, up to eight, saved between sessions.
  Click a folder to expand/collapse it. Supported WAV/FLAC/MP3 and `.hbt` files
  appear beneath their folders; folders sort before files. Click samples to
  select and preview, or drag them onto Rack channels to load them.
  While Browser has focus, **Up/Down** or **k/j** move through visible nodes and
  preview samples. **Right/l** expands a folder or enters its first child;
  **Left/h** collapses a folder or selects its parent. Trees replace `..`, Home,
  and filesystem-root navigation buttons; add `/` as a root if desired.
  The selected audio sample has a waveform preview at the bottom.
  Scroll the wheel to browse. Drag the Browser's right edge to change its width;
  dragging below its minimum width collapses it to a narrow strip. Drag the
  strip's right edge outward to restore it.
  Dialogs open centred and have draggable title bars. The pattern dropdown
  opens beneath its selector and can also be dragged.
  Added roots persist in `$XDG_CONFIG_HOME/libreloop/folders.txt`, or
  `~/.config/libreloop/folders.txt` when XDG_CONFIG_HOME is unset. Old Homebeat
  folder settings are read when no LibreLoop settings file exists.

- Save writes `project.hbt` in the launch directory. Open reloads it.
  Drop another `.hbt` to open that project; subsequent saves use that filename.
- WAV exports the entire Playlist to `song.wav`: 48 kHz, stereo PCM16.

Sample paths are stored as absolute paths; keep imported files available when
reopening projects. Generated demo sounds need no external files. Save/load
and export report errors in the status bar. Save and export replace their
existing target files. Project writes use a temporary file and rename.
Dialogs open centred and remain draggable. Hover controls to see their function
and relevant optional keys in the bottom helper.

Project format version 15 saves polyphonic notes, durations, pattern names,
insert settings, routing, source lengths and individual clip lengths, master pitch, fractional BPM, insert outputs, pattern
count, channel names/count, fractional note/clip timing, 100-track clips and sampler processing controls. Versions
1–14 still load; older projects keep their previous one-bar clip lengths.

## Editor windows

- Drag a title bar to move a window, or click a window to bring it forward.
- Drag the bottom-right corner to resize it. The square maximizes/restores;
  the cross hides it.
- Right-click a title bar for Stay on top, Maximize/Restore, or Hide.
- Toolbar order is Rack, Piano Roll, Playlist, Mixer. Clicking brings a window
  forward; clicking it again while it is in front hides it.
- Stay-on-top windows retain their own stacking priority. Window positions
  currently reset on launch.

## Instruments and keyboard audition

- Click a Rack instrument name to open its movable sampler window, with
  waveform, volume and pan. Click the waveform to preview it; a playback line
  moves through it. The + beneath the final Rack channel opens a movable instrument picker
  containing Sampler. Selecting it adds an empty channel; drag a Browser
  sample onto its window to load it. Empty samplers persist when saved. A ghost row beneath the final channel
  shows where a dropped sample will be added; the Rack scrollbar browses channels.
  Future instrument types will open their
  own interface here. Right-click a channel and choose Go to Piano Roll to
  assign that editor; selecting another Rack channel keeps its notes unchanged.
- The toolbar Keys switch enables polyphonic audition of the selected channel.
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
The waveform previews the cropped source envelope live while dragging Start
or Length; reverse, polarity and normalization also update immediately. Pitch and Time retain the last processed waveform until the new audio is ready.
This lightweight crop envelope preview is
marked Preview until the exact processed waveform is ready; pitch/stretch
detail requires processing. Playback and WAV export use the processed audio.
Knob processing starts on release in a background worker,
and the original sample file is never modified. Stretch/pitch use a compact
WSOLA implementation, so complex material and extreme settings may have
audible artifacts.

