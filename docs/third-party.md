# Third-party notices

LibreLoop's original source uses GPL-3.0-only. Dependencies keep their own
licenses and copyright notices; they are fetched during the desktop build,
not vendored in this repository, except the adapted DX7 core described below.

| Dependency | Version/source | License notice |
| --- | --- | --- |
| raylib | [5.5](https://github.com/raysan5/raylib/tree/5.5) | [zlib/libpng license](licenses/raylib.txt) |
| GLFW | Bundled in raylib 5.5 | [zlib/libpng license](licenses/glfw.txt) |
| miniaudio | [0.11.21](https://github.com/mackron/miniaudio/tree/0.11.21), bundled in raylib | [Public domain or MIT No Attribution](licenses/miniaudio.txt) |

The full notices above are copied from the corresponding upstream versions.
The fetched source also includes utility, image, and audio-decoder headers
with their own notices. Keep those notices with the dependency source when
redistributing it. raylib's audio module is disabled; LibreLoop uses miniaudio
directly for playback and decoding. System libraries are linked from the host.

The bundled Kick, Snare, Hat and Tone WAVs are generated from LibreLoop's own
synthesis code rather than third-party sample packs.

The UI bundles Liberation Sans Regular 2.1.5, copyright Google Corporation
and Red Hat, Inc., under the SIL Open Font License 1.1. Its full notice is in
[assets/fonts/LICENSE](../assets/fonts/LICENSE). Font support uses raylib's
existing TrueType loader; no additional library is required.

## DX7 / MSFA adaptation

`src/dx7.c`, `dx7.h` and `dx7_tables.h` adapt the Apache-2.0 MSFA Modern engine
from [Dexed](https://github.com/asb2m10/dexed/tree/2e182b3db85c09083ab13c8b9b00565ce7d9ff85/Source/msfa),
reference commit `2e182b3db85c09083ab13c8b9b00565ce7d9ff85`, copyright
2012–2013 Google Inc. and 2017 Pascal Gauthier. The C adaptation uses bounded
48 kHz state, unsigned phase arithmetic and no JUCE, MIDI-hosting or C++ runtime.
The original license is included in [msfa-Apache-2.0.txt](licenses/msfa-Apache-2.0.txt).

`src/dx7_factory.h` contains voice programming parameters for four Yamaha DX7
ROM1A sounds: E.PIANO 1, BASS 1, MARIMBA and TUB BELLS. The source bank is
[ROM1A.SYX](https://github.com/mmontag/dx7-synth-js/blob/master/roms/ROM1A.SYX).
These are parameter values, with no sampled audio or ROM firmware. MK80 and Juno
preset names describe LibreLoop's own inspired variations; no Roland samples,
firmware or ZENOLOGY content are included.

Native MIDI links Apple CoreMIDI on macOS and the system ALSA userspace library
(LGPL-2.1-or-later) on Linux; neither implementation is vendored.
