#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: run-packages.sh <script> [--changed | --staged | --all]

Runs a package script (e.g. check, test) in the packages a set of changes
affects, the same set CI checks (scripts/changes.sh), then lists the other CI
jobs those changes start, with the local command for each.

  --changed  changes since the base (default; see scripts/base-ref.sh)
  --staged   staged changes; clang-tidy then checks only the staged C++ files
  --all      every package

  pnpm check            # what CI checks for this branch
  pnpm check --staged   # the pre-commit hook
EOF
}

set -euo pipefail

if [[ $# -eq 0 || "$1" == "--help" || "$1" == "-h" ]]; then
  print_help
  [[ $# -eq 0 ]] && exit 1
  exit 0
fi

SCRIPT="$1"
MODE="${2:---changed}"
ROOT="$(git rev-parse --show-toplevel)"

case "$MODE" in
  --changed) changes="$(bash "${ROOT}/scripts/changes.sh")" ;;
  --staged)
    changes="$(bash "${ROOT}/scripts/changes.sh" --staged)"
    export CHECK_STAGED=1
    ;;
  --all) changes="$(bash "${ROOT}/scripts/changes.sh" --all)" ;;
  *)
    echo "error: unexpected argument '$MODE'" >&2
    print_help >&2
    exit 1
    ;;
esac

names=()
while IFS= read -r name; do
  names+=("$name")
done < <(jq -r '.packages[].name' <<<"$changes")

failed=()
if [[ ${#names[@]} -eq 0 ]]; then
  echo "No package is affected."
else
  for name in "${names[@]}"; do
    echo "▶ ${name}: ${SCRIPT}"
    (cd "$ROOT" && pnpm --filter "$name" run --if-present "$SCRIPT") || failed+=("$name")
  done
fi

hints=()
if jq -e '.android' <<<"$changes" >/dev/null; then
  hints+=("Android build: pnpm android")
fi
if jq -e '.ios' <<<"$changes" >/dev/null; then
  hints+=("iOS build: pnpm ios; XCTests: pnpm --filter fabric-example test:ios")
fi
if jq -e '.wpt' <<<"$changes" >/dev/null; then
  hints+=("Web Platform Tests: pnpm --filter react-native-audio-api wpt")
fi
if [[ ${#hints[@]} -gt 0 ]]; then
  echo
  echo "CI also runs, for these changes:"
  printf '  %s\n' "${hints[@]}"
fi

if [[ ${#failed[@]} -gt 0 ]]; then
  echo
  echo "❌ ${SCRIPT} failed in: ${failed[*]}"
  exit 1
fi
if [[ ${#names[@]} -gt 0 ]]; then
  echo
  echo "✅ ${SCRIPT} passed in: ${names[*]}"
fi
