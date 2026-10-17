# Laptop-keyboard recording and count-in — 2026-10-17

## Completed

Notes recording now accepts the enabled laptop typing keyboard without requiring
an external MIDI device. Presses and releases use the same take timing/length
formulas as external MIDI. The new production typing path captures its target
channel, releases that same channel even if focus/selection changes, and handles
focus loss, modal blocking and disabling typing. Default recording selection for
new preferences is Notes; existing saved Audio/Notes/Automation selections remain.

Typing holds use caller-owned note slots, separate from all sixteen external MIDI
channels. Playing the same pitch on a laptop and external MIDI keyboard therefore
does not prematurely terminate either note. Keys held when capture begins are
seeded at take time zero. Stopping recording finalizes their durations even if
physical keys have not yet been released. The original audition-only typing
entry point remains for unchanged reference replays; the app uses the timestamped
production entry point. Project format and DSP are unchanged.

The optional visual count-in uses whole sixteenth-note steps: one step lasts
15/BPM seconds, four steps are one beat and sixteen steps are one bar of 4/4.
It is off by default, with 16 steps selected; whole values 1–4096 are accepted.
There are no added click sounds. The separate metronome retains its playback role.

- Click `IN 16` beneath the metronome to enable/disable count-in.
- Right-click that button to enter the step count, or right-click Record to open
  MIDI / Recording, with explicit Count-in and steps buttons.
- Record starts the countdown. The transport numeric display shows remaining
  steps with a COUNT-IN STEPS caption. Stop, Record again or Escape cancels.
- No take/pattern/audio file is prepared until the countdown finishes. Cancelling
  does not leave an empty take or alter the project. Existing keyboard audition
  can continue during the lead-in; held keys are captured from take time zero.
- If Record is pressed during Song playback, playback pauses for the count-in and
  capture starts at the Song position captured when Record was pressed. Audio
  and Notes use the same captured origin. Without count-in, the original immediate
  recording path remains.
- The count-in enable state and steps persist in MIDI preferences, not projects.
  Existing three-line settings remain readable; empty device-ID/name lines are
  preserved when reading the additional count-in line.

The metronome's previous toolbar rectangle is now two vertically stacked controls
in studio mode. This is the intentional geometry change needed for the visible
count-in button. Its settings are locked during countdown/capture. The original
blocking idle loop is retained; active count-in requests normal frame updates,
then returns to ordinary waiting when idle.

## Verified

Deterministic tests exercise note start/duration timestamps, held keys at capture
start/stop, focus loss, frozen recording targets, simultaneous external MIDI at
the same pitch, count-in math at fractional beats and arbitrary step counts,
invalid values, actual frontend start/poll/cancel without hardware, combined
Audio/Notes origin alignment with controlled writer/device services, and old/new
settings parsing plus actual atomic-save round trip. Original MIDI/recording
fixtures remain unchanged, including hardware-only failure cases.

Actual desktop captures with SDL dummy video/software renderer were inspected at
1448x1086 and 900x562. Artifacts:
`../baseline-artifacts/keyboard-recording/{count-in,settings,compact}`.
The development flag `--smoke-count-in 16` sets up the countdown capture.

Release executable is 3,743,248 bytes versus 3,726,272 at 310bafb, an increase of
16,976 bytes. This measures the complete executable, not isolated component size.
No additional dependency, font, bitmap, audio callback allocation or audio-thread
work was introduced. Capture/bookkeeping runs on the UI command producer.

## Unverified / timing limits

Native physical keyboard/device playback, audible monitoring, input latency and
real hardware MIDI interaction remain unverified in this environment. Keyboard
notes are timestamped when the UI processes their frame; countdown capture begins
on the first UI update at/after its monotonic deadline. Tests verify deterministic
state/time math, not sample-accurate native input latency. No new active/idle CPU,
GPU or battery measurements were taken, and no performance improvement is claimed.
The existing system SDL macOS deployment-version warning remains unresolved.

Final checks: Release build, all 135 CTest cases, app signature verification and
`git diff --check` passed. The read-only original working tree remains clean.
