# LibreLoop development principles

- Keep one user-facing desktop layout. Do not retain old renderers or hidden layout switches for comparison.
- Smoke captures and visual tests must exercise the same UI as a normal launch. If a test needs special state, set up that state explicitly; do not select another UI.
- Prefer fixes in shared code so Windows, macOS, Linux and OpenBSD benefit together. Add OS-specific code only where the operating system API requires it, and keep that boundary small.
- Keep dependencies few, maintained, and straightforward to build on supported platforms.
- When retiring a feature or UI path, remove its state, drawing code, tests, and active documentation together.
