#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/../scripts/test-harness.sh"

init_test \
  --subsystem "audioapi|js" \
  --module "AudioSessionManager|AudioEngine|SystemNotificationManager" \
  --level trace \
  --grep "interruption|routeChange|AppState|\\[playground\\]" \
  "$@"

run_flow "01-warmup-record.yaml"

run_flow "02-start-recording.yaml"

prompt_action "While it is recording, send FabricExample to the background. In Safari open https://meet.jit.si, join a meeting so it takes the audio session, then disconnect and close the Safari tab. Return to FabricExample."

run_flow "03-stop-and-play-interrupted-take.yaml"

prompt_choice "What did you hear while that take showed playing?" \
  --pass "2" \
  --continue "3" \
  "Only the speech from before the meeting was audible" \
  "The whole take was clearly audible" \
  "Playback ran to the end, but was silent" \
  "Playback was distorted or cut off"

run_flow "04-record-again.yaml"

prompt_action "Stay on this screen and listen while it records. Speak for about 3 seconds."

prompt_choice "During that recording, did audio from the previous take play out of the speaker?" \
  --pass "2" \
  "Yes, the previous take was audible while recording" \
  "No, only the room was audible"
