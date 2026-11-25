#!/bin/bash
# Gtest filters and change detection for the C++ test suites
# (smoke / extended-by-category). Sourced by run-tests.sh and run-coverage.sh.
#
# Invariant: smoke and extended are disjoint. Extended lists only slow suites.
# Override any computed filter with GTEST_FILTER=...

# Slow graph suites only. GraphNodeGrowthTest stays in smoke (short; needs
# unsanitized AudioThreadGuard — see TESTING.md).
CPP_TEST_EXTENDED_GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*"

# Space-separated registered extended categories (add new names here).
CPP_TEST_EXTENDED_CATEGORIES="graph"

# Paths below are relative to the repository root.
CPP_TEST_PATH_PREFIX="packages/react-native-audio-api/common/cpp"

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

# Paths (globs) whose changes can change a suite's result. Args = smoke or a category.
cpp_test_paths_for_suite() {
  # The test build itself, for every suite.
  printf '%s\n' \
    "${CPP_TEST_PATH_PREFIX}/test/filters.sh" \
    "${CPP_TEST_PATH_PREFIX}/test/run-tests.sh" \
    "${CPP_TEST_PATH_PREFIX}/test/CMakeLists.txt"

  case "$1" in
    smoke)
      # The tests compile against React Native's headers, whose version comes from
      # the package manifest and the lockfile.
      printf '%s\n' \
        "${CPP_TEST_PATH_PREFIX}/**" \
        "packages/react-native-audio-api/package.json" \
        "pnpm-lock.yaml" \
        "pnpm-workspace.yaml"
      ;;
    graph)
      printf '%s\n' \
        "${CPP_TEST_PATH_PREFIX}/audioapi/core/utils/graph/**" \
        "${CPP_TEST_PATH_PREFIX}/test/src/graph/**"
      ;;
    *)
      echo "error: unknown suite '$1'" >&2
      return 1
      ;;
  esac
}

# The revision the current changes are measured against: CPP_TEST_BASE when set;
# HEAD^1 on a pull request in GitHub Actions, whose checkout is a merge commit with
# the base branch as first parent (needs fetch-depth 2); otherwise the merge-base
# with origin/main.
cpp_test_base_ref() {
  if [[ -n "${CPP_TEST_BASE:-}" ]]; then
    printf '%s' "$CPP_TEST_BASE"
  elif [[ "${GITHUB_EVENT_NAME:-}" == "pull_request" ]]; then
    printf 'HEAD^1'
  else
    git merge-base origin/main HEAD
  fi
}

# Files the current changes touch since cpp_test_base_ref, one per line, relative to
# the repository root. Uncommitted and untracked files count too.
cpp_test_changed_files() {
  local base
  base="$(cpp_test_base_ref)" || return 1

  local root
  root="$(git rev-parse --show-toplevel)" || return 1
  (
    cd "$root" &&
      git diff --name-only "$base" -- &&
      git ls-files --others --exclude-standard
  )
}

# Prints the suites to run, one per line: smoke and/or extended categories whose
# paths match a changed file. TEST_ALL=1 selects every suite (CI sets it outside
# pull requests); so does a base that can't be resolved.
cpp_test_affected_suites() {
  local suites=(smoke)
  # shellcheck disable=SC2206
  suites+=(${CPP_TEST_EXTENDED_CATEGORIES})

  if [[ "${TEST_ALL:-}" == "1" ]]; then
    printf '%s\n' "${suites[@]}"
    return
  fi

  local changed
  if ! changed="$(cpp_test_changed_files)"; then
    echo "warning: could not determine changed files; running every suite" >&2
    printf '%s\n' "${suites[@]}"
    return
  fi

  local suite pattern file
  for suite in "${suites[@]}"; do
    while IFS= read -r pattern; do
      while IFS= read -r file; do
        # shellcheck disable=SC2053 # the pattern is a glob on purpose
        if [[ -n "$file" && "$file" == $pattern ]]; then
          echo "$suite"
          continue 3
        fi
      done <<<"$changed"
    done < <(cpp_test_paths_for_suite "$suite")
  done
}

parallel_job_count() {
  sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 10
}
