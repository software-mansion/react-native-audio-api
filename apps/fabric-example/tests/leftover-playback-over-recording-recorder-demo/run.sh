#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/../scripts/test-harness.sh"

init_test \
  --subsystem "audioapi|js" \
  --module "AudioSessionManager|AudioEngine|SystemNotificationManager" \
  --level trace \
  --grep "interruption|routeChange|AppState|\\[record\\]" \
  "$@"

run_flow "01-warmup-record.yaml"

prompt_action "Speak into the microphone for about 2 seconds so the warmup take has audible speech."

run_flow "02-warmup-stop-and-play.yaml"

prompt_choice "Did the warmup take play back?" \
  --pass "1" \
  "Yes, the speech was clearly audible" \
  "No, playback was silent" \
  "No, playback was distorted or cut off"

run_flow "03-start-recording.yaml"

prompt_action "While it is recording, send FabricExample to the background. In Safari open https://meet.jit.si, join a meeting so it takes the audio session, then disconnect and close the Safari tab. Return to FabricExample."

run_flow "04-stop-and-play-interrupted-take.yaml"

prompt_choice "What did you hear while that take showed Playing?" \
  --pass "2" \
  --continue "3" \
  "Only the speech from before the meeting was audible" \
  "The whole take was clearly audible" \
  "Playback ran to the end, but was silent" \
  "Playback was distorted or cut off"

run_flow "05-record-again.yaml"

prompt_action "Stay on this screen and listen while it records. Speak for about 3 seconds."

prompt_choice "During that recording, did audio from the previous take play out of the speaker?" \
  --pass "2" \
  "Yes, the previous take was audible while recording" \
  "No, only the room was audible"
