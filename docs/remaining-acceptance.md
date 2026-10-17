# Remaining completion requirements

This document is an acceptance audit, not a reduced rewrite objective. The user
requested continuation through the complete original task; the goal remains
unfinished until its actual requirements are met.

| Requirement | Current evidence | Remaining |
|---|---|---|
| Frozen original and concept references | Original commit5e860696, clean tree,182 pinned hashes | Continue checking immutability |
| Original audio reference | Unchanged64 original scenario outputs, same-toolchain exact PCM/state/export checks | Other CPUs/toolchains and native device timing |
| Complete source behavior inventory | behavior-spec, constants/parameter/API/source inventories | Keep inventory aligned with any binding fixes |
| All original app functionality | Complete carried desktop bodies/services, built macOS app;207-function lexical audit and original-derived real project/preset/asset I/O handler captures | Full native and broader whole-loop acceptance; changed-handler evidence must remain separate from lexical equivalence |
| Interaction feel | Original-derived model and actual desktop captures for notes, rack/ruler, navigation, controls, MIDI, shortcuts, clipboard/history, drops, recording lifecycle, popup text/numeric/name editing, menu navigation and chooser handler flows | Hands-on mouse/trackpad/pinch; whole-loop dialogs with real project services/rendering and remaining replay combinations |
| Audio thread correctness | Callback guards and stress/sanitizer tests on host | Native device deadlines/latency, other-platform atomics |
| Offline vendored static SDL | Verified-local-archive importer and CMake selection prepared | Obtain/import pinned SDL source, build static without network, verify shipping desktop |
| Linux/macOS toolchains | macOS arm64 AppleClang21/SDK15.4/CMake4.4.3 developer path verified | Linux build/run; source-SDL macOS toolchain/minimum deployment |
| GPL and notices | Original/DX7/font/decoder notices retained; developer SDL license bundled | Vendored SDL source/provenance after import |
| Visual direction | Default image-2 classic layout, responsive panes, source picker relocation, owned dialogs, expanded scene and geometry/import regressions; see phase-3-layout-proposal.md | Real-display/HiDPI and hands-on pointer acceptance; no invented art features |
| Windows | Native MIDI/time/file/Unicode/mapping/writer/DPI source candidates, path math checks, install layout prepared | Real Windows compilation/tests, packaging, device/DPI/drop/precision-touchpad verification |
| Idle/active baseline comparisons | Original wait loop audited, same idle threshold/wait design carried; component measurements | Native idle/active CPU/GPU, playing waveform-visible/physical HiDPI measurements |
| Experiments | Four optimization candidates pass89 checkpoint tests, repeated engine/software-render measurements; CPU candidate tests/dirty upload/fidelity measurements | Static/shared SDL source comparison, all unavailable platforms/hardware metrics; user decides adoption |
| Small clean history | Logical commits only in new repository | Maintain reviewable commits and current reports |

The frozen reference contains no count-in state/delay, despite the prompt naming
it. That discrepancy was recorded in Phase0 behavior-spec.md. No count-in has
been added: source remains the behavioral target, and new functionality requires
explicit authorization. The remaining recording audit concerns the implemented
start/stop/take/error paths, not inventing a count-in feature.

Known external constraints: SDL source archive is absent; shell download DNS is
unavailable and the browser download attempt did not yield a local archive.
Native graphical/hardware checks and Linux/Windows toolchains are unavailable in
this execution environment. These are missing evidence/resources, not measured
successes. Independent source/replay work and the concrete layout proposal were
advanced while these constraints persisted. The current completion audit and
specific external gates are recorded in completion-blockers.md; the full goal
is unfinished and is not represented as a shipping/parity success.
