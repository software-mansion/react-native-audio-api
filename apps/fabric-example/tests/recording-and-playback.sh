#!/usr/bin/env bash
source "$(dirname "${BASH_SOURCE[0]}")/harness.sh"

start_app
run_flow open-playground
run_flow start-recording
ask_to "Speak into the microphone for a few seconds."
run_flow stop-and-play-take
confirm "Did you hear your speech played back clearly?"
