#!/usr/bin/env bash
# Reproduces playback of the previous take leaking into a new recording after an interruption, on
# the Recorder demo. Needs a physical iPhone: the iOS simulator never interrupts an audio session.
source "$(dirname "${BASH_SOURCE[0]}")/harness.sh"

INTERRUPT="While it is recording, send FabricExample to the background. In Safari open https://meet.jit.si, join a meeting so it takes the audio session, then disconnect and close the Safari tab. Return to FabricExample."

start_app
run_flow open-recorder-demo
run_flow recorder-demo-record
ask_to "Speak into the microphone for about 2 seconds."
run_flow recorder-demo-stop-and-play
run_flow recorder-demo-await-playback-end
confirm "Did you hear your speech played back clearly?"
run_flow recorder-demo-record
ask_to "$INTERRUPT"
run_flow recorder-demo-stop-and-play
check "Was the whole interrupted take clearly audible?"
run_flow recorder-demo-record
ask_to "Stay on this screen and listen while it records. Speak for about 3 seconds."
confirm "Did the speaker stay silent during that recording, with nothing from the previous take?"
