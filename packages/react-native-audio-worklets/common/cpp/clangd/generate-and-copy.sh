#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

REPO_ROOT="$(cd "$SCRIPT_DIR/../../../../.." && pwd)"
PACKAGE_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

cmake -B build .
cp build/compile_commands.json "$PACKAGE_ROOT/compile_commands.json"

# Workspace clangd uses --compile-commands-dir=<repo root>, so also merge into the
# root DB. Without this, worklets files get inferred from audio-api flags.
python3 - <<PY
import json
from pathlib import Path

repo_root = Path("$REPO_ROOT")
package_db = Path("$PACKAGE_ROOT/compile_commands.json")
root_db = repo_root / "compile_commands.json"

worklets_prefix = str(package_db.parent.resolve())
by_file = {}
if root_db.exists():
    for entry in json.loads(root_db.read_text()):
        if not entry.get("file", "").startswith(worklets_prefix + "/"):
            by_file[entry["file"]] = entry
for entry in json.loads(package_db.read_text()):
    by_file[entry["file"]] = entry
root_db.write_text(json.dumps(list(by_file.values()), indent=2) + "\n")
print(f"Merged {len(json.loads(package_db.read_text()))} worklets entries into {root_db}")
PY
