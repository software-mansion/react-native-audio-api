#!/bin/bash

set -e

cd common/cpp/test

cmake -S . -B build -Wno-dev

cd build
make -j10

GRAPH_FILTER="AudioGraphTest.*:AudioGraphFuzzTest.*:GraphTest.*:GraphFuzzTest.*:GraphCycleDebugTest.*:HostGraphTest.*:Seeds/*"
./tests --gtest_print_time=1 --gtest_filter="-${GRAPH_FILTER}"