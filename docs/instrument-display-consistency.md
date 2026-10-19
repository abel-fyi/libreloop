# Instrument display consistency

Studio Analog, Custom FM and DX7 envelope plots share a 160-logical-unit height,
12-unit side margins and the same guides, handles, frame and time labels.
At the default 440-unit window width, each plot is 416 units wide. DX7 starts
at 544 units tall to fit its native rate/level controls; Analog starts at 452.
The plots retain their actual envelope shapes and linear time scales. Legacy
non-studio debug views and the old Analog overview retain their own layout.

Motion previews start with one complete cycle at 0.1 Hz and narrow continuously:
cycles = 1 + max(0, rate * 0.5 - 0.05). There is no integer-cycle rounding.
Pitch and Volume select normalized vibrato and tremolo depth respectively;
zero depth draws a flat line. These are modulation previews, not audio waveforms.
LFO attack is the existing per-note modulation fade-in time, not amplitude ADSR
attack. PWM moves pulse width using the LFO and is audible with Pulse oscillators.
The DSP, parameter ranges and saved values are unchanged.

Geometry, continuous LFO rate and zero-depth behavior are covered by
instrument_layout and studio_fm_graph tests. Native macOS smoke captures were
reviewed at 1448×1086. Other platforms and hands-on interaction remain unverified.
