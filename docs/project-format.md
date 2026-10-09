# LibreLoop project format

`.llp` is LibreLoop's native project extension. Version 1 is a UTF-8 text
format with named, typed fields and compact runs of repeated values. It stores
project state, not C structs or DSP playback state. Audio remains in separate
files referenced relative to the project directory; Collect samples and save
creates a portable companion directory.

The previous experimental `.hbt` format is unsupported. Renaming an HBT file
does not convert it. Device presets keep their independent `.llpreset` format.

## Records

A file begins with `LIBRELOOP_PROJECT 1` on its own line. Each field has a
header containing its name, type and logical element count, followed by enough
runs to fill that count exactly. A run contains a positive repeat count and
one value. For example:

```text
LIBRELOOP_PROJECT 1
bpm f32 1
1 120
volume f32 32
32 1
paths str 32
1 16:samples/kick.wav
31 6:@empty
```

This fragment is illustrative; a complete file requires every core field.
The final line is `end`. Only whitespace may follow it. Writers use LF;
readers also accept CRLF record endings.

Types are `f32`, `i32`, `u32`, `u8` and `str`. Numeric values use a decimal
point and the C numeric locale. Floating values are finite IEEE binary32
values; nine significant decimal digits preserve them exactly on reload,
including negative zero. Strings use a decimal UTF-8 **byte** length followed
by `:` and that exact number of bytes. Spaces, quotes and colons need no
escaping. Embedded NUL, CR and LF are forbidden.

Arrays are flattened in C index order, with the final index varying fastest.
Nested settings have separate fields, such as `notes.pitch`, `fm.attack_depth`,
`fm.dx7.value`, `eq.bands.frequency` and `automations.points.value`. Runs may
cross array-row boundaries. Struct padding and bytes after string terminators
are never written. Device settings are explicit even for inactive slots so
changing a future factory preset cannot change an existing project's sound.

The complete version-1 schema is the `fields` table in
[src/project_io.c](../src/project_io.c). It defines stable names, types,
counts and string capacities. These names are file-format identifiers.
Internal struct layout can change without changing the format, provided the
schema and mapping are preserved.

## Evolution and validation

Fields can appear in any order. Duplicate fields, missing core fields,
unknown core fields and incorrect types or counts are rejected. Names beginning
with `x-` are reserved for optional non-musical metadata: readers validate and
skip these records, and saving does not preserve them. Instruments, effects,
notes and other musical data must use core fields, never this metadata escape
hatch. Unsupported format versions fail explicitly.

A change to core field meaning, required fields or array capacity requires a
new format version and an explicit reader migration. Existing parameter IDs
must remain stable. This gives future instruments room to grow without
silently discarding settings in older applications.

Readers limit files to 32 MiB, headers to 255 bytes, field counts to 1,048,576,
strings to 1,023 bytes (or the smaller field capacity), and optional metadata
to 64 records. They reject truncated records, invalid runs, numeric overflow,
non-finite values, out-of-range settings and mixer routing cycles. A failed
load leaves the current project untouched. Saving validates first and uses
an atomic temporary-file replacement; failed writes preserve the previous file.
