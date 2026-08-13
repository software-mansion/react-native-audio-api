#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

REPO_ROOT="$(cd "$SCRIPT_DIR/../../../../.." && pwd)"
PACKAGE_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
WORKLETS_DB="$REPO_ROOT/packages/react-native-audio-worklets/compile_commands.json"

# --clean: refresh the real Pods/Gradle builds this script depends on before
# regenerating. Slow — only needed after native deps/config actually change.
if [[ "${1:-}" == "--clean" ]]; then
  (cd "$REPO_ROOT/apps/fabric-example/ios" && pod deintegrate && pod install)
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api run build:android)
fi

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

# Merge in Android compile commands Gradle already produced under android/.cxx
# — those sources need the NDK's own clang++ (--target/--sysroot), which this
# CMakeLists.txt can't reproduce safely with the host compiler. Requires
# having built the Android app (or opened it in Android Studio) once.
python3 - <<PY
import json
from pathlib import Path

repo_root = Path("$REPO_ROOT")
package_dir = Path("$PACKAGE_DIR")
root_db = repo_root / "compile_commands.json"

android_dbs = list(package_dir.glob("android/.cxx/*/*/arm64-v8a/compile_commands.json"))
if not android_dbs:
    print("No Android compile_commands.json found under android/.cxx — build the Android app once to populate it.")
else:
    newest = max(android_dbs, key=lambda p: p.stat().st_mtime)
    entries = json.loads(newest.read_text())

    # Rewrite the node_modules symlink path Gradle resolved through to the
    # real path, so clangd matches the files open in the editor.
    node_modules_dir = str(repo_root / "node_modules" / "react-native-audio-api")
    real_dir = str(package_dir)

    by_file = {e["file"]: e for e in json.loads(root_db.read_text())}
    merged = 0
    for entry in entries:
        if node_modules_dir not in entry["file"]:
            continue
        entry["file"] = entry["file"].replace(node_modules_dir, real_dir)
        entry["command"] = entry["command"].replace(node_modules_dir, real_dir)
        by_file[entry["file"]] = entry
        merged += 1

    root_db.write_text(json.dumps(list(by_file.values()), indent=2) + "\n")
    print(f"Merged {merged} Android entries from {newest} (total {len(by_file)})")
PY
