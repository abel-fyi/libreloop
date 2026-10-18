# Pattern capacity

Projects support 64 patterns, increased from the inherited eight-pattern cap.
The picker and tests use the same `pattern_add` engine operation for blank notes,
unique default names and automatic colors. Deletion shifts later patterns and
updates their arrangement clips. The final available slot can be saved, loaded,
played and highlighted normally; the next add is rejected without changing the
project. Capacity remains fixed rather than unbounded.

## Project files

Version 1 files reserve eight pattern source IDs followed by audio and automation
sources. Loading them expands the note/name/color/length arrays and remaps audio
and automation clip IDs into the 64-pattern namespace. Saving a project that
fits the old eight-pattern representation retains the original version 1 field
sizes and source IDs. Existing frozen project fixtures remain byte-identical.

Projects using extra pattern slots or modified reserved slot data use version 2,
with 64-pattern arrays and the new source IDs. These files are not readable by
older builds. All current source IDs fit in the existing byte-sized clip storage;
a compile-time assertion checks that constraint. No audio sample, instrument or
effect parameter encoding changed.

## Storage and real-time behavior

On the verified macOS arm64 build, `sizeof(Project)` increases from 1,624,344 to
4,379,992 bytes. This is reserved project storage, not a measurement of process
RSS. Undo snapshots retain metadata plus nonzero note rows, omitting empty rows
without changing the restored project state. The existing 64-entry/256 MiB undo
limits remain. Dense projects or retained PCM can still reach the byte budget
before the entry limit, as before. Loading assets uses a heap temporary to avoid
nesting multi-megabyte project objects on the stack.

Published note highlights use packed bits internally. The complete 64-pattern
highlight array occupies 32,768 bytes, matching the previous eight-pattern byte
array; its callback clear size therefore does not grow. The UI expands the
selected pattern into the existing note-activity interface. No allocation or
lock was added to the callback. Larger project synchronization costs have not
been separately measured on hardware.

The UI's large empty state now stays in zero-fill storage, with small defaults
set explicitly at startup. This avoids embedding the mostly empty project
objects in the executable. The current Release executable is 1,553,488 bytes,
versus 5,739,648 bytes before this task; runtime project capacity is larger even
though the file is smaller.

## Verification

All 158 Release tests pass. Engine tests add through pattern 64, reject pattern
65, round-trip its notes and clip, delete an earlier pattern and reuse the final
slot. Format tests round-trip both version 1 audio/automation clips and expanded
version 2 projects. History tests retain the original undo depth for small
projects and restore notes in pattern 64. Audio tests check highlights across
bit boundaries and note 128 in pattern 64, including clearing after Stop.

Address/undefined-behavior sanitizer checks pass for engine, project format,
history and the real callback/mailbox tests. A frontend smoke capture also creates
twelve patterns and verifies version 2 output. Native Windows/Linux execution
and hands-on pattern editing on those platforms remain unverified.

Frozen replay fixtures were not edited. Their capture harnesses retain the
original eight-slot ruler/selection output width and translate shifted audio
source IDs back into the old namespace. This keeps the existing assertions
unchanged; separate tests cover the expanded slots. The UI literal audit now
checks the unchanged default naming string in its new engine location.
