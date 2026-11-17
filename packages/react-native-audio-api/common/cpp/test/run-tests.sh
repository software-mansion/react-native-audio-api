#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: run-tests.sh [smoke|extended|full|affected] [category…] [options]

Modes (default: smoke):
  smoke       Fast suites — PR default and coverage. Disjoint from extended.
  extended    Slow suites for the given categories (default: all registered).
  full        smoke, then extended for all categories (each mode’s defaults).
  affected    The suites (smoke, extended categories) whose paths in filters.sh the
              current changes touch; nothing when none do. TEST_ALL=1 runs all of them.

Categories (extended only):
  graph       Slow graph suites (see filters.sh). GraphNodeGrowthTest is smoke.

Sanitizer options (choose at most one; --ubasan and --tsan are incompatible):
  --ubasan    AddressSanitizer + UndefinedBehaviorSanitizer (tests_asan).
              Default for extended.
  --no-ubasan Plain tests binary (no sanitizers). Default for smoke.
  --tsan      ThreadSanitizer (tests_tsan).
              Incompatible with --ubasan (ASan and TSan cannot run together).

Other:
  --help, -h  Show this help.
  --gtest_*   Forwarded to the gtest binary.
  GTEST_FILTER  If set, overrides the mode/category gtest filter.

Examples:
  run-tests.sh
  run-tests.sh smoke --ubasan
  run-tests.sh extended graph
  run-tests.sh extended graph --no-ubasan
  run-tests.sh extended graph --tsan
  run-tests.sh full
  run-tests.sh affected

See TESTING.md.
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# shellcheck source=filters.sh
source "${SCRIPT_DIR}/filters.sh"

MODE=""
SANITIZER="" # empty = mode default; none|ubasan|tsan
CATEGORIES=()
PASSTHROUGH=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help|-h)
      print_help
      exit 0
      ;;
    smoke|extended|full|affected)
      if [[ -n "$MODE" ]]; then
        echo "error: mode already set to '${MODE}', got '$1'" >&2
        exit 1
      fi
      MODE="$1"
      shift
      ;;
    --ubasan)
      if [[ "$SANITIZER" == "tsan" ]]; then
        echo "error: --ubasan is incompatible with --tsan (AddressSanitizer and ThreadSanitizer cannot be combined)" >&2
        exit 1
      fi
      SANITIZER=ubasan
      shift
      ;;
    --tsan)
      if [[ "$SANITIZER" == "ubasan" ]]; then
        echo "error: --tsan is incompatible with --ubasan (AddressSanitizer and ThreadSanitizer cannot be combined)" >&2
        exit 1
      fi
      SANITIZER=tsan
      shift
      ;;
    --no-ubasan)
      if [[ "$SANITIZER" == "ubasan" || "$SANITIZER" == "tsan" ]]; then
        echo "error: --no-ubasan conflicts with a sanitizer flag already set (${SANITIZER})" >&2
        exit 1
      fi
      SANITIZER=none
      shift
      ;;
    # pnpm forwards a literal "--" before script args (`pnpm cmd -- graph`).
    --)
      shift
      ;;
    --gtest_*)
      PASSTHROUGH+=("$1")
      shift
      ;;
    -*)
      echo "error: unknown option '$1'" >&2
      print_help >&2
      exit 1
      ;;
    *)
      CATEGORIES+=("$1")
      shift
      ;;
  esac
done

MODE="${MODE:-smoke}"

