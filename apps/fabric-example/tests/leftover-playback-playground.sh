#!/usr/bin/env bash
# Reproduces playback of the previous take leaking into a new recording after an interruption, on
# the Audio I/O Playground. Needs a physical iPhone: the iOS simulator never interrupts an audio session.
source "$(dirname "${BASH_SOURCE[0]}")/harness.sh"

INTERRUPT="While it is recording, send FabricExample to the background. In Safari open https://meet.jit.si, join a meeting so it takes the audio session, then disconnect and close the Safari tab. Return to FabricExample."

start_app
run_flow open-playground
run_flow start-recording
run_flow stop-and-play-take
run_flow record-next-take
ask_to "$INTERRUPT"
run_flow stop-and-play-take
check "Was the whole interrupted take clearly audible?"
run_flow disconnect-playback
run_flow record-next-take
ask_to "Stay on this screen and listen while it records. Speak for about 3 seconds."
confirm "Did the speaker stay silent during that recording, with nothing from the previous take?"
