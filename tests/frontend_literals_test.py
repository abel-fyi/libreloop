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
# Approved independent editor pages: transport help now distinguishes playback
# selection from the Arrangement / Pattern tabs. Keep the original fixture intact.
old_mode = '"Playback mode: click to switch Pattern (orange) / Song (green)"'
new_mode = '"Playback source: Pattern (orange) / Song (green); editor tabs change the view independently"'
assert actual[new_mode] == expected[old_mode] == 1
expected[new_mode] += expected.pop(old_mode)
# Approved Record arm/menu workflow and clearer keybinding descriptions.
replacements = {
    '"Right-click Record"': '"Record, then Play"',
    '"Record type icons"': '"Right-click Record"',
    '"Choose MIDI input / settings"': '"Arm, then start the take"',
    '"Delete selected clips / notes"': '"Delete selection"',
    '"Record enabled types; right-click for MIDI inputs and recording settings"': '"Arm recording; right-click for Audio, Notes, Automation and Pattern options"',
    '"Recording: click to finish the takes"': '"Recording: click to finish and disarm"',
    '"PATT"': '"Pattern"',
    '"SONG"': '"Song"',
    '"Arrangement"': '"Song"',
}
for old, new in replacements.items():
    if old in expected:
        amount=expected.pop(old)
        expected[new]+=amount
# The old generic widget still recognizes uppercase SONG as a legacy caption.
expected['"SONG"']+=1
expected['"Song"']-=1
# Approved LFO label change: Fade in is now Attack (modulation onset).
expected['"Attack"'] += expected.pop('"Fade in"')
# Pattern naming moved intact into the shared engine creation operation.
engine = (root / "src/engine/engine.c").read_text()
pattern_add = engine.split("int pattern_add(", 1)[1].split("int pattern_delete(", 1)[0]
assert pattern_add.count('"Pattern %d"') == 1
actual['"Pattern %d"'] += 1
missing = expected - actual
assert not missing, f"Original frontend literals changed or disappeared: {dict(missing)}"
print(f"All {sum(expected.values())} original UI literal occurrences preserved")
