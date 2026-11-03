#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/run-e2e-test.sh [<test-name> ...] [options]
       bash scripts/run-e2e-test.sh --list

Runs semi-automated E2E tests in apps/fabric-example/tests/<test-name>/run.sh.
When no <test-name> is given, runs all available tests sequentially.
All options are forwarded to each test's run.sh script.

Options:
  --list                List available tests in apps/fabric-example/tests/.
  --device UDID         Run against this physical device.
  --simulator           Run against the booted simulator, even when a device is connected.
  --subsystem LIST      Override log subsystem filter.
  --module LIST         Override log category filter.
  --level LEVEL         Override minimum log filter level: error, warning, debug, or trace.
  --grep PATTERN        Override regex filter on log lines.
  --gui                 Display human prompts as native macOS dialogs (osascript).
  --help, -h            Show this help.

Examples:
  yarn test:e2e
  yarn test:e2e --list
  yarn test:e2e recording-and-playback
  yarn test:e2e recording-and-playback --simulator --level debug
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
TESTS_DIR="$(cd -P "$SCRIPT_DIR/../tests" && pwd -P)"

list_tests() {
  local run_script test_dir
  for run_script in "$TESTS_DIR"/*/run.sh; do
    [[ -f "$run_script" ]] || continue
    test_dir="$(dirname "$run_script")"
    basename "$test_dir"
  done
}

selected_tests=()
forwarded_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help|-h)
      print_help
      exit 0
      ;;
    --list)
      list_tests
      exit 0
      ;;
    --device|--subsystem|--module|--level|--grep|--platform)
      forwarded_args+=("$1" "${2:?$1 needs a value}")
      shift
      ;;
    --simulator|--gui)
      forwarded_args+=("$1")
      ;;
    -*)
      echo "Unknown option: $1" >&2
      print_help >&2
      exit 1
      ;;
    *)
      selected_tests+=("$1")
      ;;
  esac
  shift
done

if [[ ${#selected_tests[@]} -eq 0 ]]; then
  while IFS= read -r test_name; do
    [[ -n "$test_name" ]] && selected_tests+=("$test_name")
  done < <(list_tests)
fi

if [[ ${#selected_tests[@]} -eq 0 ]]; then
  echo "No E2E tests found in $TESTS_DIR." >&2
  exit 1
fi

for test_name in "${selected_tests[@]}"; do
  if [[ ! -f "$TESTS_DIR/$test_name/run.sh" ]]; then
    echo "Unknown test '$test_name'. Available tests:" >&2
    list_tests >&2
    exit 1
  fi
done

passed_tests=()
failed_tests=()

for test_name in "${selected_tests[@]}"; do
  echo "═══ E2E Test: $test_name ═══" >&2
  if bash "$TESTS_DIR/$test_name/run.sh" "${forwarded_args[@]+"${forwarded_args[@]}"}"; then
    passed_tests+=("$test_name")
  else
    failed_tests+=("$test_name")
  fi
done

if [[ ${#selected_tests[@]} -gt 1 ]]; then
  echo "" >&2
  echo "═══ E2E Summary: ${#passed_tests[@]} passed, ${#failed_tests[@]} failed ═══" >&2
  if [[ ${#failed_tests[@]} -gt 0 ]]; then
    printf '  Failed: %s\n' "${failed_tests[*]}" >&2
  fi
fi

if [[ ${#failed_tests[@]} -gt 0 ]]; then
  exit 1
fi
