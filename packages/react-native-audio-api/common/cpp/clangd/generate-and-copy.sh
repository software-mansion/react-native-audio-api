#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

REPO_ROOT="$(cd "$SCRIPT_DIR/../../../../.." && pwd)"
WORKLETS_DB="$REPO_ROOT/packages/react-native-audio-worklets/compile_commands.json"

cmake -B build .
cp build/compile_commands.json "$REPO_ROOT/compile_commands.json"

# Preserve worklets entries in the root DB (clangd is pinned to the repo root).
if [[ -f "$WORKLETS_DB" ]]; then
  python3 - <<PY
import json
from pathlib import Path

repo_root = Path("$REPO_ROOT")
root_db = repo_root / "compile_commands.json"
worklets_db = Path("$WORKLETS_DB")
worklets_prefix = str(worklets_db.parent.resolve())

by_file = {e["file"]: e for e in json.loads(root_db.read_text())}
for entry in json.loads(worklets_db.read_text()):
    by_file[entry["file"]] = entry
root_db.write_text(json.dumps(list(by_file.values()), indent=2) + "\n")
print(f"Preserved worklets entries in {root_db} (total {len(by_file)})")
PY
fi