if [[ "$MODE" != "extended" && "$MODE" != "full" && ${#CATEGORIES[@]} -gt 0 ]]; then
  echo "error: categories are only valid with extended (or full); got mode=${MODE}" >&2
  exit 1
fi

if [[ "$MODE" == "full" && ${#CATEGORIES[@]} -gt 0 ]]; then
  echo "error: full always runs all extended categories; omit category args" >&2
  exit 1
fi

resolve_sanitizer_for_mode() {
  local mode="$1"
  if [[ -n "$SANITIZER" ]]; then
    printf '%s' "$SANITIZER"
    return
  fi
  case "$mode" in
    smoke) printf 'none' ;;
    extended) printf 'ubasan' ;;
    *)
      echo "error: internal: no sanitizer default for mode '${mode}'" >&2
      return 1
      ;;
  esac
}

binary_for_sanitizer() {
  case "$1" in
    none) printf 'tests' ;;
    ubasan) printf 'tests_asan' ;;
    tsan) printf 'tests_tsan' ;;
    *)
      echo "error: unknown sanitizer '$1'" >&2
      return 1
      ;;
  esac
}

label_for_sanitizer() {
  case "$1" in
    none) printf 'tests (normal)' ;;
    ubasan) printf 'tests with AddressSanitizer + UndefinedBehaviorSanitizer' ;;
    tsan) printf 'tests with ThreadSanitizer' ;;
  esac
}

filter_for_mode() {
  local mode="$1"
  shift
  if [[ -n "${GTEST_FILTER:-}" ]]; then
    printf '%s' "$GTEST_FILTER"
    return
  fi
  case "$mode" in
    smoke)
      cpp_test_smoke_filter
      ;;
    extended)
      cpp_test_extended_filter "$@"
      ;;
    *)
      echo "error: internal: no filter for mode '${mode}'" >&2
      return 1
      ;;
  esac
}

run_mode() {
  local mode="$1"
  shift
  local categories=("$@")
  local sanitizer binary filter label
  sanitizer="$(resolve_sanitizer_for_mode "$mode")"
  binary="$(binary_for_sanitizer "$sanitizer")"
  label="$(label_for_sanitizer "$sanitizer")"
  filter="$(filter_for_mode "$mode" "${categories[@]+"${categories[@]}"}")"

  cmake --build build --target "$binary" -j "$(parallel_job_count)"

  echo ""
  echo "=== ${label} (mode=${mode}, filter=${filter}) ==="
  echo ""
  "./build/${binary}" --gtest_print_time=1 --gtest_filter="${filter}" \
    ${PASSTHROUGH[@]+"${PASSTHROUGH[@]}"}
}

AFFECTED_CATEGORIES=()
RUN_SMOKE=false
if [[ "$MODE" == "affected" ]]; then
  if [[ -n "$SANITIZER" ]]; then
    echo "error: affected runs each suite with its mode's default sanitizer; drop the sanitizer flag" >&2
    exit 1
  fi
  while IFS= read -r suite; do
    if [[ "$suite" == "smoke" ]]; then
      RUN_SMOKE=true
    elif [[ -n "$suite" ]]; then
      AFFECTED_CATEGORIES+=("$suite")
    fi
  done < <(cpp_test_affected_suites)

  if [[ "$RUN_SMOKE" == false && ${#AFFECTED_CATEGORIES[@]} -eq 0 ]]; then
    echo "No C++ test inputs changed; skipping the C++ tests."
    exit 0
  fi
  echo "Affected suites: $([[ "$RUN_SMOKE" == true ]] && echo "smoke ")${AFFECTED_CATEGORIES[*]+"${AFFECTED_CATEGORIES[*]}"}"
fi

cmake -S . -B build -Wno-dev

case "$MODE" in
  smoke)
    run_mode smoke
    ;;
  extended)
    run_mode extended "${CATEGORIES[@]+"${CATEGORIES[@]}"}"
    ;;
  full)
    # Each sub-mode uses its own default sanitizer (ignore flags for the pair).
    SANITIZER=""
    run_mode smoke
    SANITIZER=""
    run_mode extended
    ;;
  affected)
    if [[ "$RUN_SMOKE" == true ]]; then
      run_mode smoke
    fi
    if [[ ${#AFFECTED_CATEGORIES[@]} -gt 0 ]]; then
      run_mode extended "${AFFECTED_CATEGORIES[@]}"
    fi
    ;;
  *)
    echo "error: unknown mode '${MODE}'" >&2
    exit 1
    ;;
esac
