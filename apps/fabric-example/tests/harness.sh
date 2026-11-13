#!/usr/bin/env bash
# Sourced by the test scripts in this directory. A test alternates between `run_flow`, where argent
# replays a fragment from flows/ on the device, and `ask_to` / `confirm`, where a person does or
# judges what argent cannot (speaking, interrupting the audio session, listening). Argent's own
# `script:` step cannot prompt, because the tool-server runs it without a terminal.
#
# On a simulator every run is recorded by scripts/record-session-ios.sh: the test's own steps, the
# prompts and what the person typed are written as `[test]` lines next to the library's native logs. That
# recorder streams through simctl, so a run on a physical iPhone is not recorded.
#
# E2E_DEVICE     device to drive; defaults to the only booted iOS simulator
# AUDIO_LOG_LEVEL verbosity of the library's native logs on a simulator; defaults to trace
# E2E_LOG        log file of the run; defaults to /tmp/e2e-<test>-<time>.log
# APP_BUNDLE_ID  FabricExample's bundle id; a build signed for a physical iPhone carries its own

set -euo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
APP_BUNDLE_ID="${APP_BUNDLE_ID:-fdsfsdfsdcom-eseg3222233e.--PRODUCT-NAME-rfc1034identifier-}"

# Written straight into the recorder's file, in its timestamp format. Its own marks are two lines
# (Enter stamps the moment, a description follows later), which suits a person typing, not a script.
journal() {
  kill -0 "${RECORDER_PID:-}" 2>/dev/null || return 0
  echo "$(date '+%F %T') [test] $*" >>"$LOG_FILE"
}

fail() {
  journal "FAIL: $*"
  echo "FAIL: $*" >&2
  exit 1
}

only_booted_simulator() {
  local booted
  booted="$(xcrun simctl list devices booted | grep -oE '[0-9A-F]{8}(-[0-9A-F]{4}){3}-[0-9A-F]{12}' || true)"
  [[ "$(grep -c . <<<"$booted")" -eq 1 ]] ||
    fail "expected exactly one booted simulator, set E2E_DEVICE=<udid> to choose a device"
  echo "$booted"
}

is_simulator() {
  xcrun simctl list devices | grep -q "$DEVICE"
}

start_recorder() {
  if ! is_simulator; then
    echo "Not recording: $DEVICE is not a simulator."
    return 0
  fi
  # The recorder runs until its input ends, so an open pipe that is never written keeps it alive.
  RECORDER_INPUT="$(mktemp -u)"
  mkfifo "$RECORDER_INPUT"
  bash "$TESTS_DIR/../scripts/record-session-ios.sh" "$DEVICE" "$LOG_FILE" <"$RECORDER_INPUT" >/dev/null &
  RECORDER_PID=$!
  exec 3>"$RECORDER_INPUT"
}

finish_run() {
  local status=$?
  trap - EXIT
  if [[ $status -eq 0 && $FAILED_CHECKS -gt 0 ]]; then
    status=1
    journal "FAIL: $FAILED_CHECKS failed check(s)"
    echo "FAIL: $FAILED_CHECKS failed check(s)" >&2
  elif [[ $status -eq 0 ]]; then
    journal "PASS"
    echo "PASS: $TEST_NAME"
  fi
  if [[ -n "${RECORDER_PID:-}" ]]; then
    # Closing the pipe ends the recorder's input, which makes it stop the log stream and exit.
    exec 3>&-
    wait "$RECORDER_PID" 2>/dev/null || true
    rm -f "$RECORDER_INPUT"
    echo "Log: $LOG_FILE"
  fi
  exit "$status"
}

# The library reads AUDIO_LOG_LEVEL once per launch and logs only warnings without it. Argent's
# restart-app cannot set an environment, so a simulator is launched through simctl instead.
start_app() {
  journal "start app $APP_BUNDLE_ID"
  if is_simulator; then
    xcrun simctl terminate "$DEVICE" "$APP_BUNDLE_ID" 2>/dev/null || true
    SIMCTL_CHILD_AUDIO_LOG_LEVEL="${AUDIO_LOG_LEVEL:-trace}" \
      xcrun simctl launch "$DEVICE" "$APP_BUNDLE_ID" >/dev/null ||
      fail "could not start $APP_BUNDLE_ID on $DEVICE"
  else
    argent run restart-app --udid "$DEVICE" --bundleId "$APP_BUNDLE_ID" >/dev/null ||
      fail "could not start $APP_BUNDLE_ID on $DEVICE"
  fi
}

run_flow() {
  journal "flow '$1' started"
  argent e2e run "$TESTS_DIR/flows/$1.yaml" --device "$DEVICE" || fail "flow '$1' did not pass"
  journal "flow '$1' passed"
}

# Anything the person types before Enter is kept in the log as a note about what they did.
ask_to() {
  local note
  journal "asked: $1"
  printf '\n>> %s\n   Press Enter when done (a typed note goes to the log). ' "$1"
  read -r note || fail "nobody answered: $1"
  journal "done${note:+, note: $note}"
}

confirm() {
  local answer observed
  journal "asked: $1"
  printf '\n?? %s [y/n] ' "$1"
  read -r answer || fail "nobody answered: $1"
  journal "answered: $answer"
  if [[ "$answer" != [yY]* ]]; then
    read -r -p "   What did you observe instead? " observed || true
    fail "answered '$answer' to: $1${observed:+ (observed: $observed)}"
  fi
}

# Like `confirm`, but an "n" is remembered and fails the test when it ends, so the steps after it
# still run.
check() {
  local answer observed
  journal "asked: $1"
  printf '\n?? %s [y/n] ' "$1"
  read -r answer || fail "nobody answered: $1"
  journal "answered: $answer"
  if [[ "$answer" != [yY]* ]]; then
    read -r -p "   What did you observe instead? " observed || true
    journal "FAILED CHECK: $1${observed:+ (observed: $observed)}"
    FAILED_CHECKS=$((FAILED_CHECKS + 1))
  fi
}

FAILED_CHECKS=0
command -v argent >/dev/null || fail "argent CLI not found, install it with: npm i -g @swmansion/argent"
DEVICE="${E2E_DEVICE:-$(only_booted_simulator)}"
TEST_NAME="$(basename "$0" .sh)"
LOG_FILE="${E2E_LOG:-/tmp/e2e-$TEST_NAME-$(date +%H%M%S).log}"
trap finish_run EXIT
start_recorder
journal "test $TEST_NAME on device $DEVICE"
