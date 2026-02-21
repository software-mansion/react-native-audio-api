#!/bin/bash

set -e

cleanup() {
    echo "Cleaning up..."
    rm -rf build_graph/
}

trap cleanup EXIT

cd packages/react-native-audio-api/common/cpp/test

GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:SandboxTest.*:Seeds/*"

cmake -S . -B build_graph -Wno-dev

cd build_graph
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

rm -rf build_graph/
