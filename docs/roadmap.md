# Development priorities

LibreLoop's current development order is:

1. Finish consistency and accessibility of shared controls while preserving
   interaction behavior and the existing regression fixtures.
2. Settle Mixer layout and navigation before expanding the built-in effects.
   Project assets, routing and automation must remain compatible.
3. Add built-in effects incrementally, each with real-time-safe DSP,
   deterministic render tests and explicit UI/parameter design.
4. Refine instruments with measured reference behavior and openly licensed
   assets. Do not claim hardware-identical output without validation.
5. Keep shared controls small and consistent inside LibreLoop. A separate Luice
   UI library is no longer planned; refine concrete controls through the DAW.

Shipping work continues alongside those priorities: verified vendored/static
SDL builds, platform/toolchain coverage, and hands-on audio/MIDI, trackpad and
HiDPI acceptance. Source, licenses and original expected-result fixtures stay
in the repository. Private notes and detailed local planning remain ignored.
