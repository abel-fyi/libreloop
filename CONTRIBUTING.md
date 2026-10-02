# Contributing

Read the [development guide](docs/development.md) and
[architecture and scope](docs/architecture.md) before changing the code.

Keep changes focused. Prefer plain C and existing raylib primitives; add a
library only when it meaningfully reduces the code we must maintain. Preserve
project compatibility and keep allocation and file I/O out of the audio callback.
Built-in instruments and effects are the planned extension path.

Follow the surrounding code style. Verify the affected workflow manually for
UI changes; run CTest for changes to the engine, sampler, Browser, window manager,
or build setup. Add regression tests for meaningful behavior rather than
mirroring implementation details. Mention what changed and how it was checked
in pull requests. Do not include private projects, imported samples, local
reference material, build outputs, or generated diagnostic screenshots.

Issues should include reproduction steps, expected and actual behavior, Linux
distribution, desktop/display backend, and relevant build output. Include a
small project or screenshot only when it helps reproduce the problem.

Contributions to LibreLoop's original source use GPL-3.0-only, as described in
[LICENSE](LICENSE). Keep third-party license notices intact.
