#!/usr/bin/env bash
# Needs a physical iPhone: the iOS simulator never interrupts an audio session.
source "$(dirname "${BASH_SOURCE[0]}")/harness.sh"

start_app
run_flow open-playground
run_flow select-interrupt-others-preset
run_flow start-recording
ask_to "Say a few words, then interrupt the recording (trigger Siri, take a call, or play audio in another app), end the interruption and bring FabricExample back to the foreground."
run_flow expect-interruption-logged
run_flow stop-and-play-take
confirm "Did the played take contain the words you said before the interruption?"
