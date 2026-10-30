# Sourced by apps/fabric-example/tests/<test-name>/run.sh.
# Orchestrates headless log + journal collection, Argent fragment execution, and human prompts.

TESTS_SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_ROOT_DIR="$(cd "$TESTS_SCRIPTS_DIR/.." && pwd)"
FABRIC_SCRIPTS_DIR="$(cd "$TESTS_ROOT_DIR/../scripts" && pwd)"

source "$FABRIC_SCRIPTS_DIR/shared/common.sh"
source "$FABRIC_SCRIPTS_DIR/shared/ios.sh"

readonly JOURNAL_SCRIPT="$FABRIC_SCRIPTS_DIR/journal-execution.mjs"
readonly PROMPT_SCRIPT="$TESTS_SCRIPTS_DIR/prompt.mjs"

test_platform="ios"
test_subsystem="audioapi"
test_module=""
test_level="trace"
test_grep=""
use_gui_prompts=false

journal_pid=""
run_dir_file=""
run_directory=""
journal_file=""
target_bundle_id=""
test_initialized=false

print_test_help() {
  cat <<EOF
Usage: bash $(basename "$0") [options]

Runs the '${TEST_NAME:-<test>}' semi-automated E2E test against the already-running FabricExample app.

Options:
  --device UDID         Run against this physical device.
  --simulator           Run against the booted simulator, even when a device is connected.
  --subsystem LIST      Override log subsystem filter (default set by test).
  --module LIST         Override log category filter (default set by test).
  --level LEVEL         Override minimum log filter level: error, warning, debug, or trace.
  --grep PATTERN        Override regex filter on log lines.
  --gui                 Display human prompts as native macOS dialogs (osascript).
  --help, -h            Show this help.
EOF
}

format_journal_timestamp() {
  node -e '
    const d = new Date();
    const p2 = (n) => String(n).padStart(2, "0");
    const p3 = (n) => String(n).padStart(3, "0");
    process.stdout.write(
      `${d.getFullYear()}-${p2(d.getMonth() + 1)}-${p2(d.getDate())} ` +
      `${p2(d.getHours())}:${p2(d.getMinutes())}:${p2(d.getSeconds())}.${p3(d.getMilliseconds())}`
    );
  '
}

journal_note() {
  local message="$*"
  if [[ -n "$journal_file" && -f "$journal_file" ]]; then
    printf '%s [journal] %s\n' "$(format_journal_timestamp)" "$message" >>"$journal_file"
  fi
}

cleanup_test() {
  local exit_code=$?
  trap - EXIT INT TERM

  if [[ "$test_initialized" == true ]]; then
    if [[ $exit_code -eq 0 ]]; then
      journal_note "[test:pass] $TEST_NAME"
    else
      journal_note "[test:fail] $TEST_NAME (exit code $exit_code)"
    fi
  fi

  if [[ -n "$journal_pid" ]] && kill -0 "$journal_pid" 2>/dev/null; then
    kill -TERM "$journal_pid" 2>/dev/null || true
    wait "$journal_pid" 2>/dev/null || true
  fi

  if [[ -n "$run_dir_file" ]]; then
    rm -f "$run_dir_file"
  fi

  exit "$exit_code"
}

init_test() {
  if [[ -z "${TEST_DIR:-}" ]]; then
    TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[1]}")" && pwd)"
  fi
  TEST_NAME="$(basename "$TEST_DIR")"
  readonly TEST_DIR TEST_NAME

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --platform)
        test_platform="${2:?--platform needs a value}"
        shift
        ;;
      --device)
        select_target device "${2:?--device needs a UDID}"
        shift
        ;;
      --simulator)
        select_target simulator
        ;;
      --subsystem)
        test_subsystem="${2:?--subsystem needs a value}"
        shift
        ;;
      --module)
        test_module="${2:?--module needs a value}"
        shift
        ;;
      --level)
        test_level="${2:?--level needs a value}"
        shift
        ;;
      --grep)
        test_grep="${2:?--grep needs a value}"
        shift
        ;;
      --gui)
        use_gui_prompts=true
        ;;
      --help|-h)
        print_test_help
        exit 0
        ;;
      *)
        echo "Unknown option: $1" >&2
        print_test_help >&2
        exit 1
        ;;
    esac
    shift
  done

  if [[ "$test_platform" != "ios" ]]; then
    fail "Only --platform ios is supported in this version."
  fi

  resolve_ios_target
  require_running_ios_target
  target_bundle_id="$(ios_bundle_id debug)"
  if [[ -z "$target_bundle_id" ]]; then
    fail "Could not determine CFBundleIdentifier for FabricExample."
  fi

  local logs_dir="$TEST_DIR/logs"
  mkdir -p "$logs_dir"
  run_dir_file="$(mktemp)"

  local journal_args
  journal_args=(
    --headless
    --platform "$test_platform"
    --name "$TEST_NAME"
    --output-dir "$logs_dir"
    --run-dir-file "$run_dir_file"
    --device "$target_id"
    --subsystem "$test_subsystem"
  )
  if [[ -n "$test_module" ]]; then
    journal_args+=(--module "$test_module")
  fi
  if [[ -n "$test_level" ]]; then
    journal_args+=(--level "$test_level")
  fi
  if [[ -n "$test_grep" ]]; then
    journal_args+=(--grep "$test_grep")
  fi

  trap cleanup_test EXIT INT TERM

  node "$JOURNAL_SCRIPT" "${journal_args[@]}" &
  journal_pid=$!

  local attempts=0
  while [[ ! -s "$run_dir_file" ]]; do
    if ! kill -0 "$journal_pid" 2>/dev/null; then
      fail "journal-execution exited before initializing the run directory."
    fi
    sleep 0.1
    attempts=$((attempts + 1))
    if [[ $attempts -ge 50 ]]; then
      fail "Timed out waiting for journal-execution to initialize."
    fi
  done

  run_directory="$(tr -d '\r\n' <"$run_dir_file")"
  journal_file="$run_directory/journal.log"
  test_initialized=true

  echo "Running test '$TEST_NAME' on $target_kind $target_id (bundleId: $target_bundle_id)" >&2
  journal_note "[test:start] $TEST_NAME ($target_kind: $target_id, bundleId: $target_bundle_id)"

  # Register the target app with Argent and bring it to the foreground without restarting it.
  (cd "$REPO_ROOT" && npx argent run launch-app --udid "$target_id" --bundleId "$target_bundle_id" >/dev/null)
}

