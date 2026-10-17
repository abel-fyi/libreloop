# Six-op instrument proportions from 90s.png

This follows the shared skin and caption/control checkpoints. Arrange/Pattern
switching remains the user-approved modern workspace. No old stacked layout or
AI-invented synthesizer controls are added.

The earlier six-op editor allocated nearly all extra window height to its
envelope, leaving a dense control area above a very tall plot. It now uses a
concrete, renderer-independent `SixOpLayout` in
`src/interaction/instrument_layout.h`. At the reference's 376×654 outer size,
the caption translation supplies a 640-unit content frame:

| Element | Content coordinate | Physical offset from outer window |
| --- | ---: | ---: |
| Preset and engine choice | 72 | 86 |
| Operator tabs | 108 | 122 |
| Operator heading | 143 | 157 |
| Operator knobs | 184 | 198 |
| Envelope/Keyboard/Ratio | 244 | 258 |
| Rate controls | 298 | 312 |
| Level controls | 370 | 384 |
| Envelope graph | 432 | 446 |
| Envelope graph height | 152 | 152 |
| Footer buttons | 596 | 610 |

The table corresponds to the real controls present in the reference. For taller
windows, rows below the first channel header use
`scale = (content_height - 94) / 546`, anchored at content y=52. The graph ends at
`content_height - 56`. Footer buttons occupy 34 units, start at
`content_height - 44`, and retain a 10-unit bottom margin. Thus the graph and
control spacing grow together instead of assigning all extra height to the
graph. Operator keyboard scaling, global motion and pitch-envelope pages use
the same row positions. All original fields and modes remain accessible.

The spacious layout applies at content widths ≥360 and heights ≥600. Narrower
or shorter windows retain the carried compact row positions, 10-unit knobs and
24-unit footer buttons. Legacy rendering always retains its original layout.
There is no forced window resize or new per-instrument size preference.

The spacious controls use 12-unit knob radii, larger captions/value labels and
clear separation between each knob, caption and value. `knob_sized` shares the
existing input and angle formulas; the legacy `knob_style` wrapper supplies its
original 8/10-unit radii. The 160-unit drag sensitivity, wheel increments,
logarithmic mapping, fixed-coarse aliases, reset/automation/MIDI actions and
envelope formulas are unchanged. The larger knob hit areas and moved control
positions are geometry changes authorized by the reference-proportion request.

The shared engine selector follows the same preset row. Footer Factory, Load,
Save and Play C4 buttons share the reference's larger height and consistent
spacing. These remain the original operations, without new synthesis behavior.

## Verification and limits

All 137 CTest cases passed after integration. A subsequent label-size adjustment
was checked with the relevant layout, graph, font, original text-cache,
original knob-control and frontend-literal cases. The new `instrument_layout`
test checks the reference coordinates exactly, retains legacy positions exactly,
and checks bounds/separation over widths 330–640 and heights 406–1200.
The real font test also verifies Tracking, Release rate and Sustain level fit
their columns at 1, 1.25, 1.5, 2 and 3x density. Original audio, project and input
fixtures and their behavioral assertions remain unchanged.

Real application software captures were inspected under
`../../baseline-artifacts/90s-sixop/`: `full`, `reference-size`, `compact`,
`keyboard`, `global`, and `pitch`. The reference-size capture uses a 1448×738
application window, which naturally constrains the floating tool to the
reference's 376×654 size; there is no special alternate renderer or screenshot
only layout. Curves show real preset data, not a traced imitation of the image.

The release executable is 3,744,176 bytes, +64 from `42a9f99` (3,744,112).
This is a total binary measurement. No new asset, dependency, idle animation or
audio-thread work is added. Isolated rendering cost, GPU cost, native HiDPI and
physical pointer-device verification remain unavailable for this checkpoint.

The overall visual goal remains active. Consistent popup captions/fields,
remaining view details and active-audio visual/performance review are still to
be completed; this does not certify exact final parity.
