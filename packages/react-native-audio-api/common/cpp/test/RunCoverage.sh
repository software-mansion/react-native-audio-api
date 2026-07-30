#!/bin/bash

# Build the gtest suite with Clang LLVM source-based coverage and emit an
# llvm-cov HTML report. Uses Xcode llvm-profdata/llvm-cov on macOS (no gcovr).
#
# From packages/react-native-audio-api:
#   yarn test:cpp:coverage

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

readonly BUILD_DIR="${SCRIPT_DIR}/build-coverage"
readonly COVERAGE_HTML_DIR="${SCRIPT_DIR}/coverage-html"
readonly PROFDATA_FILE="${BUILD_DIR}/coverage.profdata"
readonly GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*"
readonly IGNORE_FILENAME_REGEX='(/common/cpp/test/|/_deps/|/googletest|/gmock|/audioapi/libs/|/r8brain/|/jsi/|/HostObjects/)'

resolve_llvm_tool() {
  local tool_name="$1"
  if [[ "$(uname -s)" == "Darwin" ]]; then
    xcrun --find "$tool_name"
  else
    command -v "$tool_name"
  fi
}

require_llvm_tools() {
  LLVM_PROFDATA="$(resolve_llvm_tool llvm-profdata)" || {
    echo "error: llvm-profdata not found (install Xcode CLT or an llvm package)" >&2
    exit 1
  }
  LLVM_COV="$(resolve_llvm_tool llvm-cov)" || {
    echo "error: llvm-cov not found (install Xcode CLT or an llvm package)" >&2
    exit 1
  }
}

parallel_job_count() {
  sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 10
}

# ENABLE_COVERAGE requires Clang; default to clang/clang++ when unset so Linux
# CI (and local shells that still point CC/CXX at GCC) configure correctly.
export CC="${CC:-clang}"
export CXX="${CXX:-clang++}"

require_llvm_tools

cmake -S . -B "$BUILD_DIR" -Wno-dev -DENABLE_COVERAGE=ON
cmake --build "$BUILD_DIR" --target tests -j "$(parallel_job_count)"

rm -f "$BUILD_DIR"/default-*.profraw "$PROFDATA_FILE"
rm -rf "$COVERAGE_HTML_DIR"

(
  cd "$BUILD_DIR"
  export LLVM_PROFILE_FILE="${BUILD_DIR}/default-%p.profraw"
  ./tests --gtest_print_time=1 --gtest_filter="-${GRAPH_FILTER}"
)

shopt -s nullglob
profraw_files=("$BUILD_DIR"/default-*.profraw)
shopt -u nullglob

if (( ${#profraw_files[@]} == 0 )); then
  echo "error: no .profraw files produced under ${BUILD_DIR}" >&2
  exit 1
fi

"$LLVM_PROFDATA" merge -sparse "${profraw_files[@]}" -o "$PROFDATA_FILE"

coverage_report="$("$LLVM_COV" report \
  "$BUILD_DIR/tests" \
  --instr-profile="$PROFDATA_FILE" \
  --ignore-filename-regex="$IGNORE_FILENAME_REGEX")"

echo "$coverage_report"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  {
    echo "## C++ coverage (llvm-cov)"
    echo
    echo '```'
    echo "$coverage_report"
    echo '```'
  } >> "$GITHUB_STEP_SUMMARY"
fi

mkdir -p "$COVERAGE_HTML_DIR"

"$LLVM_COV" show \
  "$BUILD_DIR/tests" \
  --instr-profile="$PROFDATA_FILE" \
  --ignore-filename-regex="$IGNORE_FILENAME_REGEX" \
  --Xdemangler=c++filt \
  --format=html \
  --output-dir="$COVERAGE_HTML_DIR"

echo
echo "Coverage HTML report: ${COVERAGE_HTML_DIR}/index.html"
