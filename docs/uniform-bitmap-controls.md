# Uniform bitmap text and calibrated displays

Studio captions, labels, button text, fitted captions and instrument numerals
use a single 16-unit size. DotGothic16 replaces Liberation in studio mode; font
atlases use EM metrics and nearest sampling, while legacy text rendering and
its original fixtures retain their existing metrics. Fitting truncates text
instead of shrinking it. Native DPI still controls atlas resolution.

The default floating instrument width is 480 and default Mixer height is 276
logical units to make room for the larger consistent text. Existing custom
window sizes are retained. Short Mixers scroll their effects list with the
wheel; no project slots are deleted. Piano octave plates and velocity legend
were adjusted without changing notes, key hit testing or zoom sensitivity.

Master and EQ spectra now share the meters' interpolated green/yellow/orange/red
palette. Spectrum colors map frequency bins; meter colors map level. All 96
spectrum heights and continuous meter fill remain intact. A line joins current
spectrum values; it has no invented motion or retained peaks. The top display
adds frequency legends and L/R calibration with recessed framing. Transport
numerals retain accurate values without ghost digits. No audio code changes.

Sampler resizing now stops at 420 outer logical units in studio mode, leaving
at least 110 units of waveform even in its narrow layout. This also repairs
undersized restored sampler windows on drawing. FM keeps its existing 420-unit
minimum; legacy sampler minimum remains 300. The shared geometry test checks
all integer widths from 330 through 1920.

The new font adds 2,026,816 packaged asset bytes; no precise GPU/frame-cost
claim is made. The pixel-style font is a TrueType outline font derived from
bitmap design, not an OS font dependency. Software-renderer screenshots and
font bounds tests at 1x, 1.25x, 1.5x, 2x and 3x do not replace a native HiDPI
or hands-on readability check.

Validation on 2026-10-17: Release macOS SDL validation build succeeded; all
138 CTest cases passed, including original audio/input/text fixtures, sampler
geometry and the new bitmap font bounds test. Packaged app passed strict deep
codesign verification. Dummy-video/software-renderer sampler capture:
`../baseline-artifacts/90s-type/sampler-final/libreloop-smoke.png`.
Executable size: 3,761,328 bytes, 304 bytes above the previous bitmap-material
build; the font is a separate packaged asset. Interactive mouse resizing and
native display/DPI appearance remain unverified in this environment.
