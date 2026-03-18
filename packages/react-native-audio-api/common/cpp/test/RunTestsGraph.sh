#!/bin/bash

set -e

cleanup() {
    echo "Cleaning up..."
    rm -rf build/
}

trap cleanup EXIT


SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Allow override of GRAPH_FILTER via environment variable
GRAPH_FILTER="${GRAPH_FILTER:-AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*}"

cmake -S . -B build -Wno-dev

cd build
make tests_asan tests -j10

echo ""
echo "=== Running graph tests with AddressSanitizer + UndefinedBehaviorSanitizer ==="
echo ""
./tests_asan --gtest_print_time=1 --gtest_filter="${GRAPH_FILTER}" "$@"

echo ""
echo "=== Running graph tests (normal) ==="
echo ""
./tests --gtest_print_time=1 --gtest_filter="${GRAPH_FILTER}" "$@"

cd ..

rm -rf build/
