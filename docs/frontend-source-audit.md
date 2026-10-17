# Carried frontend source audit

This supplements runtime regression checks. It does not prove behavioral parity
of a replaced platform API, an extracted handler or an entire editor.

Run without writing to the original repository:

```sh
python3 tools/audit_frontend_source.py ../libreloop \
  ../baseline-artifacts/harness/frontend-api-map.json \
  ../baseline-artifacts/frontend-source-audit
```

The tool requires frozen original commit
`5e860696fe6cb6cd4c170f6ebf5378039b3e4af6` and a clean original working tree. It
rejects output inside that repository. It records rewrite commit/status, every
identifier normalization and every matched function, then writes source diffs
for bodies that differ. Comments/whitespace are ignored; strings, operators and
numeric spelling remain significant. This is a lexical function audit of the
carried C files, not a compiler/preprocessor or semantic equivalence checker.

At rewrite commit `92f8e97`, it recognizes207 original function definitions:

| Result | Count | Interpretation |
|---|---:|---|
| Token-equivalent after explicit boundary/type/function renames |157| Same body tokens; platform equivalents still need their own verification |
| Different bodies requiring separate review/evidence |49| Includes extracted handlers, renderer/font adapters, resources, main-loop integration and OS portability |
| Original name/body absent from carried frontend |1| Original GLFW mouse callback is replaced at the platform input boundary |

The API/type map is an audit input, retained in the output; it is not an assertion
that API semantics are identical. For example, PATH_MAX normalization is only
valid because path.h separately preserves1024 on macOS and4096 elsewhere.
Raylib text metrics, timing, DPI and event delivery still need the dedicated
checks described in raylib-dependencies.md.

Eight handlers that initially appeared absent have explicit renamed wrappers:
four Piano handlers, three Rack helpers and ruler_press. Their extracted models
and desktop bindings have separate original-derived replay checks. The remaining
mouse callback increments the original pending-click counters; its replacement
is interaction/input.c's pending/frame click handling, fed by SDL events and read
by main.c. Existing original input captures check deferred presses and queue
behavior; native event delivery and physical timing remain unverified.

The file-picker drawing function differs only by the explicit draw-context
argument after normalization: its carried click/wheel/key/double-click/scrollbar
branches remain present. Its filesystem and text helpers have their own tests.
This does not prove an entire open/save/overwrite/cancel lifecycle or native
clipboard/IME behavior. Similarly, clip preview's render-target source-height
sign differs because the SDL target orientation differs from the original GL
target; rendering evidence and coordinate conventions must justify that change.

Current runtime checkpoint:105 developer-SDL tests and86 SDL-free tests pass.
Do not substitute this source audit or those component counts for whole-loop,
native device or hands-on feel acceptance. Remaining requirements are tracked
in remaining-acceptance.md.
