#!/usr/bin/env bash
# collect-knowledge-changes.sh
#
# Collects git diff context since the last /pre-push-update run.
# Output is consumed by the Claude Code /pre-push-update command.
#
# Usage:
#   bash scripts/collect-knowledge-changes.sh
#
# Marker file: .claude/last-knowledge-update  (stores last processed SHA)
# First run fallback: HEAD~10 (or the root commit if history is short)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
MARKER_FILE="$REPO_ROOT/.claude/last-knowledge-update"

# ── Resolve base SHA ─────────────────────────────────────────────────────────

if [[ -f "$MARKER_FILE" ]]; then
  STORED=$(cat "$MARKER_FILE" | tr -d '[:space:]')
else
  STORED=""
fi

if [[ -n "$STORED" ]] && git -C "$REPO_ROOT" cat-file -e "${STORED}^{commit}" 2>/dev/null; then
  BASE_SHA="$STORED"
else
  if [[ -n "$STORED" ]]; then
    echo "# WARNING: stored SHA '$STORED' not found in history, falling back to HEAD~10" >&2
  fi
  BASE_SHA=$(git -C "$REPO_ROOT" rev-parse "HEAD~10" 2>/dev/null \
    || git -C "$REPO_ROOT" rev-list --max-parents=0 HEAD)
fi

HEAD_SHA=$(git -C "$REPO_ROOT" rev-parse HEAD)

# ── Nothing to do? ────────────────────────────────────────────────────────────

if [[ "$BASE_SHA" == "$HEAD_SHA" ]]; then
  echo "NO_NEW_COMMITS base=$BASE_SHA"
  exit 0
fi

# ── Header ────────────────────────────────────────────────────────────────────

echo "=== KNOWLEDGE UPDATE CONTEXT ==="
echo "base_sha: $BASE_SHA"
echo "head_sha: $HEAD_SHA"
echo ""

# ── Commit log ────────────────────────────────────────────────────────────────

echo "=== COMMITS (newest first) ==="
git -C "$REPO_ROOT" log "$BASE_SHA".."$HEAD_SHA" --oneline --reverse
echo ""

# ── Full file list (ALL changes, unfiltered) ──────────────────────────────────
# Claude uses this to decide which files are worth deep-reading.

echo "=== ALL CHANGED FILES ==="
git -C "$REPO_ROOT" diff --stat "$BASE_SHA".."$HEAD_SHA"
echo ""

# ── Source diff (filtered to paths that affect skills) ───────────────────────
# Paths:
#   common/cpp/audioapi/          → audio-nodes, host-objects, thread-safety, utilities
#   android/src/main/             → native-android, build
#   ios/                          → native-ios, build
#   src/                          → turbo-modules, web-audio-api
#   .claude/                      → CLAUDE.md / skill files themselves
#
# Extensions: .h .hpp .cpp .mm .kt .ts .tsx
# Excluded:   node_modules, Pods, build artifacts, generated files

DIFF_ARGS=(
  --unified=3
  --diff-filter=ACMR
)

# Build the pathspec: combinations of interesting dirs × interesting extensions.
# We list them explicitly so git can optimise the tree walk.
INTERESTING=(
  "packages/react-native-audio-api/common/cpp/audioapi/*.h"
  "packages/react-native-audio-api/common/cpp/audioapi/**/*.h"
  "packages/react-native-audio-api/common/cpp/audioapi/*.hpp"
  "packages/react-native-audio-api/common/cpp/audioapi/**/*.hpp"
  "packages/react-native-audio-api/common/cpp/audioapi/*.cpp"
  "packages/react-native-audio-api/common/cpp/audioapi/**/*.cpp"
  "packages/react-native-audio-api/android/src/main/**/*.kt"
  "packages/react-native-audio-api/android/src/main/**/*.cpp"
  "packages/react-native-audio-api/android/src/main/**/*.h"
  "packages/react-native-audio-api/ios/**/*.mm"
  "packages/react-native-audio-api/ios/**/*.h"
  "packages/react-native-audio-api/src/**/*.ts"
  "packages/react-native-audio-api/src/**/*.tsx"
  ".claude/**"
)

EXCLUDE=(
  ":(exclude)**/node_modules/**"
  ":(exclude)**/Pods/**"
  ":(exclude)**/build/**"
  ":(exclude)**/lib/**"
  ":(exclude)**/.turbo/**"
  ":(exclude)**/.gradle/**"
  ":(exclude)**/.cxx/**"
  ":(exclude)**/DerivedData/**"
  ":(exclude)**/CMakeFiles/**"
  ":(exclude)**/CMakeLists.txt"
  ":(exclude)**/*.lock"
  ":(exclude)**/*.pbxproj"
  # Prebuilt / vendored external libraries — not our code
  ":(exclude)**/external/**"
  ":(exclude)**/prebuilt_libs/**"
  ":(exclude)**/ffmpeg_ios/**"
  ":(exclude)**/openssl-prebuilt/**"
  ":(exclude)**/jniLibs/**"
  ":(exclude)**/vendor/**"
  ":(exclude)**/libs/ffmpeg/**"
  ":(exclude)**/libs/miniaudio/**"
  ":(exclude)**/libs/opus/**"
  ":(exclude)**/libs/ogg/**"
)

DIFF_OUTPUT=$(git -C "$REPO_ROOT" diff "${DIFF_ARGS[@]}" \
  "$BASE_SHA".."$HEAD_SHA" \
  -- "${INTERESTING[@]}" "${EXCLUDE[@]}" 2>/dev/null || true)

DIFF_LINES=$(echo "$DIFF_OUTPUT" | wc -l | tr -d ' ')
MAX_LINES=4000

echo "=== SOURCE DIFF (filtered: C++/ObjC++/Kotlin/TypeScript in audioapi paths) ==="

if [[ -z "$DIFF_OUTPUT" ]]; then
  echo "(no changes in tracked source paths)"
elif [[ "$DIFF_LINES" -gt "$MAX_LINES" ]]; then
  echo "# TRUNCATED: diff is ${DIFF_LINES} lines, showing first ${MAX_LINES}."
  echo "# Check git diff manually for the full picture."
  echo "$DIFF_OUTPUT" | head -n "$MAX_LINES"
  echo ""
  echo "... [diff truncated at $MAX_LINES lines] ..."
else
  echo "$DIFF_OUTPUT"
fi

echo ""

# ── Maintenance tables ─────────────────────────────────────────────────────────
# Collected from each skill's maintenance.md so /pre-push-update can triage
# without making additional file reads.

echo "=== MAINTENANCE TABLES ==="
SKILLS_DIR="$REPO_ROOT/.claude/skills"
for mfile in "$SKILLS_DIR"/*/maintenance.md; do
  skill_name=$(basename "$(dirname "$mfile")")
  echo "--- $skill_name ---"
  cat "$mfile"
  echo ""
done

echo ""
echo "=== END ==="
echo "HEAD_SHA=$HEAD_SHA"
