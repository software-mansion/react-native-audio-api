#!/bin/bash

# Build the gtest suite with Clang coverage instrumentation and emit a gcovr
# HTML report. Uses Clang's gcov-compatible --coverage mode plus
# `llvm-cov gcov` so gcovr works with Apple Clang (native LLVM JSON v3 from
# Apple Clang 21 is not yet readable by released gcovr).
#
# From packages/react-native-audio-api:
#   yarn test:cpp:coverage

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

readonly REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../../.." && pwd)"
readonly BUILD_DIR="${SCRIPT_DIR}/build-coverage"
readonly COVERAGE_HTML_DIR="${SCRIPT_DIR}/coverage-html"
readonly GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*"

ensure_llvm_cov_on_path() {
  if command -v llvm-cov >/dev/null 2>&1; then
    return
  fi

  if [[ "$(uname -s)" == "Darwin" ]]; then
    local llvm_cov_path
    llvm_cov_path="$(xcrun --find llvm-cov)"
    export PATH="$(dirname "$llvm_cov_path"):${PATH}"
  fi

  if ! command -v llvm-cov >/dev/null 2>&1; then
    echo "error: llvm-cov not found (install Xcode CLT or an llvm package)" >&2
    exit 1
  fi
}

require_gcovr() {
  if ! command -v gcovr >/dev/null 2>&1; then
    echo "error: gcovr not found. Install with: brew install gcovr" >&2
    exit 1
  fi
}

parallel_job_count() {
  sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 10
}

ensure_llvm_cov_on_path
require_gcovr

cmake -S . -B "$BUILD_DIR" -Wno-dev -DENABLE_COVERAGE=ON
cmake --build "$BUILD_DIR" --target tests -j "$(parallel_job_count)"

find "$BUILD_DIR" -name '*.gcda' -delete
rm -rf "$COVERAGE_HTML_DIR"

(
  cd "$BUILD_DIR"
  ./tests --gtest_print_time=1 --gtest_filter="-${GRAPH_FILTER}"
)

mkdir -p "$COVERAGE_HTML_DIR"

# gcovr resolves non-absolute --filter/--exclude paths against cwd, not --root.
cd "$REPO_ROOT"

# .cpp units resolve via node_modules/...; headers often via packages/...
gcovr \
  --gcov-executable "llvm-cov gcov" \
  --root "$REPO_ROOT" \
  --filter 'packages/react-native-audio-api/common/cpp/audioapi/' \
  --filter 'node_modules/react-native-audio-api/common/cpp/audioapi/' \
  --exclude 'packages/react-native-audio-api/common/cpp/audioapi/libs/' \
  --exclude 'node_modules/react-native-audio-api/common/cpp/audioapi/libs/' \
  --html-details "$COVERAGE_HTML_DIR/index.html" \
  --txt - \
  "$BUILD_DIR"

echo
echo "Coverage HTML report: ${COVERAGE_HTML_DIR}/index.html"
