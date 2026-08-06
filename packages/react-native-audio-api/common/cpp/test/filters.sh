#!/bin/bash
# Gtest filters for C++ test modes (smoke / extended-by-category / full).
# Sourced by RunTests.sh and RunCoverage.sh.
#
# Invariant: smoke and extended are disjoint. Extended lists only slow suites.
# Override any computed filter with GTEST_FILTER=...

# Slow graph suites only. GraphNodeGrowthTest stays in smoke (short; needs
# unsanitized AudioThreadGuard — see TESTING.md).
CPP_TEST_EXTENDED_GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*"

# Space-separated registered extended categories (add new names here).
CPP_TEST_EXTENDED_CATEGORIES="graph"

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
