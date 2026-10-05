#!/bin/bash
# Gtest filters and CI path filters for C++ test modes
# (smoke / extended-by-category / full).
# Sourced by run-tests.sh and run-coverage.sh. Also runnable:
#   bash filters.sh path-filters <category>…
#
# Invariant: smoke and extended are disjoint. Extended lists only slow suites.
# Override any computed filter with GTEST_FILTER=...

# Slow graph suites only. GraphNodeGrowthTest stays in smoke (short; needs
# unsanitized AudioThreadGuard — see TESTING.md).
CPP_TEST_EXTENDED_GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*"

# Space-separated registered extended categories (add new names here).
CPP_TEST_EXTENDED_CATEGORIES="graph"

# Repo-root-relative prefix for dorny/paths-filter globs in CI.
CPP_TEST_CI_PATH_PREFIX="packages/react-native-audio-api/common/cpp"

cpp_test_extended_filter_for_category() {
  local category="$1"
  case "$category" in
    graph)
      printf '%s' "${CPP_TEST_EXTENDED_GRAPH_FILTER}"
      ;;
    *)
      echo "error: unknown extended category '${category}' (registered: ${CPP_TEST_EXTENDED_CATEGORIES})" >&2
      return 1
      ;;
  esac
}

# Join category filters with ':'. Args = category names (default: all registered).
cpp_test_extended_filter() {
  local categories=("$@")
  if [[ ${#categories[@]} -eq 0 ]]; then
    # shellcheck disable=SC2206
    categories=(${CPP_TEST_EXTENDED_CATEGORIES})
  fi

  local parts=()
  local category filter
  for category in "${categories[@]}"; do
    filter="$(cpp_test_extended_filter_for_category "$category")" || return 1
    parts+=("$filter")
  done

  local IFS=':'
  printf '%s' "${parts[*]}"
}

cpp_test_smoke_filter() {
  local extended
  extended="$(cpp_test_extended_filter)" || return 1
  printf '%s' "-${extended}"
}

cpp_test_ci_shared_path_filters() {
  printf '%s\n' \
    "${CPP_TEST_CI_PATH_PREFIX}/test/filters.sh" \
    "${CPP_TEST_CI_PATH_PREFIX}/test/run-tests.sh" \
    "${CPP_TEST_CI_PATH_PREFIX}/test/CMakeLists.txt"
}

cpp_test_ci_path_filters_for_category() {
  local category="$1"
  case "$category" in
    graph)
      printf '%s\n' \
        "${CPP_TEST_CI_PATH_PREFIX}/audioapi/core/utils/graph/**" \
        "${CPP_TEST_CI_PATH_PREFIX}/test/src/graph/**"
      ;;
    *)
      echo "error: unknown extended category '${category}' (registered: ${CPP_TEST_EXTENDED_CATEGORIES})" >&2
      return 1
      ;;
  esac
}

# Prints a dorny/paths-filter YAML document (top-level key `run`).
# Args = category names (at least one).
cpp_test_ci_path_filters_yaml() {
  if [[ $# -eq 0 ]]; then
    echo "error: cpp_test_ci_path_filters_yaml requires at least one category" >&2
    return 1
  fi

  local category path globs
  local category_globs=""
  for category in "$@"; do
    globs="$(cpp_test_ci_path_filters_for_category "$category")" || return 1
    category_globs+="${globs}"$'\n'
  done

  echo 'run:'
  while IFS= read -r path; do
    [[ -n "$path" ]] && echo "  - '${path}'"
  done < <(cpp_test_ci_shared_path_filters; printf '%s' "$category_globs")
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  set -euo pipefail
  command="${1:-}"
  shift || true
  case "$command" in
    path-filters)
      cpp_test_ci_path_filters_yaml "$@"
      ;;
    *)
      echo "usage: filters.sh path-filters <category>..." >&2
      exit 1
      ;;
  esac
fi
