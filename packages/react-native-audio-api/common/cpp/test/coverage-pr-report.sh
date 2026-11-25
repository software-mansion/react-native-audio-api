#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: coverage-pr-report.sh <output.md>

Writes a Markdown coverage report for a pull request comment: the totals from
run-coverage.sh, then diff-cover's coverage of the lines changed since the base
filters.sh picks (cpp_test_base_ref). Run run-coverage.sh first. Needs
diff-cover on PATH.

The report fits in a GitHub comment: per-file details are collapsed and
dropped when the report would exceed the comment size limit.

  run-coverage.sh && coverage-pr-report.sh report.md
EOF
}

set -euo pipefail

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  print_help
  exit 0
fi
if [[ $# -ne 1 ]]; then
  print_help >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-coverage"
OUTPUT="$1"

# shellcheck source=filters.sh
source "${SCRIPT_DIR}/filters.sh"
# GitHub rejects comments over 65536 characters; leave room for the comment's header.
readonly MAX_REPORT_BYTES=60000

for file in coverage.lcov coverage-totals.md; do
  if [[ ! -f "${BUILD_DIR}/${file}" ]]; then
    echo "error: ${BUILD_DIR}/${file} not found; run run-coverage.sh first" >&2
    exit 1
  fi
done

BASE_REF="$(cpp_test_base_ref)"
mkdir -p "$(dirname "$OUTPUT")"

PATCH_REPORT="$(mktemp)"
trap 'rm -f "$PATCH_REPORT"' EXIT

(
  cd "$(git rev-parse --show-toplevel)"
  diff-cover "${BUILD_DIR}/coverage.lcov" \
    --compare-branch="$BASE_REF" \
    --format "markdown:${PATCH_REPORT}" >/dev/null
)

# diff-cover's Markdown: a file list, "## Summary", then one "## <file>" section with
# code snippets per file that has uncovered lines.
changed_files="$(awk '/^## Summary/ { exit } /^- / { print }' "$PATCH_REPORT")"
summary="$(awk '/^## Summary/ { inside = 1; next } inside && /^## / { exit } inside && NF { print }' "$PATCH_REPORT")"
details="$(awk '/^## Summary/ { summary = 1; next } summary && /^## / { inside = 1 } inside { print }' "$PATCH_REPORT")"

write_report() {
  local with_details="$1"
  echo '## C++ coverage'
  echo
  echo 'Smoke suites, Clang source-based coverage.'
  echo
  cat "${BUILD_DIR}/coverage-totals.md"
  echo
  echo '### Lines changed in this pull request'
  echo
  if [[ -z "$summary" ]]; then
    echo 'No changed lines with coverage information.'
    return
  fi
  echo "$summary"
  echo
  echo "$changed_files"
  if [[ -n "$details" ]]; then
    echo
    if [[ "$with_details" == true ]]; then
      echo '<details><summary>Uncovered changed lines</summary>'
      echo
      echo "$details"
      echo
      echo '</details>'
    else
      echo '_Uncovered lines are listed in the workflow run summary (too long for a comment)._'
    fi
  fi
}

write_report true >"$OUTPUT"
if (($(wc -c <"$OUTPUT") > MAX_REPORT_BYTES)); then
  write_report false >"$OUTPUT"
fi

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  {
    echo '## Coverage of changed lines'
    echo
    tail -n +2 "$PATCH_REPORT"
  } >>"$GITHUB_STEP_SUMMARY"
fi
