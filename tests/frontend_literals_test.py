#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Original UI literals survive API renaming and incremental handler extraction."""
from collections import Counter
import json
from pathlib import Path
import re
import sys

root = Path(sys.argv[1])
fixture = json.loads((root / "tests/fixtures/interaction/frontend-literals.json").read_text())
# Skip comments and preprocessor include paths; preserve literal escape spelling.
tokens = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"')
actual = Counter()
for directory in ("src/ui/frontend", "src/interaction"):
    for path in (root / directory).glob("*.c"):
        source = path.read_text()
        for token in tokens.finditer(source):
            if not token.group().startswith('"'):
                continue
            prefix = source[source.rfind("\n", 0, token.start()) + 1:token.start()]
            if not prefix.lstrip().startswith("#include"):
                actual[token.group()] += 1
expected = Counter(fixture["literals"])
for literal in fixture["resource_path_exceptions"]:
    del expected[literal]
# Approved transport change: Space preserves tails; the old tooltip claimed panic.
old_stop = '"Play from ruler / stop all audio, including previews (Space)"'
new_stop = '"Play from ruler / stop sources, preserving effect tails (Space)"'
assert actual[new_stop] == expected[old_stop] == 1
expected[new_stop] += expected.pop(old_stop)
# Pattern naming moved intact into the shared engine creation operation.
engine = (root / "src/engine/engine.c").read_text()
pattern_add = engine.split("int pattern_add(", 1)[1].split("int pattern_delete(", 1)[0]
assert pattern_add.count('"Pattern %d"') == 1
actual['"Pattern %d"'] += 1
missing = expected - actual
assert not missing, f"Original frontend literals changed or disappeared: {dict(missing)}"
print(f"All {sum(expected.values())} original UI literal occurrences preserved")
