#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only structural audit. Token equivalence is not runtime parity proof."""
import argparse
from collections import Counter
import difflib
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("reference", type=Path)
parser.add_argument("api_map", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
rewrite = Path(__file__).resolve().parents[1]
reference = args.reference.resolve()
commit = subprocess.check_output(["git", "-C", str(reference), "rev-parse", "HEAD"], text=True).strip()
if commit != "5e860696fe6cb6cd4c170f6ebf5378039b3e4af6":
    parser.error("Reference is not the frozen original commit")
if subprocess.check_output(["git", "-C", str(reference), "status", "--porcelain"], text=True):
    parser.error("Reference working tree is not clean")
output = args.output.resolve()
if output == reference or reference in output.parents:
    parser.error("Audit artifacts must remain outside the original repository")
mapping = json.loads(args.api_map.read_text(encoding='utf-8'))
mapping.update({
    "Vector2": "DrawPoint", "Vector3": "DrawHSV", "Rectangle": "DrawRect", "Color": "DrawColor",
    "Texture2D": "HostTexture", "Texture": "HostTexture", "RenderTexture2D": "HostTarget",
    "Image": "HostImage", "Camera2D": "HostCamera", "PATH_MAX": "LL_PATH_CAPACITY",
    "pthread_create": "host_thread_create", "pthread_join": "host_thread_join",
    "remove": "platform_file_remove", "fopen": "platform_file_open", "realpath": "host_realpath",
    "rack_channels": "desktop_rack_channels", "rack_channel_at": "desktop_rack_channel_at",
    "ruler_press": "desktop_ruler_press",
    "piano_inactive": "desktop_piano_inactive", "piano_gesture_update": "desktop_piano_gesture_update",
    "piano_note_drag_update": "desktop_piano_note_drag_update", "piano_key_update": "desktop_piano_key_update",
    "rack_melodic": "desktop_rack_melodic",
})
for match in re.finditer(r"#define (HOST_\w+)", (rewrite / "src/ui/frontend/host_constants.h").read_text(encoding='utf-8')):
    mapping[match[1][5:]] = match[1]
ignored = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
lexer = re.compile(ignored + r"|[A-Za-z_]\w*|\s+|.", re.S)

def functions(source):
    masked = re.sub(ignored, lambda match: re.sub(r"[^\n]", " ", match[0]), source)
    result = {}
    for match in re.finditer(r"^[A-Za-z_][^;{}\n]*?\b(\w+)\s*\([^;{}]*\)\s*\{", masked, re.M):
        end, depth = match.end(), 1
        while end < len(masked) and depth:
            depth += (masked[end] == "{") - (masked[end] == "}")
            end += 1
        if depth:
            raise ValueError(f"Unbalanced function: {match[1]}")
        result[match[1]] = source[match.start():end]
    return result

def tokens(source, original=False):
    result = []
    for match in lexer.finditer(source):
        token = match[0]
        if token.isspace() or token.startswith(("//", "/*")):
            continue
        result.append(mapping.get(token, token) if original else token)
    return result

rows, differences = [], []
for path in sorted((rewrite / "src/ui/frontend").glob("*.c")):
    original = reference / "src" / path.name
    if not original.exists():
        continue
    old, new = functions(original.read_text(encoding='utf-8')), functions(path.read_text(encoding='utf-8'))
    for name, source in old.items():
        renamed = mapping.get(name, name)
        state = "missing from carried file" if renamed not in new else (
            "token-equivalent after explicit boundary renames" if tokens(source, True) == tokens(new[renamed])
            else "requires review")
        rows.append({"file": path.name, "function": name, "rewrite_function": renamed, "state": state})
        if state == "requires review":
            differences.extend(difflib.unified_diff(source.splitlines(True), new[renamed].splitlines(True),
                fromfile=f"original/{path.name}:{name}", tofile=f"rewrite/{path.name}:{renamed}"))
output.mkdir(parents=True, exist_ok=True)
metadata = {"reference_commit": commit,
    "rewrite_commit": subprocess.check_output(["git", "-C", str(rewrite), "rev-parse", "HEAD"], text=True).strip(),
    "rewrite_status": subprocess.check_output(["git", "-C", str(rewrite), "status", "--porcelain"], text=True),
    "normalization": mapping, "functions": rows}
(output / "functions.json").write_text(json.dumps(metadata, indent=2) + "\n")
(output / "changed-functions.diff").write_text("".join(differences))
print(dict(Counter(row["state"] for row in rows)))
