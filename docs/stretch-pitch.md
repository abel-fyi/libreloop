# Duration-preserving Stretch pitch

The requested behavior replaces the original pitch-dependent Stretch duration.
Stretch advances its nominal source cursor using tempo only; grain reads use
the combined note, sampler playback and master pitch. It applies even without
Fit to tempo. Resample keeps its pitch/speed relationship. Existing offline
sample processing remains unchanged.

Clip lengths, offsets, waveform scaling, trimmed/cut placements, seek position
and pitch-automation integration use duration-preserving source time in Stretch.
Song note seeking and end fades use that same cursor rate. Laptop-keyboard
voices use the shared instrument processor. Single-note sample previews now use
the same granular reader in Stretch, with fixed gate duration and a source-time
preview cursor; master pitch and sampler playback pitch affect their sound.
No new dependency, callback allocation, or file format change is involved.

`stretch_pitch_test` checks sampler/master/note octave shifts against a known
440 Hz source: approximately 880 Hz output and exactly one source second of
cursor travel per output second in Stretch, versus two in Resample. It also
checks unchanged Stretch clip length/seek geometry under manual pitch and pitch
automation. Existing tempo, clip, trim and pitch-automation tests remain active.

The immutable Phase 0 feature scenes 3 and 4 encode the old behavior and
necessarily differ in rendered PCM/export WAV. The CTest comparator opts into
`--stretch-duration-contract` for those two scenes only, deferring audio
correctness to the explicit duration/frequency contract above. It still checks
their frozen file hashes, finite float output, PCM lengths, WAV channels/rate/
format and every source/project/timing file. All other scenes retain their
original exact PCM, timing and export comparisons. Reports mark the two
approved scenarios' audio files separately; no frozen expected file was edited.
Running `tests/compare_fixtures.py` without that flag still performs the original
strict comparison and reports the intentional Stretch difference.

Release build and seven focused audio/reference checks passed. Hardware audio,
preview gesture feel and subjective granular sound quality remain unverified.
