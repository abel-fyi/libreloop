# Third-party notices

LibreLoop's original source uses GPL-3.0-only. Dependencies keep their own
licenses and copyright notices; they are fetched during the desktop build,
not vendored in this repository.

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