run_flow() {
  local relative_flow="$1"
  local flow_path
  flow_path="$(cd "$TEST_DIR" && cd "$(dirname "$relative_flow")" && pwd)/$(basename "$relative_flow")"

  if [[ ! -f "$flow_path" ]]; then
    fail "Argent workflow file not found: $flow_path"
  fi

  echo "── Running Argent flow: $relative_flow ──" >&2
  journal_note "[flow:start] $relative_flow"

  local output_file status=0
  output_file="$(mktemp)"

  if (cd "$REPO_ROOT" && npx argent e2e run "$flow_path" --platform "$test_platform" --device "$target_id" 2>&1 | tee "$output_file"); then
    rm -f "$output_file"
    journal_note "[flow:pass] $relative_flow"
    return 0
  else
    status=$?
  fi

  if [[ "$target_kind" == "device" ]] && grep -Eq "XCTDaemonErrorDomain Code=41|Not authorized for performing UI testing actions|Lost connection to the application|is running in the background" "$output_file"; then
    echo "── Recovering stale iOS device runner session on $target_id and retrying $relative_flow ──" >&2
    journal_note "[flow:retry] Restarting stale ArgentRunner on $target_id for $relative_flow"
    if grep -Eq "XCTDaemonErrorDomain Code=41|Not authorized for performing UI testing actions|Lost connection to the application" "$output_file"; then
      (cd "$REPO_ROOT" && npx argent run stop-simulator-server --udid "$target_id" >/dev/null) || true
    fi
    (cd "$REPO_ROOT" && npx argent run launch-app --udid "$target_id" --bundleId "$target_bundle_id" >/dev/null)
    rm -f "$output_file"

    if (cd "$REPO_ROOT" && npx argent e2e run "$flow_path" --platform "$test_platform" --device "$target_id"); then
      journal_note "[flow:pass] $relative_flow"
      return 0
    else
      status=$?
    fi
  fi

  rm -f "$output_file"
  journal_note "[flow:fail] $relative_flow (exit code $status)"
  return "$status"
}

prompt_action() {
  local message="$1"
  local args
  args=(--mode action --message "$message" --journal-file "$journal_file")
  if [[ "$use_gui_prompts" == true ]]; then
    args+=(--gui)
  fi
  node "$PROMPT_SCRIPT" "${args[@]}"
}

prompt_text() {
  local message="$1"
  local args
  args=(--mode text --message "$message" --journal-file "$journal_file")
  if [[ "$use_gui_prompts" == true ]]; then
    args+=(--gui)
  fi
  node "$PROMPT_SCRIPT" "${args[@]}"
}

prompt_choice() {
  local message="$1"
  shift

  local pass_options=""
  if [[ "${1:-}" == "--pass" ]]; then
    pass_options="${2:?--pass needs comma-separated option numbers}"
    shift 2
  fi

  local args
  args=(--mode choice --message "$message" --journal-file "$journal_file")
  if [[ -n "$pass_options" ]]; then
    args+=(--pass-options "$pass_options")
  fi
  if [[ "$use_gui_prompts" == true ]]; then
    args+=(--gui)
  fi
  for option in "$@"; do
    args+=(--option "$option")
  done

  node "$PROMPT_SCRIPT" "${args[@]}"
}
