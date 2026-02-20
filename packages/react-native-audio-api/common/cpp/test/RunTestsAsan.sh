#!/bin/bash

set -e

cleanup() {
    echo "Cleaning up..."
    rm -rf build_asan/
}

trap cleanup EXIT

cd packages/react-native-audio-api/common/cpp/test

cmake -S . -B build_asan -Wno-dev

cd build_asan
make tests_asan -j10
echo ""
echo "=== Running tests with AddressSanitizer + UndefinedBehaviorSanitizer ==="
echo ""
./tests_asan --gtest_print_time=1 "$@"
cd ..

rm -rf build_asan/
