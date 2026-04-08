#!/bin/bash

set -e

cleanup() {
    echo "Cleaning up..."
    rm -rf build/
}

trap cleanup EXIT

cd common/cpp/test

cmake -S . -B build -Wno-dev

cd build
make -j10

GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*"
./tests --gtest_print_time=1 --gtest_filter="-${GRAPH_FILTER}"
cd ..

rm -rf build/
