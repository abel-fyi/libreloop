# Pattern samples in Audio Clips

Audio Clips lists every sampler channel with a sample path, as well as channels
in the existing Audio group. Samples imported into Pattern, the Rack or a
Sampler therefore become arrangement sources automatically. Existing projects
get the same behavior when opened; no conversion or new saved field is needed.
Empty samplers and synth channels are excluded. Missing sample paths remain
listed for relinking.

An entry shares its channel, sample settings, mixer route and sample storage
with Pattern playback. No duplicate channels or decoded sample buffers are
created. The channel keeps its Audio/Unsorted group and voice settings. Editing
or deleting the source affects both uses. The deletion tooltip calls out Pattern
notes for shared samplers. Replacing the instrument with a synth is unavailable
while arrangement audio clips reference that sampler, as for Audio-group channels.

Audio clip persistence and clipboard validation accept these sources. Tests cover
playback, seek, crop, sequencing, persistence, export and copying Pattern sampler
clips, plus preservation of notes and voice settings and replacement safeguards.

The obsolete Back to Arrangement button has been removed from Pattern's channel
list; PATT/SONG remains the workspace switch. The top-bar audio spectrum no longer
has decorative grid lines. EQ frequency guides and compressor editing grids remain.
