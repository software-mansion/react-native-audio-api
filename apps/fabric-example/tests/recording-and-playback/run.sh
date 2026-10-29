#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/../scripts/test-harness.sh"

init_test \
  --subsystem "audioapi|js" \
  --level trace \
  "$@"

run_flow "01-start-recording.yaml"

prompt_action "Speak into the microphone for 3 seconds so the recording captures audio."

run_flow "02-stop-and-play-take.yaml"

prompt_choice "Did the recorded take play back clearly?" \
  --pass "1" \
  "Yes, the recorded speech was clearly audible" \
  "No, playback was silent" \
  "No, playback was distorted or cut off"
