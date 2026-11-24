#!/bin/bash

# Runs each package script named as an argument with `pnpm run`, in order, and keeps
# going after a failure. Exits non-zero if any step failed, so a package's `check`
# reports every failing step in one run instead of stopping at the first.
#
# On GitHub Actions each step's output is a collapsible group, and the failed steps are
# also reported as an error annotation on the run.
#
# Usage (from a package's package.json): bash ../../scripts/run-steps.sh lint typecheck build

if [ $# -eq 0 ]; then
  echo "usage: run-steps.sh <script>..." >&2
  exit 2
fi

failed=()

for step in "$@"; do
  if [ -n "$GITHUB_ACTIONS" ]; then
    echo "::group::$step"
  else
    echo "▶ $step"
  fi

  pnpm run "$step" || failed+=("$step")

  if [ -n "$GITHUB_ACTIONS" ]; then
    echo "::endgroup::"
  fi
done

if [ ${#failed[@]} -gt 0 ]; then
  echo "❌ Failed: ${failed[*]}"
  if [ -n "$GITHUB_ACTIONS" ]; then
    echo "::error::Failed steps: ${failed[*]}"
  fi
  exit 1
fi

echo "✅ Passed: $*"
