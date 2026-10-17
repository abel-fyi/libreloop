# Frozen reference

- Original directory: `../libreloop/`, read-only.
- Commit: `5e860696fe6cb6cd4c170f6ebf5378039b3e4af6`.
- Initial `git status --short`: empty; clean tracked and untracked working tree.
- Session date: 2026-10-10, Europe/Madrid.
- Actual concept directory: `../ai-screenshots/`, containing `1.png` and `2.png`; the prompt calls this `screenshots/`.
- All builds use `../baseline-build/`; no configuration, harness, test artifact or formatting operation runs in the original directory.
- Core CTest runs use the out-of-source build working directory. Audio capture runs use `../baseline-artifacts/audio/`.
- Original and concept-file SHA-256 manifests: `../baseline-artifacts/reference-files.json`. End-of-session status is recorded separately in the Phase 0 report.

The source at this commit is authoritative when usage documentation or the task's illustrative feature list differs. Potential quirks are preserved pending user decisions; no original bug has been fixed in this phase.
