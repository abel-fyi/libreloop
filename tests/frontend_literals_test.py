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
missing = expected - actual
assert not missing, f"Original frontend literals changed or disappeared: {dict(missing)}"
print(f"All {sum(expected.values())} original UI literal occurrences preserved")
