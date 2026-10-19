# Compressor curve magnet

Magnet toggles grid snapping for inserted and dragged compressor curve points.
It starts off and is an editor preference per effect window, not project data.
Toggling it does not move existing points. Both axes use the same intersections
as the displayed grid, including its quiet boundary. Nearest means distance on
the displayed nonlinear dB axis.

Ctrl locks input and Shift locks output, with either keyboard side accepted.
Endpoints retain fixed input positions. Interior points retain their ordering
and 0.25 dB minimum separation; if no grid intersection fits between neighbors,
the input stays put. Bend handles remain free. With Magnet off, existing point
movement and integer-dB insertion behavior remain unchanged.

The dynamics tests cover grid values, snapping, locks, endpoints, crowded points
and unrestricted movement. The toggle is outside the curve hit area. Native
macOS captures were reviewed; hands-on editing and other platforms are unverified.
