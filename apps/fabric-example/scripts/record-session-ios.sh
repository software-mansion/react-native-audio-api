#!/usr/bin/env bash
# Records the simulator's native audio logs and a person's marks in one file, in arrival order.
# Usage: bash scripts/record-session-ios.sh [SIMULATOR_UDID] [OUTPUT_FILE]

set -euo pipefail

simulator="${1:-booted}"
output="${2:-/tmp/audio-session-$(date +%H%M%S).log}"

xcrun simctl spawn "$simulator" log stream --style compact --level debug \
  --predicate 'subsystem == "com.swmansion.audioapi"' >>"$output" 2>&1 &
stream_pid=$!
trap 'kill "$stream_pid" 2>/dev/null || true; echo "Saved to $output"' EXIT

sleep 1
if ! kill -0 "$stream_pid" 2>/dev/null; then
  cat "$output" >&2
  exit 1
fi

mark=0
echo "Press Enter the moment you want to add something, then describe it. Ctrl+D to finish."
while read -r; do
  mark=$((mark + 1))
  # Written before the description, so the mark sits next to the logs of that moment.
  echo "$(date '+%F %T') [mark $mark]" >>"$output"
  read -r -p "mark $mark, what happened? " note || break
  echo "$(date '+%F %T') [mark $mark] $note" >>"$output"
done
