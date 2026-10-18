# Compact instrument and effect layout audit

The review uses software-rendered studio captures at the normal 1448×1086
window size. Capture scenes cover Custom FM, Six-op FM operator/envelope,
keyboard and global pages, Analog Sound/Amplitude/Filter/Motion, Chorus,
Equalizer, Delay, Tube, Reverb, Multiband Compressor, Phaser and Flanger.
The sampler mode/name cleanup was reviewed in the preceding dropdown pass.

## Corrected

- Compact FM preset fields previously crossed the PAN/VOL/PITCH captions. The
  preset now occupies the spare left side of the channel-controls row, with the
  engine selector directly below it. Channel names remain in the title bar.
  Long preset names have their complete text in the hover status. Custom FM
  shows the actual sound name instead of a clipped static engine description.
- Six-op envelope and global labels overlapped adjacent columns at the shared
  bitmap font size. Short studio captions such as Rel. rate, Rel. level, Vel.
  and Track fit their columns; caption/value fitting enforces a column margin.
  Full parameter descriptions remain available through the existing knobs.
- Pitch/volume motion captions are shortened in compact studio rows. Custom
  routing uses B + A with a full routing explanation on hover.
- Custom FM now uses the same three evenly spaced Factory/Load/Save buttons as
  Six-op and Analog, replacing its short left-aligned footer cluster.
- Analog Amplitude had a heading-only 68-unit row before its envelope controls.
  Its controls move from y=207 to y=139, and the envelope begins at y=184 instead
  of y=252. The recovered 68 units enlarge the graph; the window/footer stay put.
- Master now has the requested functional pan control beside stereo width.

The FM preset/engine selectors, Custom FM footer and Analog Amplitude controls
have new hit coordinates. Envelope editing still derives positions from its
actual plot rectangle; the Amplitude plot is taller. Existing snap, modifier,
parameter ranges and drag sensitivity formulas are unchanged. No interaction
assertions or audio-reference fixtures were weakened or replaced.

## Retained

The current compact Chorus, Delay, Tube, Reverb, Phaser and Flanger panels have
clear control rows and compact footers. Equalizer and Multiband Compressor keep
their established graph/control proportions. No decorative controls were added
to fill blank space. Synth tabs keep one stable window size so switching pages
does not make the window jump. The legacy/tall presentation remains available
and was not restyled.

## Verification and limits

The complete 158-test Release suite passes, including frozen same-toolchain
audio fixtures, automation, persistence, input replays and real-time callback
checks. After the final tooltip/formatting cleanup, targeted engine, automation,
MIDI, instrument layout, FM graph and literal-preservation checks were rerun.

Before/after software captures were inspected, including a 900×562 window with
200% requested UI scale (the existing safe-scale limit still applies). The
compact FM controls and footer fit. Native Retina rendering, physical-pointer
feel and listening to Master balance on hardware remain hands-on checks. No
new hardware performance measurements are claimed.
